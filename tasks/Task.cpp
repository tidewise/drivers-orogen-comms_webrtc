/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"

using namespace iodrivers_base;
using namespace comms_webrtc;
using namespace std::this_thread;
using namespace std;
using namespace rtc;

void Task::onOffer()
{
    string description = mDecoder.getDescription();
    mRemotePeerID = mDecoder.getFrom();
    createPeerConnection();

    try {
        mPeerConnection->setRemoteDescription(rtc::Description(description, "offer"));
    }
    catch (logic_error const& error) {
        mPeerConnection.reset();
        LOG_ERROR_S << error.what();
        LOG_ERROR_S << endl;
    }
}

void Task::onAnswer()
{
    string description = mDecoder.getDescription();

    try {
        mPeerConnection->setRemoteDescription(rtc::Description(description, "answer"));
    }
    catch (logic_error const& error) {
        mPeerConnection.reset();
        LOG_ERROR_S << error.what();
        LOG_ERROR_S << endl;
    }
}

void Task::onCandidate()
{
    string candidate = mDecoder.getCandidate();
    string mid;
    if (mDecoder.isMidFieldPresent()) {
        mid = mDecoder.getMid();
    }
    try {
        mPeerConnection->addRemoteCandidate(rtc::Candidate(candidate, mid));
    }
    catch (logic_error const& error) {
        mPeerConnection.reset();
        LOG_ERROR_S << error.what();
        LOG_ERROR_S << endl;
    }
}

void Task::parseIncomingMessage(string const& data)
{
    string error;
    if (!mDecoder.parseJSONMessage(data, error)) {
        throw invalid_argument(error);
    }
}

void Task::createPeerConnection()
{
    mPeerConnection = initiatePeerConnection();
    configurePeerDataChannel();
}

shared_ptr<rtc::PeerConnection> Task::initiatePeerConnection()
{
    auto peer_connection = make_shared<rtc::PeerConnection>(mConfig);

    peer_connection->onStateChange([&](rtc::PeerConnection::State state) {
        switch (state) {
            case rtc::PeerConnection::State::Disconnected:
                mState.peer_connection.state = Disconnected;
            case rtc::PeerConnection::State::Closed: {
                mState.peer_connection.state = Closed;
                mPeerConnectionClosePromise.set_value();
            }
            case rtc::PeerConnection::State::Connected:
                mState.peer_connection.state = Connected;
            case rtc::PeerConnection::State::Connecting:
                mState.peer_connection.state = Connecting;
            case rtc::PeerConnection::State::Failed:
                mState.peer_connection.state = Failed;
            case rtc::PeerConnection::State::New:
                mState.peer_connection.state = NewConnection;
            default:
                break;
        }
    });

    peer_connection->onGatheringStateChange(
        [&](rtc::PeerConnection::GatheringState state) {
            switch (state) {
                case rtc::PeerConnection::GatheringState::Complete:
                    mState.peer_connection.gathering_state = Complete;
                case rtc::PeerConnection::GatheringState::InProgress:
                    mState.peer_connection.gathering_state = InProgress;
                case rtc::PeerConnection::GatheringState::New:
                    mState.peer_connection.gathering_state = NewGathering;
                default:
                    break;
            }
        });

    peer_connection->onSignalingStateChange(
        [&](rtc::PeerConnection::SignalingState state) {
            switch (state) {
                case rtc::PeerConnection::SignalingState::HaveLocalOffer:
                    mState.peer_connection.signaling_state = HaveLocalOffer;
                case rtc::PeerConnection::SignalingState::HaveLocalPranswer:
                    mState.peer_connection.signaling_state = HaveLocalPranswer;
                case rtc::PeerConnection::SignalingState::HaveRemoteOffer:
                    mState.peer_connection.signaling_state = HaveRemoteOffer;
                case rtc::PeerConnection::SignalingState::HaveRemotePranswer:
                    mState.peer_connection.signaling_state = HaveRemotePranswer;
                case rtc::PeerConnection::SignalingState::Stable:
                    mState.peer_connection.signaling_state = Stable;
                default:
                    break;
            }
        });

    peer_connection->onLocalDescription([&](rtc::Description description) {
        Json::Value message;
        message["protocol"] = "one-to-one";
        message["to"] = mRemotePeerID;
        message["action"] = description.typeString();
        message["data"]["from"] = _local_peer_id.get();
        message["data"]["description"] = string(description);


        Json::FastWriter fast;
        mWebSocket->send(fast.write(message));
    });

    peer_connection->onLocalCandidate([&](rtc::Candidate candidate) {
        Json::Value message;
        message["protocol"] = "one-to-one";
        message["to"] = mRemotePeerID;
        message["action"] = "candidate";
        message["data"]["from"] = _local_peer_id.get();
        message["data"]["candidate"] = string(candidate);
        message["data"]["mid"] = candidate.mid();

        Json::FastWriter fast;
        mWebSocket->send(fast.write(message));
    });

    return peer_connection;
}

void Task::configurePeerDataChannel()
{
    mPeerConnection->onDataChannel([&](shared_ptr<rtc::DataChannel> data_channel) {
        mDataChannel = data_channel;
        registerDataChannelCallBacks(data_channel);
    });
}

void Task::configureWebSocket()
{
    promise<void> ws_promise;
    future<void> ws_future = ws_promise.get_future();

    mWebSocket->onOpen([&]() {
        LOG_INFO_S << "WebSocket connected, signaling ready" << endl;
        mState.web_socket = WebSocketOpened;
        ws_promise.set_value();
    });

    mWebSocket->onError([&](string const& error) {
        LOG_ERROR_S << "WebSocket failed: " << error << endl;
        mState.web_socket = WebSocketFailed;
        ws_promise.set_exception(make_exception_ptr(runtime_error(error)));
        mPeerConnectionClosePromise.set_exception(
            make_exception_ptr(runtime_error(error)));
        mWebSocketClosePromise.set_exception(make_exception_ptr(runtime_error(error)));
        trigger();
    });

    mWebSocket->onClosed([&]() {
        LOG_INFO_S << "WebSocket closed" << endl;
        mState.web_socket = WebSocketClosed;
        mWebSocketClosePromise.set_value();
        trigger();
    });

    mWebSocket->onMessage([&](variant<binary, string> data) {
        if (!holds_alternative<string>(data)) {
            return;
        }

        parseIncomingMessage(get<string>(data));
        string actiontype = mDecoder.getActionType();

        if (actiontype == "ping") {
            pong();
        }
        if (actiontype == "pong") {
            if (_remote_peer_id.get() == mDecoder.getFrom()) {
                mWaitRemotePeerPromise.set_value();
            }
            else {
                mWaitRemotePeerPromise.set_exception(
                    make_exception_ptr(runtime_error("Remote peer unreachable")));
            }
        }

        if (actiontype == "offer") {
            onOffer();
        }
        else if (actiontype == "answer") {
            onAnswer();
        }
        else if (actiontype == "candidate") {
            onCandidate();
        }
    });

    // wss://signalserverhost?user=yourname
    const string url = _signaling_server_name.get() + "?user=" + _local_peer_id.get();
    mWebSocket->open(url);
    future_status status = ws_future.wait_for(
        chrono::microseconds(_wait_remote_peer_time_out.get().toMicroseconds()));
    if (status == future_status::ready) {
        ws_future.get();
    }
    else {
        mWebSocket.reset();
        throw runtime_error("Timed out waiting for the websocket connection to be ready");
    }
}

void Task::send(std::string const& action, Json::Value const& data)
{
    Json::Value message;
    message["protocol"] = "one-to-one";
    message["to"] = _remote_peer_id.get();
    message["action"] = action;
    message["data"] = data;
    Json::FastWriter fast;
    mWebSocket->send(fast.write(message));
}

void Task::ping()
{
    Json::Value data;
    data["from"] = _local_peer_id.get();
    send("ping", data);
}

void Task::pong()
{
    Json::Value data;
    data["from"] = _local_peer_id.get();
    send("pong", data);
}

void Task::registerDataChannelCallBacks(shared_ptr<rtc::DataChannel> data_channel)
{
    LOG_INFO_S << "DataChannel from " << mRemotePeerID << " received with label \""
               << data_channel->label() << "\"" << endl;
    data_channel->onOpen([&]() {
        mState.data_channel = DataChannelOpened;
        mDataChannelPromise.set_value();
    });

    data_channel->onError([&](string const& error) {
        LOG_ERROR_S << "DataChannel failed: " << error << endl;
        mState.data_channel = DataChannelFailed;
        mDataChannelPromise.set_exception(make_exception_ptr(runtime_error(error)));
        mDataChannelClosePromise.set_exception(make_exception_ptr(runtime_error(error)));
        trigger();
    });

    data_channel->onClosed([&]() {
        LOG_INFO_S << "DataChannel closed" << endl;
        mState.data_channel = DataChannelClosed;
        mDataChannelClosePromise.set_value();
        trigger();
    });

    data_channel->onMessage([&](variant<binary, string> data) {
        RawPacket dataPacket;
        dataPacket.time = base::Time::now();

        if (holds_alternative<string>(data)) {
            string data_string = get<string>(data);
            vector<uint8_t> new_data(data_string.begin(), data_string.end());
            dataPacket.data.resize(new_data.size());
            dataPacket.data = new_data;
        }
        else {
            vector<byte> data_byte = get<binary>(data);
            dataPacket.data.resize(data_byte.size());
            for (unsigned int i = 0; i < data_byte.size(); i++) {
                dataPacket.data[i] = to_integer<uint8_t>(data_byte[i]);
            }
        }
        _data_out.write(dataPacket);
    });
}

void Task::evaluateDataChannel()
{
    switch (mState.data_channel) {
        case DataChannelOpened: {
            iodrivers_base::RawPacket raw_packet;
            if (_data_in.read(raw_packet) != RTT::NewData) {
                return;
            }

            vector<byte> data;
            data.resize(raw_packet.data.size());
            for (unsigned int i = 0; i < raw_packet.data.size(); i++) {
                data[i] = static_cast<byte>(raw_packet.data[i]);
            }
            mDataChannel->send(&data.front(), data.size());
            break;
        }
        case DataChannelClosed:
            throw runtime_error("DataChannel closed");
        case DataChannelFailed:
            throw runtime_error("DataChannel failed");
        default:
            break;
    }
}

void Task::evaluateWebSocket()
{
    switch (mState.web_socket) {
        case WebSocketClosed:
            throw runtime_error("WebSocket closed");
        case WebSocketFailed:
            throw runtime_error("WebSocket failed");
        default:
            break;
    }
}

Task::Task(string const& name)
    : TaskBase(name)
{
}

Task::~Task()
{
}

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Task.hpp for more detailed
// documentation about them.

bool Task::configureHook()
{
    if (!TaskBase::configureHook()) {
        return false;
    }

    mConfig.iceServers.emplace_back(_stun_server.get());
    mWebSocket = make_shared<rtc::WebSocket>();

    configureWebSocket();

    return true;
}
bool Task::startHook()
{
    if (!TaskBase::startHook()) {
        return false;
    }

    if (!_remote_peer_id.get().empty()) {
        mRemotePeerID = _remote_peer_id.get();
        future<void> wait_remote_peer_future = mWaitRemotePeerPromise.get_future();
        // Try to get contact with the remote peer and create datachannel
        base::Time deadline = base::Time::now() + _wait_remote_peer_time_out.get();
        while (true) {
            ping();
            if (wait_remote_peer_future.wait_for(100ms) == future_status::ready) {
                wait_remote_peer_future.get();
                break;
            }
            else if (base::Time::now() > deadline) {
                _status.write(mState);
                throw runtime_error("Timeout to get contact with the remote peer");
            }
        }
        // Create datachannel
        mPeerConnection = initiatePeerConnection();
        mDataChannel = mPeerConnection->createDataChannel(_data_channel_label.get());
        registerDataChannelCallBacks(mDataChannel);
    }
    future<void> dc_future = mDataChannelPromise.get_future();
    // check datachannel timeout
    future_status status = dc_future.wait_for(
        chrono::microseconds(_data_channel_time_out.get().toMicroseconds()));
    if (status == future_status::ready) {
        dc_future.get();
    }
    else {
        _status.write(mState);
        throw runtime_error("Timeout to open datachannel");
    }

    _status.write(mState);

    return true;
}
void Task::updateHook()
{
    _status.write(mState);

    evaluateDataChannel();
    evaluateWebSocket();

    TaskBase::updateHook();
}
void Task::errorHook()
{
    TaskBase::errorHook();
}
void Task::stopHook()
{
    if (mDataChannel) {
        future<void> dc_close_future = mDataChannelClosePromise.get_future();
        // Try to close datachannel
        mDataChannel.reset();
        // check timeout
        future_status status = dc_close_future.wait_for(
            chrono::microseconds(_data_channel_time_out.get().toMicroseconds()));
        if (status == future_status::ready) {
            mState.data_channel = NoDataChannel;
            dc_close_future.get();
        }
        else {
            _status.write(mState);
            throw runtime_error("Timeout waiting for the data channel to close");
        }
    }
    if (mPeerConnection) {
        future<void> pc_close_future = mPeerConnectionClosePromise.get_future();
        // Try to close peerconnection
        mPeerConnection.reset();
        // check timeout
        future_status status = pc_close_future.wait_for(
            chrono::microseconds(_peer_connection_time_out.get().toMicroseconds()));
        if (status == future_status::ready) {
            mState.peer_connection = PeerConnectionState();
            pc_close_future.get();
        }
        else {
            _status.write(mState);
            throw runtime_error("Timeout waiting for the peer connection to close");
        }
    }
    if (mWebSocket->isOpen()) {
        // Wait for the websocket close
        future<void> ws_close_future = mWebSocketClosePromise.get_future();
        mWebSocket->close();
        // check timeout
        future_status status = ws_close_future.wait_for(
            chrono::microseconds(_websocket_time_out.get().toMicroseconds()));
        if (status == future_status::ready) {
            ws_close_future.get();
        }
        else {
            _status.write(mState);
            throw runtime_error("Timeout waiting for the web socket to close");
        }
    }

    _status.write(mState);

    TaskBase::stopHook();
}
void Task::cleanupHook()
{
    if (mWebSocket) {
        mWebSocket.reset();
    }

    TaskBase::cleanupHook();
}
