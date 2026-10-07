#pragma once
#include "RollbackSession.hpp"
#include "FrameTransport.hpp"
#include <chrono>
#include <map>
#include <string>

namespace netlink {
// All methods, socket callbacks and cores belong to the game thread.
class ProductSession {
public:
    enum State { Listening, Connecting, Ready, Error, Disconnected };
    ProductSession(unsigned slot,const std::string& host,int port,const std::string& directory,const std::string& expectedRom="");
    ~ProductSession();
    static void preparePlayer(mCore*, const std::string& directory);
    void poll();
    bool advance(uint16_t keys);
    State state() const{return state_;}
    const std::string& error() const{return error_;}
    const std::vector<color_t>& pixels() const{return link_->pixels(slot_);}
    std::vector<int16_t> takeAudio(){auto a=std::move(audio_);audio_.clear();return a;}
    bool exportSave();
    bool restorePlayer(mCore*);
    std::string listeningStatus() const;
    uint32_t initialGameFrame() const {return initialFrame_;}
    uint64_t nextFrame() const{return timeline_ ? timeline_->nextFrame() : 0;}
    uint64_t matchedFrames() const{return matched_;}
    unsigned rollbacks() const{return timeline_ ? timeline_->rollbacks() : 0;}
private:
    unsigned slot_;
    int port_=8765;
    mutable std::string listeningText_;
    int preparationFd_=-1;
    std::unique_ptr<LocalLink::Resources> localResources_;
    std::vector<uint8_t> preparationTx_,preparationRx_;
    size_t preparationSent_=0;
    uint32_t remoteSaveSize_=0,remoteStateSize_=0,initialFrame_=0;
    void initialize(const LocalLink::Resources&);
    void prepareConnection(int);
    void pumpPreparation();
    std::string directory_,error_;
    State state_;
    int fd_=-1;
    bool connecting_=false;
    std::unique_ptr<LocalLink> link_;
    std::unique_ptr<RollbackSession> timeline_;
    std::unique_ptr<FrameProtocol> protocol_;
    std::unique_ptr<FrameSocket> socket_;
    uint64_t sampled_=3,matched_=0,exported_=0,lastRemoteCheckpoint_=0;
    std::map<uint64_t,LocalLink::Snapshot> checkpoints_;
    std::map<uint64_t,uint64_t> remoteCheckpoints_;
    LocalLink::Snapshot durable_;
    std::vector<int16_t> audio_;
    std::chrono::steady_clock::time_point started_,lastReceived_,lastHeartbeat_;
    void connected(int fd);
    void receive(const FrameMessage&);
    void match();
    void fail(const std::string&,bool disconnected=false);
};
}
