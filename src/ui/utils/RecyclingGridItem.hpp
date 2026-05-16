#pragma once

#include <borealis.hpp>
#include <string>
#include <functional>

class RecyclingGrid;

class RecyclingGridItem : public brls::Box {
public:
    RecyclingGridItem();
    ~RecyclingGridItem() override;

    size_t getIndex() const;
    void setIndex(size_t value);

    virtual void prepareForReuse();
    virtual void cacheForReuse();

    std::string reuseIdentifier;

private:
    size_t index = 0;
};
