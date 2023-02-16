# frozen_string_literal: true

using_task_library "comms_webrtc"
import_types_from "iodrivers_base"
import_types_from "base"

describe OroGen.comms_webrtc.Task do
    run_live

    attr_reader :task1, :task2, :rustysignal_pid

    before do
        timeout = Time.now + 1
        server = TCPServer.new(0)
        @rusty_port = server.local_address.ip_port
        server.close
        @rustysignal_pid = Process.spawn("rustysignal", "127.0.0.1:#{@rusty_port}")
        while Time.now < timeout
            begin
                TCPSocket.open("127.0.0.1", @rusty_port)
                break
            rescue Errno::ECONNREFUSED
                sleep(0.01)
            end
        end
    end

    after do
        Process.kill("KILL", @rustysignal_pid)
        Process.waitpid @rustysignal_pid
    end

    def deploy(task_a, task_b, time_out, rusty_address = "127.0.0.1:#{@rusty_port}")
        syskit_stub_conf(OroGen.comms_webrtc.Task, task_a.to_s, task_b.to_s)

        @task1 = syskit_deploy(
            OroGen.comms_webrtc.Task.with_conf(task_a.to_s).deployed_as(task_a.to_s)
        )
        @task1.properties.data_channel_time_out = time_out
        @task1.properties.peer_connection_time_out = time_out
        @task1.properties.wait_remote_peer_time_out = time_out
        @task1.properties.websocket_time_out = time_out
        @task1.properties.stun_server = "stun:stun.l.google.com:19302"
        @task1.properties.local_peer_id = task_a.to_s
        @task1.properties.remote_peer_id = task_b.to_s
        @task1.properties.passive = false
        @task1.properties.data_channel_label = "test_data_label"
        @task1.properties.signaling_server_name = rusty_address

        @task2 = syskit_deploy(
            OroGen.comms_webrtc.Task.with_conf(task_b.to_s).deployed_as(task_b.to_s)
        )
        @task2.properties.data_channel_time_out = time_out
        @task2.properties.peer_connection_time_out = time_out
        @task2.properties.wait_remote_peer_time_out = time_out
        @task2.properties.websocket_time_out = time_out
        @task2.properties.stun_server = "stun:stun.l.google.com:19302"
        @task2.properties.local_peer_id = task_b.to_s
        @task2.properties.remote_peer_id = task_a.to_s
        @task1.properties.passive = true
        @task2.properties.data_channel_label = "test_data_label"
        @task2.properties.signaling_server_name = rusty_address
    end

    def configure_and_start()
        syskit_configure([task1, task2])

        output = expect_execution do
            task1.start!
            task2.start!
        end.to do
            emit task1.start_event
            emit task2.start_event
            [have_one_new_sample(task1.status_port),
             have_one_new_sample(task2.status_port)]
        end
        output
    end

    it "emits start only after the data channel is fully established" do
        deploy("task_a", "task_b", Time.at(2))
        output = configure_and_start

        assert_equal :DataChannelOpened, output[0].data_channel
        assert_equal :DataChannelOpened, output[1].data_channel
        assert_equal :WebSocketOpened, output[0].web_socket
        assert_equal :WebSocketOpened, output[1].web_socket
        assert_equal :NewGathering, output[0].peer_connection.gathering_state
        assert_equal :NewGathering, output[1].peer_connection.gathering_state
    end

    it "transmits data from the active to the passive task once started" do
        deploy("task_a", "task_b", Time.at(2))
        configure_and_start

        data_in = Types.iodrivers_base.RawPacket.new
        data_in.time = Time.now
        data_in.data = [1, 0, 1, 0, 1, 1, 1, 0]

        data = expect_execution do
            syskit_write task1.data_in_port, data_in
        end.to { have_one_new_sample task2.data_out_port }

        assert_equal data_in.data, data.data
    end

    it "transmits data from the passive to the active task once started" do
        deploy("task_a", "task_b", Time.at(2))
        configure_and_start

        data_in_task2 = Types.iodrivers_base.RawPacket.new
        data_in_task2.time = Time.now
        data_in_task2.data = [0, 0, 0, 0, 1, 0, 1, 0]

        output = expect_execution do
            syskit_write task2.data_in_port, data_in_task2
        end.to { have_one_new_sample task1.data_out_port }

        assert_equal data_in_task2.data, output.data
    end

    it "successfully reestablishes connection after a stop/start cycle" do
        deploy("task_a", "task_b", Time.at(2))
        configure_and_start

        expect_execution do
            task1.stop!
            task2.stop!
        end.to do
            emit task1.stop_event
            emit task2.stop_event
        end

        deploy("task_c", "task_d", Time.at(2))
        configure_and_start
    end

    it "disconnects the WebRTC connection on stop" do
        deploy("task_a", "task_b", Time.at(2))
        configure_and_start

        state_task = Types.comms_webrtc.WebRTCState.new
        state_task.peer_connection.state = :NoConnection
        state_task.peer_connection.gathering_state = :NoGathering
        state_task.peer_connection.signaling_state = :NoSignaling
        state_task.data_channel = :NoDataChannel
        state_task.web_socket = :WebSocketClosed

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

    it "returns exception when the wait to find the remote peer id exceeds the timeout" do
        deploy("task_a", "task_b", Time.at(2))
        syskit_configure(task1)

        output = expect_execution do
            task1.start!
        end.to do
            fail_to_start task1
            have_one_new_sample(task1.status_port)
        end

        assert_equal :NoDataChannel, output.data_channel
    end

    it "returns exception when the wait for the datachannel exceeds the timeout" do
        deploy("task_a", "task_b", Time.at(2))
        syskit_configure(task2)

        output = expect_execution do
            task2.start!
        end.to do
            fail_to_start task2
            have_one_new_sample(task2.status_port)
        end

        assert_equal :NoDataChannel, output.data_channel
    end

    it "detects that the channel has been closed from the active side" do
        deploy("task_a", "task_b", Time.at(2))
        configure_and_start

        output = expect_execution do
            task1.stop!
        end.to do
            emit task1.stop_event
            emit task2.exception_event
            [have_one_new_sample(task1.status_port),
             have_one_new_sample(task2.status_port)]
        end

        assert_equal :NoDataChannel, output[0].data_channel
        assert_equal :DataChannelClosed, output[1].data_channel
    end

    it "detects that the channel has been closed from the passive side" do
        deploy("task_a", "task_b", Time.at(2))
        configure_and_start

        output = expect_execution do
            task2.stop!
        end.to do
            emit task2.stop_event
            emit task1.exception_event
            [have_one_new_sample(task2.status_port),
             have_one_new_sample(task1.status_port)]
        end

        assert_equal :NoDataChannel, output[0].data_channel
        assert_equal :DataChannelClosed, output[1].data_channel
    end

    it "returns timeout exception when the websocket connection times out" do
        deploy("task_a", "task_b", Time.at(2), "240.24.15.234")

        expect_execution do
            task1.start!
            task2.start!
        end.to do
            fail_to_start task1
            fail_to_start task2
        end
    end
end
