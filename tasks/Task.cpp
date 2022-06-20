/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"

using namespace comms_webrtc;
using namespace iodrivers_base;
using namespace std;
using namespace rtc;

template <class T> weak_ptr<T> make_weak_ptr(shared_ptr<T> ptr) { return ptr; }

struct comms_webrtc::MessageDecoder
{
    Json::Value jdata;
    Json::CharReaderBuilder rbuilder;
    unique_ptr<Json::CharReader> const reader;

    MessageDecoder() : reader(rbuilder.newCharReader()) {}

    bool parseJSONMessage(char const* data, string& errors)
    {
        return reader->parse(data, data + strlen(data), &jdata, &errors);
    }

    void validateFieldPresent(string fieldName)
    {
        if (!jdata.isMember(fieldName))
        {
            throw invalid_argument("message does not contain the " + fieldName + " field");
        }
    }

    string getType()
    {
        validateFieldPresent("type");
        return jdata["type"].asString();
    }

    string getId()
    {
        validateFieldPresent("id");
        return jdata["id"].asString();
    }

    string getDescription()
    {
        validateFieldPresent("description");
        return jdata["description"].asString();
    }

    string getCandidate()
    {
        validateFieldPresent("candidate");
        return jdata["candidate"].asString();
    }

    string getMid()
    {
        validateFieldPresent("mid");
        return jdata["mid"].asString();
    }
};

void Task::onOffer(rtc::shared_ptr<rtc::WebSocket> wws)
{
    string remote_peer_id = decoder->getId();
    string description = decoder->getDescription();
    
    mPeerConnection = createPeerConnection(wws, remote_peer_id);

    try
    {
        mPeerConnection->setRemoteDescription(rtc::Description(description, "offer"));
    }
    catch (const std::logic_error& error)
    {
        mPeerConnection.reset();
        LOG_ERROR_S << error.what();
        LOG_ERROR_S << std::endl;
    }
}

void Task::onAnswer()
{
    string description = decoder->getDescription();

    try
    {
        mPeerConnection->setRemoteDescription(rtc::Description(description, "answer"));
    }
    catch (const std::logic_error& error)
    {
        mPeerConnection.reset();
        LOG_ERROR_S << error.what();
        LOG_ERROR_S << std::endl;
    }
}

void Task::onCandidate()
{
    string candidate = decoder->getCandidate();
    string mid = decoder->getMid();

    try
    {
        mPeerConnection->addRemoteCandidate(rtc::Candidate(candidate, mid));
    }
    catch (const logic_error& error)
    {
        mPeerConnection.reset();
        LOG_ERROR_S << error.what();
        LOG_ERROR_S << std::endl;
    }
}

bool Task::parseIncomingMessage(char const* data)
{
    string error;
    if (!decoder->parseJSONMessage(data, error))
    {
        LOG_ERROR_S << "Failed parsing the message, got error: " << error << std::endl;
        return false;
    }
    return true;
}

// Create and setup a PeerConnection
shared_ptr<rtc::PeerConnection> Task::createPeerConnection(
    shared_ptr<rtc::WebSocket> const& wws,
    string const& remote_peer_id)
{
    shared_ptr<rtc::PeerConnection> peer_connection =
        configurePeerConnection(wws, remote_peer_id);

    configureDataChannel(peer_connection, remote_peer_id);

    mPeerConnectionMap.emplace(remote_peer_id, peer_connection);
    return peer_connection;
};

shared_ptr<rtc::PeerConnection> Task::configurePeerConnection(
    shared_ptr<rtc::WebSocket> const& wws,
    string const& remote_peer_id)
{
    auto peer_connection = std::make_shared<rtc::PeerConnection>(mConfig);

    peer_connection->onStateChange([](rtc::PeerConnection::State state)
                                   { LOG_INFO_S << "State: " << state << std::endl; });

    peer_connection->onGatheringStateChange(
        [](rtc::PeerConnection::GatheringState state)
        { LOG_INFO_S << "Gathering State: " << state << std::endl; });

    peer_connection->onLocalDescription(
        [&, wws](rtc::Description description)
        {
            Json::Value message;
            Json::FastWriter fast;
            message["id"] = remote_peer_id;
            message["type"] = description.typeString();
            message["description"] = string(description);

            if (auto ws = make_weak_ptr(wws).lock())
                ws->send(fast.write(message));
        });

    peer_connection->onLocalCandidate(
        [&, wws](rtc::Candidate candidate)
        {
            Json::Value message;
            Json::FastWriter fast;
            message["id"] = remote_peer_id;
            message["type"] = "candidate";
            message["candidate"] = string(candidate);
            message["mid"] = candidate.mid();

            if (auto ws = make_weak_ptr(wws).lock())
                ws->send(fast.write(message));
        });

    return peer_connection;
}

void Task::configureDataChannel(
    shared_ptr<rtc::PeerConnection> const& peer_connection,
    string const& remote_peer_id)
{
    peer_connection->onDataChannel(
        [&](shared_ptr<rtc::DataChannel> data_channel)
        {
            LOG_INFO_S << "DataChannel from " << remote_peer_id
                       << " received with label \"" << data_channel->label() << "\""
                       << std::endl;

            data_channel->onOpen(
                [&, wdc = make_weak_ptr(data_channel)]()
                {
                    if (auto data_channel = wdc.lock())
                        data_channel->send("DataChannel open");
                });

            data_channel->onMessage(
                [&](variant<binary, string> data)
                {
                    RawPacket dataPacket;
                    dataPacket.time = base::Time::now();

                    if (holds_alternative<string>(data))
                    {
                        // TODO
                    }
                    else
                    {
                        std::vector<byte> data_byte = get<binary>(data);
                        dataPacket.data.resize(data_byte.size());
                        for (unsigned int i = 0; i < data_byte.size(); i++)
                        {
                            dataPacket.data[i] = to_integer<uint8_t>(data_byte[i]);
                        }
                    }

                    _data_out.write(dataPacket);
                });

            mDataChannelMap.emplace(remote_peer_id, data_channel);
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

    mWs->onClosed([]() { LOG_INFO_S << "WebSocket closed" << std::endl; });

    mWs->onMessage(
        [&](variant<binary, string> data)
        {
            if (!holds_alternative<string>(data))
                return;

            if (!parseIncomingMessage(get<string>(data).c_str()))
            {
                return;
            }

            string type = decoder->getType();

            // Ether the peerconnection is already created or we give an offer, creating one.
            if (auto jt = mPeerConnectionMap.find("id"); jt != mPeerConnectionMap.end())
            {
                mPeerConnection = jt->second;
            }
            else if (type == "offer")
            {
                onOffer(mWs);
            }
            else
            {
                return;
            }

            if (type == "answer")
            {
                onAnswer();
            }
            else if (type == "candidate")
            {
                onCandidate();
            }
        });

    // wss://signalserverhost?user=yourname
    const string url = _websocket_server_name.get() + "?user=" + _local_peer_id.get();
    mWs->open(url);
    ws_future.get();
}

Task::Task(string const& name) : TaskBase(name) {}

Task::~Task() {}

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Task.hpp for more detailed
// documentation about them.

bool Task::configureHook()
{
    if (!TaskBase::configureHook())
        return false;

    mConfig.iceServers.emplace_back(_stun_server.get());
    mLocalPeerId = _local_peer_id.get();

    configureWebSocket();

    return true;
}
bool Task::startHook()
{
    if (!TaskBase::startHook())
        return false;
    return true;
}
void Task::updateHook()
{
    RawPacket raw_packet;
    if (_data_in.read(raw_packet) == RTT::NewData && !_remote_peer_id.get().empty())
    {
        shared_ptr<rtc::PeerConnection> peer_connection =
            createPeerConnection(mWs, _remote_peer_id.get());

        // We are the offerer, so create a data channel to initiate the process
        string label = _data_channel_label.get();
        LOG_INFO_S << "Creating DataChannel with label \"" << label << "\"" << std::endl;
        shared_ptr<rtc::DataChannel> data_channel =
            peer_connection->createDataChannel(label);

        peer_connection->onDataChannel(
            [&data_channel, raw_packet](shared_ptr<rtc::DataChannel> incoming)
            {
                data_channel = incoming;
                vector<byte> data;
                data.resize(raw_packet.data.size());
                for(unsigned int i=0;i<raw_packet.data.size();i++)
                {
                    data[i] = static_cast<byte>(raw_packet.data[i]);
                }
                data_channel->send(&data.front(), data.size());
            });
    }
    // TODO - If we have data_in, but we don't have remote peer id.

    TaskBase::updateHook();
}
void Task::errorHook() { TaskBase::errorHook(); }
void Task::stopHook() { TaskBase::stopHook(); }
void Task::cleanupHook()
{
    mDataChannelMap.clear();
    mPeerConnectionMap.clear();

    TaskBase::cleanupHook();
}
