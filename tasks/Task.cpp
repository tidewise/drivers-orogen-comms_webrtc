/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"

using namespace iodrivers_base;
using namespace comms_webrtc;
using namespace std;
using namespace rtc;

template <class T>
weak_ptr<T> make_weak_ptr(shared_ptr<T> ptr) { return ptr; }

void Task::onOffer()
{
    string description = mDecoder.getDescription();
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
    createPeerConnection();

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
    createPeerConnection();

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

bool Task::parseIncomingMessage(char const *data)
{
    string error;
    if (!mDecoder.parseJSONMessage(data, error))
    {
        LOG_ERROR_S << "Failed parsing the message, got error: " << error << std::endl;
        return false;
    }
    return true;
}

void Task::createPeerConnection()
{
    string protocol = mDecoder.getProtocol();
    string remote_peer_id = mDecoder.getId();
    mPeerConnection = initiatePeerConnection(protocol, remote_peer_id);
    configurePeerDataChannel(remote_peer_id);
}

shared_ptr<rtc::PeerConnection> Task::initiatePeerConnection(string const &protocol, string const &remote_peer_id)
{
    auto peer_connection = std::make_shared<rtc::PeerConnection>(mConfig);

    peer_connection->onStateChange([](rtc::PeerConnection::State state)
                                   { LOG_INFO_S << "State: " << state << std::endl; });

    peer_connection->onGatheringStateChange(
        [](rtc::PeerConnection::GatheringState state)
        { LOG_INFO_S << "Gathering State: " << state << std::endl; });

    peer_connection->onLocalDescription(
        [&, protocol, remote_peer_id](rtc::Description description)
        {
            Json::Value message;
            Json::FastWriter fast;
            message["protocol"] = protocol;
            message["to"] = remote_peer_id;
            message["actiontype"] = description.typeString();
            message["description"] = string(description);

            if (auto ws = make_weak_ptr(mWs).lock())
                ws->send(fast.write(message));
        });

    peer_connection->onLocalCandidate(
        [&, protocol, remote_peer_id](rtc::Candidate candidate)
        {
            Json::Value message;
            Json::FastWriter fast;
            message["protocol"] = protocol;
            message["to"] = remote_peer_id;
            message["actiontype"] = "candidate";
            message["candidate"] = string(candidate);
            message["mid"] = candidate.mid();

            if (auto ws = make_weak_ptr(mWs).lock())
                ws->send(fast.write(message));
        });

    return peer_connection;
}

void Task::configurePeerDataChannel(string const &remote_peer_id)
{
    mPeerConnection->onDataChannel(
        [&](shared_ptr<rtc::DataChannel> data_channel)
        {
            LOG_INFO_S << "DataChannel from " << remote_peer_id
                       << " received with label \"" << data_channel->label() << "\""
                       << std::endl;

            mDataChannel = data_channel;
            data_channel->onOpen([&, wdc = make_weak_ptr(data_channel)]() {});

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
        });
}

void Task::configureWebSocket()
{
    promise<void> ws_promise;
    auto ws_future = ws_promise.get_future();

    mWs->onOpen(
        [&ws_promise]()
        {
            LOG_INFO_S << "WebSocket connected, signaling ready" << std::endl;
            ws_promise.set_value();
        });

    mWs->onError(
        [&ws_promise](string s)
        {
            LOG_INFO_S << "WebSocket error" << std::endl;
            ws_promise.set_exception(std::make_exception_ptr(std::runtime_error(s)));
        });

    mWs->onClosed([]()
                  { LOG_INFO_S << "WebSocket closed" << std::endl; });

    mWs->onMessage(
        [&](variant<binary, string> data)
        {
            if (!holds_alternative<string>(data))
            {
                return;
            }

            if (!parseIncomingMessage(get<string>(data).c_str()))
            {
                return;
            }

            string actiontype = mDecoder.getActionType();

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
    const string url = _websocket_server_name.get() + "?user=" + _local_peer_id.get();
    mWs->open(url);
    ws_future.get();
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
        mPeerConnection = initiatePeerConnection(_protocol.get(), _remote_peer_id.get());
        configurePeerDataChannel(_remote_peer_id.get());

        mDataChannel = mPeerConnection->createDataChannel(_data_channel_label.get());
    }

    return true;
}
void Task::updateHook()
{
    if (mDataChannel)
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
    }

    TaskBase::updateHook();
}
void Task::errorHook() { TaskBase::errorHook(); }
void Task::stopHook() { TaskBase::stopHook(); }
void Task::cleanupHook() { TaskBase::cleanupHook(); }
