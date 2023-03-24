#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>
#include "SDL.h"

#include "assets.h"
#include "game.h"
#include "pcgrandom.h"

#define COLOR_GRASS 0xffffffff
#define COLOR_ROCK 0xff44C4FF
#define COLOR_WATER 0xff3A5EFF
#define COLOR_ICE 0xffB5C2FF
#define COLOR_FLOWER 0xff00FF04
#define COLOR_TREE 0xff156B20
#define COLOR_TORCH 0xffFFFF00

#define COLOR_TRANSPARENT 0xffff00ff
#define COLOR_LINE 0xff7f007f

#define COLOR_FEMALE_RED 0xffBC2823
#define COLOR_FEMALE_TEAL 0xff4ED35B

typedef struct Png
{
    uint8_t channels;
    uint32_t width;
    uint32_t height;
    int num_palette;
    png_colorp palette;
    png_structp png_ptr;
    png_infop info_ptr;
    uint8_t **rowpointers;
    FILE *file;
} Png;

static void load_png(const char *filename, Png *png)
{
    memset(png, 0, sizeof(Png));
    png->file = fopen(filename, "rb");
    if (png->file == NULL) {
        fprintf(stderr, "fopen failed for %s: %s\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }
    png->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png->png_ptr == NULL) {
        fprintf(stderr, "png_create_read_struct failed\n");
        exit(EXIT_FAILURE);
    }
    png->info_ptr = png_create_info_struct(png->png_ptr);
    if (png->info_ptr == NULL) {
        fprintf(stderr, "png_create_info_struct failed\n");
        exit(EXIT_FAILURE);
    }
    png_init_io(png->png_ptr, png->file);
    png_read_png(png->png_ptr, png->info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    png->width = png_get_image_width(png->png_ptr, png->info_ptr);
    png->height = png_get_image_height(png->png_ptr, png->info_ptr);
    if (png->width == 0 || png->height == 0) {
        fprintf(stderr, "Invalid size. Width: %u Height: %u\n", png->width, png->height);
        exit(EXIT_FAILURE);
    }
    uint8_t depth = png_get_bit_depth(png->png_ptr, png->info_ptr);
    if (depth != 8) {
        fprintf(stderr, "Invalid bit depth: %hhu\n", depth);
        exit(EXIT_FAILURE);
    }
    png->channels = png_get_channels(png->png_ptr, png->info_ptr);
    uint8_t color_type = png_get_color_type(png->png_ptr, png->info_ptr);
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        if (png->channels != 1) {
            fprintf(stderr, "Invalid channels for pallete PNG: %hhu\n", png->channels);
            exit(EXIT_FAILURE);
        }
        png_get_PLTE(png->png_ptr, png->info_ptr, &png->palette, &png->num_palette);
    } else if (png->channels != 3 && png->channels != 4) {
        fprintf(stderr, "Invalid channels for non-pallete PNG: %hhu\n", png->channels);
        exit(EXIT_FAILURE);
    }
    size_t rowbytes = png_get_rowbytes(png->png_ptr, png->info_ptr);
    size_t expected_rowbytes = png->width * png->channels;
    if (rowbytes != expected_rowbytes) {
        fprintf(stderr, "Invalid rowbytes: %zu Expceted: %zu\n", rowbytes, expected_rowbytes);
        exit(EXIT_FAILURE);
    }
    png->rowpointers = png_get_rows(png->png_ptr, png->info_ptr);
}

static void destroy_png(Png *png)
{
    png_destroy_read_struct(&png->png_ptr, &png->info_ptr, NULL);
    fclose(png->file);
}

static uint32_t get_png_pixel(uint32_t x, uint32_t y, Png *png)
{
    if (png->channels == 1 && png->palette) {
        uint8_t index = png->rowpointers[y][x];
        if (index >= png->num_palette) {
            fprintf(stderr, "Invalid palette index: %hhu\n", index);
            exit(EXIT_FAILURE);
        }
        return (255 << 24) | (png->palette[index].red << 16) | (png->palette[index].green << 8) | (png->palette[index].blue);
    } else if (png->channels == 3) {
        uint8_t *row = png->rowpointers[y];
        row += (x * 3);
        return (255 << 24) | (row[0] << 16) | (row[1] << 8) | row[2];
    } else if (png->channels == 4) {
        uint8_t *row = png->rowpointers[y];
        row += (x * 4);
        return (row[3] << 24) | (row[0] << 16) | (row[1] << 8) | (row[2]);
    } else {
        fprintf(stderr, "get_png_pixel invalid state\n");
        exit(EXIT_FAILURE);
    }
}

SDL_Texture *load_sprites(const char *filename)
{
    Png png;
    load_png(filename, &png);
    if (png.width != 256 || png.height != 256) {
        fprintf(stderr, "Invalid sprite size. Width: %u Height: %u\n", png.width, png.height);
        exit(EXIT_FAILURE);
    }
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, png.width, png.height, 32, SDL_PIXELFORMAT_ARGB8888);
    if (surface == NULL) {
        fprintf(stderr, "SDL_CreateRGBSurfaceWithFormat failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    SDL_LockSurface(surface);
    uint32_t *pixels = surface->pixels;
    for (uint32_t y = 0; y < png.height; y++) {
        for (uint32_t x = 0; x < png.width; x++) {
            uint32_t color = get_png_pixel(x, y, &png);
            if (color == COLOR_TRANSPARENT || color == COLOR_LINE) {
                *pixels++ = 0;
            } else {
                *pixels++ = color;
            }
        }
    }
    // Copy the female sprites, replacing the red pixels with teal for the "virgin" sprites.
    // Place them in an empty area of the sprite sheet.
    pixels = surface->pixels;
    for (int src_y = 112, dst_y = 144; src_y < 128; src_y++, dst_y++) {
        for (int src_x = 96, dst_x = 0; src_x < 192; src_x++, dst_x++) {
            uint32_t color = pixels[(src_y * png.width) + src_x];
            if (color == COLOR_FEMALE_RED) {
                color = COLOR_FEMALE_TEAL;
            }
            pixels[(dst_y * png.width) + dst_x] = color;
        }
    }
    SDL_UnlockSurface(surface);
    destroy_png(&png);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer.sdl, surface);
    if (texture == NULL) {
        fprintf(stderr, "SDL_CreateTextureFromSurface failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    SDL_FreeSurface(surface);
    return texture;
}

static void set_tree_collision(TileMap *tile_map, int x, int y)
{
    if (x + 1 < tile_map->width) {
        tile_map->tiles[(y * tile_map->width) + x + 1] |= SOLID;
    }
    if (y + 1 < tile_map->height) {
        tile_map->tiles[((y + 1) * tile_map->width) + x] |= SOLID;
        if (x + 1 < tile_map->width) {
            tile_map->tiles[((y + 1) * tile_map->width) + x + 1] |= SOLID;
        }
    }
}

void load_level(TileMap *tile_map, const char *filename)
{
    Png png;
    load_png(filename, &png);
    tile_map->width = png.width;
    tile_map->height = png.height;
    if (tile_map->width <= 0 || tile_map->height <= 0) {
        fprintf(stderr, "Invalid tilemap dementions. Width: %d Height: %d\n", tile_map->width, tile_map->height);
        exit(EXIT_FAILURE);
    }
    tile_map->tiles = calloc(tile_map->width * tile_map->height, sizeof(uint16_t));
    if (tile_map->tiles == NULL) {
        fprintf(stderr, "calloc failed\n");
        exit(EXIT_FAILURE);
    }
    uint16_t *dst = tile_map->tiles;
    for (int y = 0; y < tile_map->height; y++) {
        for (int x = 0; x < tile_map->width; x++) {
            uint32_t color = get_png_pixel(x, y, &png);
            switch (color) {
                case COLOR_GRASS:
                    if (pcg_ranged_random(8) == 0) {
                        *dst |= TILE_GRASS;
                    }
                    break;
                case COLOR_ROCK:
                    *dst |= TILE_ROCK;
                    break;
                case COLOR_WATER:
                    *dst |= TILE_WATER;
                    break;
                case COLOR_ICE:
                    *dst |= TILE_ICE;
                    break;
                case COLOR_FLOWER:
                    *dst |= TILE_FLOWER;
                    break;
                case COLOR_TORCH:
                    *dst |= TILE_TORCH;
                    if (pcg_ranged_random(8) == 0) {
                        *dst |= TILE_GRASS;
                    }
                    break;
                case COLOR_TREE:
                    *dst |= TILE_TREE;
                    set_tree_collision(tile_map, x, y);
                    break;
            }
            dst++;
        }
    }
    destroy_png(&png);
}
