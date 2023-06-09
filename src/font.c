#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "SDL.h"

#include "game.h"
#include "font.h"

typedef struct Glyph
{
    int width;
    int height;
    int bearingX;
    int bearingY;
    int advance;
    SDL_Texture *texture;
} Glyph;

static Glyph glyphs[94];
static int space_advance;

void init_fonts(void)
{
    FT_Library library;
    if (FT_Init_FreeType(&library) != 0) {
        fprintf(stderr, "FT_Init_FreeType failed\n");
        exit(-1);
    }
    FT_Face face;
    if (FT_New_Face(library, "res/fonts/FreeSans.otf", 0, &face) != 0) {
        fprintf(stderr, "FT_New_Face failed\n");
        exit(-1);
    }
    FT_Set_Pixel_Sizes(face, 0, 30);
    for (int i = 0; i < 94; i++) {
        char ascii_code = i + 33;
        if (FT_Load_Char(face, ascii_code, FT_LOAD_RENDER) != 0) {
            fprintf(stderr, "Warning: glyph for %c not found", ascii_code);
            continue;
        }
        glyphs[i].width = face->glyph->bitmap.width;
        glyphs[i].height = face->glyph->bitmap.rows;
        glyphs[i].bearingX = face->glyph->bitmap_left;
        glyphs[i].bearingY = face->glyph->bitmap_top;
        glyphs[i].advance = (face->glyph->advance.x + 31) / 64;

        SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, glyphs[i].width, glyphs[i].height, 32, SDL_PIXELFORMAT_ARGB8888);
        if (surface == NULL) {
            fprintf(stderr, "SDL_CreateRGBSurfaceWithFormat failed: %s\n", SDL_GetError());
            exit(-1);
        }
        SDL_LockSurface(surface);
        uint8_t *src = face->glyph->bitmap.buffer;
        uint32_t *dst = surface->pixels;
        uint32_t color = 0x00ffffff;
        int pixels = glyphs[i].width * glyphs[i].height;
        for (int p = 0; p < pixels; p++) {
            *dst++ = (*src++ << 24) | color;
        }
        SDL_UnlockSurface(surface);
        glyphs[i].texture = SDL_CreateTextureFromSurface(renderer.sdl, surface);
        if (glyphs[i].texture == NULL) {
            fprintf(stderr, "SDL_CreateTextureFromSurface failed: %s\n", SDL_GetError());
            exit(-1);
        }
        SDL_FreeSurface(surface);
    }
    if (FT_Load_Char(face, ' ', 0) == 0) {
        space_advance = (face->glyph->advance.x + 31) / 64;
    } else {
        fprintf(stderr, "FT_Load_Char: Failed to get advance information for space. Using default of 10.\n");
        space_advance = 10;
    }
    FT_Done_FreeType(library);
}

static int get_string_width(const char *string)
{
    int width = 0;
    for (const char *s = string; *s != '\0'; s++) {
        int i = *s - 33;
        if (i >= 0 && i < 94) {
            width += glyphs[i].advance;
        } else {
            width += space_advance;
        }
    }
    return width;
}

void render_string(const char *string, int x, int y)
{
    for (const char *s = string; *s != '\0'; s++) {
        int i = *s - 33;
        if (i < 0 || i >= 94) {
            x += space_advance;
            continue;
        }
        SDL_Rect rect;
        rect.x = x + glyphs[i].bearingX;
        rect.y = y - glyphs[i].bearingY;
        rect.w = glyphs[i].width;
        rect.h = glyphs[i].height;
        SDL_RenderCopy(renderer.sdl, glyphs[i].texture, NULL, &rect);
        x += glyphs[i].advance;
    }
}

void render_string_centered(const char *string, int x, int y)
{
    render_string(string, x - (get_string_width(string) / 2), y);
}

void render_string_right(const char *string, int x, int y)
{
    render_string(string, x - get_string_width(string), y);
}
