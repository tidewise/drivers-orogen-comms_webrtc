# drivers-orogen-comms_webrtc

WebRTC peer using [rustysignal](https://github.com/rasviitanen/rustysignal) as a signalling server

The peer will send the following messages:

- offer/answer:
  ```
  { protocol: "one-to-one",
    to: "remote-peer-id",
    action: "offer" | "answer",
    data: { "from": "local-peer-id", description: "sdp" }
  }
  ```
- candidate:
  ```
  { protocol: "one-to-one",
    to: "remote-peer-id",
    action: "candidate",
    data: { "from": "local-peer-id", candidate: "candidate", mid: "mid" }
  }
  ```

It expects the same messages coming from the other peer, just switching the peer IDs.