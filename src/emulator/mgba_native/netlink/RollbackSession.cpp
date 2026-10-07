#include "RollbackSession.hpp"
#include <algorithm>

namespace netlink {
void RollbackSession::execute(uint64_t frame) {
    auto& f=frames_.at(frame);
    f.before=link_.capture();
    auto previous=frames_.find(frame ? frame-1 : frame);
    f.used=(frame && previous!=frames_.end())?previous->second.used:baseKeys_;
    for(unsigned slot=0;slot<2;++slot)if(f.actual[slot])f.used[slot]=*f.actual[slot];
    link_.advance(f.used);
    if(audioCallback_)f.audio=link_.audio(local_);
}
void RollbackSession::confirm() {
    while(confirmed_<next_) {
        auto it=frames_.find(confirmed_);
        if(it==frames_.end() || !it->second.actual[0] || !it->second.actual[1])break;
        if(confirmedCallback_){
            auto next=frames_.find(confirmed_+1);
            auto state=confirmed_+1<next_?next->second.before:link_.capture();
            confirmedCallback_(confirmed_,state);
        }
        if(audioCallback_)audioCallback_(it->second.audio);
        baseKeys_=it->second.used;
        frames_.erase(it);++confirmed_;
    }
}
void RollbackSession::publish(unsigned slot,uint64_t frame,uint16_t keys) {
    if(slot>1 || keys>1023 || frame<confirmed_ || (frame>=next_ && frame-next_>=120))
        throw std::invalid_argument("input outside session window");
    auto& f=frames_[frame];
    if(f.actual[slot] && *f.actual[slot]!=keys)throw std::invalid_argument("conflicting session input");
    f.actual[slot]=keys;
    if(frame<next_ && f.used[slot]!=keys) {
        if(!f.before.valid())throw std::runtime_error("missing rollback snapshot");
        auto depth=unsigned(next_-frame);
        if(depth>window_)throw std::runtime_error("rollback depth exceeds window");
        link_.restore(f.before);
        for(uint64_t replay=frame;replay<next_;++replay)execute(replay);
        ++rollbacks_;maxDepth_=std::max(maxDepth_,depth);
    }
    confirm();
}
bool RollbackSession::advance() {
    auto it=frames_.find(next_);
    if(it==frames_.end() || !it->second.actual[local_])return false;
    // A known current input cannot let us run beyond an earlier missing frame.
    if(next_!=confirmed_ && next_-confirmed_>=window_)return false;
    if(!it->second.actual[1-local_] && next_-confirmed_>=window_)return false;
    execute(next_);++next_;confirm();return true;
}
}
