#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "SDL.h"

#include "audio.h"
#include "assets.h"
#include "font.h"
#include "pcgrandom.h"
#include "game.h"

static void init_sdl()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init failed\n");
        exit(EXIT_FAILURE);
    }
    SDL_Window *window = SDL_CreateWindow("Genesis", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WORLD_WIDTH * 3, WORLD_HEIGHT * 3, SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    renderer.sdl = SDL_CreateRenderer(window, -1, SDL_RENDERER_TARGETTEXTURE);
    if (renderer.sdl == NULL) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    if (!SDL_RenderTargetSupported(renderer.sdl)) {
        fprintf(stderr, "Renderer does not support texture rendering targets\n");
        exit(EXIT_FAILURE);
    }
    renderer.world_target = SDL_CreateTexture(renderer.sdl, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, WORLD_WIDTH, WORLD_HEIGHT);
    if (renderer.world_target == NULL) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    #ifndef BURNUP_MY_CPU
    if (SDL_RenderSetVSync(renderer.sdl, 1) != 0) {
        fprintf(stderr, "Warning: SDL_RenderSetVSync failed: %s\n", SDL_GetError());
    }
    #endif
    keyboard = SDL_GetKeyboardState(NULL);
    if (SDL_GetRendererOutputSize(renderer.sdl, &renderer.width, &renderer.height) != 0) {
        fprintf(stderr, "SDL_GetRendererOutputSize failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
}

int main(int argv, char **argc)
{
    seed_rng();
    init_sdl();
    init_fonts();
    init_audio();
    float delta = 0.0f;
    float fps_report = 0.0f;
    int frames = 0;
    float frequency = SDL_GetPerformanceFrequency();
    Uint64 start = SDL_GetPerformanceCounter();
    int64_t ticks = ((start / frequency) * 1000.0f) + 0.5f;
    init_game(ticks);
    while (1) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                return EXIT_SUCCESS;
            }
        }
        SDL_SetRenderTarget(renderer.sdl, NULL);
        SDL_RenderClear(renderer.sdl);
        if (SDL_GetRendererOutputSize(renderer.sdl, &renderer.width, &renderer.height) != 0) {
            fprintf(stderr, "SDL_GetRendererOutputSize failed: %s\n", SDL_GetError());
            exit(EXIT_FAILURE);
        }
        SDL_SetRenderTarget(renderer.sdl, renderer.world_target);
        SDL_RenderClear(renderer.sdl);
        update_game(delta);
        render_game(delta, ticks);
        SDL_SetRenderTarget(renderer.sdl, NULL);
        float x_scale = (float)renderer.width / (float)WORLD_WIDTH;
        float y_scale = (float)renderer.height / (float)WORLD_HEIGHT;
        float scale;
        if (x_scale < y_scale) {
            scale = x_scale;
        } else {
            scale = y_scale;
        }
        SDL_FRect dstrect;
        dstrect.w = (float)WORLD_WIDTH * scale;
        dstrect.h = (float)WORLD_HEIGHT * scale;
        dstrect.x = ((float)renderer.width - dstrect.w) * 0.5f;
        dstrect.y = ((float)renderer.height - dstrect.h) * 0.5f;
        SDL_RenderCopyF(renderer.sdl, renderer.world_target, NULL, &dstrect);
        render_overlay(ticks);
        SDL_RenderPresent(renderer.sdl);
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
