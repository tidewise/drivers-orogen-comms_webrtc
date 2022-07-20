/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"

using namespace iodrivers_base;
using namespace comms_webrtc;
using namespace std::this_thread;
using namespace std;
using namespace rtc;

template <class T>
weak_ptr<T> make_weak_ptr(shared_ptr<T> ptr) { return ptr; }

void Task::onOffer()
{
    string description = mDecoder.getDescription();
    mRemotePeerID = mDecoder.getFrom();
    createPeerConnection();

    try
    {
        mPeerConnection->setRemoteDescription(rtc::Description(description, "offer"));
    }
    catch (const std::logic_error &error)
    {
        mPeerConnection.reset();
        LOG_ERROR_S << error.what();
        LOG_ERROR_S << std::endl;
    }
}

void Task::onAnswer()
{
    string description = mDecoder.getDescription();

    try
    {
        mPeerConnection->setRemoteDescription(rtc::Description(description, "answer"));
    }
    catch (const std::logic_error &error)
    {
        mPeerConnection.reset();
        LOG_ERROR_S << error.what();
        LOG_ERROR_S << std::endl;
    }
}

void Task::onCandidate()
{
    string mid = mDecoder.getMid();
    string candidate = mDecoder.getCandidate();

    try
    {
        mPeerConnection->addRemoteCandidate(rtc::Candidate(candidate, mid));
    }
    catch (const logic_error &error)
    {
        mPeerConnection.reset();
        LOG_ERROR_S << error.what();
        LOG_ERROR_S << std::endl;
    }
}

void Task::parseIncomingMessage(char const *data)
{
    string error;
    if (!mDecoder.parseJSONMessage(data, error))
    {
        throw std::invalid_argument(error);
    }
}

void Task::createPeerConnection()
{
    mPeerConnection = initiatePeerConnection();
    configurePeerDataChannel();
}

shared_ptr<rtc::PeerConnection> Task::initiatePeerConnection()
{
    auto peer_connection = std::make_shared<rtc::PeerConnection>(mConfig);

    peer_connection->onStateChange(
        [&](rtc::PeerConnection::State state)
        {
            switch (state)
            {
            case rtc::PeerConnection::State::Disconnected:
                mState.peer_connection.state = Disconnected;
            case rtc::PeerConnection::State::Closed:
            {
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
                mState.peer_connection.state = NoConnection;
            }
        });

    peer_connection->onGatheringStateChange(
        [&](rtc::PeerConnection::GatheringState state)
        {
            switch (state)
            {
            case rtc::PeerConnection::GatheringState::Complete:
                mState.peer_connection.gathering_state = Complete;
            case rtc::PeerConnection::GatheringState::InProgress:
                mState.peer_connection.gathering_state = InProgress;
            case rtc::PeerConnection::GatheringState::New:
                mState.peer_connection.gathering_state = NewGathering;
            default:
                mState.peer_connection.gathering_state = NoGathering;
            }
        });

    peer_connection->onSignalingStateChange(
        [&](rtc::PeerConnection::SignalingState state)
        {
            switch (state)
            {
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
                mState.peer_connection.signaling_state = NoSignaling;
            }
        });

    peer_connection->onLocalDescription(
        [&](rtc::Description description)
        {
            Json::Value message;
            message["protocol"] = "one-to-one";
            message["to"] = mRemotePeerID;
            message["action"] = description.typeString();
            message["data"]["from"] = _local_peer_id.get();
            message["data"]["description"] = string(description);

            if (auto ws = make_weak_ptr(mWs).lock())
            {
                Json::FastWriter fast;
                ws->send(fast.write(message));
            }
        });

    peer_connection->onLocalCandidate(
        [&](rtc::Candidate candidate)
        {
            Json::Value message;
            message["protocol"] = "one-to-one";
            message["to"] = mRemotePeerID;
            message["action"] = "candidate";
            message["data"]["from"] = _local_peer_id.get();
            message["data"]["candidate"] = string(candidate);
            message["data"]["mid"] = candidate.mid();

            if (auto ws = make_weak_ptr(mWs).lock())
            {
                Json::FastWriter fast;
                ws->send(fast.write(message));
            }
        });

    return peer_connection;
}

void Task::configurePeerDataChannel()
{
    mPeerConnection->onDataChannel(
        [&](shared_ptr<rtc::DataChannel> data_channel)
        {
            mDataChannel = data_channel;
            registerDataChannelCallBacks(data_channel);
        });
}

void Task::configureWebSocket()
{
    std::promise<void> ws_promise;
    auto ws_future = ws_promise.get_future();

    mWs->onOpen(
        [&]()
        {
            LOG_INFO_S << "WebSocket connected, signaling ready" << std::endl;
            mState.web_socket = WebSocketOpened;
            ws_promise.set_value();
        });

    mWs->onError(
        [&](const string &error)
        {
            LOG_ERROR_S << "WebSocket failed: " << error << endl;
            mState.web_socket = WebSocketFailed;
            ws_promise.set_exception(std::make_exception_ptr(std::runtime_error(error)));
            mPeerConnectionClosePromise.set_exception(std::make_exception_ptr(std::runtime_error(error)));
        });

    mWs->onClosed(
        [&]()
        {
            LOG_INFO_S << "WebSocket closed" << std::endl;
            mState.web_socket = WebSocketClosed;
        });

    mWs->onMessage(
        [&](variant<binary, string> data)
        {
            if (!holds_alternative<string>(data))
            {
                return;
            }

            parseIncomingMessage(get<string>(data).c_str());
            string actiontype = mDecoder.getActionType();

            if (actiontype == "ping")
            {
                pong();
            }
            if (actiontype == "pong")
            {
                if (_remote_peer_id.get() == mDecoder.getFrom())
                {
                    mRemotePeerAnswerReceived = true;
                    mWaitRemotePeerPromise.set_value();
                }
            }

            if (actiontype == "offer")
            {
                onOffer();
            }
            else if (actiontype == "answer")
            {
                onAnswer();
            }
            else if (actiontype == "candidate")
            {
                onCandidate();
            }
        });

    // wss://signalserverhost?user=yourname
    const string url = _signaling_server_name.get() + "?user=" + _local_peer_id.get();
    mWs->open(url);
    ws_future.get();
}

void Task::ping()
{
    Json::Value message;
    message["protocol"] = "one-to-one";
    message["to"] = _remote_peer_id.get();
    message["action"] = "ping";
    message["data"]["from"] = _local_peer_id.get();
    Json::FastWriter fast;
    if (auto ws = make_weak_ptr(mWs).lock())
    {
        ws->send(fast.write(message));
    }
}

void Task::pong()
{
    Json::Value message;
    message["protocol"] = "one-to-one";
    message["to"] = mDecoder.getFrom();
    message["action"] = "pong";
    message["data"]["from"] = _local_peer_id.get();
    Json::FastWriter fast;
    if (auto ws = make_weak_ptr(mWs).lock())
    {
        ws->send(fast.write(message));
    }
}

void Task::registerDataChannelCallBacks(shared_ptr<rtc::DataChannel> data_channel)
{
    LOG_INFO_S << "DataChannel from " << mRemotePeerID
               << " received with label \"" << data_channel->label() << "\""
               << std::endl;
    data_channel->onOpen(
        [&]()
        {
            mState.data_channel = DataChannelOpened;
            mDataChannelPromise.set_value();
        });

    data_channel->onError(
        [&](const string &error)
        {
            LOG_ERROR_S << "DataChannel failed: " << error << endl;
            mState.data_channel = DataChannelFailed;
            mDataChannelPromise.set_exception(std::make_exception_ptr(std::runtime_error(error)));
            mDataChannelClosePromise.set_exception(std::make_exception_ptr(std::runtime_error(error)));
             trigger();
        });

    data_channel->onClosed(
        [&]()
        {
            LOG_INFO_S << "DataChannel closed" << std::endl;
            mState.data_channel = DataChannelClosed;
            mDataChannelClosePromise.set_value();
            trigger();
        });

    data_channel->onMessage(
        [&](variant<binary, string> data)
        {
            RawPacket dataPacket;
            dataPacket.time = base::Time::now();

            if (holds_alternative<string>(data))
            {
                string data_string = get<string>(data);
                vector<uint8_t> new_data(data_string.begin(), data_string.end());
                dataPacket.data.resize(new_data.size());
                dataPacket.data = new_data;
            }
            else
            {
                vector<byte> data_byte = get<binary>(data);
                dataPacket.data.resize(data_byte.size());
                for (unsigned int i = 0; i < data_byte.size(); i++)
                {
                    dataPacket.data[i] = to_integer<uint8_t>(data_byte[i]);
                }
            }
            _data_out.write(dataPacket);
        });
}

void Task::evaluateDataChannel()
{
    switch (mState.data_channel)
    {
    case DataChannelOpened:
    {
        iodrivers_base::RawPacket raw_packet;
        if (_data_in.read(raw_packet) != RTT::NewData)
            return;

        vector<byte> data;
        data.resize(raw_packet.data.size());
        for (unsigned int i = 0; i < raw_packet.data.size(); i++)
        {
            data[i] = static_cast<byte>(raw_packet.data[i]);
        }
        mDataChannel->send(&data.front(), data.size());
        break;
    }
    case DataChannelClosed:
        throw std::runtime_error("DataChannel closed");
    case DataChannelFailed:
        throw std::runtime_error("DataChannel failed");
    default:
        break;
    }
}

Task::Task(string const &name) : TaskBase(name) {}

Task::~Task() {}

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Task.hpp for more detailed
// documentation about them.

bool Task::configureHook()
{
    if (!TaskBase::configureHook())
        return false;

    mConfig.iceServers.emplace_back(_stun_server.get());
    mWs = std::make_shared<rtc::WebSocket>();

    configureWebSocket();

    return true;
}
bool Task::startHook()
{
    if (!TaskBase::startHook())
        return false;

    if (!_remote_peer_id.get().empty())
    {
        std::future<void> wait_remote_peer_future = mWaitRemotePeerPromise.get_future();
        mRemotePeerID = _remote_peer_id.get();
        mRemotePeerAnswerReceived = false;

        base::Time start_time = base::Time::now();
        // Get contact with the remote peer and create datachannel
        while (!mRemotePeerAnswerReceived)
        {
            base::Time duration = base::Time::now() - start_time;
            if (duration > _wait_remote_peer_time_out.get())
            {
                mWaitRemotePeerPromise.set_exception(std::make_exception_ptr(std::runtime_error("Timeout to find the remote peer")));
                break;
            }
            ping();
            sleep_for(100ms);
        }
        wait_remote_peer_future.get();

        std::future<void> dc_future = mDataChannelPromise.get_future();
        mPeerConnection = initiatePeerConnection();
        string label = _data_channel_label.get();
        mDataChannel = mPeerConnection->createDataChannel(label);
        registerDataChannelCallBacks(mDataChannel);
        dc_future.get();
    }
    else
    {
        std::future<void> dc_future = mDataChannelPromise.get_future();
        base::Time start_time = base::Time::now();
        // Wait for the datachannel open until the timeout
        while (mState.data_channel != DataChannelOpened)
        {
            base::Time duration = base::Time::now() - start_time;
            if (duration > _data_channel_open_time_out.get())
            {
                mDataChannelPromise.set_exception(std::make_exception_ptr(std::runtime_error("Timeout to open datachannel")));
                break;
            }
        }
        dc_future.get();
    }

    return true;
}
void Task::updateHook()
{
    _status.write(mState);

    evaluateDataChannel();

    TaskBase::updateHook();
}
void Task::errorHook() { TaskBase::errorHook(); }
void Task::stopHook()
{
    if (mDataChannel->isOpen())
    {
        std::future<void> dc_close_future = mDataChannelClosePromise.get_future();
        mDataChannel->close();
        dc_close_future.get();
    }
    if (mPeerConnection)
    {
        std::future<void> pc_close_future = mPeerConnectionClosePromise.get_future();
        mPeerConnection.reset();
        pc_close_future.get();
    }
    mState = WebRTCState();
    _status.write(mState);

    TaskBase::stopHook();
}
void Task::cleanupHook() { TaskBase::cleanupHook(); }
