//
// Created by cpasjuste on 17/10/18.
//

#ifndef C2D_CONFIG_OPTION_H
#define C2D_CONFIG_OPTION_H

#include <string>
#include <vector>

#include "cross2d/skeleton/sfml/Vector2.hpp"
#include "cross2d/skeleton/sfml/Rect.hpp"
#include "cross2d/skeleton/sfml/Color.hpp"

namespace c2d::config {
    class Option {
    public:
        enum class Type {
            Integer,
            String,
            Float,
            Vector2f,
            FloatRect,
            Color,
            Array
        };

        /// Constructors
        Option(const std::string &name, const std::string &value,
               int id = -1, const std::string &comment = "");

        Option(const std::string &name, int value,
               int id = -1, const std::string &comment = "");

        Option(const std::string &name, float value,
               int id = -1, const std::string &comment = "");

        Option(const std::string &name, const Vector2f &value,
               int id = -1, const std::string &comment = "");

        Option(const std::string &name, const FloatRect &value,
               int id = -1, const std::string &comment = "");

        Option(const std::string &name, const Color &value,
               int id = -1, const std::string &comment = "");

        Option(const std::string &name, const std::vector<std::string> &values,
               int index = 0, int id = -1, const std::string &comment = "");

        /// Type::String
        [[nodiscard]] std::string getString() const;

        void setString(const std::string &value);

        /// Type::Integer
        int getInteger();

        void setInteger(int value);

        /// Type::Float
        float getFloat();

        void setFloat(float value);

        /// Type::Vector2f
        Vector2f getVector2f();

        void setVector2f(const Vector2f &value);

        /// Type::FloatRect
        FloatRect getFloatRect();

        void setFloatRect(const FloatRect &value);

        /// Type::Color
        Color getColor();

        void setColor(const Color &color);

        /// Type::Array
        int getArrayIndex();

        void setArrayIndex(int index);

        void setArrayIndex(const std::string &value);

        std::vector<std::string> getArray();

        void setArray(const std::vector<std::string> &values, int index = -1);

        void setArrayMoveNext();

        void setArrayMovePrev();

        /// ...
        [[nodiscard]] std::string getName() const;

        void setName(const std::string &name);

        [[nodiscard]] int getId() const;

        void setId(int id);

        [[nodiscard]] std::string getComment() const;

        void setComment(const std::string &info);

        [[nodiscard]] Type getType() const;

        void setType(Option::Type type);

        [[nodiscard]] unsigned int getFlags() const { return m_flags; }

        void setFlags(unsigned int flags) { m_flags = flags; }

        void *getUserData();

        void setUserData(void *data);

    private:
        std::string string{};
        int integer = 0;
        FloatRect floatRect{};
        std::vector<std::string> array{};
        int array_index = 0;

        std::string name{};
        std::string info{};
        Type type{};
        int id = -1;
        unsigned int m_flags = 0;
        void *user_data = nullptr;
    };
}

#endif //C2D_CONFIG_OPTION_H
