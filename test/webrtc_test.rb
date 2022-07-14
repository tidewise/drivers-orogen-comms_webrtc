# frozen_string_literal: true

using_task_library "comms_webrtc"
import_types_from "iodrivers_base"
import_types_from "base"

describe OroGen.comms_webrtc.Task do
    run_live

    attr_reader :task1, :task2

    before do
        syskit_stub_conf(OroGen.comms_webrtc.Task, "task1", "task2")

        @task1 = syskit_deploy(
            OroGen.comms_webrtc.Task.with_conf("task1").deployed_as("task1")
        )
        @task1.properties.stun_server = "stun:stun.l.google.com:19302"
        @task1.properties.local_peer_id = "task1"
        @task1.properties.remote_peer_id = "task2"
        @task1.properties.data_channel_label = "test_data_label"
        @task1.properties.signaling_server_name = "127.0.0.1:3012"

        @task2 = syskit_deploy(
            OroGen.comms_webrtc.Task.with_conf("task2").deployed_as("task2")
        )
        @task2.properties.stun_server = "stun:stun.l.google.com:19302"
        @task2.properties.local_peer_id = "task2"
        @task2.properties.remote_peer_id = ""
        @task2.properties.data_channel_label = "test_data_label"
        @task2.properties.signaling_server_name = "127.0.0.1:3012"
    end

    it "configure and starts the tasks" do
        syskit_configure(task1)
        syskit_configure(task2)

        expect_execution do
            task1.start!
            task2.start!
        end.to do
            emit task1.start_event
            emit task2.start_event
        end

    end
end
