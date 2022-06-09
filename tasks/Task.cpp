/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"

using namespace comms_webrtc;
using namespace std;
using namespace rtc;

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

    bool validatePeerId()
    {
        if (!jdata.isMember("peer_id"))
        {
            LOG_ERROR_S << "Invalid message, it doesn't contain the peer_id field.";
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

    std::string getPeerId() { return jdata["peer_id"].asString(); }

    std::string getDescription() { return jdata["description"].asString(); }

    std::string getCandidate() { return jdata["candidate"].asString(); }

    std::string getMid() { return jdata["mid"].asString(); }
};

void Task::onOffer(shared_ptr<rtc::WebSocket> wws)
{
    tmpPc.reset();

    if (!getIdFromMessage(tmpPeerID))
    {
        return;
    }

    std::string peerId;
    if (!getPeerIdFromMessage(peerId))
    {
        return;
    }

    if (peerId != _local_id.get())
    {
        return;
    }

    std::string offer;
    if (!getDescriptionFromMessage(offer))
    {
        return;
    }

    // createPeerConnection(wws, tmpPeerID);

    try
    {
        tmpPc->setRemoteDescription(rtc::Description(offer, "offer"));
    }
    catch (const std::logic_error& e)
    {
        tmpPc.reset();
        LOG_ERROR_S << e.what();
        LOG_ERROR_S << std::endl;
        // Json::Value msg;
        // msg["type"] = "offer";
        // msg["error"] = e.what();
        // if (auto ws = wws.lock())
        // 	ws->send(fast.write(msg));
    }
}

void Task::onAnswer(shared_ptr<rtc::WebSocket> wws)
{
    if (!getIdFromMessage(tmpPeerID))
    {
        return;
    }

    if (tmpPeerID != _peer_id.get())
    {
        return;
    }

    std::string peerId;
    if (!getPeerIdFromMessage(peerId))
    {
        return;
    }

    if (peerId != _local_id.get())
    {
        return;
    }

    std::string answer;
    if (!getDescriptionFromMessage(answer))
    {
        return;
    }

    try
    {
        tmpPc->setRemoteDescription(rtc::Description(answer, "answer"));
    }
    catch (const std::logic_error& e)
    {
        tmpPc.reset();
        LOG_ERROR_S << e.what();
        LOG_ERROR_S << std::endl;
        // Json::Value msg;
        // msg["type"] = "answer";
        // msg["error"] = e.what();
        // if (auto ws = wws.lock())
        // 	ws->send(fast.write(msg));
    }
}

void Task::onCandidate(shared_ptr<rtc::WebSocket> wws)
{
    if (!tmpPc)
    {
        // Json::Value msg;
        // msg["type"] = "candidate";
        // msg["error"] = "must send offer before candidates";
        // if (auto ws = wws.lock())
        // 	ws->send(fast.write(msg));
        return;
    }

    std::string id;
    if (!getIdFromMessage(id))
    {
        return;
    }

    if (id != tmpPeerID)
    {
        return;
    }

    std::string peerId;
    if (!getPeerIdFromMessage(peerId))
    {
        return;
    }

    if (peerId != _local_id.get())
    {
        return;
    }

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
        tmpPc->addRemoteCandidate(rtc::Candidate(candidate, mid));
    }
    catch (const std::logic_error& e)
    {
        LOG_ERROR_S << e.what();
        LOG_ERROR_S << std::endl;
        // Json::Value msg;
        // msg["type"] = "candidate";
        // msg["error"] = e.what();
        // if (auto ws = wws.lock())
        // 	ws->send(fast.write(msg));
    }
}

bool Task::parseIncomingMessage(char const* data)
{
    std::string errs;
    if (!decoder->parseJSONMessage(data, errs))
    {
        LOG_ERROR_S << "Failed parsing the message, got error: " << errs << std::endl;
        return false;
    }
    return true;
}

bool Task::getTypeFromMessage(std::string& out_str)
{
    if (!decoder->validateType())
    {
        return false;
    }
    out_str = decoder->getType();
    return true;
}

bool Task::getIdFromMessage(std::string& out_str)
{
    if (!decoder->validateId())
    {
        return false;
    }
    out_str = decoder->getId();
    return true;
}

bool Task::getPeerIdFromMessage(std::string& out_str)
{
    if (!decoder->validatePeerId())
    {
        return false;
    }
    out_str = decoder->getPeerId();
    return true;
}

bool Task::getDescriptionFromMessage(std::string& out_str)
{
    if (!decoder->validateDescription())
    {
        return false;
    }
    out_str = decoder->getDescription();
    return true;
}

bool Task::getCandidateFromMessage(std::string& out_str)
{
    if (!decoder->validateCandidate())
    {
        return false;
    }
    out_str = decoder->getCandidate();
    return true;
}

bool Task::getMidFromMessage(std::string& out_str)
{
    if (!decoder->validateMid())
    {
        return false;
    }
    out_str = decoder->getMid();
    return true;
}

// // Create and setup a PeerConnection
// shared_ptr<rtc::PeerConnection> Task::createPeerConnection(const rtc::Configuration
// &config,
//                                                      shared_ptr<rtc::WebSocket> wws,
//                                                      std::string id) {
// 	auto pc = std::make_shared<rtc::PeerConnection>(config);

// 	pc->onStateChange(
// 	    [](rtc::PeerConnection::State state) { std::cout << "State: " << state <<
// std::endl; });

// 	pc->onGatheringStateChange([](rtc::PeerConnection::GatheringState state) {
// 		std::cout << "Gathering State: " << state << std::endl;
// 	});

// 	pc->onLocalDescription([wws, id](rtc::Description description) {
// 		json message = {{"id", id},
// 		                {"type", description.typeString()},
// 		                {"description", std::string(description)}};

// 		if (auto ws = wws.lock())
// 			ws->send(message.dump());
// 	});

// 	pc->onLocalCandidate([wws, id](rtc::Candidate candidate) {
// 		json message = {{"id", id},
// 		                {"type", "candidate"},
// 		                {"candidate", std::string(candidate)},
// 		                {"mid", candidate.mid()}};

// 		if (auto ws = wws.lock())
// 			ws->send(message.dump());
// 	});

// 	pc->onDataChannel([id](shared_ptr<rtc::DataChannel> dc) {
// 		std::cout << "DataChannel from " << id << " received with label \"" << dc->label() <<
// "\""
// 		          << std::endl;

// 		dc->onOpen([wdc = make_weak_ptr(dc)]() {
// 			if (auto dc = wdc.lock())
// 				dc->send("Hello from " + localId);
// 		});

// 		dc->onClosed([id]() { std::cout << "DataChannel from " << id << " closed" <<
// std::endl; });

// 		dc->onMessage([id](auto data) {
// 			// data holds either std::string or rtc::binary
// 			if (std::holds_alternative<std::string>(data))
// 				std::cout << "Message from " << id << " received: " <<
// std::get<std::string>(data)
// 				          << std::endl;
// 			else
// 				std::cout << "Binary message from " << id
// 				          << " received, size=" << std::get<rtc::binary>(data).size() <<
// std::endl;
// 		});

// 		dataChannelMap.emplace(id, dc);
// 	});

// 	peerConnectionMap.emplace(id, pc);
// 	return pc;
// };

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

    m_ws = std::make_shared<rtc::WebSocket>();
    std::promise<void> wsPromise;
    auto wsFuture = wsPromise.get_future();

    m_ws->onOpen(
        [&wsPromise]()
        {
            LOG_INFO_S << "WebSocket connected, signaling ready" << std::endl;
            wsPromise.set_value();
        });

    // m_ws->onMessage([&](variant<binary, string> data) {
    // 	if (!holds_alternative<string>(data))
    // 		return;

    //     if (!this->parseIncomingMessage(get<std::string>(data).c_str())) {
    //         return;
    //     }

    //     std::string type;
    //     if (!this->getTypeFromMessage(type)) {
    //         return;
    //     }

    //     if (type == "offer")
    //     {
    //         onOffer(m_ws);
    //     }
    //     else if (type == "answer")
    //     {
    //         onAnswer(m_ws);
    //     }
    //     else if (type == "candidate")
    //     {
    //         onCandidate(m_ws);
    //     }

    // });

    string wsPrefix = "";
    if (_websocket_server.get().substr(0, 5).compare("ws://") != 0)
    {
        wsPrefix = "ws://";
    }
    const string url = wsPrefix + _websocket_server.get() + ":" +
                       to_string(_websocket_port.get()) + "/" + _local_id.get();

    m_ws->open(url);

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
void Task::cleanupHook() { TaskBase::cleanupHook(); }
