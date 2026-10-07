// Experimental cooperative adapter for the vendored mGBA local cable driver.
// MPL-2.0; callbacks follow the cycle-credit contract in MultiplayerController.cpp.
#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <memory>
#include <mgba/core/core.h>
#include <mgba/internal/gba/sio/lockstep.h>

namespace netlink {
class LocalLink {
public:
    class Snapshot {
        struct Data;
        std::shared_ptr<const Data> data_;
        friend class LocalLink;
    public:
        bool valid() const { return bool(data_); }
        size_t bytes() const;
        const std::vector<uint8_t>& save(unsigned slot) const;
        const std::vector<uint8_t>& state(unsigned slot) const;
        // Same-build checksum; not a canonical portable state hash.
        bool sameRegressionState(const Snapshot&) const;
        uint64_t regressionDigest() const;
    };
    struct Resources {
        std::array<std::vector<uint8_t>,2> roms;
        std::array<std::vector<uint8_t>,2> saves;
        std::vector<uint8_t> bios;
        bool skipBios=true;
        double sampleRate=0;
        int64_t rtcEpochMs=1700000000000LL;
        std::array<std::vector<uint8_t>,2> states;
    };
    explicit LocalLink(const Resources&);
    LocalLink(const uint8_t* rom0, const uint8_t* rom1, size_t size);
    ~LocalLink();
    LocalLink(const LocalLink&) = delete;
    LocalLink& operator=(const LocalLink&) = delete;
    // One reference-core video frame; both key masks latch at this sequence point.
    // The peer's cycle phase is preserved, not forced to a second frame boundary.
    unsigned advance(std::array<uint16_t, 2> keys);
    uint32_t read(unsigned slot, uint32_t address) const;
    uint32_t frame(unsigned slot) const;
    const std::vector<int16_t>& audio(unsigned slot) const {return audio_.at(slot);}
    const std::vector<color_t>& pixels(unsigned slot) const {return pixels_.at(slot);}
    std::array<std::vector<uint8_t>, 2> coreStates() const;
    uint64_t schedulerDigest() const;
    Snapshot capture() const;
    void restore(const Snapshot&);
    void write(unsigned slot, uint32_t address, uint8_t value);
    void write16(unsigned slot,uint32_t address,uint16_t value);
    bool transferBusy() const { return link_.d.transferActive != TRANSFER_IDLE; }
private:
    std::array<mCore*, 2> cores_{};
    std::array<std::vector<color_t>, 2> pixels_;
    std::array<std::vector<int16_t>,2> audio_;
    GBASIOLockstep link_{};
    std::array<GBASIOLockstepNode, 2> nodes_{};
    std::array<int32_t, 2> credits_{};
    std::array<bool, 2> awake_{{true, true}};
    unsigned waiting_ = 0;
    unsigned attached_ = 0;
    uint64_t identity_ = 0;
    void release();
    static LocalLink& owner(mLockstep*);
    static bool signal(mLockstep*, unsigned);
    static bool wait(mLockstep*, unsigned);
    static void addCycles(mLockstep*, int, int32_t);
    static int32_t useCycles(mLockstep*, int, int32_t);
    static int32_t unusedCycles(mLockstep*, int);
    static void unload(mLockstep*, int);
};
}
