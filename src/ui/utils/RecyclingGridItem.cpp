#include "RecyclingGridItem.hpp"
#include "RecyclingGrid.hpp"

RecyclingGridItem::RecyclingGridItem()
{
    setFocusable(true);
    setHideHighlight(false);
    setHideClickAnimation(false);

    registerClickAction([this](brls::View* view) {
        (void)view;
        auto* recycler = dynamic_cast<RecyclingGrid*>(getParent()->getParent());
        if (recycler && recycler->getDataSource())
            recycler->getDataSource()->onItemSelected(recycler, index);
        return true;
    });
}

RecyclingGridItem::~RecyclingGridItem() = default;

size_t RecyclingGridItem::getIndex() const { return index; }

void RecyclingGridItem::setIndex(size_t value) { index = value; }

void RecyclingGridItem::prepareForReuse() {}

void RecyclingGridItem::cacheForReuse() {}
