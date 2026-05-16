#pragma once

#include <borealis.hpp>
#include <string>
#include <functional>

namespace beiklive {

class RecyclingGridItem : public brls::Box {
public:
    RecyclingGridItem();
    virtual ~RecyclingGridItem() = default;

    virtual void prepareForReuse();

    int getGridIndex() const { return m_gridIndex; }
    void setGridIndex(int index) { m_gridIndex = index; }

private:
    int m_gridIndex = -1;
};

} // namespace beiklive
