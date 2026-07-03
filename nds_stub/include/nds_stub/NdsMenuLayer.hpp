#pragma once

#include <cstdint>

namespace beiklive::nds_stub {

enum class NdsMenuAction {
    None,
    ResetGame,
    ExitGame,
};

class NdsMenuLayer {
public:
    NdsMenuAction update(std::uint64_t buttonsDown);
    void draw(double fps, long long runMs) const;

    bool visible() const { return m_visible; }

private:
    bool m_visible = false;
};

} // namespace beiklive::nds_stub
