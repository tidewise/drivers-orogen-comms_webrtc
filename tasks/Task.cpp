/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"

#include <base-logging/Logging.hpp>
#include <iodrivers_base/RawPacket.hpp>
#include <json/json.h>

using namespace comms_webrtc;

struct comms_webrtc::MessageDecoder {
    Json::Value jdata;
    Json::CharReaderBuilder rbuilder;
    std::unique_ptr<Json::CharReader> const reader;

    MessageDecoder() : reader(rbuilder.newCharReader()) {

    }

    static std::string mapFieldName(Mapping::Type type) {
        if (type == Mapping::Type::Button) {
            return "buttons";
        }
        if (type == Mapping::Type::Axis) {
            return "axes";
        }
        throw "Failed to get Field Name. The type set is invalid";
    }

    bool parseJSONMessage(char const* data, std::string &errors) {
        return reader->parse(data, data+std::strlen(data), &jdata, &errors);
    }

    bool validateType() {
        if (!jdata.isMember("type")) {
            LOG_ERROR_S << "Invalid message, it doesn't contain the type field.";
            LOG_ERROR_S << std::endl;
            return false;
        }
        return true;
    }

    bool validateId() {
        if (!jdata.isMember("id")) {
            LOG_ERROR_S << "Invalid message, it doesn't contain the id field.";
            LOG_ERROR_S << std::endl;
            return false;
        }
        return true;
    }

    bool validatePeerId() {
        if (!jdata.isMember("peer_id")) {
            LOG_ERROR_S << "Invalid message, it doesn't contain the peer_id field.";
            LOG_ERROR_S << std::endl;
            return false;
        }
        return true;
    }

    bool validateDescription() {
        if (!jdata.isMember("description")) {
            LOG_ERROR_S << "Invalid message, it doesn't contain the description field.";
            LOG_ERROR_S << std::endl;
            return false;
        }
        return true;
    }

    bool validateCandidate() {
        if (!jdata.isMember("candidate")) {
            LOG_ERROR_S << "Invalid message, it doesn't contain the candidate field.";
            LOG_ERROR_S << std::endl;
            return false;
        }
        return true;
    }

    bool validateMid() {
        if (!jdata.isMember("mid")) {
            LOG_ERROR_S << "Invalid message, it doesn't contain the mid field.";
            LOG_ERROR_S << std::endl;
            return false;
        }
        return true;
    }

    std::string getType() {
        auto const& field = jdata["type"];
        return field.asString();
    }

    std::string getId( ) {
        auto const& field = jdata["id"];
        return field.asString();
    }

    std::string getPeerId( ) {
        auto const& field = jdata["peer_id"];
        return field.asString();
    }

    std::string getDescription( ) {
        auto const& field = jdata["description"];
        return field.asString();
    }

    std::string getCandidate( ) {
        auto const& field = jdata["candidate"];
        return field.asString();
    }

    std::string getMid( ) {
        auto const& field = jdata["mid"];
        return field.asString();
    }
};

void Task::onOffer(weak_ptr<rtc::WebSocket> wws) {
    tmpPc.reset();

    if (!this->getIdFromMessage(tmpPeerID)) {
        return;
    }

    std::string peerId;
    if (!this->getPeerIdFromMessage(peerId)) {
        return;
    }

    if (peerId != _local_id.get()) {
        return;
    }

    std::string offer;
    if (!this->getDescriptionFromMessage(offer)) {
        return;
    }

    createPeerConnection(wws, tmpPeerID);

    try
    {
        tmpPc->setRemoteDescription(rtc::Description(offer, "offer"));
    }
    catch(const std::logic_error& e)
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

void Task::onAnswer(weak_ptr<rtc::WebSocket> wws) {
    if (!this->getIdFromMessage(tmpPeerID)) {
        return;
    }

    if (tmpPeerID != _peer_id.get()) {
        return;
    }

    std::string peerId;
    if (!this->getPeerIdFromMessage(peerId)) {
        return;
    }

    if (peerId != _local_id.get()) {
        return;
    }

    std::string answer;
    if (!this->getDescriptionFromMessage(answer)) {
        return;
    }

    try
    {
        tmpPc->setRemoteDescription(rtc::Description(answer, "answer"));
    }
    catch(const std::logic_error& e)
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

void Task::onCandidate(weak_ptr<rtc::WebSocket> wws) {
    if (!tmpPc) {
        // Json::Value msg;
        // msg["type"] = "candidate";
        // msg["error"] = "must send offer before candidates";
        // if (auto ws = wws.lock())
		// 	ws->send(fast.write(msg));
        return;
    }

    std::string id;
    if (!this->getIdFromMessage(id)) {
        return;
    }

    if (id != tmpPeerID) {
        return;
    }

    std::string peerId;
    if (!this->getPeerIdFromMessage(peerId)) {
        return;
    }

    if (peerId != _local_id.get()) {
        return;
    }

    std::string candidate;
    if (!this->getCandidateFromMessage(candidate)) {
        return;
    }

    std::string mid;
    if (!this->getMidFromMessage(mid)) {
        return;
    }

    try
    {
        tmpPc->addRemoteCandidate(rtc::Candidate(candidate, mid));
    }
    catch(const std::logic_error& e)
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

void Task::createPeerConnection(weak_ptr<rtc::WebSocket> wws, string peerId) {
    rtc::Configuration config;
    config.iceServers.emplace_back(_ice_server.get());

    tmpPc = make_shared<PeerConnection>(config);

    tmpPc->onStateChange([&this, tmpPc, tmpDc, peerId](PeerConnection::State state) {
        if (state == PeerConnection::State::Connected) {
            this->m_pc = std::move(tmpPc);
            this->m_dc = std::move(tmpDc);

            // handleNewConnection(pending, controlling);
            // controlling = pending();
            // pending = Client();
        }
    });

    // Send local description to the remote peer
    tmpPc->onLocalDescription([wws, peerId](rtc::Description description) {
        Json::Value msg;
        msg["type"] = description.typeString();
        msg["id"] = _local_id.get();
        msg["peer_id"] = peerId;
        msg["description"] = string(description);

        if (auto ws = wws.lock())
			ws->send(fast.write(msg));
    });
 
    // Send candidate to the remote peer
    tmpPc->onLocalCandidate([wws, peerId](rtc::Candidate candidate) {
        Json::Value msg;
        msg["type"] = "candidate";
        msg["id"] = _local_id.get();
        msg["peer_id"] = peerId;
        msg["candidate"] = string(candidate);
        msg["mid"] = candidate.mid();
        if (auto ws = wws.lock())
			ws->send(fast.write(msg));
    });

    tmpPc->onDataChannel([&this, &tmpDc](std::shared_ptr<rtc::DataChannel> incoming) {
        tmpDc = incoming;
        specifyDataChannel();
    });
}

void Task::specifyDataChannel() {
        tmpDc->onOpen([wdc = make_weak_ptr(tmpDc)]() {
            if (auto dc = wdc.lock())
                dc->send("Hello from " + _local_id.get() );
        });

        tmpDc->onClosed([]() { cout << "DataChannel from " << _local_id.get() << " closed" << endl; });

        tmpDc->onMessage([&this](variant<binary, string> data) {
            RawPacket dataPacket;
            dataPacket.time = base::Time.now()

            if (holds_alternative<string>(data)) {
                std::vector<uint8_t> v(data.begin(), data.end());
                dataPacket.data = v;
            }
            else {
                dataPacket.data = data;
            }

            this->_data_out.write(dataPacket);
        });
}

bool Task::parseIncomingMessage(char const* data) {
    std::string errs;
    if (!decoder->parseJSONMessage(data, errs)) {
        LOG_ERROR_S << "Failed parsing the message, got error: " << errs << std::endl;
        return false;
    }
    return true;
}

bool Task::getTypeFromMessage(std::string &out_str) {
    if (!decoder->validateType()) {
        return false;
    }
    out_str = decoder->getType();
    return true;
}

bool Task::getIdFromMessage(std::string &out_str) {
    if (!decoder->validateId()) {
        return false;
    }
    out_str = decoder->getId();
    return true;
}

bool Task::getPeerIdFromMessage(std::string &out_str) {
    if (!decoder->validatePeerId()) {
        return false;
    }
    out_str = decoder->getPeerId();
    return true;
}

bool Task::getDescriptionFromMessage(std::string &out_str) {
    if (!decoder->validateDescription()) {
        return false;
    }
    out_str = decoder->getDescription();
    return true;
}

bool Task::getCandidateFromMessage(std::string &out_str) {
    if (!decoder->validateCandidate()) {
        return false;
    }
    out_str = decoder->getCandidate();
    return true;
}

bool Task::getMidFromMessage(std::string &out_str) {
    if (!decoder->validateMid()) {
        return false;
    }
    out_str = decoder->getMid();
    return true;
}

Task::Task(std::string const& name, TaskCore::TaskState initial_state)
    : TaskBase(name, initial_state)
{
}

Task::~Task()
{
}

bool Task::configureHook()
{
    if (! TaskBase::configureHook())
        return false;

    m_ws = make_shared<rtc::WebSocket>();

	std::promise<void> wsPromise;
	auto wsFuture = wsPromise.get_future();

	m_ws->onOpen([&wsPromise]() {
		cout << "WebSocket connected, signaling ready" << endl;
		wsPromise.set_value();
	});

	m_ws->onError([&wsPromise](string s) {
		cout << "WebSocket error" << endl;
		wsPromise.set_exception(std::make_exception_ptr(std::runtime_error(s)));
	});

	m_ws->onClosed([]() { cout << "WebSocket closed" << endl; });

	m_ws->onMessage([&](variant<binary, string> data) {
		if (!holds_alternative<string>(data))
			return;

        if (!task->parseIncomingMessage(data.c_str())) {
            return;
        }

        std::string type;
        if (!task->getTypeFromMessage(type)) {
            return;
        }

        switch (type) {
            case "offer":
                onOffer(m_ws);
                break;
            case "answer":
                onAnswer(m_ws);
                break;
            case "candidate":
                onCandidate(m_ws);
                break;
            default:
                break;
        }
	});

	string wsPrefix = "";
	if (_websocket_server.get().substr(0, 5).compare("ws://") != 0) {
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
    if (! TaskBase::startHook())
        return false;

    if (!_peer_id.get().empty() && !m_pc) {
        createPeerConnection(m_ws, _peer_id.get());

        const string label = "test";
        tmpDc = tmpPc->createDataChannel(_dc_label.get());

        specifyDataChannel();
    }

    return true;
}
void Task::updateHook()
{
    TaskBase::updateHook();

    RawPacket packet;
    if (m_dc) {
        while (_data_in.read(packet) == RTT::NewData) {
            m_dc->send(packet.data);
        }
    }
}
void Task::errorHook()
{
    TaskBase::errorHook();
}
void Task::stopHook()
{
    TaskBase::stopHook();
}
void Task::cleanupHook()
{
    TaskBase::cleanupHook();

    delete decoder;
}
