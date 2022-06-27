#ifndef comms_webrtc_TYPES_HPP
#define comms_webrtc_TYPES_HPP

#include "json/json.h"
#include "stdio.h"
#include "string.h"
#include "memory"

namespace comms_webrtc {
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

        void validateFieldPresent(std::string fieldName)
        {
            if (!jdata.isMember(fieldName))
            {
                std::invalid_argument(
                    "message does not contain the " + fieldName + " field");
            }
        }

        void validateDataFieldPresent(std::string fieldName)
        {
            if (!jdata["data"].isMember(fieldName))
            {
                std::invalid_argument(
                    "message does not contain the " + fieldName + " field");
            }
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
            validateDataFieldPresent("description");
            return jdata["data"]["description"].asString();
        }

        std::string getCandidate()
        {
            validateDataFieldPresent("candidate");
            return jdata["data"]["candidate"].asString();
        }

        std::string getMid()
        {
            validateDataFieldPresent("mid");
            return jdata["data"]["mid"].asString();
        }
    };
}

#endif

