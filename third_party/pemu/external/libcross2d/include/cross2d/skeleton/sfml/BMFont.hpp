////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2017 Laurent Gomila (laurent@sfml-dev.org)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

#ifndef C2D_BMFONT_HPP
#define C2D_BMFONT_HPP

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <map>
#include <string>
#include <vector>

#include "cross2d/skeleton/sfml/Font.hpp"
#include "cross2d/skeleton/texture.h"
#include "Glyph.hpp"
#include "Rect.hpp"
#include "Vector2.hpp"

namespace c2d {

////////////////////////////////////////////////////////////
/// \brief Class for loading and manipulating character fonts
///
////////////////////////////////////////////////////////////
    class BMFont : public Font {

    public:

        ~BMFont() override;

        void cleanup() override;

        bool loadFromFile(const std::string &fntPath) override;

        bool loadDefault() override {
            printf("BMFont::loadDefault: not implemented...\n");
            return false;
        }

        bool loadFromMemory(const void *data, std::size_t size) override {
            printf("BMFont::loadFromMemory: not implemented...\n");
            return false;
        }

        bool loadFromMemory(const char *fontData, size_t fontDataSize,
                            const char *texData, size_t texDataSize);

        const Glyph &getGlyph(uint32_t codePoint, unsigned int characterSize,
                              bool bold, float outlineThickness = 0) const override;

        float getKerning(uint32_t first, uint32_t second, unsigned int characterSize, bool bold) const override;

        float getLineSpacing(unsigned int characterSize) const override;

        float getUnderlinePosition(unsigned int characterSize) const override;

        float getUnderlineThickness(unsigned int characterSize) const override;

        Texture *getTexture(unsigned int characterSize) override;

        void setFilter(Texture::Filter filter) override;

        bool setCurrentSize(unsigned int characterSize) const override {
            return true;
        };

        bool isBmFont() override {
            return true;
        }

    private:

        Vector2f offset;
        Texture *m_texture = nullptr;

#define BMFONT_BLOCK_TYPE_INFO 1
#define BMFONT_BLOCK_TYPE_COMMON 2
#define BMFONT_BLOCK_TYPE_PAGES 3
#define BMFONT_BLOCK_TYPE_CHARS 4
#define BMFONT_BLOCK_TYPE_KERNINGS 5
#define BMFONT_MAX_PAGES 2

        struct bmfont_info {
            int16_t fontSize;
            uint8_t bitField;
            uint8_t charSet;
            uint16_t stretchH;
            uint8_t aa;
            uint8_t paddingUp;
            uint8_t paddingRight;
            uint8_t paddingDown;
            uint8_t paddingLeft;
            uint8_t spacingHoriz;
            uint8_t spacingVert;
            uint8_t outline;
        };
        struct bmfont_common {
            uint16_t lineHeight;
            uint16_t base;
            uint16_t scaleW;
            uint16_t scaleH;
            uint16_t pages;
            uint8_t bitField;
            uint8_t alphaChnl;
            uint8_t redChnl;
            uint8_t greenChnl;
            uint8_t blueChnl;
        };
        struct bmfont_char {
            uint32_t id;
            uint16_t x;
            uint16_t y;
            uint16_t width;
            uint16_t height;
            int16_t xoffset;
            int16_t yoffset;
            int16_t xadvance;
            uint8_t page;
            uint8_t chnl;
        };

        struct bmfont_kerningpairs {
            uint32_t first;
            uint32_t second;
            int16_t amount;
        };

        struct bmfont_stream {
            bmfont_stream(uint8_t *pBuffer, size_t size) :
                    pos(0),
                    size(size),
                    pByteData(pBuffer) {}

            // Unsigned
            uint8_t currentU8() {
                return (uint8_t) pByteData[pos];
            }

            uint8_t getU8() {
                uint8_t value = currentU8();
                pos += 1;
                return value;
            }

            uint16_t currentU16() {
                return (uint16_t) ((pByteData[pos + 1] << 8) | pByteData[pos]);
            }

            uint16_t getU16() {
                uint16_t value = currentU16();
                pos += 2;
                return value;
            }

            uint32_t currentU32() {
                return (uint32_t) ((pByteData[pos + 3] << 24) | (pByteData[pos + 2] << 16) | (pByteData[pos + 1] << 8) |
                                   pByteData[pos]);
            }

            uint32_t getU32() {
                uint32_t value = currentU32();
                pos += 4;
                return value;
            }

            // Signed
            int8_t currentS8() {
                return (int8_t) pByteData[pos];
            }

            int8_t getS8() {
                auto value = (int8_t) currentU8();
                pos += 1;
                return value;
            }

            int16_t currentS16() {
                return (int16_t) ((pByteData[pos + 1] << 8) | pByteData[pos + 0]);
            }

            int16_t getS16() {
                auto value = (int16_t) currentU16();
                pos += 2;
                return value;
            }

            int32_t currentS32() {
                return (int32_t) ((pByteData[pos + 3] << 24) | (pByteData[pos + 2] << 16) | (pByteData[pos + 1] << 8) |
                                  pByteData[pos]);
            }

            int32_t getS32() {
                auto value = (int32_t) currentU32();
                pos += 4;
                return value;
            }

            uint8_t *getPtr() {
                return &pByteData[pos];
            }

            void offsetBy(size_t byteCount) {
                pos += byteCount;
            }

            [[nodiscard]] bool isEOF() const {
                return pos >= size;
            }

        private:
            size_t pos;
            size_t size;
            uint8_t *pByteData;
        };

        struct bmfont_page {
            char name[32];
            uint8_t length;
        };

        struct bmfont_with_kp {
            bmfont_info info;
            bmfont_common common;
            bmfont_page pages[BMFONT_MAX_PAGES];
            std::map<int, Glyph> glyphs;
            std::vector<bmfont_kerningpairs> kerningPairs;
        };

        bmfont_with_kp m_bmfont;
    };

} // namespace c2d

#endif // C2D_BMFONT_HPP

////////////////////////////////////////////////////////////
/// \class sf::Font
/// \ingroup graphics
///
/// Fonts can be loaded from a file, from memory or from a custom
/// stream, and supports the most common types of fonts. See
/// the loadFromFile function for the complete list of supported formats.
///
/// Once it is loaded, a sf::Font instance provides three
/// types of information about the font:
/// \li Global metrics, such as the line spacing
/// \li Per-glyph metrics, such as bounding box or kerning
/// \li Pixel representation of glyphs
///
/// Fonts alone are not very useful: they hold the font data
/// but cannot make anything useful of it. To do so you need to
/// use the sf::Text class, which is able to properly output text
/// with several options such as character size, style, color,
/// position, rotation, etc.
/// This separation allows more flexibility and better performances:
/// indeed a sf::Font is a heavy resource, and any operation on it
/// is slow (often too slow for real-time applications). On the other
/// side, a sf::Text is a lightweight object which can combine the
/// glyphs data and metrics of a sf::Font to display any text on a
/// render target.
/// Note that it is also possible to bind several sf::Text instances
/// to the same sf::Font.
///
/// It is important to note that the sf::Text instance doesn't
/// copy the font that it uses, it only keeps a reference to it.
/// Thus, a sf::Font must not be destructed while it is
/// used by a sf::Text (i.e. never write a function that
/// uses a local sf::Font instance for creating a text).
///
/// Usage example:
/// \code
/// // Declare a new font
/// sf::Font font;
///
/// // Load it from a file
/// if (!font.loadFromFile("arial.ttf"))
/// {
///     // error...
/// }
///
/// // Create a text which uses our font
/// sf::Text text1;
/// text1.setFont(font);
/// text1.setCharacterSize(30);
/// text1.setStyle(sf::Text::Regular);
///
/// // Create another text using the same font, but with different parameters
/// sf::Text text2;
/// text2.setFont(font);
/// text2.setCharacterSize(50);
/// text2.setStyle(sf::Text::Italic);
/// \endcode
///
/// Apart from loading font files, and passing them to instances
/// of sf::Text, you should normally not have to deal directly
/// with this class. However, it may be useful to access the
/// font metrics or rasterized glyphs for advanced usage.
///
/// Note that if the font is a bitmap font, it is not scalable,
/// thus not all requested sizes will be available to use. This
/// needs to be taken into consideration when using sf::Text.
/// If you need to display text of a certain size, make sure the
/// corresponding bitmap font that supports that size is used.
///
/// \see sf::Text
///
////////////////////////////////////////////////////////////
