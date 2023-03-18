#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "SDL.h"

#include "audio.h"
#include "assets.h"
#include "font.h"
#include "pcgrandom.h"
#include "game.h"

static SDL_Renderer *init_sdl()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init failed\n");
        exit(EXIT_FAILURE);
    }
    SDL_Window *window = SDL_CreateWindow("Genesis", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    if (renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    #ifndef BURNUP_MY_CPU
    if (SDL_RenderSetVSync(renderer, 1) != 0) {
        fprintf(stderr, "Warning: SDL_RenderSetVSync failed: %s\n", SDL_GetError());
    }
    #endif
    keyboard = SDL_GetKeyboardState(NULL);
    return renderer;
}

int main(int argv, char **argc)
{
    seed_rng();
    SDL_Renderer *renderer = init_sdl();
    init_fonts(renderer);
    init_audio();
    float delta = 0.0f;
    float fps_report = 0.0f;
    int frames = 0;
    float frequency = SDL_GetPerformanceFrequency();
    Uint64 start = SDL_GetPerformanceCounter();
    int64_t ticks = ((start / frequency) * 1000.0f) + 0.5f;
    init_game(ticks, renderer);
    while (1) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                return EXIT_SUCCESS;
            }
        }
        update_game(delta);
        render_game(delta, ticks, renderer);
        Uint64 end = SDL_GetPerformanceCounter();
        delta = (end - start) / frequency;
        #ifndef BURNUP_MY_CPU
        // FPS cap to 500fps.  Should not happen if vsync is working correctly.
        if (delta < 0.002f) {
            SDL_Delay(2);
            end = SDL_GetPerformanceCounter();
            delta = (end - start) / frequency;
        }
        #endif
        frames++;
        fps_report += delta;
        if (fps_report >= 1.0f) {
            printf("FPS: %f\n", frames / fps_report);
            frames = 0;
            fps_report = 0.0f;
        }
        start = end;
        ticks = ((end / frequency) * 1000.0f) + 0.5f;
    }
    return EXIT_SUCCESS;
}
