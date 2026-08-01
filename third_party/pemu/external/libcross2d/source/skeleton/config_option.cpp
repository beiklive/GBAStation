//
// Created by cpasjuste on 17/10/18.
//

#ifdef __DREAMCAST__

#include <cstdio>

#endif

#include "cross2d/skeleton/config_option.h"
#include "cross2d/skeleton/utility.h"

using namespace c2d;
using namespace c2d::config;

Option::Option(const std::string &name, const std::string &value, int id, const std::string &comment) {
    setName(name);
    setId(id);
    setString(value);
    setComment(comment);
    setType(Type::String);
}

Option::Option(const std::string &name, int value, int id, const std::string &comment) {
    setName(name);
    setId(id);
    setInteger(value);
    setComment(comment);
    setType(Type::Integer);
}

Option::Option(const std::string &name, float value, int id, const std::string &comment) {
    setName(name);
    setId(id);
    setFloat(value);
    setComment(comment);
    setType(Type::Float);
}

Option::Option(const std::string &name, const c2d::Vector2f &value, int id, const std::string &comment) {
    setName(name);
    setId(id);
    setVector2f(value);
    setComment(comment);
    setType(Type::Vector2f);
}

Option::Option(const std::string &name, const c2d::FloatRect &value, int id, const std::string &comment) {
    setName(name);
    setId(id);
    setFloatRect(value);
    setComment(comment);
    setType(Type::FloatRect);
}

Option::Option(const std::string &name, const Color &value, int id, const std::string &comment) {
    setName(name);
    setId(id);
    setColor(value);
    setComment(comment);
    setType(Type::Color);
}

Option::Option(const std::string &name, const std::vector<std::string> &values, int index, int id,
               const std::string &comment) {
    setName(name);
    setId(id);
    setArray(values, index);
    setComment(comment);
    setType(Type::Array);
}

std::string Option::getName() const {
    return name;
}

void Option::setName(const std::string &name) {
    this->name = name;
}

std::string Option::getComment() const {
    return info;
}

void Option::setComment(const std::string &info) {
    this->info = info;
}

int Option::getId() const {
    return id;
}

void Option::setId(int id) {
    this->id = id;
}

Option::Type Option::getType() const {
    return type;
}

void Option::setType(Option::Type type) {
    this->type = type;
}

void *Option::getUserData() {
    return user_data;
}

void Option::setUserData(void *data) {
    user_data = data;
}

std::string Option::getString() const {
#if defined(__DREAMCAST__) || defined(__PS3__)
    char str[128];
    if (type == Type::Integer) {
        snprintf(str, 127, "%i", integer);
        return std::string(str);
    } else if (type == Type::Float) {
        snprintf(str, 127, "%f", floatRect.left);
        return std::string(str);
    } else if (type == Type::Vector2f) {
        snprintf(str, 127, "%f, %f", floatRect.left, floatRect.top);
        return std::string(str);
    } else if (type == Type::FloatRect) {
        snprintf(str, 127, "%f, %f, %f, %f",
                 floatRect.left, floatRect.top, floatRect.width, floatRect.height);
        return std::string(str);
    } else if (type == Type::Color) {
        snprintf(str, 127, "%i, %i, %i, %i",
                 (int) floatRect.left, (int) floatRect.top, (int) floatRect.width, (int) floatRect.height);
        return std::string(str);
    } else if (type == Type::Array) {
        return array_index < (int) array.size() ? array[array_index] : "";
    }
#else
    if (type == Type::Integer) {
        return std::to_string(integer);
    } else if (type == Type::Float) {
        return std::to_string(floatRect.left);
    } else if (type == Type::Vector2f) {
        return std::to_string(floatRect.left) + ", " + std::to_string(floatRect.top);
    } else if (type == Type::FloatRect) {
        return std::to_string(floatRect.left) + ", " + std::to_string(floatRect.top) + ", "
               + std::to_string(floatRect.width) + ", " + std::to_string(floatRect.height);
    } else if (type == Type::Color) {
        return std::to_string((int) floatRect.left) + ", " + std::to_string((int) floatRect.top) + ", "
               + std::to_string((int) floatRect.width) + ", " + std::to_string((int) floatRect.height);
    } else if (type == Type::Array) {
        return array_index < (int) array.size() ? array[array_index] : "";
    }
#endif

    return string;
}

void Option::setString(const std::string &value) {
    string = value;
}

int Option::getInteger() {
    if (type == Type::Array) {
        if (array[array_index] == "OFF") return 0;
        else if (array[array_index] == "ON") return 1;
        else {
            return Utility::parseInt(array[array_index], 0);
        }
    }

    return integer;
}

void Option::setInteger(int value) {
    integer = value;
}

float Option::getFloat() {
    return floatRect.left;
}

void Option::setFloat(float value) {
    floatRect.left = value;
}

Vector2f Option::getVector2f() {
    return {floatRect.left, floatRect.top};
}

void Option::setVector2f(const Vector2f &value) {
    floatRect.left = value.x;
    floatRect.top = value.y;
}

FloatRect Option::getFloatRect() {
    return floatRect;
}

void Option::setFloatRect(const FloatRect &value) {
    floatRect = value;
}

Color Option::getColor() {
    return {(uint8_t) floatRect.left, (uint8_t) floatRect.top,
            (uint8_t) floatRect.width, (uint8_t) floatRect.height};
}

void Option::setColor(const Color &color) {
    floatRect.left = color.r;
    floatRect.top = color.g;
    floatRect.width = color.b;
    floatRect.height = color.a;
}

int Option::getArrayIndex() {
    return array_index;
}

void Option::setArrayIndex(int index) {
    if (index < (int) array.size()) array_index = index;
}

void Option::setArrayIndex(const std::string &value) {
    for (size_t i = 0; i < array.size(); i++) {
        if (array.at(i) == value) {
            array_index = (int) i;
            return;
        }
    }
    array_index = 0;
}

std::vector<std::string> Option::getArray() {
    return array;
}

void Option::setArray(const std::vector<std::string> &values, int index) {
    std::string current = getString();
    array = values;
    array_index = index;
    if (array_index < 0) {
        setArrayIndex(current);
    }
}

void Option::setArrayMoveNext() {
    array_index++;
    if (array_index >= (int) array.size()) {
        array_index = 0;
    }
}

void Option::setArrayMovePrev() {
    array_index--;
    if (array_index < 0) {
        array_index = (int) array.size() - 1;
    }
}
