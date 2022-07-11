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
        @task1.properties.protocol = "one-to-one"
        @task1.properties.stun_server = "stun:stun.l.google.com:19302"
        @task1.properties.local_peer_id = "task1"
        @task1.properties.remote_peer_id = "task2"
        @task1.properties.data_channel_label = "test_data_label"
        @task1.properties.websocket_server_name = "127.0.0.1:3012"

        @task2 = syskit_deploy(
            OroGen.comms_webrtc.Task.with_conf("task2").deployed_as("task2")
        )
        @task2.properties.protocol = "one-to-one"
        @task2.properties.stun_server = "stun:stun.l.google.com:19302"
        @task2.properties.local_peer_id = "task2"
        @task2.properties.remote_peer_id = ""
        @task2.properties.data_channel_label = "test_data_label"
        @task2.properties.websocket_server_name = "127.0.0.1:3012"
    end

    it "changes message between the tasks" do
        syskit_configure_and_start(task1)
        syskit_configure_and_start(task2)

        data_in = Types.iodrivers_base.RawPacket.new
        data_in.time = Time.now
        data_in.data = [1, 0, 1, 0, 1, 1, 1, 0]

        outputs = expect_execution do
            syskit_write task1.data_in_port, data_in
        end.to do
            have_one_new_sample(task2.data_out_port)
        end
    end
end
