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

    it "configure, starts and send message between the tasks" do
        syskit_configure(task1)
        syskit_configure(task2)

        expect_execution do
            task1.start!
            task2.start!
        end.to do
            emit task1.start_event
            emit task2.start_event
        end

        data_in = Types.iodrivers_base.RawPacket.new
        data_in.time = Time.now
        data_in.data = [1, 0, 1, 0, 1, 1, 1, 0]

        data = expect_execution do
            syskit_write task1.data_in_port, data_in
        end.to { have_one_new_sample task2.data_out_port }

        assert_equal data_in.data, data.data

        state_task = Types.comms_webrtc.WebRTCState.new
        state_task.peer_connection.state = :NoConnection
        state_task.peer_connection.gathering_state = :NoGathering
        state_task.peer_connection.signaling_state = :NoSignaling
        state_task.data_channel = :DcClosed
        state_task.web_socket = :WsClosed

        state = expect_execution do
            task1.stop!
            task2.stop!
        end.to do
            emit task1.stop_event
            emit task2.stop_event
            [have_one_new_sample(task1.status_port),
             have_one_new_sample(task2.status_port)]
        end

        assert_equal state_task, state[0]
        assert_equal state_task, state[1]
    end
end
