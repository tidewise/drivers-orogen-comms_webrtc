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

    enum LocalDescriptionState
    {
        NoDescription,
        NewDescription
    };

    enum LocalCandidateState
    {
        NoCandidate,
        NewCandidate
    };

    struct PeerConnectionState
    {
        comms_webrtc::ConnectionState state;
        comms_webrtc::GatheringState gathering_state;
        comms_webrtc::SignalingState signaling_state;
        comms_webrtc::LocalDescriptionState local_description;
        comms_webrtc::LocalCandidateState local_candidate;
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
        comms_webrtc::PeerConnectionState peer_connection;
        comms_webrtc::DatachannelState data_channel;
        comms_webrtc::WebSocketState web_socket;
    };
}

#endif
