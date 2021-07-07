/* Generated from orogen/lib/orogen/templates/tasks/Task.hpp */

#ifndef COMMS_WEBRTC_TASK_TASK_HPP
#define COMMS_WEBRTC_TASK_TASK_HPP

#include "comms_webrtc/TaskBase.hpp"
#include "rtc/rtc.hpp"

namespace comms_webrtc{
    struct MessageDecoder;

    class Task : public TaskBase
    {
        friend class TaskBase;

        MessageDecoder *decoder = nullptr;
        std::shared_ptr<rtc::PeerConnection> m_pc;
        std::shared_ptr<rtc::DataChannel> m_dc;
        std::shared_ptr<rtc::WebSocket> m_ws;

        bool parseIncomingMessage(char const* data);
        bool getTypeFromMessage(std::string &out_str);
        bool getIdFromMessage(std::string &out_str);
        bool getPeerIdFromMessage(std::string &out_str);
        bool getDescriptionFromMessage(std::string &out_str);
        bool getCandidateFromMessage(std::string &out_str);
        bool getMidFromMessage(std::string &out_str);

        protected:



        public:
            /** TaskContext constructor for Task
             * \param name Name of the task. This name needs to be unique to make it identifiable via nameservices.
             * \param initial_state The initial TaskState of the TaskContext. Default is Stopped state.
             */
            Task(std::string const& name = "comms_webrtc::Task", TaskCore::TaskState initial_state = Stopped);

            ~Task();

            bool configureHook();

            bool startHook();

            void updateHook();

            void errorHook();

            void stopHook();

            void cleanupHook();
        private:
            std::shared_ptr<rtc::DataChannel> tmpDc;
            std::shared_ptr<rtc::PeerConnection> tmpPc;
            std::string tmpPeerID;
            Json::FastWriter fast;
    };
}

#endif
