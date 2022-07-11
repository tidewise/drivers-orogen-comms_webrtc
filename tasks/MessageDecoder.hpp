#ifndef COMMS_WEBRTC_MESSAGE_DECODER_HPP
#define COMMS_WEBRTC_MESSAGE_DECODER_HPP

#include "json/json.h"
#include "stdio.h"
#include "string.h"
#include "memory"

namespace comms_webrtc
{
    struct MessageDecoder
    {
        Json::Value jdata;
        Json::CharReaderBuilder rbuilder;
        std::unique_ptr<Json::CharReader> const reader;

        MessageDecoder() : reader(rbuilder.newCharReader()) {}

        bool parseJSONMessage(char const *data, std::string &errors)
        {
            return reader->parse(data, data + strlen(data), &jdata, &errors);
        }

        void validateFieldPresent(std::string const &fieldName)
        {
            if (!jdata.isMember(fieldName))
            {
                throw std::invalid_argument(
                    "message does not contain the " + fieldName + " field");
            }
        }

        void validateDataFieldPresent(std::string const &fieldName)
        {
            if (!jdata["data"].isMember(fieldName))
            {
                throw std::invalid_argument(
                    "message does not contain the " + fieldName + " field");
            }
        }

        std::string getActionType()
        {
            validateFieldPresent("action");
            return jdata["action"].asString();
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
