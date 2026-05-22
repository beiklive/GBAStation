#pragma once

#include <cstddef>

struct GridDrawItem;

class GameGridDataSource {
public:
    virtual ~GameGridDataSource() = default;

    virtual size_t getItemCount() = 0;
    virtual void populateItem(GridDrawItem& item, size_t index) = 0;
    virtual void onItemSelected(size_t index) = 0;
    virtual void clearData() = 0;
};
