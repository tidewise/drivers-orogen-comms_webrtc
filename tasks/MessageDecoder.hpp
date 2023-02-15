#ifndef COMMS_WEBRTC_MESSAGE_DECODER_HPP
#define COMMS_WEBRTC_MESSAGE_DECODER_HPP

#include <json/json.h>
#include <memory>
#include <stdio.h>
#include <string.h>

namespace comms_webrtc {
    struct MessageDecoder {
        Json::Value jdata;
        Json::CharReaderBuilder rbuilder;
        std::unique_ptr<Json::CharReader> const reader;

        MessageDecoder()
            : reader(rbuilder.newCharReader())
        {
        }

        bool parseJSONMessage(std::string const& data, std::string& errors)
        {
            return reader->parse(data.data(),
                data.data() + data.length(),
                &jdata,
                &errors);
        }

        void validateFieldPresent(Json::Value const& value, std::string const& fieldName)
        {
            if (!value.isMember(fieldName)) {
                throw std::invalid_argument(
                    "message does not contain the " + fieldName + " field");
            }
        }

        std::string getActionType()
        {
            validateFieldPresent(jdata, "action");
            return jdata["action"].asString();
        }

        std::string getTo()
        {
            validateFieldPresent(jdata, "to");
            return jdata["to"].asString();
        }

        std::string getFrom()
        {
            validateFieldPresent(jdata, "data");
            validateFieldPresent(jdata["data"], "from");
            return jdata["data"]["from"].asString();
        }

        std::string getDescription()
        {
            validateFieldPresent(jdata, "data");
            validateFieldPresent(jdata["data"], "description");
            return jdata["data"]["description"].asString();
        }

        std::string getCandidate()
        {
            validateFieldPresent(jdata, "data");
            validateFieldPresent(jdata["data"], "candidate");
            return jdata["data"]["candidate"].asString();
        }

        bool isMidFieldPresent()
        {
            return jdata.isMember("mid");
        }

        std::string getMid()
        {
            return jdata["data"]["mid"].asString();
        }
    };
} // namespace comms_webrtc

#endif
