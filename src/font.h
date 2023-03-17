#ifndef FONT_H
#define FONT_H

#include <SDL2/SDL.h>

void init_fonts(SDL_Renderer *renderer);
void render_string(char *string, int x, int y, SDL_Renderer *renderer);

#endif
