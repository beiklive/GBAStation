#pragma once
#include "LocalLink.hpp"
#include <map>
#include <optional>
#include <stdexcept>
#include <functional>

namespace netlink {
// Game-thread input timeline. Transport hands complete immutable inputs here.
class RollbackSession {
public:
    RollbackSession(LocalLink& link,unsigned localSlot,unsigned window)
        :link_(link),local_(localSlot),window_(window) {
        if(localSlot>1 || window>12)throw std::invalid_argument("invalid rollback parameters");
    }
    void publish(unsigned slot,uint64_t frame,uint16_t keys);
    bool advance();
    uint64_t nextFrame() const{return next_;}
    uint64_t confirmedFrames() const{return confirmed_;}
    unsigned rollbacks() const{return rollbacks_;}
    unsigned maxDepth() const{return maxDepth_;}
    // Validation hook: immutable state after each newly confirmed frame.
    void onConfirmed(std::function<void(uint64_t,const LocalLink::Snapshot&)> callback){confirmedCallback_=std::move(callback);}
void onAudio(std::function<void(const std::vector<int16_t>&)> callback){audioCallback_=std::move(callback);}
private:
    struct Frame {
        std::array<std::optional<uint16_t>,2> actual;
        std::array<uint16_t,2> used{};
        LocalLink::Snapshot before;
        std::vector<int16_t> audio;
    };
    LocalLink& link_;
    unsigned local_,window_,rollbacks_=0,maxDepth_=0;
    uint64_t next_=0,confirmed_=0;
    std::array<uint16_t,2> baseKeys_{};
    std::map<uint64_t,Frame> frames_;
    std::function<void(uint64_t,const LocalLink::Snapshot&)> confirmedCallback_;
    std::function<void(const std::vector<int16_t>&)> audioCallback_;
    void execute(uint64_t frame);
    void confirm();
};
}
