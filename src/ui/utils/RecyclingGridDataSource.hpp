#pragma once

#include <cstddef>

namespace beiklive {

class RecyclingGrid;
class RecyclingGridItem;

class RecyclingGridDataSource {
public:
    virtual ~RecyclingGridDataSource() = default;

    virtual size_t getItemCount() const = 0;

    virtual RecyclingGridItem* cellForRow(RecyclingGrid* grid, size_t index) = 0;

    virtual float heightForRow(RecyclingGrid* grid, size_t index) { return 120.0f; }
};

} // namespace beiklive
