#ifndef ASSETS_H
#define ASSETS_H

#include <SDL2/SDL.h>

#include "game.h"

SDL_Texture *load_sprites(const char *filename, SDL_Renderer *renderer);
void load_level(TileMap *tile_map, const char *filename);

#endif
