/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"

using namespace comms_webrtc;
using namespace std;
using namespace rtc;

template <class T> weak_ptr<T> make_weak_ptr(shared_ptr<T> ptr) { return ptr; }

struct comms_webrtc::MessageDecoder
{
    Json::Value jdata;
    Json::CharReaderBuilder rbuilder;
    std::unique_ptr<Json::CharReader> const reader;

    MessageDecoder() : reader(rbuilder.newCharReader()) {}

    bool parseJSONMessage(char const* data, std::string& errors)
    {
        return reader->parse(data, data + std::strlen(data), &jdata, &errors);
    }

    bool validateType()
    {
        if (!jdata.isMember("type"))
        {
            LOG_ERROR_S << "Invalid message, it doesn't contain the type field.";
            LOG_ERROR_S << std::endl;
            return false;
        }
        return true;
    }

    bool validateId()
    {
        if (!jdata.isMember("id"))
        {
            LOG_ERROR_S << "Invalid message, it doesn't contain the id field.";
            LOG_ERROR_S << std::endl;
            return false;
        }
        return true;
    }

    bool validateDescription()
    {
        if (!jdata.isMember("description"))
        {
            LOG_ERROR_S << "Invalid message, it doesn't contain the description field.";
            LOG_ERROR_S << std::endl;
            return false;
        }
        return true;
    }

    bool validateCandidate()
    {
        if (!jdata.isMember("candidate"))
        {
            LOG_ERROR_S << "Invalid message, it doesn't contain the candidate field.";
            LOG_ERROR_S << std::endl;
            return false;
        }
        return true;
    }

    bool validateMid()
    {
        if (!jdata.isMember("mid"))
        {
            LOG_ERROR_S << "Invalid message, it doesn't contain the mid field.";
            LOG_ERROR_S << std::endl;
            return false;
        }
        return true;
    }

    std::string getType() { return jdata["type"].asString(); }

    std::string getId() { return jdata["id"].asString(); }

    std::string getDescription() { return jdata["description"].asString(); }

    std::string getCandidate() { return jdata["candidate"].asString(); }

    std::string getMid() { return jdata["mid"].asString(); }
};

void Task::onOffer(rtc::Configuration const& config, rtc::shared_ptr<rtc::WebSocket> wws, string local_peer_id)
{
    std::string remote_peer_id;
    if (!getIdFromMessage(remote_peer_id))
    {
        return;
    }

    std::string offer;
    if (!getDescriptionFromMessage(offer))
    {
        return;
    }

    createPeerConnection(config, wws, local_peer_id, remote_peer_id);

    try
    {
        mPeerConnection->setRemoteDescription(rtc::Description(offer, "offer"));
    }
    catch (const std::logic_error& error)
    {
        mPeerConnection.reset();
        LOG_ERROR_S << error.what();
        LOG_ERROR_S << std::endl;
    }
}

void Task::onAnswer(shared_ptr<rtc::WebSocket> wws)
{
    std::string answer;
    if (!getDescriptionFromMessage(answer))
    {
        return;
    }

    try
    {
        mPeerConnection->setRemoteDescription(rtc::Description(answer, "answer"));
    }
    catch (const std::logic_error& error)
    {
        mPeerConnection.reset();
        LOG_ERROR_S << error.what();
        LOG_ERROR_S << std::endl;
    }
}

void Task::onCandidate(shared_ptr<rtc::WebSocket> wws)
{
    std::string candidate;
    if (!getCandidateFromMessage(candidate))
    {
        return;
    }

    std::string mid;
    if (!getMidFromMessage(mid))
    {
        return;
    }

    try
    {
        mPeerConnection->addRemoteCandidate(rtc::Candidate(candidate, mid));
    }
    catch (const std::logic_error& error)
    {
        mPeerConnection.reset();
        LOG_ERROR_S << error.what();
        LOG_ERROR_S << std::endl;
    }
}

bool Task::parseIncomingMessage(char const* data)
{
    std::string error;
    if (!decoder->parseJSONMessage(data, error))
    {
        LOG_ERROR_S << "Failed parsing the message, got error: " << error << std::endl;
        return false;
    }
    return true;
}

bool Task::getTypeFromMessage(std::string& message)
{
    if (!decoder->validateType())
    {
        return false;
    }
    message = decoder->getType();
    return true;
}

bool Task::getIdFromMessage(std::string& message)
{
    if (!decoder->validateId())
    {
        return false;
    }
    message = decoder->getId();
    return true;
}

bool Task::getDescriptionFromMessage(std::string& message)
{
    if (!decoder->validateDescription())
    {
        return false;
    }
    message = decoder->getDescription();
    return true;
}

bool Task::getCandidateFromMessage(std::string& message)
{
    if (!decoder->validateCandidate())
    {
        return false;
    }
    message = decoder->getCandidate();
    return true;
}

bool Task::getMidFromMessage(std::string& message)
{
    if (!decoder->validateMid())
    {
        return false;
    }
    message = decoder->getMid();
    return true;
}

// Create and setup a PeerConnection
shared_ptr<rtc::PeerConnection> Task::createPeerConnection(
    rtc::Configuration const& config,
    shared_ptr<rtc::WebSocket> const& wws,
    string const& local_peer_id,
    string const& remote_peer_id)
{
    auto peer_connection = std::make_shared<rtc::PeerConnection>(config);

    peer_connection->onStateChange([](rtc::PeerConnection::State state)
                                   { LOG_INFO_S << "State: " << state << std::endl; });

    peer_connection->onGatheringStateChange(
        [](rtc::PeerConnection::GatheringState state)
        { LOG_INFO_S << "Gathering State: " << state << std::endl; });

    peer_connection->onLocalDescription(
        [wws, remote_peer_id](rtc::Description description)
        {
            Json::Value message;
            Json::FastWriter fast;
            message["id"] = remote_peer_id;
            message["type"] = description.typeString();
            message["description"] = std::string(description);

            if (auto ws = make_weak_ptr(wws).lock())
                ws->send(fast.write(message));
        });

    peer_connection->onLocalCandidate(
        [wws, remote_peer_id](rtc::Candidate candidate)
        {
            Json::Value message;
            Json::FastWriter fast;
            message["id"] = remote_peer_id;
            message["type"] = "candidate";
            message["candidate"] = std::string(candidate);
            message["mid"] = candidate.mid();

            if (auto ws = make_weak_ptr(wws).lock())
                ws->send(fast.write(message));
        });

    unordered_map<std::string, std::shared_ptr<rtc::DataChannel>> data_channel_map =
        mDataChannelMap;
    peer_connection->onDataChannel(
        [local_peer_id, remote_peer_id, &data_channel_map](
            shared_ptr<rtc::DataChannel> data_channel)
        {
            LOG_INFO_S << "DataChannel from " << remote_peer_id
                       << " received with label \"" << data_channel->label() << "\""
                       << std::endl;

            data_channel->onOpen(
                [local_peer_id, wdc = make_weak_ptr(data_channel)]()
                {
                    if (auto data_channel = wdc.lock())
                        data_channel->send("Hello from " + local_peer_id);
                });

            data_channel->onMessage(
                [remote_peer_id](auto data)
                {
                    // data holds either std::string or rtc::binary
                    if (std::holds_alternative<std::string>(data))
                        LOG_INFO_S << "Message from " << remote_peer_id
                                   << " received: " << std::get<std::string>(data)
                                   << std::endl;
                    else
                        LOG_INFO_S
                            << "Binary message from " << remote_peer_id
                            << " received, size=" << std::get<rtc::binary>(data).size()
                            << std::endl;
                });

            data_channel_map.emplace(remote_peer_id, data_channel);
        });

    mPeerConnectionMap.emplace(remote_peer_id, peer_connection);
    return peer_connection;
};

Task::Task(std::string const& name) : TaskBase(name) {}

Task::~Task() {}

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Task.hpp for more detailed
// documentation about them.

bool Task::configureHook()
{
    if (!TaskBase::configureHook())
        return false;

    rtc::Configuration config;
    config.iceServers.emplace_back(_ice_server.get());

    mWs = std::make_shared<rtc::WebSocket>();
    mLocalPeerId = _local_peer_id.get();
    std::promise<void> wsPromise;
    auto wsFuture = wsPromise.get_future();

    mWs->onOpen(
        [&wsPromise]()
        {
            LOG_INFO_S << "WebSocket connected, signaling ready" << std::endl;
            wsPromise.set_value();
        });

    mWs->onMessage(
        [&](variant<binary, string> data)
        {
            if (!holds_alternative<string>(data))
                return;

            if (!parseIncomingMessage(get<std::string>(data).c_str()))
            {
                return;
            }

            std::string type;
            if (!getTypeFromMessage(type))
            {
                return;
            }

            if (type == "offer")
            {
                onOffer(config, mWs, mLocalPeerId);
            }
            else if (type == "answer")
            {
                onAnswer(mWs);
            }
            else if (type == "candidate")
            {
                onCandidate(mWs);
            }
        });

    // wss://signalserverhost?user=yourname
    const string url = _websocket_server_name.get() + "?user=" + _local_peer_id.get();
    mWs->open(url);
    wsFuture.get();

    return true;
}
bool Task::startHook()
{
    if (!TaskBase::startHook())
        return false;
    return true;
}
void Task::updateHook() { TaskBase::updateHook(); }
void Task::errorHook() { TaskBase::errorHook(); }
void Task::stopHook() { TaskBase::stopHook(); }
void Task::cleanupHook()
{
    mDataChannelMap.clear();
    mPeerConnectionMap.clear();

    TaskBase::cleanupHook();
}
