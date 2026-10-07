#include "ProductSession.hpp"
#include <fstream>
#include <filesystem>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <mgba/internal/gba/gba.h>
#include <mgba-util/vfs.h>
#ifndef _WIN32
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#ifndef __SWITCH__
#include <ifaddrs.h>
#include <net/if.h>
#endif
#ifdef __SWITCH__
#include <switch.h>
#elif defined(__APPLE__)
#include <CommonCrypto/CommonDigest.h>
#else
// Already linked by the existing Snes9x core; no new crypto dependency.
void sha256sum(unsigned char*,unsigned int,unsigned char*);
#endif
namespace netlink {
namespace {
std::array<uint8_t,32> sha(const std::vector<uint8_t>& bytes){
    std::array<uint8_t,32> out{};
#ifdef __SWITCH__
    sha256CalculateHash(out.data(),bytes.data(),bytes.size());
#else
#ifdef __APPLE__
    CC_SHA256(bytes.data(),static_cast<CC_LONG>(bytes.size()),out.data());
#else
    auto mutableBytes=bytes;sha256sum(mutableBytes.data(),static_cast<unsigned int>(mutableBytes.size()),out.data());
#endif
#endif
    return out;
}
std::vector<uint8_t> read(const std::string& path,size_t maximum){
    std::ifstream f(path,std::ios::binary|std::ios::ate);
    if(!f)throw std::runtime_error("无法读取联机资源: "+path);
    auto n=f.tellg();if(n<=0 || uint64_t(n)>maximum)throw std::runtime_error("联机资源大小错误: "+path);
    std::vector<uint8_t> bytes(static_cast<size_t>(n));f.seekg(0);
    if(!f.read(reinterpret_cast<char*>(bytes.data()),n))throw std::runtime_error("联机资源读取失败: "+path);
    return bytes;
}
std::array<uint8_t,32> manifest(const LocalLink::Resources& r){
    // Separate from headless v1 tests: output rate and build contract are part of identity.
    std::string tag="GBLR-product-v2;mgba-0.10.5;audio48000;D3;W8;checkpoint60";
    std::vector<uint8_t> bytes(tag.begin(),tag.end());
    auto append=[&](const std::vector<uint8_t>& b){
        for(unsigned s=64;s;s-=8)bytes.push_back(uint8_t(uint64_t(b.size())>>(s-8)));
        auto h=sha(b);bytes.insert(bytes.end(),h.begin(),h.end());
    };
    for(unsigned i=0;i<2;++i){append(r.roms[i]);append(r.saves[i]);append(r.states[i]);}append(r.bios);
    return sha(bytes);
}
void nonblocking(int fd){int flags=fcntl(fd,F_GETFL,0);if(flags<0 || fcntl(fd,F_SETFL,flags|O_NONBLOCK)<0)throw std::runtime_error("无法配置非阻塞连接");}
}
ProductSession::ProductSession(unsigned slot,const std::string& host,int port,const std::string& directory,const std::string& expectedRom)
    :slot_(slot),port_(port),directory_(directory),state_(slot?Connecting:Listening),started_(std::chrono::steady_clock::now()) {
    try{
        if(slot>1 || port<1 || port>65535)throw std::runtime_error("联机参数错误");
        LocalLink::Resources resources;resources.sampleRate=48000;
        bool automatic=std::filesystem::exists(directory+"/local.gba");
        for(unsigned i=0;i<2;++i){
            if(automatic && i!=slot)continue;
            auto base=directory+(automatic?"/local":"/slot"+std::to_string(i));
            resources.roms[i]=read(base+".gba",32*1024*1024);
            if(resources.roms[i].size()<192)throw std::runtime_error("GBA ROM 头不完整");
            resources.saves[i]=read(base+".sav",131072);
            if(automatic && std::filesystem::exists(base+".state")) resources.states[i]=read(base+".state",1024*1024);
            auto n=resources.saves[i].size();if(n!=512 && n!=8192 && n!=32768 && n!=65536 && n!=131072)throw std::runtime_error("联机存档必须是原始 SAV");
        }
        // BIOS is mandatory in this profile; both machines use the exact same bytes.
        if(!automatic || std::filesystem::exists(directory+"/gba_bios.bin")) resources.bios=read(directory+"/gba_bios.bin",16384);
        if(!resources.bios.empty() && resources.bios.size()!=16384)throw std::runtime_error("GBA BIOS 必须为 16384 字节");
        if(!automatic && !expectedRom.empty() && sha(read(expectedRom,32*1024*1024))!=sha(resources.roms[slot]))
            throw std::runtime_error("当前游戏与本机联机槽 ROM 不一致（主机 slot0，加入方 slot1）");
        if(automatic) localResources_=std::make_unique<LocalLink::Resources>(std::move(resources));
        else initialize(resources);
        fd_=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(fd_<0)throw std::runtime_error("无法创建局域网连接");
        nonblocking(fd_);sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(uint16_t(port));
        if(slot){
            if(inet_pton(AF_INET,host.c_str(),&address.sin_addr)!=1)throw std::runtime_error("请输入有效 IPv4 地址，不含端口");
            int result=::connect(fd_,reinterpret_cast<sockaddr*>(&address),sizeof(address));
            if(result==0){int fd=fd_;fd_=-1;connected(fd);}
            else if(errno==EINPROGRESS)connecting_=true;else throw std::runtime_error("无法连接主机");
        }else{
            address.sin_addr.s_addr=htonl(INADDR_ANY);int yes=1;setsockopt(fd_,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
            if(bind(fd_,reinterpret_cast<sockaddr*>(&address),sizeof(address))<0 || listen(fd_,1)<0)throw std::runtime_error("无法监听端口，可能已被占用");
        }
    }catch(...){if(fd_>=0)close(fd_);fd_=-1;throw;}
}
void ProductSession::initialize(const LocalLink::Resources& resources){
    auto fingerprint=manifest(resources);
    link_=std::make_unique<LocalLink>(resources);
    initialFrame_=link_->frame(slot_);durable_=link_->capture();
    timeline_=std::make_unique<RollbackSession>(*link_,slot_,8);
    for(unsigned i=0;i<3;++i)for(unsigned s=0;s<2;++s)timeline_->publish(s,i,0);
    timeline_->onAudio([this](const std::vector<int16_t>& a){
        if(audio_.size()+a.size()>32768)audio_.clear();
        audio_.insert(audio_.end(),a.begin(),a.end());
    });
    timeline_->onConfirmed([this](uint64_t frame,const LocalLink::Snapshot& state){
        auto count=frame+1;if(count%60)return;
        if(checkpoints_.size()>=4)throw std::runtime_error("对端确认进度停止，联机已暂停");
        checkpoints_.emplace(count,state);socket_->queue(protocol_->checkpoint(count,state.regressionDigest()));match();
    });
    auto id=uint64_t(started_.time_since_epoch().count());if(!id)id=1;
    protocol_=std::make_unique<FrameProtocol>(slot_,slot_?0:id,fingerprint,3,8);
 }
void ProductSession::preparePlayer(mCore* core,const std::string& directory){
    if(!core || core->platform(core)!=mPLATFORM_GBA)throw std::runtime_error("请先打开 GBA 游戏");
    auto* gba=static_cast<GBA*>(core->board);
    if(!gba->romVf)throw std::runtime_error("无法读取当前游戏");
    std::filesystem::create_directories(directory);
    auto writeFile=[&](const char* name,const void* bytes,size_t n){
        std::ofstream f(directory+"/"+name,std::ios::binary|std::ios::trunc);
        if(!f || !f.write(static_cast<const char*>(bytes),n))throw std::runtime_error("无法准备本机联机数据");
    };
    auto n=gba->romVf->size(gba->romVf);if(n<192 || n>32*1024*1024)throw std::runtime_error("当前 ROM 大小不支持");
    std::vector<uint8_t> rom(n);auto offset=gba->romVf->seek(gba->romVf,0,SEEK_CUR);
    gba->romVf->seek(gba->romVf,0,SEEK_SET);auto got=gba->romVf->read(gba->romVf,rom.data(),rom.size());gba->romVf->seek(gba->romVf,offset,SEEK_SET);
    if(got!=n)throw std::runtime_error("当前 ROM 读取失败");writeFile("local.gba",rom.data(),rom.size());
    void* data=nullptr;size_t size=core->savedataClone(core,&data);
    std::unique_ptr<void,decltype(&std::free)> owned(data,&std::free);
    if(!data || !size)throw std::runtime_error("请先在游戏内保存，再开始联机");
    writeFile("local.sav",data,size);
    std::vector<uint8_t> state(core->stateSize(core),0);
    if(state.empty() || state.size()>1024*1024 || !core->saveState(core,state.data()))throw std::runtime_error("无法保存当前游戏现场");
    writeFile("local.state",state.data(),state.size());
    if(gba->biosVf){
        std::vector<uint8_t> bios(16384);auto pos=gba->biosVf->seek(gba->biosVf,0,SEEK_CUR);
        gba->biosVf->seek(gba->biosVf,0,SEEK_SET);auto count=gba->biosVf->read(gba->biosVf,bios.data(),bios.size());gba->biosVf->seek(gba->biosVf,pos,SEEK_SET);
        if(count!=16384)throw std::runtime_error("当前 BIOS 读取失败");writeFile("gba_bios.bin",bios.data(),bios.size());
    }else std::filesystem::remove(directory+"/gba_bios.bin");
}
void ProductSession::prepareConnection(int fd){
    preparationFd_=fd;nonblocking(fd);state_=Connecting;started_=std::chrono::steady_clock::now();
#ifdef SO_NOSIGPIPE
    int yes=1;setsockopt(fd,SOL_SOCKET,SO_NOSIGPIPE,&yes,sizeof(yes));
#endif
    const auto& r=*localResources_;const auto& save=r.saves[slot_];
    preparationTx_={'G','B','A','P',2,uint8_t(slot_),0,0};
    for(int i=24;i>=0;i-=8)preparationTx_.push_back(uint8_t(save.size()>>i));
    for(int i=24;i>=0;i-=8)preparationTx_.push_back(uint8_t(r.states[slot_].size()>>i));
    for(const auto* bytes:{&r.roms[slot_],&r.bios,&save,&r.states[slot_]}){auto hash=sha(*bytes);preparationTx_.insert(preparationTx_.end(),hash.begin(),hash.end());}
    preparationTx_.insert(preparationTx_.end(),save.begin(),save.end());
    preparationTx_.insert(preparationTx_.end(),r.states[slot_].begin(),r.states[slot_].end());
}
void ProductSession::pumpPreparation(){
    if(preparationSent_<preparationTx_.size()){
#ifdef MSG_NOSIGNAL
        auto n=send(preparationFd_,preparationTx_.data()+preparationSent_,preparationTx_.size()-preparationSent_,MSG_NOSIGNAL);
#else
        auto n=send(preparationFd_,preparationTx_.data()+preparationSent_,preparationTx_.size()-preparationSent_,0);
#endif
        if(n>0)preparationSent_+=size_t(n);else if(n==0 || (errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR))throw std::runtime_error("联机数据发送失败");
    }
    size_t wanted=144+(remoteSaveSize_?remoteSaveSize_+remoteStateSize_:0);
    if(preparationRx_.size()<wanted){
        uint8_t buf[4096];auto n=recv(preparationFd_,buf,std::min(sizeof(buf),wanted-preparationRx_.size()),0);
        if(n>0)preparationRx_.insert(preparationRx_.end(),buf,buf+n);
        else if(!n)throw std::runtime_error("对端在准备联机数据时断开");
        else if(errno==ECONNRESET)throw std::runtime_error("对端在准备联机数据时断开");
        else if(errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR)throw std::runtime_error("联机数据接收失败");
    }
    if(!remoteSaveSize_ && preparationRx_.size()==144){
        const auto* p=preparationRx_.data();
        if(std::memcmp(p,"GBAP",4) || p[4]!=2 || p[5]!=1-slot_ || p[6] || p[7])throw std::runtime_error("对端不支持自动准备，请更新两端程序");
        uint32_t size=0;for(unsigned i=8;i<12;++i)size=(size<<8)|p[i];
        if(size!=512 && size!=8192 && size!=32768 && size!=65536 && size!=131072)throw std::runtime_error("对端存档大小错误");
        uint32_t stateSize=0;for(unsigned i=12;i<16;++i)stateSize=(stateSize<<8)|p[i];
        if(stateSize>1024*1024)throw std::runtime_error("对端游戏现场大小错误");
        auto rh=sha(localResources_->roms[slot_]),bh=sha(localResources_->bios);
        if(!std::equal(rh.begin(),rh.end(),p+16))throw std::runtime_error("双方游戏版本不同，请打开相同版本的游戏");
        if(!std::equal(bh.begin(),bh.end(),p+48))throw std::runtime_error("双方 BIOS 配置不同，请使用相同 BIOS，或双方都使用内置 BIOS");
        remoteSaveSize_=size;remoteStateSize_=stateSize;
    }
    if(remoteSaveSize_ && preparationRx_.size()==144+remoteSaveSize_+remoteStateSize_ && preparationSent_==preparationTx_.size()){
        auto& r=*localResources_;r.roms[1-slot_]=r.roms[slot_];r.saves[1-slot_].assign(preparationRx_.begin()+144,preparationRx_.begin()+144+remoteSaveSize_);
        r.states[1-slot_].assign(preparationRx_.begin()+144+remoteSaveSize_,preparationRx_.end());
        auto h=sha(r.saves[1-slot_]);if(!std::equal(h.begin(),h.end(),preparationRx_.begin()+80))throw std::runtime_error("对端存档校验失败");
        auto sh=sha(r.states[1-slot_]);if(!std::equal(sh.begin(),sh.end(),preparationRx_.begin()+112))throw std::runtime_error("对端游戏现场校验失败");
        initialize(r);localResources_.reset();preparationTx_.clear();preparationRx_.clear();
        int fd=preparationFd_;preparationFd_=-1;connected(fd);
    }
}
ProductSession::~ProductSession(){if(fd_>=0)close(fd_);if(preparationFd_>=0)close(preparationFd_);}
void ProductSession::connected(int fd){
    if(localResources_){prepareConnection(fd);return;}
    socket_=std::make_unique<FrameSocket>(fd);connecting_=false;state_=Connecting;started_=std::chrono::steady_clock::now();
    if(!slot_)socket_->queue(protocol_->hello());
}
void ProductSession::fail(const std::string& why,bool disconnected){
    if(why=="ROM/save/BIOS/settings manifest mismatch")error_="两端的 ROM、初始存档、BIOS 或联机参数不一致";
    else if(why=="incompatible frame protocol version" || why=="wrong frame magic")error_="对端不是兼容的新版双核联机程序";
    else if(why=="TCP send failed" || why=="TCP receive failed")error_="局域网连接读写失败，请断开后重新连接";
    else error_=why;
    state_=disconnected?Disconnected:Error;socket_.reset();if(preparationFd_>=0)close(preparationFd_);preparationFd_=-1;if(fd_>=0)close(fd_);fd_=-1;audio_.clear();}
void ProductSession::receive(const FrameMessage& message){
    lastReceived_=std::chrono::steady_clock::now();
    if(message.type==FrameMessage::Heartbeat)return;
    if(message.type==FrameMessage::Hello){if(slot_)socket_->queue(protocol_->hello());state_=Ready;lastHeartbeat_=lastReceived_;}
    else if(message.type==FrameMessage::Input)timeline_->publish(1-slot_,message.frame,message.keys);
    else if(message.type==FrameMessage::Checkpoint){
        if(!message.frame || message.frame%60 || message.frame<=lastRemoteCheckpoint_ || message.frame>timeline_->nextFrame()+120)
            throw std::runtime_error("对端确认帧错误");
        lastRemoteCheckpoint_=message.frame;remoteCheckpoints_.emplace(message.frame,message.state);match();
    }else throw std::runtime_error("对端结束联机");
}
void ProductSession::match(){
    for(auto it=remoteCheckpoints_.begin();it!=remoteCheckpoints_.end();){
        auto local=checkpoints_.find(it->first);if(local==checkpoints_.end()){++it;continue;}
        if(local->second.regressionDigest()!=it->second)throw std::runtime_error("双方模拟状态不一致，已停止联机，原存档未修改");
        if(it->first>matched_){durable_=local->second;matched_=it->first;}
        checkpoints_.erase(local);it=remoteCheckpoints_.erase(it);
    }
}
void ProductSession::poll(){
    if(state_==Error || state_==Disconnected)return;
    try{
        if(fd_>=0){
            if(!slot_){int accepted=accept(fd_,nullptr,nullptr);if(accepted>=0){close(fd_);fd_=-1;connected(accepted);}
                else if(errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR)throw std::runtime_error("接受连接失败");}
            else if(connecting_){pollfd p{fd_,POLLOUT,0};if(::poll(&p,1,0)>0){int error=0;socklen_t len=sizeof(error);if(getsockopt(fd_,SOL_SOCKET,SO_ERROR,&error,&len)<0 || error)throw std::runtime_error("连接主机失败");int fd=fd_;fd_=-1;connected(fd);}}
        }
        if(state_==Connecting && std::chrono::steady_clock::now()-started_>std::chrono::seconds(30))throw std::runtime_error("连接或资源校验超时");
        if(preparationFd_>=0)pumpPreparation();
        if(state_==Ready){
            auto now=std::chrono::steady_clock::now();
            if(now-lastReceived_>std::chrono::seconds(8)){fail("对端连接已失去响应，已结束联机",true);return;}
            if(now-lastHeartbeat_>=std::chrono::seconds(1)){socket_->queue(protocol_->heartbeat());lastHeartbeat_=now;}
        }
        if(socket_){socket_->pump(*protocol_,[this](const FrameMessage& m){receive(m);});if(socket_->closed())throw std::runtime_error("对端已断开，请断开后重新连接");}
    }catch(const std::exception& e){
        const std::string why=e.what();
        bool offline=state_==Ready && (why=="对端已断开，请断开后重新连接" || why=="TCP receive failed" || why=="TCP send failed" || why=="truncated TCP frame");
        fail(offline?"对端已离线，已结束联机":why,offline);
    }
}
bool ProductSession::advance(uint16_t keys){
    poll();if(state_!=Ready)return false;
    try{
        auto target=timeline_->nextFrame()+3;
        if(sampled_==target){timeline_->publish(slot_,sampled_,keys);socket_->queue(protocol_->input(sampled_,keys));++sampled_;}
        poll();if(state_!=Ready)return false;
        return timeline_->advance();
    }catch(const std::exception& e){fail(e.what());return false;}
}
bool ProductSession::restorePlayer(mCore* core){
    if(!durable_.valid())return true;
    if(!core || core->platform(core)!=mPLATFORM_GBA)return false;
    const auto& state=durable_.state(slot_);const auto& save=durable_.save(slot_);
    if(state.size()!=core->stateSize(core))return false;
    std::vector<uint8_t> backup(core->stateSize(core),0);
    if(!core->saveState(core,backup.data()))return false;
    if(!core->loadState(core,state.data())){core->loadState(core,backup.data());return false;}
    return save.empty() || core->savedataRestore(core,save.data(),save.size(),false);
}
std::string ProductSession::listeningStatus() const {
    if(!listeningText_.empty())return listeningText_;
    std::string result="等待加入；主机地址：";char address[INET_ADDRSTRLEN];bool found=false;
#ifdef __SWITCH__
    u32 ip=0;if(R_SUCCEEDED(nifmInitialize(NifmServiceType_User))){
        if(R_SUCCEEDED(nifmGetCurrentIpAddress(&ip)) && ip && inet_ntop(AF_INET,&ip,address,sizeof(address))){result+=address;found=true;}nifmExit();
    }
#else
    ifaddrs* list=nullptr;if(!getifaddrs(&list)){
        for(auto* p=list;p;p=p->ifa_next){
            if(!p->ifa_addr || p->ifa_addr->sa_family!=AF_INET || !(p->ifa_flags&IFF_UP) || (p->ifa_flags&IFF_LOOPBACK))continue;
#ifdef __APPLE__
            if(std::strncmp(p->ifa_name,"en",2) && std::strncmp(p->ifa_name,"bridge",6))continue;
#endif
            auto* a=reinterpret_cast<sockaddr_in*>(p->ifa_addr);
            if(a->sin_addr.s_addr && inet_ntop(AF_INET,&a->sin_addr,address,sizeof(address))){if(found)result+=", ";result+=address;found=true;}
        }freeifaddrs(list);
    }
#endif
    if(!found)result+="未取得局域网 IP，请检查网络";
    listeningText_=result+"；端口 "+std::to_string(port_);return listeningText_;
}
bool ProductSession::exportSave(){
    if(!durable_.valid() || exported_==matched_)return true;
    auto path=directory_+"/slot"+std::to_string(slot_)+".confirmed-"+std::to_string(protocol_->session())+".sav";
    auto temporary=path+".tmp";const auto& bytes=durable_.save(slot_);if(bytes.empty())return true;
    std::ofstream file(temporary,std::ios::binary|std::ios::trunc);if(!file)return false;
    file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());file.close();if(!file)return false;
    std::error_code ec;std::filesystem::rename(temporary,path,ec);if(!ec)exported_=matched_;return !ec;
}
}
#else
namespace netlink {
ProductSession::ProductSession(unsigned slot,const std::string&,int,const std::string& dir,const std::string&)
    :slot_(slot),directory_(dir),state_(Error){throw std::runtime_error("新版双核联机暂不支持 Windows 前端");}
ProductSession::~ProductSession()=default;
void ProductSession::poll(){}
bool ProductSession::advance(uint16_t){return false;}
bool ProductSession::exportSave(){return true;}
}
#endif
