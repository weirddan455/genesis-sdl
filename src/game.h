#ifndef GAME_H
#define GAME_H

#include <stdint.h>

#include "SDL.h"

#define SPRITE_GROUND 0
#define SPRITE_GRASS 1
#define SPRITE_FLOWER 2
#define SPRITE_TORCH 3
#define SPRITE_ROCK 4
#define SPRITE_WATER_0 5
#define SPRITE_WATER_1 6
#define SPRITE_ICE 7
#define SPRITE_TREE_TOP 8
#define SPRITE_TREE_BOTTOM 9

#define SPRITE_MOB_UP_0 0
#define SPRITE_MOB_UP_1 1
#define SPRITE_MOB_DOWN_0 2
#define SPRITE_MOB_DOWN_1 3
#define SPRITE_MOB_RIGHT_0 4
#define SPRITE_MOB_RIGHT_1 5

#define SOLID (1 << 15)

#define TILE_GROUND SPRITE_GROUND
#define TILE_GRASS SPRITE_GRASS
#define TILE_FLOWER SPRITE_FLOWER
#define TILE_TORCH (SPRITE_TORCH << 7)
#define TILE_ROCK (SPRITE_ROCK | SOLID)
#define TILE_WATER (SPRITE_WATER_0 | SOLID)
#define TILE_ICE (SPRITE_ICE | SOLID)
#define TILE_TREE ((SPRITE_TREE_TOP << 7) | SOLID)

typedef struct TileMap
{
    int width;
    int height;
    uint16_t *tiles;
} TileMap;

extern const Uint8 *keyboard;

void init_game(int64_t ticks, SDL_Renderer *renderer);
void render_game(float delta, int64_t ticks, SDL_Renderer *renderer);
void update_game(float delta);

#endif
