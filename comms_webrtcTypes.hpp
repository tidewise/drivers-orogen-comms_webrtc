#ifndef comms_webrtc_TYPES_HPP
#define comms_webrtc_TYPES_HPP

namespace comms_webrtc
{
    enum ConnectionState
    {
        NewConnection,
        Connecting,
        Connected,
        Disconnected,
        Failed,
        Closed
    };

    enum GatheringState
    {
        NewGathering,
        InProgress,
        Complete
    };

    enum SignalingState
    {
        Stable,
        HaveLocalOffer,
        HaveRemoteOffer,
        HaveLocalPranswer,
        HaveRemotePranswer
    };

    enum LocalDescription
    {
        NoDescription,
        NewDescription
    };

    enum LocalCandidate
    {
        NoCandidate,
        NewCandidate
    };

    struct PeerConnection
    {
        comms_webrtc::ConnectionState state;
        comms_webrtc::GatheringState gathering_state;
        comms_webrtc::SignalingState signaling_state;
        comms_webrtc::LocalDescription local_description;
        comms_webrtc::LocalCandidate local_candidate;
    };

    enum Datachannel
    {
        DcClosed,
        DcOpened,
        DcFailed
    };

    enum WebSocket
    {
        WsClosed,
        WsOpened,
        WsFailed
    };
    struct WebRTCState
    {
        comms_webrtc::PeerConnection peer_connection;
        comms_webrtc::Datachannel data_channel;
        comms_webrtc::WebSocket web_socket;
    };
}

#endif
