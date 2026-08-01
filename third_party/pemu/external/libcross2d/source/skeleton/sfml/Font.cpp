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

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "cross2d/c2d.h"
#include "cross2d/skeleton/sfml/Font.hpp"

#ifndef __NO_FREETYPE__

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_BITMAP_H
#include FT_STROKER_H

#endif

extern unsigned char c2d_font_default[];
extern int c2d_font_default_length;

namespace c2d {
////////////////////////////////////////////////////////////
    Font::Font() :
            m_library(nullptr),
            m_face(nullptr),
            m_streamRec(nullptr),
            m_stroker(nullptr),
            m_refCount(nullptr),
            m_info() {
    }

////////////////////////////////////////////////////////////
    Font::~Font() {
        Font::cleanup();
    }

////////////////////////////////////////////////////////////
    bool Font::loadFromFile(const std::string &filename) {
#ifndef __NO_FREETYPE__
        // Cleanup the previous resources
        cleanup();
        m_refCount = new int(1);

        // Initialize FreeType
        // Note: we initialize FreeType for every font instance in order to avoid having a single
        // global manager that would create a lot of issues regarding creation and destruction order.
        FT_Library library;
        if (FT_Init_FreeType(&library) != 0) {
            printf("Failed to load font %s (failed to initialize FreeType)\n", filename.c_str());
            return false;
        }
        m_library = library;

        // Load the new font face from the specified file
        FT_Face face;
        if (FT_New_Face(static_cast<FT_Library>(m_library), filename.c_str(), 0, &face) != 0) {
            printf("Failed to load font %s (failed to create the font face)\n", filename.c_str());
            return false;
        }

        // Load the stroker that will be used to outline the font
        FT_Stroker stroker;
        if (FT_Stroker_New(static_cast<FT_Library>(m_library), &stroker) != 0) {
            printf("Failed to load font %s (failed to create the stroker)\n", filename.c_str());
            FT_Done_Face(face);
            return false;
        }

        // Select the unicode character map
        if (FT_Select_Charmap(face, FT_ENCODING_UNICODE) != 0) {
            printf("Failed to load font %s (failed to set the Unicode character set)\n", filename.c_str());
            FT_Stroker_Done(stroker);
            FT_Done_Face(face);
            return false;
        }

        // Store the loaded font in our ugly void* :)
        m_stroker = stroker;
        m_face = face;

        // Store the font information
        m_info.family = face->family_name ? face->family_name : std::string();
        m_font_path = filename;

        return true;
#else
        return false;
#endif
    }

////////////////////////////////////////////////////////////
    bool Font::loadDefault() {
        bool res = loadFromMemory(
                c2d_font_default, (std::size_t) c2d_font_default_length);
#ifdef __DREAMCAST__
        setOffset({0, 1});
#else
        setOffset({0, -5});
#endif
        return res;
    }

////////////////////////////////////////////////////////////
    bool Font::loadFromMemory(const void *data, std::size_t sizeInBytes) {
#ifndef __NO_FREETYPE__
        // Cleanup the previous resources
        cleanup();
        m_refCount = new int(1);

        // Initialize FreeType
        // Note: we initialize FreeType for every font instance in order to avoid having a single
        // global manager that would create a lot of issues regarding creation and destruction order.
        FT_Library library;
        if (FT_Init_FreeType(&library) != 0) {
            printf("Failed to load font from memory (failed to initialize FreeType)\n");
            return false;
        }
        m_library = library;

        // Load the new font face from the specified file
        FT_Face face;
        if (FT_New_Memory_Face(static_cast<FT_Library>(m_library), reinterpret_cast<const FT_Byte *>(data),
                               static_cast<FT_Long>(sizeInBytes), 0, &face) != 0) {
            printf("Failed to load font from memory (failed to create the font face)\n");
            return false;
        }

        // Load the stroker that will be used to outline the font
        FT_Stroker stroker;
        if (FT_Stroker_New(static_cast<FT_Library>(m_library), &stroker) != 0) {
            printf("Failed to load font from memory (failed to create the stroker)\n");
            FT_Done_Face(face);
            return false;
        }

        // Select the Unicode character map
        if (FT_Select_Charmap(face, FT_ENCODING_UNICODE) != 0) {
            printf("Failed to load font from memory (failed to set the Unicode character set)\n");
            FT_Stroker_Done(stroker);
            FT_Done_Face(face);
            return false;
        }

        // Store the loaded font in our ugly void* :)
        m_stroker = stroker;
        m_face = face;

        // Store the font information
        m_info.family = face->family_name ? face->family_name : std::string();

        return true;
#else
        return false;
#endif
    }

////////////////////////////////////////////////////////////
    const Font::Info &Font::getInfo() const {
        return m_info;
    }


////////////////////////////////////////////////////////////
    const Glyph &Font::getGlyph(uint32_t codePoint, unsigned int characterSize,
                                bool bold, float outlineThickness) const {
        // Get the page corresponding to the character size
        GlyphTable &glyphs = m_pages[characterSize].glyphs;
        if (m_pages[characterSize].texture != nullptr) {
            m_pages[characterSize].texture->setFilter(m_filtering);
        }

        // Build the key by combining the code point, bold flag, and outline thickness
        uint64_t key = (static_cast<uint64_t>(*&outlineThickness) << 32)
                       | (static_cast<uint64_t>(bold ? 1 : 0) << 31)
                       | static_cast<uint64_t>(codePoint);

        // Search the glyph into the cache
        if (auto it = glyphs.find(key); it != glyphs.end()) {
            // Found: just return it
            return it->second;
        } else {
            // Not found: we have to load it
            Glyph glyph = loadGlyph(codePoint, characterSize, bold, outlineThickness);
            //printf("Font::getGlyph(%c): advance: %f\n", codePoint, glyph.advance);
            return glyphs.emplace(key, glyph).first->second;
        }
    }


////////////////////////////////////////////////////////////
    float Font::getKerning(uint32_t first, uint32_t second, unsigned int characterSize, bool bold) const {
#ifndef __NO_FREETYPE__
        // Special case where first or second is 0 (null character)
        if (first == 0 || second == 0)
            return 0.f;

        auto face = static_cast<FT_Face>(m_face);

        if (face && setCurrentSize(characterSize)) {
            // Convert the characters to indices
            FT_UInt index1 = FT_Get_Char_Index(face, first);
            FT_UInt index2 = FT_Get_Char_Index(face, second);

            // Retrieve position compensation deltas generated by FT_LOAD_FORCE_AUTOHINT flag
            auto firstRsbDelta = static_cast<float>(getGlyph(first, characterSize, bold).rsbDelta);
            auto secondLsbDelta = static_cast<float>(getGlyph(second, characterSize, bold).lsbDelta);

            // Get the kerning vector if present
            FT_Vector kerning;
            kerning.x = kerning.y = 0;
            if (FT_HAS_KERNING(face))
                FT_Get_Kerning(face, index1, index2, FT_KERNING_UNFITTED, &kerning);

            // X advance is already in pixels for bitmap fonts
            if (!FT_IS_SCALABLE(face))
                return static_cast<float>(kerning.x);

            // Combine kerning with compensation deltas and return the X advance
            // Flooring is required as we use FT_KERNING_UNFITTED flag which is not quantized in 64 based grid
            return std::floor(
                    (secondLsbDelta - firstRsbDelta + static_cast<float>(kerning.x) + 32) / static_cast<float>(1 << 6));
        } else {
            // Invalid font, or no kerning
            return 0.f;
        }
#else
        return 0;
#endif
    }


////////////////////////////////////////////////////////////
    float Font::getLineSpacing(unsigned int characterSize) const {
#ifndef __NO_FREETYPE__
        auto face = static_cast<FT_Face>(m_face);

        if (face && setCurrentSize(characterSize)) {
            return ((float) face->size->metrics.height / (float) (1 << 6));
        } else {
            return 0.f;
        }
#else
        return 0;
#endif
    }


////////////////////////////////////////////////////////////
    float Font::getUnderlinePosition(unsigned int characterSize) const {
#ifndef __NO_FREETYPE__
        auto face = static_cast<FT_Face>(m_face);

        if (face && setCurrentSize(characterSize)) {
            // Return a fixed position if font is a bitmap font
            if (!FT_IS_SCALABLE(face))
                return (float) characterSize / 10.f;

            return -static_cast<float>(FT_MulFix(face->underline_position, face->size->metrics.y_scale)) /
                   static_cast<float>(1 << 6);
        } else {
            return 0.f;
        }
#else
        return 0;
#endif
    }


////////////////////////////////////////////////////////////
    float Font::getUnderlineThickness(unsigned int characterSize) const {
#ifndef __NO_FREETYPE__
        auto face = static_cast<FT_Face>(m_face);

        if (face && setCurrentSize(characterSize)) {
            // Return a fixed thickness if font is a bitmap font
            if (!FT_IS_SCALABLE(face))
                return (float) characterSize / 14.f;

            return static_cast<float>(FT_MulFix(face->underline_thickness, face->size->metrics.y_scale)) /
                   static_cast<float>(1 << 6);
        } else {
            return 0.f;
        }
#else
        return 0;
#endif
    }


////////////////////////////////////////////////////////////
    Texture *Font::getTexture(unsigned int characterSize) {
        return m_pages[characterSize].texture;
    }

    void Font::setFilter(Texture::Filter filter) {
        m_filtering = filter;
        for (unsigned int i = 0; i < m_pages.size(); i++) {
            m_pages[i].texture->setFilter(filter);
        }
    }

    Texture::Filter Font::getFilter() {
        return m_filtering;
    }

    void Font::setOffset(Vector2f off) {
        m_offset = off;
    }

    Vector2f Font::getOffset() const {
        return m_offset;
    }

////////////////////////////////////////////////////////////
    void Font::cleanup() {
#ifndef __NO_FREETYPE__
        // Check if we must destroy the FreeType pointers
        if (m_refCount) {
            // Decrease the reference counter
            (*m_refCount)--;

            // Free the resources only if we are the last owner
            if (*m_refCount == 0) {
                // Delete the reference counter
                delete m_refCount;

                // Destroy the stroker
                if (m_stroker)
                    FT_Stroker_Done(static_cast<FT_Stroker>(m_stroker));

                // Destroy the font face
                if (m_face)
                    FT_Done_Face(static_cast<FT_Face>(m_face));

                // Destroy the stream rec instance, if any (must be done after FT_Done_Face!)
                if (m_streamRec)
                    delete static_cast<FT_StreamRec *>(m_streamRec);

                // Close the library
                if (m_library)
                    FT_Done_FreeType(static_cast<FT_Library>(m_library));
            }
        }

        // Reset members
        m_library = nullptr;
        m_face = nullptr;
        m_stroker = nullptr;
        m_streamRec = nullptr;
        m_refCount = nullptr;
        m_pages.clear();
        std::vector<uint8_t>().swap(m_pixelBuffer);
#endif
    }


////////////////////////////////////////////////////////////
    Glyph Font::loadGlyph(uint32_t codePoint, unsigned int characterSize, bool bold, float outlineThickness) const {
#ifndef __NO_FREETYPE__
        // The glyph to return
        Glyph glyph;

        // First, transform our ugly void* to a FT_Face
        auto face = static_cast<FT_Face>(m_face);
        if (!face)
            return glyph;

        // Set the character size
        if (!setCurrentSize(characterSize))
            return glyph;

        // Load the glyph corresponding to the code point
#ifdef __3DS__
        // FT_LOAD_FORCE_AUTOHINT crash on 3ds (?!)
        FT_Int32 flags = FT_LOAD_TARGET_NORMAL;
#else
        FT_Int32 flags = FT_LOAD_TARGET_NORMAL | FT_LOAD_FORCE_AUTOHINT;
#endif

        if (outlineThickness != 0)
            flags |= FT_LOAD_NO_BITMAP;
        if (FT_Load_Char(face, codePoint, flags) != 0)
            return glyph;

        // Retrieve the glyph
        FT_Glyph glyphDesc = nullptr;
        if (FT_Get_Glyph(face->glyph, &glyphDesc)) {
            printf("Font::loadGlyph: FT_Get_Glyph error\n");
            return glyph;
        }

        // Apply bold and outline (there is no fallback for outline) if necessary -- first technique using outline (highest quality)
        FT_Pos weight = 1 << 6;
        bool outline = (glyphDesc->format == FT_GLYPH_FORMAT_OUTLINE);
        if (outline) {
            if (bold) {
                auto outlineGlyph = (FT_OutlineGlyph) glyphDesc;
                FT_Outline_Embolden(&outlineGlyph->outline, weight);
            }

            if (outlineThickness != 0) {
                auto stroker = static_cast<FT_Stroker>(m_stroker);

                FT_Stroker_Set(stroker, static_cast<FT_Fixed>(outlineThickness * static_cast<float>(1 << 6)),
                               FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
                FT_Glyph_Stroke(&glyphDesc, stroker, true);
            }
        }

        // Convert the glyph to a bitmap (i.e. rasterize it)
        if (FT_Glyph_To_Bitmap(&glyphDesc, FT_RENDER_MODE_NORMAL, nullptr, 1)) {
            printf("Font::loadGlyph(%u): FT_Glyph_To_Bitmap error\n", codePoint);
            FT_Done_Glyph(glyphDesc);
            return glyph;
        }

        auto bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyphDesc);
        FT_Bitmap &bitmap = bitmapGlyph->bitmap;

        // Apply bold if necessary -- fallback technique using bitmap (lower quality)
        if (!outline) {
            if (bold)
                FT_Bitmap_Embolden(static_cast<FT_Library>(m_library), &bitmap, weight, weight);

            if (outlineThickness != 0)
                printf("Failed to outline glyph (no fallback available)\n");
        }

        // Compute the glyph's advance offset
        glyph.advance = static_cast<float>(bitmapGlyph->root.advance.x >> 16);
        if (bold)
            glyph.advance += static_cast<float>(weight) / static_cast<float>(1 << 6);

        unsigned int width = bitmap.width;
        unsigned int height = bitmap.rows;

        if ((width > 0) && (height > 0)) {
            // Leave a small padding around characters, so that filtering doesn't
            // pollute them with pixels from neighbors
            const unsigned int padding = 2;

            width += 2 * padding;
            height += 2 * padding;

            // Get the glyphs page corresponding to the character size
            Page &page = m_pages[characterSize];
            page.texture->setFilter(m_filtering);

            // Find a good position for the new glyph into the texture
            glyph.textureRect = findGlyphRect(page, width, height);
            if (glyph.textureRect == IntRect()) {
                return glyph;
            }

            // Make sure the texture data is positioned in the center
            // of the allocated texture rectangle
            glyph.textureRect.left += padding;
            glyph.textureRect.top += padding;
            glyph.textureRect.width -= 2 * padding;
            glyph.textureRect.height -= 2 * padding;

            // Compute the glyph's bounding box
            glyph.bounds.left = static_cast<float>(bitmapGlyph->left);
            glyph.bounds.top = static_cast<float>(-bitmapGlyph->top);
            glyph.bounds.width = static_cast<float>(bitmap.width);
            glyph.bounds.height = static_cast<float>(bitmap.rows);

            // Resize the pixel buffer to the new size and fill it with transparent white pixels
            m_pixelBuffer.resize(width * height * 4);
            uint8_t *current = m_pixelBuffer.data();
            uint8_t *end = current + width * height * 4;

#ifdef __C2D_ARGB__
            while (current != end) {
                (*current++) = 0;
                (*current++) = 255;
                (*current++) = 255;
                (*current++) = 255;
            }
#else
            while (current != end) {
                (*current++) = 255;
                (*current++) = 255;
                (*current++) = 255;
                (*current++) = 0;
            }
#endif
            // Extract the glyph's pixels from the bitmap
            const uint8_t *pixels = bitmap.buffer;
            if (bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
                // Pixels are 1 bit monochrome values
                for (unsigned int y = padding; y < height - padding; ++y) {
                    for (unsigned int x = padding; x < width - padding; ++x) {
                        // The color channels remain white, just fill the alpha channel
                        std::size_t index = x + y * width;
#ifdef __C2D_ARGB__
                        m_pixelBuffer[index * 4 + 0] =
                                static_cast<unsigned char>(((pixels[(x - padding) / 8]) &
                                                            (1 << (7 - ((x - padding) % 8))))
                                                           ? 255 : 0);
#else
                        m_pixelBuffer[index * 4 + 3] =
                                ((pixels[(x - padding) / 8]) & (1 << (7 - ((x - padding) % 8)))) ? 255 : 0;
#endif
                    }
                    pixels += bitmap.pitch;
                }
            } else {
                // Pixels are 8 bits gray levels
                for (unsigned int y = padding; y < height - padding; ++y) {
                    for (unsigned int x = padding; x < width - padding; ++x) {
                        // The color channels remain white, just fill the alpha channel
                        std::size_t index = x + y * width;
#ifdef __C2D_ARGB__
                        m_pixelBuffer[index * 4 + 0] = pixels[x - padding];
#else
                        m_pixelBuffer[index * 4 + 3] = pixels[x - padding];
#endif
                    }
                    pixels += bitmap.pitch;
                }
            }

            // Write the pixels to the texture
            unsigned int x = glyph.textureRect.left - padding;
            unsigned int y = glyph.textureRect.top - padding;
            unsigned int w = glyph.textureRect.width + 2 * padding;
            unsigned int h = glyph.textureRect.height + 2 * padding;

            uint8_t *dst;
            int pitch;
            IntRect rect = {(int) x, (int) y, (int) w, (int) h};
            page.texture->lock(&dst, &pitch, rect);

            const uint8_t *src = m_pixelBuffer.data();

            for (unsigned int i = 0; i < h; i++) {
                memcpy(dst, src, w * 4);
                src += w * 4;
                dst += pitch;
            }

            page.texture->setUnpackRowLength(page.texture->getTextureSize().x);
            page.texture->unlock();
            page.texture->setUnpackRowLength(0);
        }

        // Delete the FT glyph
        FT_Done_Glyph(glyphDesc);

        // Done :)
        return glyph;
#else
        return {};
#endif
    }


////////////////////////////////////////////////////////////
    IntRect Font::findGlyphRect(Page &page, unsigned int width, unsigned int height) const {
        // Find the line that fits well the glyph
        Row *row = nullptr;
        float bestRatio = 0;
        for (auto it = page.rows.begin(); it != page.rows.end() && !row; ++it) {
            float ratio = static_cast<float>(height) / (float) it->height;

            // Ignore rows that are either too small or too high
            if ((ratio < 0.7f) || (ratio > 1.f))
                continue;

            // Check if there's enough horizontal space left in the row
            if (width > page.texture->getTextureSize().x - it->width)
                continue;

            // Make sure that this new row is the best found so far
            if (ratio < bestRatio)
                continue;

            // The current row passed all the tests: we can select it
            row = &*it;
            bestRatio = ratio;
        }

        // If we didn't find a matching row, create a new one (10% taller than the glyph)
        if (!row) {
            unsigned int rowHeight = height + height / 10;
            while ((page.nextRow + rowHeight >= (unsigned int) page.texture->getTextureSize().y)
                   || (width >= (unsigned int) page.texture->getTextureSize().x)) {
                // Not enough space: resize the texture if possible
                auto texWidth = (unsigned int) page.texture->getTextureSize().x;
                auto texHeight = (unsigned int) page.texture->getTextureSize().y;
                if ((texWidth * 2 <= 2048) && (texHeight * 2 <= 2048)) {
                    //printf("Font:: resizing font texture from %ix%i to %ix%i\n",
                    //     texWidth, texHeight, texWidth * 2, texHeight * 2);
                    page.texture->resize({(int) texWidth * 2, (int) texHeight * 2}, true);
                } else {
                    // Oops, we've reached the maximum texture size...
                    printf("Failed to add a new character to the font: the maximum texture size has been reached\n");
                    return {0, 0, 0, 0};
                }
            }

            // We can now create the new row
            page.rows.emplace_back(page.nextRow, rowHeight);
            page.nextRow += rowHeight;
            row = &page.rows.back();
        }

        // Find the glyph's rectangle on the selected row
        IntRect rect(Rect<unsigned int>({row->width, row->top}, {width, height}));

        // Update the row informations
        row->width += width;

        return rect;
    }


////////////////////////////////////////////////////////////
    bool Font::setCurrentSize(unsigned int characterSize) const {
#ifndef __NO_FREETYPE__
        // FT_Set_Pixel_Sizes is an expensive function, so we must call it
        // only when necessary to avoid killing performances

        auto face = static_cast<FT_Face>(m_face);
        FT_UShort currentSize = face->size->metrics.x_ppem;

        if (currentSize != characterSize) {
            FT_Error result = FT_Set_Pixel_Sizes(face, 0, characterSize);

            if (result == FT_Err_Invalid_Pixel_Size) {
                // In the case of bitmap fonts, resizing can
                // fail if the requested size is not available
                if (!FT_IS_SCALABLE(face)) {
                    printf("Failed to set bitmap font size to %i\n", characterSize);
                    printf("Available sizes are: ");
                    for (int i = 0; i < face->num_fixed_sizes; ++i)
                        printf("%i ", face->available_sizes[i].height);
                    printf("\n");
                }
            }

            return result == FT_Err_Ok;
        } else {
            return true;
        }
#else
        return false;
#endif
    }

    const std::string &Font::getPath() const {
        return m_font_path;
    }


////////////////////////////////////////////////////////////

    Font::Page::Page() : nextRow(3) {
        //printf("Font:: create new tex\n");
        texture = new C2DTexture({128, 128}, Texture::Format::RGBA8);

        // Reserve a 2x2 white square for texturing underlines
        uint8_t *buffer;
        texture->lock(&buffer, nullptr, {0, 0, 2, 2});
        int w = texture->getTextureSize().x;
        int bpp = texture->m_bpp;
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                uint8_t *pixel = &buffer[(x + y * w) * bpp];
                *pixel++ = 255;
                *pixel++ = 255;
                *pixel++ = 255;
                *pixel++ = 255;
            }
        }
        texture->unlock();
    }

    Font::Page::~Page() {
        //printf("~Page\n");
        delete (texture);
    }

} // namespace sf
