#ifndef FONT_H
#define FONT_H

#include "SDL.h"

void init_fonts(SDL_Renderer *renderer);
void render_string(const char *string, int x, int y, SDL_Renderer *renderer);
void render_string_centered(const char *string, int x, int y, SDL_Renderer *renderer);
void render_string_right(const char *string, int x, int y, SDL_Renderer *renderer);

#endif
