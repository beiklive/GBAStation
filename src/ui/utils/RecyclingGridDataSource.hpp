#pragma once

#include <cstddef>

class RecyclingGrid;
class RecyclingGridItem;

class RecyclingGridDataSource {
public:
    virtual ~RecyclingGridDataSource() = default;

    virtual size_t getItemCount() = 0;
    virtual RecyclingGridItem* cellForRow(RecyclingGrid* grid, size_t index) = 0;
    virtual void onItemSelected(RecyclingGrid* grid, size_t index) = 0;
    virtual void clearData() = 0;
};
