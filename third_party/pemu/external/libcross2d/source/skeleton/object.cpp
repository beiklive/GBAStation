//
// Created by cpasjuste on 12/12/17.
//

#include "cross2d/c2d.h"

using namespace c2d;

void C2DObject::add(C2DObject *object) {
    if (object) {
        //printf("C2DObject(%p): add(%p)\n", this, object);
        object->parent = this;
        object->setLayer(object->getLayer());
        object->transformation = transformation * ((Transformable *) this)->getTransform();
        childs.push_back(object);
    }
}

void C2DObject::remove(C2DObject *object) {
    if (!childs.empty()) {
        childs.erase(std::remove(childs.begin(), childs.end(), object), childs.end());
    }
}

void C2DObject::add(Tween *tween) {
    if (tween) {
        tween->setTransform((Transformable *) this);
        tweens.push_back(tween);
    }
}

void C2DObject::remove(Tween *tween) {
    if (tween && !tweens.empty()) {
        tweens.erase(std::remove(tweens.begin(), tweens.end(), tween), tweens.end());
    }
}

bool C2DObject::onInput(Input::Player *players) {
    for (auto &child: childs) {
        if (child && child->isVisible()) {
            if (child->onInput(players)) {
                return true;
            }
        }
    }

    return false;
}

void C2DObject::onUpdate() {
    for (auto &child: childs) {
        if (child) {
            child->onUpdate();
        }
    }
}

void C2DObject::onDraw(Transform &transform, bool draw) {
    //printf("C2DObject(%p): draw\n", this);
    if (visibility_current == Visibility::Hidden) {
        return;
    }

    // handle tweeners
    for (auto &tween: tweens) {
        if (tween) {
            tween->step();
            // hide object if needed
            if (tween->getState() == TweenState::Stopped
                && visibility_current != visibility_wanted) {
                visibility_current = visibility_wanted;
            }
        }
    }

    Transform combinedTransform = transformation = transform;
    combinedTransform *= ((Transformable *) this)->getTransform();
    for (auto &child: childs) {
        if (child && child->visibility_current == Visibility::Visible) {
            child->onDraw(combinedTransform, draw);
        }
    }
}

void C2DObject::setVisibility(Visibility v, bool tweenPlay) {
    if (v == visibility_wanted) {
        return;
    }

    if (tweenPlay && !tweens.empty()) {
        if (v == Visibility::Visible) {
            // we want the object to be visible (drawn) immediately
            visibility_current = visibility_wanted = Visibility::Visible;
        } else {
            visibility_wanted = Visibility::Hidden;
        }
        for (auto &tween: tweens) {
            if (tween) {
                tween->play(visibility_wanted == Visibility::Visible ?
                            TweenDirection::Forward : TweenDirection::Backward);
                // reset transform/color to initial values
                tween->step();
            }
        }
    } else {
        visibility_current = visibility_wanted = v;
        for (auto &tween: tweens) {
            if (tween) {
                tween->reset();
            }
        }
    }

    // set child drawing state (for "isVisible")
    setDrawingState(visibility_wanted == Visibility::Visible);
}

bool C2DObject::isVisible() {
    return m_is_drawn && visibility_wanted == Visibility::Visible;
}

void C2DObject::setDrawingState(bool isDrawn) {
    m_is_drawn = isDrawn;
    for (auto &child: childs) {
        if (child) {
            child->setDrawingState(isDrawn);
        }
    }
}

void C2DObject::setAlpha(uint8_t alpha, bool recursive) {
    if (recursive) {
        for (auto &child: childs) {
            if (child) {
                child->setAlpha(alpha, recursive);
            }
        }
    }
}

DeleteMode C2DObject::getDeleteMode() {
    return deleteMode;
}

void C2DObject::setDeleteMode(DeleteMode mode) {
    deleteMode = mode;
}

int C2DObject::getLayer() {
    return layer;
}

static bool sortByLayer(C2DObject *o1, C2DObject *o2) {
    return o1->getLayer() < o2->getLayer();
}

void C2DObject::setLayer(int l) {
    this->layer = l;
    if (parent) {
        std::stable_sort(parent->childs.begin(),
                         parent->childs.end(),
                         sortByLayer);
    }
}

std::vector<C2DObject *> C2DObject::getChilds() {
    return childs;
}

C2DObject *C2DObject::getChild(const std::string &name) {
    for (const auto &child: childs) {
        if (child->m_name == name) {
            return child;
        }
        C2DObject *obj = child->getChild(name);
        if (obj) {
            return obj;
        }
    }

    return nullptr;
}

Type C2DObject::getType() const {
    return type;
}

C2DObject::~C2DObject() {
    //printf("~C2DObject(%p): childs = %i\n", this, (int) childs.size());
    // delete tweeners
    for (auto tween = tweens.begin(); tween != tweens.end();) {
        if (*tween) {
            delete (*tween);
            C2DObject::remove(*tween);
        }
    }
    tweens.clear();

    // delete childs
    for (auto widget = childs.begin(); widget != childs.end();) {
        if (*widget) {
            if ((*widget)->deleteMode == DeleteMode::Auto) {
                //printf("\t~C2DObject(%p): delete child(%p)\n", this, *widget);
                delete (*widget);
            }
        }
    }
    childs.clear();

    // remove from parent
    if (parent) {
        //printf("~C2DObject(%p): remove from parent(%p)\n", this, parent);
        parent->remove(this);
    }
}
