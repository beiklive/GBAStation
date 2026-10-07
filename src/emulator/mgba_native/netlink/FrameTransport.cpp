#include "FrameTransport.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#ifndef _WIN32
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace netlink {
namespace {
void put(std::vector<uint8_t>& b,uint64_t v,unsigned n){for(unsigned i=n;i;--i)b.push_back(uint8_t(v>>(8*(i-1))));}
uint64_t get(const uint8_t* p,unsigned n){uint64_t v=0;for(unsigned i=0;i<n;++i)v=(v<<8)|p[i];return v;}
void require(bool v,const char* why){if(!v)throw std::runtime_error(why);}
}
FrameProtocol::FrameProtocol(unsigned slot,uint64_t session,std::array<uint8_t,32> manifest,unsigned delay,unsigned window)
    :slot_(slot),delay_(delay),window_(window),session_(session),manifest_(manifest){
    require(slot<=1 && delay==3 && window<=12,"unsupported frame configuration");
    require(slot!=0 || session!=0,"host needs a fresh nonzero session ID");
}
std::vector<uint8_t> FrameProtocol::encode(FrameMessage::Type type,const std::vector<uint8_t>& payload){
    require(session_!=0 && payload.size()<=256 && txSequence_!=0,"invalid outgoing frame");
    std::vector<uint8_t> b{'G','B','L','R'};put(b,2,2);put(b,type,2);put(b,payload.size(),4);put(b,session_,8);put(b,txSequence_++,4);
    b.insert(b.end(),payload.begin(),payload.end());return b;
}
std::vector<uint8_t> FrameProtocol::hello(){
    require(!helloSent_,"duplicate outgoing handshake");
    std::vector<uint8_t> p{uint8_t(slot_),uint8_t(delay_),uint8_t(window_),0};p.insert(p.end(),manifest_.begin(),manifest_.end());
    auto b=encode(FrameMessage::Hello,p);helloSent_=true;return b;
}
std::vector<uint8_t> FrameProtocol::heartbeat(){
    require(ready_ && helloSent_ && !doneSent_,"heartbeat before handshake completion");return encode(FrameMessage::Heartbeat,{});
}
std::vector<uint8_t> FrameProtocol::input(uint64_t frame,uint16_t keys){
    require(ready_ && helloSent_ && !doneSent_ && frame==outgoingFrame_ && keys<=1023,"invalid outgoing input");
    std::vector<uint8_t> p;put(p,frame,8);put(p,keys,2);auto b=encode(FrameMessage::Input,p);++outgoingFrame_;return b;
}
std::vector<uint8_t> FrameProtocol::done(uint64_t frame,uint64_t state,uint64_t scheduler){
    require(ready_ && helloSent_ && !doneSent_ && frame==outgoingFrame_,"invalid completion frame");
    std::vector<uint8_t> p;put(p,frame,8);put(p,state,8);put(p,scheduler,8);auto b=encode(FrameMessage::Done,p);doneSent_=true;return b;
}
std::vector<uint8_t> FrameProtocol::checkpoint(uint64_t frame,uint64_t digest){
    require(ready_ && helloSent_ && !doneSent_,"checkpoint before handshake");
    std::vector<uint8_t> p;put(p,frame,8);put(p,digest,8);return encode(FrameMessage::Checkpoint,p);
}
void FrameProtocol::feed(const uint8_t* data,size_t size,const std::function<void(const FrameMessage&)>& callback){
    require(size<=4096 && rx_.size()+size<=8192,"receive buffer limit exceeded");
    rx_.insert(rx_.end(),data,data+size);
    while(rx_.size()>=24){
        auto* h=rx_.data();require(std::memcmp(h,"GBLR",4)==0,"wrong frame magic");require(get(h+4,2)==2,"incompatible frame protocol version");
        unsigned type=unsigned(get(h+6,2)),length=unsigned(get(h+8,4));uint64_t sid=get(h+12,8);
        require(length<=256,"oversized frame payload");require(get(h+20,4)==rxSequence_ && rxSequence_!=0,"frame sequence mismatch");
        require(!doneReceived_,"message after completion");
        require(sid!=0 && (session_==0 || sid==session_),"stale or wrong session");
        require(type>=1 && type<=5,"unknown frame type");
        require(length==(type==1?36:type==2?10:type==3?24:type==4?16:0),"invalid frame length");
        if(rx_.size()<24+length)return;
        const auto* p=h+24;FrameMessage message{FrameMessage::Type(type)};
        if(type==FrameMessage::Hello){
            require(!ready_ && rxSequence_==1,"unexpected handshake");
            require(p[0]==1-slot_ && p[1]==delay_ && p[2]==window_ && p[3]==0,"peer configuration mismatch");
            require(std::equal(manifest_.begin(),manifest_.end(),p+4),"ROM/save/BIOS/settings manifest mismatch");
            session_=sid;ready_=true;
        }else if(type==FrameMessage::Heartbeat){
            require(ready_ && helloSent_,"heartbeat before handshake completion");
        }else{
            require(ready_ && helloSent_,"input before handshake completion");message.frame=get(p,8);
            if(type==FrameMessage::Input){message.keys=uint16_t(get(p+8,2));require(message.frame==incomingFrame_ && message.keys<=1023,"invalid incoming input");++incomingFrame_;}
            else if(type==FrameMessage::Checkpoint){message.state=get(p+8,8);}
            else{require(message.frame==incomingFrame_,"completion before all inputs");message.state=get(p+8,8);message.scheduler=get(p+16,8);doneReceived_=true;}
        }
        ++rxSequence_;rx_.erase(rx_.begin(),rx_.begin()+24+length);callback(message);
    }
}
void FrameProtocol::endOfStream() const { require(rx_.empty(),"truncated TCP frame"); }
#ifndef _WIN32
FrameSocket::FrameSocket(int fd):fd_(fd){
    try{
        require(fd>=0,"invalid socket");int flags=fcntl(fd,F_GETFL,0);require(flags>=0 && fcntl(fd,F_SETFL,flags|O_NONBLOCK)==0,"nonblocking socket setup failed");
        int yes=1;require(setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,&yes,sizeof(yes))==0,"TCP_NODELAY failed");
#ifdef SO_NOSIGPIPE
        require(setsockopt(fd,SOL_SOCKET,SO_NOSIGPIPE,&yes,sizeof(yes))==0,"SO_NOSIGPIPE failed");
#endif
    }catch(...){if(fd_>=0)close(fd_);fd_=-1;throw;}
}
FrameSocket::~FrameSocket(){if(fd_>=0)close(fd_);}
void FrameSocket::queue(const std::vector<uint8_t>& bytes){
    require(!closed_,"peer disconnected");
    if(sent_){tx_.erase(tx_.begin(),tx_.begin()+sent_);sent_=0;}
    require(bytes.size()<=65536 && tx_.size()+bytes.size()<=65536,"send buffer limit exceeded");tx_.insert(tx_.end(),bytes.begin(),bytes.end());
}
void FrameSocket::pump(FrameProtocol& protocol,const std::function<void(const FrameMessage&)>& callback){
    if(closed_)return;
    size_t budget=65536;
    while(sent_<tx_.size() && budget){
        int flags=0;
#ifdef MSG_NOSIGNAL
        flags=MSG_NOSIGNAL;
#endif
        auto n=send(fd_,tx_.data()+sent_,std::min(budget,tx_.size()-sent_),flags);
        if(n<0){if(errno==EINTR)continue;if(errno==EAGAIN || errno==EWOULDBLOCK)break;throw std::runtime_error("TCP send failed");}
        require(n>0,"TCP send made no progress");sent_+=size_t(n);budget-=size_t(n);
    }
    if(sent_==tx_.size()){tx_.clear();sent_=0;}
    budget=65536;uint8_t bytes[4096];
    while(budget){
        auto n=recv(fd_,bytes,std::min(budget,sizeof(bytes)),0);
        if(n<0){if(errno==EINTR)continue;if(errno==EAGAIN || errno==EWOULDBLOCK)break;throw std::runtime_error("TCP receive failed");}
        if(!n){closed_=true;protocol.endOfStream();return;}
        protocol.feed(bytes,size_t(n),callback);budget-=size_t(n);
    }
}
#else
FrameSocket::FrameSocket(int):fd_(-1){throw std::runtime_error("experimental frame socket is unavailable on Windows");}
FrameSocket::~FrameSocket()=default;
void FrameSocket::queue(const std::vector<uint8_t>&){throw std::runtime_error("frame socket unavailable");}
void FrameSocket::pump(FrameProtocol&,const std::function<void(const FrameMessage&)>&){throw std::runtime_error("frame socket unavailable");}
#endif
}
