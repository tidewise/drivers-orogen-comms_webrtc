#ifndef comms_webrtc_TYPES_HPP
#define comms_webrtc_TYPES_HPP

#include "json/json.h"
#include "stdio.h"
#include "string.h"
#include "memory"

namespace comms_webrtc {

    enum ConnectionState
    {
        NewConnection,
        Connecting,
        Connected,
        Disconnected,
        Failed,
        Closed
    };

    enum GatheringState
    {
        NewGathering,
        InProgress,
        Complete
    };

    enum SignalingState
    {
        Stable,
        HaveLocalOffer,
        HaveRemoteOffer,
        HaveLocalPranswer,
        HaveRemotePranswer
    };

    struct PeerConnection
    {
        ConnectionState state;
        GatheringState gathering_state;
        SignalingState signaling_state;
    };

    struct MessageDecoder
    {
        Json::Value jdata;
        Json::CharReaderBuilder rbuilder;
        std::unique_ptr<Json::CharReader> const reader;

        MessageDecoder() : reader(rbuilder.newCharReader()) {}

        bool parseJSONMessage(char const* data, std::string& errors)
        {
            return reader->parse(data, data + strlen(data), &jdata, &errors);
        }

        void validateFieldPresent(std::string const& fieldName)
        {
            if (!jdata.isMember(fieldName))
            {
                std::invalid_argument(
                    "message does not contain the " + fieldName + " field");
            }
        }

        std::string getProtocol()
        {
            validateFieldPresent("protocol");
            return jdata["protocol"].asString();
        }

        std::string getActionType()
        {
            validateFieldPresent("actiontype");
            return jdata["actiontype"].asString();
        }

        std::string getId()
        {
            validateFieldPresent("to");
            return jdata["to"].asString();
        }

        std::string getDescription()
        {
            validateFieldPresent("description");
            return jdata["description"].asString();
        }

        std::string getCandidate()
        {
            validateFieldPresent("candidate");
            return jdata["candidate"].asString();
        }

        std::string getMid()
        {
            validateFieldPresent("mid");
            return jdata["mid"].asString();
        }
    };
}

#endif

