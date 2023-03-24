#ifndef FONT_H
#define FONT_H

#include "SDL.h"

void init_fonts(void);
void render_string(const char *string, int x, int y);
void render_string_centered(const char *string, int x, int y);
void render_string_right(const char *string, int x, int y);

#endif
