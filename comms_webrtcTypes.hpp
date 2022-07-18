#ifndef comms_webrtc_TYPES_HPP
#define comms_webrtc_TYPES_HPP

namespace comms_webrtc
{
    enum ConnectionState
    {
        NoConnection,
        NewConnection,
        Connecting,
        Connected,
        Disconnected,
        Failed,
        Closed
    };

    enum GatheringState
    {
        NoGathering,
        NewGathering,
        InProgress,
        Complete
    };

    enum SignalingState
    {
        NoSignaling,
        Stable,
        HaveLocalOffer,
        HaveRemoteOffer,
        HaveLocalPranswer,
        HaveRemotePranswer
    };

    struct PeerConnectionState
    {
        ConnectionState state = NoConnection;
        GatheringState gathering_state = NoGathering;
        SignalingState signaling_state = NoSignaling;
    };

    enum DatachannelState
    {
        DcClosed,
        DcOpened,
        DcFailed
    };

    enum WebSocketState
    {
        WsClosed,
        WsOpened,
        WsFailed
    };
    struct WebRTCState
    {
        PeerConnectionState peer_connection;
        DatachannelState data_channel = DcClosed;
        WebSocketState web_socket = WsClosed;
    };
}

#endif
