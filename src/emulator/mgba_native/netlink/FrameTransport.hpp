#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <vector>
#include <stdexcept>

namespace netlink {
// Version 2 of the experimental frame-input protocol; incompatible with v8 SIO.
struct FrameMessage {
    enum Type : uint16_t { Hello=1, Input=2, Done=3, Checkpoint=4, Heartbeat=5 };
    Type type;
    uint64_t frame=0, state=0, scheduler=0;
    uint16_t keys=0;
};
class FrameProtocol {
public:
    FrameProtocol(unsigned slot,uint64_t session,std::array<uint8_t,32> manifest,unsigned delay,unsigned window);
    std::vector<uint8_t> hello();
    std::vector<uint8_t> heartbeat();
    std::vector<uint8_t> input(uint64_t frame,uint16_t keys);
    std::vector<uint8_t> done(uint64_t frame,uint64_t state,uint64_t scheduler);
    std::vector<uint8_t> checkpoint(uint64_t frame,uint64_t digest);
    void feed(const uint8_t*,size_t,const std::function<void(const FrameMessage&)>&);
    void endOfStream() const;
    bool ready() const{return ready_;}
    uint64_t session() const{return session_;}
private:
    unsigned slot_,delay_,window_;
    uint64_t session_,incomingFrame_=3,outgoingFrame_=3;
    std::array<uint8_t,32> manifest_;
    uint32_t txSequence_=1,rxSequence_=1;
    bool ready_=false,helloSent_=false,doneReceived_=false,doneSent_=false;
    std::vector<uint8_t> rx_;
    std::vector<uint8_t> encode(FrameMessage::Type,const std::vector<uint8_t>&);
};
// Owns an already connected BSD socket. Nonblocking and bounded on both sides.
// Pump on the owning thread; it never invokes mGBA itself.
class FrameSocket {
public:
    explicit FrameSocket(int fd);
    ~FrameSocket();
    FrameSocket(const FrameSocket&)=delete;
    FrameSocket& operator=(const FrameSocket&)=delete;
    void queue(const std::vector<uint8_t>&);
    void pump(FrameProtocol&,const std::function<void(const FrameMessage&)>&);
    bool closed() const{return closed_;}
    bool drained() const{return tx_.empty();}
private:
    int fd_;
    bool closed_=false;
    std::vector<uint8_t> tx_;
    size_t sent_=0;
};
}
