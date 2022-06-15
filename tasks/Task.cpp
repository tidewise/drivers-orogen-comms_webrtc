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

void Task::onOffer(rtc::shared_ptr<rtc::WebSocket> wws)
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

    mPeerConnection = createPeerConnection(wws, remote_peer_id);

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
    shared_ptr<rtc::WebSocket> const& wws,
    std::string const& remote_peer_id)
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
            message["description"] = std::string(description);

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
            message["candidate"] = std::string(candidate);
            message["mid"] = candidate.mid();

            if (auto ws = make_weak_ptr(wws).lock())
                ws->send(fast.write(message));
        });

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
                        data_channel->send("Hello from " + mLocalPeerId);
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
                        // TODO
                    }

                    _data_out.write(dataPacket);
                });

            mDataChannelMap.emplace(remote_peer_id, data_channel);
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

    mConfig.iceServers.emplace_back(_ice_server.get());

    mWs = std::make_shared<rtc::WebSocket>();
    mLocalPeerId = _local_peer_id.get();
    std::promise<void> ws_promise;
    auto ws_future = ws_promise.get_future();

    mWs->onOpen(
        [&ws_promise]()
        {
            LOG_INFO_S << "WebSocket connected, signaling ready" << std::endl;
            ws_promise.set_value();
        });

    mWs->onError(
        [&ws_promise](std::string s)
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

            if (!parseIncomingMessage(get<std::string>(data).c_str()))
            {
                return;
            }

            std::string type;
            if (!getTypeFromMessage(type))
            {
                return;
            }

            // Or the peerconnection is already created or we give a offer, creating one.
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
    ws_future.get();

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
        const std::string label = _data_channel_label.get();
        LOG_INFO_S << "Creating DataChannel with label \"" << label << "\"" << std::endl;
        shared_ptr<rtc::DataChannel> data_channel =
            peer_connection->createDataChannel(label);

        peer_connection->onDataChannel(
            [&data_channel, raw_packet](std::shared_ptr<rtc::DataChannel> incoming)
            {
                data_channel = incoming;
                // TODO
                // data_channel->send();
            });
    }

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
