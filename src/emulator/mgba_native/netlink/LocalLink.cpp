// MPL-2.0. Cycle-credit callbacks adapted from the vendored mGBA Qt controller
// (Copyright 2013-2016 Jeffrey Pfau); no Qt threads or socket waits are used here.
#include "LocalLink.hpp"
#include <stdexcept>
#include <atomic>
#include <cstring>
#include <mgba/gba/core.h>
#include <mgba/internal/gba/gba.h>
#include <mgba-util/vfs.h>
#include <mgba/core/blip_buf.h>

namespace netlink {
struct LocalLink::Snapshot::Data {
    struct Event { mTimingEvent* binding; uint32_t when; unsigned priority; };
    struct Node {
        int32_t nextEvent, eventDiff;
        bool normalSO, transferFinished;
        GBASIOMode mode;
        uint16_t (*writeRegister)(GBASIODriver*,uint32_t,uint16_t);
    };
    struct Extra {
        GBAudio psg;
        GBAAudioFIFO fifo[2];
        GBADMA dma[4];
        int audioClock, sampleIndex, lastSample, activeDMA, performingDMA, springIRQ;
        int16_t lastLeft,lastRight;
        mStereoSample samples[GBA_MAX_SAMPLES];
        uint16_t keys, keysLast, rcnt, siocnt;
        uint16_t io[512]; uint32_t bus;
        GBASIOMode sioMode;
        bool driverActive;
        bool earlyExit, haltPending;
        int idleStep,idleFailures;
        uint32_t lastJump;
        int32_t cachedRegisters[16]; bool taintedRegisters[16];
        int saveDirty; uint32_t saveAge;
        std::vector<uint8_t> save;
        std::vector<Event> events;
        uint32_t master; uint64_t global;
        int32_t relative, nextEvent;
    };
    uint64_t identity, regressionScheduler;
    std::array<std::vector<uint8_t>,2> cores;
    std::array<std::vector<color_t>,2> pixels;
    std::array<Extra,2> extra;
    std::array<Node,2> nodes;
    std::array<int32_t,2> credits;
    std::array<bool,2> awake;
    unsigned waiting;
    mLockstepPhase phase;
    int32_t cycles;
    int attachedMulti,attachedNormal;
    uint16_t multiRecv[MAX_GBAS]; uint32_t normalRecv[MAX_GBAS];
};
const std::vector<uint8_t>& LocalLink::Snapshot::save(unsigned slot) const {
    if (!data_) throw std::invalid_argument("invalid save snapshot");
    return data_->extra.at(slot).save;
}
const std::vector<uint8_t>& LocalLink::Snapshot::state(unsigned slot) const {
    if(!data_)throw std::invalid_argument("invalid state snapshot");return data_->cores.at(slot);
}
size_t LocalLink::Snapshot::bytes() const {
    if (!data_) return 0;
    size_t n=sizeof(Data);
    for(unsigned i=0;i<2;++i) n+=data_->cores[i].size()+data_->pixels[i].size()*sizeof(color_t)
        +data_->extra[i].save.size()+data_->extra[i].events.size()*sizeof(Data::Event);
    return n;
}
bool LocalLink::Snapshot::sameRegressionState(const Snapshot& other) const {
    if(!data_ || !other.data_)return false;
    if(data_->cores!=other.data_->cores || data_->regressionScheduler!=other.data_->regressionScheduler)return false;
    for(unsigned i=0;i<2;++i)if(data_->extra[i].save!=other.data_->extra[i].save)return false;
    return true;
}
uint64_t LocalLink::Snapshot::regressionDigest() const {
    if(!data_)throw std::invalid_argument("invalid regression snapshot");
    uint64_t h=14695981039346656037ULL;
    auto word=[&](uint64_t v){h^=v;h*=1099511628211ULL;};
    auto buffer=[&](const std::vector<uint8_t>& bytes){
        word(bytes.size());size_t i=0;
        for(;i+8<=bytes.size();i+=8){uint64_t v;std::memcpy(&v,bytes.data()+i,8);word(v);}
        uint64_t tail=0;for(unsigned shift=0;i<bytes.size();++i,shift+=8)tail|=uint64_t(bytes[i])<<shift;
        word(tail);
    };
    for(unsigned i=0;i<2;++i){buffer(data_->cores[i]);buffer(data_->extra[i].save);}
    word(data_->regressionScheduler);return h;
}
LocalLink& LocalLink::owner(mLockstep* l) { return *static_cast<LocalLink*>(l->context); }
bool LocalLink::signal(mLockstep* l, unsigned mask) {
    auto& p = owner(l);
    p.waiting_ &= ~mask;
    bool woke = !p.waiting_ && !p.awake_[0];
    if (!p.waiting_) p.awake_[0] = true;
    return woke;
}
bool LocalLink::wait(mLockstep* l, unsigned mask) {
    auto& p = owner(l);
    bool slept = p.awake_[0];
    p.waiting_ |= mask;
    p.awake_[0] = false;
    return slept;
}
void LocalLink::addCycles(mLockstep* l, int id, int32_t cycles) {
    auto& p = owner(l);
    if (cycles < 0) throw std::runtime_error("negative local cable credit");
    if (id) {
        p.credits_[id] += cycles;
    } else if (p.nodes_[1].d.p && p.nodes_[1].d.p->mode <= SIO_MULTI) {
        p.credits_[1] += cycles;
        if (!p.awake_[1]) p.nodes_[1].nextEvent += p.credits_[1];
        p.awake_[1] = true;
    }
}
int32_t LocalLink::useCycles(mLockstep* l, int id, int32_t cycles) {
    auto& p = owner(l);
    p.credits_[id] -= cycles;
    if (p.credits_[id] <= 0) p.awake_[id] = false;
    return p.credits_[id];
}
int32_t LocalLink::unusedCycles(mLockstep* l, int id) { return owner(l).credits_[id]; }
void LocalLink::unload(mLockstep* l, int id) {
    auto& p = owner(l);
    if (id) {
        p.credits_[id] = 0;
        p.awake_[id] = true;
        signal(l, 1U << id);
    } else {
        p.credits_[1] += p.nodes_[0].eventDiff;
        if (!p.awake_[1]) p.nodes_[1].nextEvent += p.credits_[1];
        p.awake_[1] = true;
    }
}
LocalLink::LocalLink(const uint8_t* rom0, const uint8_t* rom1, size_t size)
    :LocalLink(Resources{{std::vector<uint8_t>(rom0,rom0+size),std::vector<uint8_t>(rom1,rom1+size)}, {}, {}, true, 0, 1700000000000LL, {}}) {}
LocalLink::LocalLink(const Resources& resources) {
    static std::atomic<uint64_t> identities{0};
    identity_=++identities;
    mLockstepInit(&link_.d);
    GBASIOLockstepInit(&link_);
    link_.d.context = this;
    link_.d.signal = signal;
    link_.d.wait = wait;
    link_.d.addCycles = addCycles;
    link_.d.useCycles = useCycles;
    link_.d.unusedCycles = unusedCycles;
    link_.d.unload = unload;
    try {
        for (unsigned i = 0; i < 2; ++i) {
            auto* c = GBACoreCreate();
            if (!c || !c->init(c)) throw std::runtime_error("GBA core init failed");
            cores_[i] = c;
            mCoreInitConfig(c, nullptr);
            c->opts.useBios = !resources.bios.empty();
            c->opts.skipBios = resources.skipBios;
            c->rtc.override=RTC_FAKE_EPOCH;
            c->rtc.value=resources.rtcEpochMs;
            pixels_[i].resize(240*160);
            c->setVideoBuffer(c, pixels_[i].data(), 240);
            c->setAudioBufferSize(c, 2048);
            if(resources.roms[i].empty())throw std::invalid_argument("empty GBA ROM");
            auto* vf = VFileMemChunk(resources.roms[i].data(),resources.roms[i].size());
            if (!vf || !c->loadROM(c, vf)) {
                if (vf) vf->close(vf);
                throw std::runtime_error("GBA ROM load failed");
            }
            if(!resources.saves[i].empty()) {
                auto* save=VFileMemChunk(resources.saves[i].data(),resources.saves[i].size());
                if(!save || !c->loadSave(c,save)){if(save)save->close(save);throw std::runtime_error("GBA save load failed");}
            }
            if(!resources.bios.empty()) {
                auto* bios=VFileMemChunk(resources.bios.data(),resources.bios.size());
                if(!bios || !c->loadBIOS(c,bios,0)){if(bios)bios->close(bios);throw std::runtime_error("GBA BIOS load failed");}
            }
            c->reset(c);
            if(!resources.states[i].empty()) {
                if(resources.states[i].size()!=c->stateSize(c) || !c->loadState(c,resources.states[i].data()))throw std::runtime_error("无法恢复对端游戏现场");
                if(!resources.saves[i].empty() && !c->savedataRestore(c,resources.saves[i].data(),resources.saves[i].size(),false))throw std::runtime_error("无法恢复对端存档");
            }
            if (resources.sampleRate > 0) for (int ch=0;ch<2;++ch)
                blip_set_rates(c->getAudioChannel(c,ch), c->frequency(c), resources.sampleRate);
            GBASIOLockstepNodeCreate(&nodes_[i]);
            if (!GBASIOLockstepAttachNode(&link_, &nodes_[i])) throw std::runtime_error("attach failed");
            ++attached_;
            auto* g = static_cast<GBA*>(c->board);
            GBASIOSetDriver(&g->sio, &nodes_[i].d, SIO_MULTI);
            GBASIOSetDriver(&g->sio, &nodes_[i].d, SIO_NORMAL_32);
        }
    } catch (...) { release(); throw; }
}
void LocalLink::release() {
    // Unload both drivers while both core pointers and nodes are still alive.
    for (unsigned i = 0; i < attached_; ++i) {
        auto* g = static_cast<GBA*>(cores_[i]->board);
        GBASIOSetDriver(&g->sio, nullptr, SIO_MULTI);
        GBASIOSetDriver(&g->sio, nullptr, SIO_NORMAL_32);
    }
    while (attached_) GBASIOLockstepDetachNode(&link_, &nodes_[--attached_]);
    for (auto*& c : cores_) {
        if (c) { mCoreConfigDeinit(&c->config); c->deinit(c); c = nullptr; }
    }
}
LocalLink::~LocalLink() { release(); }
unsigned LocalLink::advance(std::array<uint16_t, 2> keys) {
    for (unsigned i = 0; i < 2; ++i) cores_[i]->setKeys(cores_[i], keys[i] & 0x3ff);
    uint32_t target = frame(0) + 1;
    unsigned slices = 0;
    while (frame(0) != target) {
        bool progress = false;
        for (unsigned i = 0; i < 2; ++i) {
            if (!awake_[i] || (i == 0 && frame(0) == target)) continue;
            cores_[i]->runLoop(cores_[i]);
            progress = true;
            if (++slices > 100000) throw std::runtime_error("local cable slice limit");
        }
        if (!progress) throw std::runtime_error("local cable deadlock");
    }
    for(unsigned slot=0;slot<2;++slot) {
        auto* c=cores_[slot]; auto* left=c->getAudioChannel(c,0); auto* right=c->getAudioChannel(c,1);
        auto& out=audio_[slot]; out.clear();
        short l[1024],r[1024];
        while(blip_samples_avail(left) && blip_samples_avail(right)) {
            int n=std::min(1024,std::min(blip_samples_avail(left),blip_samples_avail(right)));
            blip_read_samples(left,l,n,0);blip_read_samples(right,r,n,0);
            for(int i=0;i<n;++i){out.push_back(l[i]);out.push_back(r[i]);}
        }
    }
    return slices;
}
uint32_t LocalLink::read(unsigned slot, uint32_t address) const {
    return cores_.at(slot)->busRead32(cores_.at(slot), address);
}
void LocalLink::write(unsigned slot,uint32_t address,uint8_t value) {
    auto* c=cores_.at(slot); c->busWrite8(c,address,value);
}
void LocalLink::write16(unsigned slot,uint32_t address,uint16_t value) {
    auto* c=cores_.at(slot);c->busWrite16(c,address,value);
}
LocalLink::Snapshot LocalLink::capture() const {
    auto s=std::make_shared<Snapshot::Data>();
    s->identity=identity_; s->regressionScheduler=schedulerDigest(); s->cores=coreStates(); s->pixels=pixels_;
    s->credits=credits_; s->awake=awake_; s->waiting=waiting_;
    s->phase=link_.d.transferActive; s->cycles=link_.d.transferCycles;
    s->attachedMulti=link_.attachedMulti; s->attachedNormal=link_.attachedNormal;
    std::copy(std::begin(link_.multiRecv),std::end(link_.multiRecv),s->multiRecv);
    std::copy(std::begin(link_.normalRecv),std::end(link_.normalRecv),s->normalRecv);
    for(unsigned i=0;i<2;++i) {
        auto* g=static_cast<GBA*>(cores_[i]->board); auto& e=s->extra[i]; auto& a=g->audio;
        e.fifo[0]=a.chA; e.fifo[1]=a.chB;
        e.psg=a.psg;
        std::copy(std::begin(g->memory.dma),std::end(g->memory.dma),e.dma);
        e.audioClock=a.clock; e.sampleIndex=a.sampleIndex; e.lastSample=a.lastSample;
        e.lastLeft=a.lastLeft; e.lastRight=a.lastRight;
        std::copy(std::begin(a.currentSamples),std::end(a.currentSamples),e.samples);
        e.activeDMA=g->memory.activeDMA; e.performingDMA=g->performingDMA; e.springIRQ=g->springIRQ;
        e.keys=g->keysActive; e.keysLast=g->keysLast; e.earlyExit=g->earlyExit; e.haltPending=g->haltPending;
        e.idleStep=g->idleDetectionStep;e.idleFailures=g->idleDetectionFailures;e.lastJump=g->lastJump;
        std::copy(std::begin(g->cachedRegisters),std::end(g->cachedRegisters),e.cachedRegisters);
        std::copy(std::begin(g->taintedRegisters),std::end(g->taintedRegisters),e.taintedRegisters);
        e.rcnt=g->sio.rcnt; e.siocnt=g->sio.siocnt;
        std::copy(std::begin(g->memory.io),std::end(g->memory.io),e.io); e.bus=g->bus;
        e.sioMode=g->sio.mode; e.driverActive=g->sio.activeDriver==&nodes_[i].d;
        auto& save=g->memory.savedata;
        e.saveDirty=save.dirty; e.saveAge=save.dirtAge;
        size_t n=GBASavedataSize(&save);
        if(n && save.data) e.save.assign(save.data,save.data+n);
        auto& t=g->timing;
        e.master=t.masterCycles; e.global=t.globalCycles; e.relative=*t.relativeCycles; e.nextEvent=*t.nextEvent;
        // Bindings are in-process only, never serialized or hashed. A snapshot can
        // only restore its original, still-live Link; preserve exact queue order.
        for(auto* ev=t.root ? t.root : t.reroot;ev;ev=ev->next) {
            if(e.events.size()>=128) throw std::runtime_error("invalid timing queue");
            e.events.push_back({ev,ev->when,ev->priority});
        }
        auto& node=nodes_[i];
        s->nodes[i]={node.nextEvent,node.eventDiff,node.normalSO,node.transferFinished,node.mode,node.d.writeRegister};
    }
    Snapshot result; result.data_=std::move(s); return result;
}
void LocalLink::restore(const Snapshot& snapshot) {
    if(!snapshot.data_ || snapshot.data_->identity!=identity_) throw std::invalid_argument("snapshot belongs to another Link");
    auto& s=*snapshot.data_;
    // Core load rebuilds IO and timing and may invoke cable mode callbacks. Both
    // loads finish before we restore the shared cable and exact queue bindings.
    for(unsigned i=0;i<2;++i) {
        if(s.cores[i].size()!=cores_[i]->stateSize(cores_[i])) throw std::invalid_argument("snapshot size mismatch");
        auto* board=static_cast<GBA*>(cores_[i]->board);
        board->sio.activeDriver=nullptr;
        board->sio.drivers={};
        if(!cores_[i]->loadState(cores_[i],s.cores[i].data())) throw std::runtime_error("core restore failed");
        auto* g=static_cast<GBA*>(cores_[i]->board); auto& e=s.extra[i]; auto& a=g->audio;
        a.chA=e.fifo[0]; a.chB=e.fifo[1]; a.clock=e.audioClock;
        a.psg=e.psg;
        a.sampleIndex=e.sampleIndex; a.lastSample=e.lastSample; a.lastLeft=e.lastLeft; a.lastRight=e.lastRight;
        std::copy(std::begin(e.samples),std::end(e.samples),a.currentSamples);
        std::copy(std::begin(e.dma),std::end(e.dma),g->memory.dma);
        g->memory.activeDMA=e.activeDMA; g->performingDMA=e.performingDMA; g->springIRQ=e.springIRQ;
        g->keysActive=e.keys; g->keysLast=e.keysLast; g->earlyExit=e.earlyExit; g->haltPending=e.haltPending;
        g->idleDetectionStep=e.idleStep;g->idleDetectionFailures=e.idleFailures;g->lastJump=e.lastJump;
        std::copy(std::begin(e.cachedRegisters),std::end(e.cachedRegisters),g->cachedRegisters);
        std::copy(std::begin(e.taintedRegisters),std::end(e.taintedRegisters),g->taintedRegisters);
        g->sio.rcnt=e.rcnt; g->sio.siocnt=e.siocnt;
        std::copy(std::begin(e.io),std::end(e.io),g->memory.io); g->bus=e.bus;
        g->sio.mode=e.sioMode;
        g->sio.drivers.multiplayer=&nodes_[i].d; g->sio.drivers.normal=&nodes_[i].d;
        g->sio.activeDriver=e.driverActive?&nodes_[i].d:nullptr;
        auto& save=g->memory.savedata;
        if(e.save.size()!=GBASavedataSize(&save)) throw std::runtime_error("savedata restore size mismatch");
        if(!e.save.empty()) std::copy(e.save.begin(),e.save.end(),save.data);
        save.dirty=e.saveDirty; save.dirtAge=e.saveAge;
        pixels_[i]=s.pixels[i]; cores_[i]->setVideoBuffer(cores_[i],pixels_[i].data(),240);
        for(int ch=0;ch<2;++ch) blip_clear(cores_[i]->getAudioChannel(cores_[i],ch));
    }
    credits_=s.credits; awake_=s.awake; waiting_=s.waiting;
    link_.d.transferActive=s.phase; link_.d.transferCycles=s.cycles;
    link_.attachedMulti=s.attachedMulti; link_.attachedNormal=s.attachedNormal;
    std::copy(std::begin(s.multiRecv),std::end(s.multiRecv),link_.multiRecv);
    std::copy(std::begin(s.normalRecv),std::end(s.normalRecv),link_.normalRecv);
    for(unsigned i=0;i<2;++i) {
        auto& n=nodes_[i]; auto& v=s.nodes[i]; auto& e=s.extra[i]; auto& t=*cores_[i]->timing;
        n.nextEvent=v.nextEvent; n.eventDiff=v.eventDiff; n.normalSO=v.normalSO;
        n.transferFinished=v.transferFinished; n.mode=v.mode;
        n.d.writeRegister=v.writeRegister;
        mTimingClear(&t); t.masterCycles=e.master; t.globalCycles=e.global;
        *t.relativeCycles=e.relative; *t.nextEvent=e.nextEvent;
        t.root=e.events.empty()?nullptr:e.events[0].binding; t.reroot=nullptr;
        for(size_t j=0;j<e.events.size();++j) {
            auto& ev=e.events[j]; ev.binding->when=ev.when; ev.binding->priority=ev.priority;
            ev.binding->next=j+1<e.events.size()?e.events[j+1].binding:nullptr;
        }
    }
}
uint32_t LocalLink::frame(unsigned slot) const { return cores_.at(slot)->frameCounter(cores_.at(slot)); }
std::array<std::vector<uint8_t>, 2> LocalLink::coreStates() const {
    std::array<std::vector<uint8_t>, 2> out;
    for (unsigned i = 0; i < 2; ++i) {
        out[i].resize(cores_[i]->stateSize(cores_[i]), 0);
        if (!cores_[i]->saveState(cores_[i], out[i].data())) throw std::runtime_error("capture failed");
    }
    return out;
}
uint64_t LocalLink::schedulerDigest() const {
    uint64_t h = 14695981039346656037ULL;
    auto add = [&](uint32_t n) { for (int b=0; b<4; ++b) { h ^= (n >> (b*8)) & 255; h *= 1099511628211ULL; } };
    add(waiting_); add(link_.d.transferActive); add(link_.d.transferCycles);
    add(link_.attachedMulti); add(link_.attachedNormal);
    for (unsigned i=0; i<2; ++i) {
        add(awake_[i]); add(credits_[i]); add(nodes_[i].nextEvent); add(nodes_[i].eventDiff);
        add(nodes_[i].mode); add(nodes_[i].normalSO); add(nodes_[i].transferFinished);
        add(link_.multiRecv[i]); add(link_.normalRecv[i]);
        auto* t = cores_[i]->timing;
        bool scheduled = mTimingIsScheduled(t, &nodes_[i].event);
        add(scheduled);
        if (scheduled) add(mTimingUntil(t, &nodes_[i].event));
    }
    return h;
}
}
