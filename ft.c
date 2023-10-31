#include "gltty.h"

FT_Face init_ft_and_load_font(FT_Library *ft, const char *font_path, int font_size)
{
    FT_Error error;
    error = FT_Init_FreeType(ft);
    if (error) {
        fatal("Failed to init freetype2");
    }

    FT_Face face;
    error = FT_New_Face(*ft, font_path, 0, &face);
    if (error) {
        fatal("Failed to load font: %s", font_path);
    }
    if (!FT_IS_FIXED_WIDTH(face)) {
        fatal("Font should be a monospace font.");
    }

    error = FT_Set_Char_Size(face, 0, font_size * 64, 96, 96);
    if (error) {
        fatal("Failed to set font size.");
    }

    return face;
}

void load_ascii_chars(FT_Face face, Character *ascii_chars, int *atlas_width, int *atlas_height)
{
    int width = 0;
    int height = face->size->metrics.height >> 6;

    for (int c = ASCII_BEGIN; c <= ASCII_END; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            fatal("Failed to load glyph for character: %c\n", c);
        }
        int index = c - ASCII_BEGIN;
        ascii_chars[index].c = c;
        ascii_chars[index].bitmap.width = face->glyph->bitmap.width;
        ascii_chars[index].bitmap.height = face->glyph->bitmap.rows;
        ascii_chars[index].bearing.left = face->glyph->bitmap_left;
        ascii_chars[index].bearing.top = face->glyph->bitmap_top;
        ascii_chars[index].advance = face->glyph->advance.x >> 6;
        size_t size = ascii_chars[index].bitmap.width * ascii_chars[index].bitmap.height;
        ascii_chars[index].bitmap.data = malloc(size);
        memcpy(ascii_chars[index].bitmap.data, face->glyph->bitmap.buffer, size);
        width += ascii_chars[index].bitmap.width;
    }

    *atlas_width = width;
    *atlas_height = height;
}
