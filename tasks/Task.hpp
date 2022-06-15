/* Generated from orogen/lib/orogen/templates/tasks/Task.hpp */

#ifndef COMMS_WEBRTC_TASK_TASK_HPP
#define COMMS_WEBRTC_TASK_TASK_HPP

#include "base-logging/Logging.hpp"
#include "comms_webrtc/TaskBase.hpp"
#include "rtc/rtc.hpp"
#include "json/json.h"

namespace comms_webrtc
{
    struct MessageDecoder;

    /*! \class Task
     * \brief The task context provides and requires services. It uses an ExecutionEngine
     to perform its functions.
     * Essential interfaces are operations, data flow ports and properties. These
     interfaces have been defined using the oroGen specification.
     * In order to modify the interfaces you should (re)use oroGen and rely on the
     associated workflow.
     *
     * \details
     * The name of a TaskContext is primarily defined via:
     \verbatim
     deployment 'deployment_name'
         task('custom_task_name','comms_webrtc::Task')
     end
     \endverbatim
     *  It can be dynamically adapted when the deployment is called with a prefix
     argument.
     */
    class Task : public TaskBase
    {
        friend class TaskBase;

      protected:
      public:
        /** TaskContext constructor for Task
         * \param name Name of the task. This name needs to be unique to make it
         * identifiable via nameservices. \param initial_state The initial TaskState of
         * the TaskContext. Default is Stopped state.
         */
        Task(std::string const& name = "comms_webrtc::Task");

        /** Default deconstructor of Task
         */
        ~Task();

        /** This hook is called by Orocos when the state machine transitions
         * from PreOperational to Stopped. If it returns false, then the
         * component will stay in PreOperational. Otherwise, it goes into
         * Stopped.
         *
         * It is meaningful only if the #needs_configuration has been specified
         * in the task context definition with (for example):
         \verbatim
         task_context "TaskName" do
           needs_configuration
           ...
         end
         \endverbatim
         */
        bool configureHook();

        /** This hook is called by Orocos when the state machine transitions
         * from Stopped to Running. If it returns false, then the component will
         * stay in Stopped. Otherwise, it goes into Running and updateHook()
         * will be called.
         */
        bool startHook();

        /** This hook is called by Orocos when the component is in the Running
         * state, at each activity step. Here, the activity gives the "ticks"
         * when the hook should be called.
         *
         * The error(), exception() and fatal() calls, when called in this hook,
         * allow to get into the associated RunTimeError, Exception and
         * FatalError states.
         *
         * In the first case, updateHook() is still called, and recover() allows
         * you to go back into the Running state.  In the second case, the
         * errorHook() will be called instead of updateHook(). In Exception, the
         * component is stopped and recover() needs to be called before starting
         * it again. Finally, FatalError cannot be recovered.
         */
        void updateHook();

        /** This hook is called by Orocos when the component is in the
         * RunTimeError state, at each activity step. See the discussion in
         * updateHook() about triggering options.
         *
         * Call recover() to go back in the Runtime state.
         */
        void errorHook();

        /** This hook is called by Orocos when the state machine transitions
         * from Running to Stopped after stop() has been called.
         */
        void stopHook();

        /** This hook is called by Orocos when the state machine transitions
         * from Stopped to PreOperational, requiring the call to configureHook()
         * before calling start() again.
         */
        void cleanupHook();

      private:
        rtc::Configuration mConfig;
        std::shared_ptr<rtc::WebSocket> mWs;
        std::string mLocalPeerId;
        MessageDecoder* decoder = nullptr;
        std::shared_ptr<rtc::DataChannel> mDataChannel;
        std::shared_ptr<rtc::PeerConnection> mPeerConnection;
        std::unordered_map<std::string, std::shared_ptr<rtc::PeerConnection>>
            mPeerConnectionMap;
        std::unordered_map<std::string, std::shared_ptr<rtc::DataChannel>>
            mDataChannelMap;

        void onOffer(
            std::shared_ptr<rtc::WebSocket> wws);
        void onAnswer(std::shared_ptr<rtc::WebSocket> wws);
        void onCandidate(std::shared_ptr<rtc::WebSocket> wws);
        bool parseIncomingMessage(char const* data);
        bool getTypeFromMessage(std::string& message);
        bool getIdFromMessage(std::string& message);
        bool getPeerIdFromMessage(std::string& message);
        bool getDescriptionFromMessage(std::string& message);
        bool getCandidateFromMessage(std::string& message);
        bool getMidFromMessage(std::string& message);
        std::shared_ptr<rtc::PeerConnection> createPeerConnection(
            std::shared_ptr<rtc::WebSocket> const& wws,
            std::string const& remote_peer_id);
    };
} // namespace comms_webrtc

#endif
