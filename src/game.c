#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL.h"

#include "audio.h"
#include "assets.h"
#include "font.h"
#include "game.h"
#include "pcgrandom.h"

#define BACKGROUND(tile) ((tile) & 127)
#define FOREGROUND(tile) (((tile) >> 7) & 127)

// Animations flip every n milliseconds.
// Using power of 2 bitwise AND operations for performance.
// Could switch to a modulo operation if we need better granularity but these values seem fine.
#define MOB_ANIMATION(ticks) ((ticks) & 128)
#define WATER_ANIMATION(ticks) ((ticks) & 1024)

#define MOB_SPEED 65.0f
#define SCALE 3

#define TILE_SIZE 16

// Returns world coordinate centered on a given tile
#define TILE_TO_WORLD(tile) (((float)(tile) * (float)TILE_SIZE) + ((float)TILE_SIZE * 0.5f))

#define DOWN 0
#define UP 1
#define RIGHT 2
#define LEFT 3

typedef struct Sprite
{
    float x;
    float y;
    uint8_t facing;
    bool walking;
} Sprite;

typedef struct Mob
{
    Sprite sprite;
    int8_t x_direction;
    int8_t y_direction;
} Mob;

typedef struct MobArray
{
    Mob *mobs;
    size_t size;
    size_t capacity;
} MobArray;

static const SDL_Rect world_sprites[] = {
    [SPRITE_GROUND] = {0, 0, 16, 16},
    [SPRITE_GRASS] = {32, 0, 16, 16},
    [SPRITE_FLOWER] = {48, 0, 16, 16},
    [SPRITE_TORCH] = {0, 16, 16, 16},
    [SPRITE_ROCK] = {64, 0, 16, 16},
    [SPRITE_WATER_0] = {16, 16, 16, 16},
    [SPRITE_WATER_1] = {32, 16, 16, 16},
    [SPRITE_ICE] = {64, 16, 16, 16},
    [SPRITE_TREE_TOP] = {80, 0, 32, 16},
    [SPRITE_TREE_BOTTOM] = {80, 16, 32, 16}
};

static const SDL_Rect player_sprites[] = {
    [SPRITE_MOB_UP_0] = {0, 112, 16, 16},
    [SPRITE_MOB_UP_1] = {16, 112, 16, 16},
    [SPRITE_MOB_DOWN_0] = {32, 112, 16, 16},
    [SPRITE_MOB_DOWN_1] = {48, 112, 16, 16},
    [SPRITE_MOB_RIGHT_0] = {64, 112, 16, 16},
    [SPRITE_MOB_RIGHT_1] = {80, 112, 16, 16}
};

static const SDL_Rect female_sprites[] = {
    [SPRITE_MOB_UP_0] = {96, 112, 16, 16},
    [SPRITE_MOB_UP_1] = {112, 112, 16, 16},
    [SPRITE_MOB_DOWN_0] = {128, 112, 16, 16},
    [SPRITE_MOB_DOWN_1] = {144, 112, 16, 16},
    [SPRITE_MOB_RIGHT_0] = {160, 112, 16, 16},
    [SPRITE_MOB_RIGHT_1] = {176, 112, 16, 16}
};

static const SDL_Rect virgin_female_sprites[] = {
    [SPRITE_MOB_UP_0] = {0, 144, 16, 16},
    [SPRITE_MOB_UP_1] = {16, 144, 16, 16},
    [SPRITE_MOB_DOWN_0] = {32, 144, 16, 16},
    [SPRITE_MOB_DOWN_1] = {48, 144, 16, 16},
    [SPRITE_MOB_RIGHT_0] = {64, 144, 16, 16},
    [SPRITE_MOB_RIGHT_1] = {80, 144, 16, 16}
};

const Uint8 *keyboard;
Renderer renderer;

static SDL_Texture *sprite_texture;
static TileMap tile_map;
static Sprite player = {TILE_TO_WORLD(54), TILE_TO_WORLD(23), DOWN, false};
static MobArray females;
static MobArray virgin_females;
static MobArray children;
static int64_t start_ticks;
static float population;
static float population_growth = 3.0f;

static void init_mob_array(MobArray *array)
{
    array->size = 0;
    array->capacity = 16;
    array->mobs = malloc(array->capacity * sizeof(Mob));
    if (array->mobs == NULL) {
        fprintf(stderr, "malloc failed\n");
        exit(EXIT_FAILURE);
    }
}

static void randomize_sprite_position(Sprite *sprite)
{
    uint32_t x_tile, y_tile;
    do {
        x_tile = pcg_ranged_random(22) + 43;
        y_tile = pcg_ranged_random(14) + 57;
    } while (tile_map.tiles[(y_tile * tile_map.width) + x_tile] & SOLID);
    sprite->x = TILE_TO_WORLD(x_tile);
    sprite->y = TILE_TO_WORLD(y_tile);
}

static void randomize_mob_direction(Mob *mob)
{
    if (pcg_ranged_random(40) == 0) {
        mob->x_direction = (int8_t)pcg_ranged_random(3) - 1;
        mob->y_direction = (int8_t)pcg_ranged_random(3) - 1;
        mob->sprite.walking = false;
        if (mob->x_direction == 1) {
            mob->sprite.facing = RIGHT;
            mob->sprite.walking = true;
        }
        if (mob->x_direction == -1) {
            mob->sprite.facing = LEFT;
            mob->sprite.walking = true;
        }
        if (mob->y_direction == -1) {
            mob->sprite.facing = UP;
            mob->sprite.walking = true;
        }
        if (mob->y_direction == 1) {
            mob->sprite.facing = DOWN;
            mob->sprite.walking = true;
        }
    }
}

static void add_mob(Mob *mob, MobArray *array)
{
    if (array->size >= array->capacity) {
        array->capacity *= 2;
        array->mobs = realloc(array->mobs, array->capacity * sizeof(Mob));
        if (array->mobs == NULL) {
            fprintf(stderr, "realloc failed\n");
            exit(EXIT_FAILURE);
        }
    }
    memcpy(array->mobs + array->size, mob, sizeof(Mob));
    array->size += 1;
}

void init_game(int64_t ticks)
{
    start_ticks = ticks;
    sprite_texture = load_sprites("res/sprites.png");
    load_level(&tile_map, "res/levels/ocean.png");
    init_mob_array(&females);
    init_mob_array(&virgin_females);
    init_mob_array(&children);
    Mob first_female = {
        {TILE_TO_WORLD(66), TILE_TO_WORLD(26), DOWN, false},
        0, 0
    };
    add_mob(&first_female, &virgin_females);
}

static void mob_move(Mob *mob, float mob_speed)
{
    float x = mob->sprite.x + (mob_speed * mob->x_direction);
    float y = mob->sprite.y + (mob_speed * mob->y_direction);

    int cur_tile_x = mob->sprite.x / TILE_SIZE;
    int cur_tile_y = mob->sprite.y / TILE_SIZE;
    int new_tile_x = x / TILE_SIZE;
    int new_tile_y = y / TILE_SIZE;
    if (!(tile_map.tiles[(cur_tile_y * tile_map.width) + new_tile_x] & SOLID)) {
        mob->sprite.x = x;
    }
    if (!(tile_map.tiles[(new_tile_y * tile_map.width) + cur_tile_x] & SOLID)) {
        mob->sprite.y = y;
    }
}

void update_game(float delta)
{
    static float mob_timer = 0.0f;
    mob_timer += delta;
    population += (population_growth * delta);
    float mob_speed = delta * MOB_SPEED;
    float x = player.x;
    float y = player.y;
    player.walking = false;
    if (keyboard[SDL_SCANCODE_RIGHT] || keyboard[SDL_SCANCODE_D]) {
        x += mob_speed;
        player.facing = RIGHT;
        player.walking = true;
    }
    if (keyboard[SDL_SCANCODE_LEFT] || keyboard[SDL_SCANCODE_A]) {
        x -= mob_speed;
        player.facing = LEFT;
        player.walking = true;
    }
    if (keyboard[SDL_SCANCODE_UP] || keyboard[SDL_SCANCODE_W]) {
        y -= mob_speed;
        player.facing = UP;
        player.walking = true;
    }
    if (keyboard[SDL_SCANCODE_DOWN] || keyboard[SDL_SCANCODE_S]) {
        y += mob_speed;
        player.facing = DOWN;
        player.walking = true;
    }

    int cur_tile_x = player.x / TILE_SIZE;
    int cur_tile_y = player.y / TILE_SIZE;
    int new_tile_x = x / TILE_SIZE;
    int new_tile_y = y / TILE_SIZE;
    if (!(tile_map.tiles[(cur_tile_y * tile_map.width) + new_tile_x] & SOLID)) {
        player.x = x;
    }
    if (!(tile_map.tiles[(new_tile_y * tile_map.width) + cur_tile_x] & SOLID)) {
        player.y = y;
    }

    if (mob_timer >= 0.02f) {
        for (size_t i = 0; i < children.size; i++) {
            randomize_mob_direction(children.mobs + i);
        }
        for (size_t i = 0; i < females.size; i++) {
            randomize_mob_direction(females.mobs + i);
        }
        for (size_t i = 0; i < virgin_females.size; i++) {
            randomize_mob_direction(virgin_females.mobs + i);
        }
        mob_timer = 0.0f;
    }

    for (size_t i = 0; i < children.size; i++) {
        mob_move(children.mobs + i, mob_speed);
    }
    for (size_t i = 0; i < females.size; i++) {
        mob_move(females.mobs + i, mob_speed);
    }
    for (size_t i = 0; i < virgin_females.size; i++) {
        mob_move(virgin_females.mobs + i, mob_speed);
    }

    SDL_FRect player_rect;
    player_rect.x = player.x - (TILE_SIZE * 0.5f);
    player_rect.y = player.y - (TILE_SIZE * 0.5f);
    player_rect.w = TILE_SIZE;
    player_rect.h = TILE_SIZE;
    for (size_t i = 0; i < virgin_females.size; i++) {
        SDL_FRect female_rect;
        female_rect.x = virgin_females.mobs[i].sprite.x - (TILE_SIZE * 0.5f);
        female_rect.y = virgin_females.mobs[i].sprite.y - (TILE_SIZE * 0.5f);
        female_rect.w = TILE_SIZE;
        female_rect.h = TILE_SIZE;
        if (SDL_HasIntersectionF(&player_rect, &female_rect)) {
            population_growth += (population_growth * 0.25f);
            play_sound(&breed);
            uint32_t num_children = pcg_ranged_random(4) + 1;
            for (uint32_t c = 0; c < num_children; c++) {
                Mob child = {
                    {virgin_females.mobs[i].sprite.x, virgin_females.mobs[i].sprite.y, DOWN, false},
                    0, 0
                };
                randomize_mob_direction(&child);
                add_mob(&child, &children);
            }
            add_mob(virgin_females.mobs + i, &females);
            randomize_sprite_position(&virgin_females.mobs[i].sprite);
            if (pcg_get_random() & 1) {
                Mob virgin;
                memset(&virgin, 0, sizeof(Mob));
                randomize_sprite_position(&virgin.sprite);
                add_mob(&virgin, &virgin_females);                
            }
        }
    }
}

static void render_mob(Sprite *sprite, const SDL_Rect *srcrect, int64_t ticks)
{
    SDL_RendererFlip flip = SDL_FLIP_NONE;
    switch (sprite->facing) {
        case DOWN:
            if (sprite->walking) {
                srcrect += SPRITE_MOB_DOWN_1;
                if (MOB_ANIMATION(ticks)) {
                    flip = SDL_FLIP_HORIZONTAL;
                }
            } else {
                srcrect += SPRITE_MOB_DOWN_0;
            }
            break;
        case UP:
            if (sprite->walking) {
                srcrect += SPRITE_MOB_UP_1;
                if (MOB_ANIMATION(ticks)) {
                    flip = SDL_FLIP_HORIZONTAL;
                }
            } else {
                srcrect += SPRITE_MOB_UP_0;
            }
            break;
        case RIGHT:
            if (sprite->walking && MOB_ANIMATION(ticks)) {
                srcrect += SPRITE_MOB_RIGHT_1;
            } else {
                srcrect += SPRITE_MOB_RIGHT_0;
            }
            break;
        case LEFT:
            if (sprite->walking && MOB_ANIMATION(ticks)) {
                srcrect += SPRITE_MOB_RIGHT_1;
            } else {
                srcrect += SPRITE_MOB_RIGHT_0;
            }
            flip = SDL_FLIP_HORIZONTAL;
            break;
    }

    SDL_FRect dstrect;
    dstrect.x = (sprite->x - player.x) + (WORLD_WIDTH * 0.5f) - (TILE_SIZE * 0.5f);
    dstrect.y = (sprite->y - player.y) + (WORLD_HEIGHT * 0.5f) - (TILE_SIZE * 0.5f);
    dstrect.w = TILE_SIZE;
    dstrect.h = TILE_SIZE;
    SDL_RenderCopyExF(renderer.sdl, sprite_texture, srcrect, &dstrect, 0, NULL, flip);
}

/*
 * Currently we're just rendering everything.  I had some code previously to only render the part of the tile map that was on screen.
 * Checkout commit a2c5b2367b409036eb8728dcfc55f7581da87e3b to see old code.
 *
 * However, it was hard to follow and I believe incorrect as well (some tiles appeared slightly off center).
 * Removing this hurt performance somewhat but SDL still does an intersect check with the viewport internally so we're not *actually* sending all these draw calls to the GPU.
 * see build/_deps/sdl2-src/src/render/SDL_render.c:3521
 *
 * May still want to revisit optimizations here in the future.
 */
void render_game(float delta, int64_t ticks)
{
    for (int y = 0; y < tile_map.height; y++) {
        for (int x = 0; x < tile_map.width; x++) {
            uint16_t tile = tile_map.tiles[(y * tile_map.width) + x];
            const SDL_Rect *sprite;
            if (tile == TILE_WATER && WATER_ANIMATION(ticks)) {
                sprite = &world_sprites[SPRITE_WATER_1];
            } else {
                sprite = &world_sprites[BACKGROUND(tile)];
            }
            SDL_FRect dstrect;
            dstrect.x = ((float)x * (float)TILE_SIZE) - player.x + (WORLD_WIDTH * 0.5f);
            dstrect.y = ((float)y * (float)TILE_SIZE) - player.y + (WORLD_HEIGHT * 0.5f);
            dstrect.w = TILE_SIZE;
            dstrect.h = TILE_SIZE;
            SDL_RenderCopyF(renderer.sdl, sprite_texture, sprite, &dstrect);
        }
    }

    for (int y = 0; y < tile_map.height; y++) {
        for (int x = 0; x < tile_map.width; x++) {
            uint16_t foreground = FOREGROUND(tile_map.tiles[(y * tile_map.width) + x]);
            if (foreground == SPRITE_TORCH) {
                SDL_FRect dstrect;
                dstrect.x = ((float)x * (float)TILE_SIZE) - player.x + (WORLD_WIDTH * 0.5f);
                dstrect.y = ((float)y * (float)TILE_SIZE) - player.y + (WORLD_HEIGHT * 0.5f);
                dstrect.w = TILE_SIZE;
                dstrect.h = TILE_SIZE;
                SDL_RenderCopyF(renderer.sdl, sprite_texture, &world_sprites[SPRITE_TORCH], &dstrect);
            } else if (foreground == SPRITE_TREE_TOP) {
                SDL_FRect tree_bottom;
                tree_bottom.x = ((float)x * (float)TILE_SIZE) - player.x + (WORLD_WIDTH * 0.5f);
                tree_bottom.y = ((float)y * (float)TILE_SIZE) - player.y + (WORLD_HEIGHT * 0.5f) + (float)TILE_SIZE;
                tree_bottom.w = TILE_SIZE * 2.0f;
                tree_bottom.h = TILE_SIZE;
                SDL_RenderCopyF(renderer.sdl, sprite_texture, &world_sprites[SPRITE_TREE_BOTTOM], &tree_bottom);
            }
        }
    }

    for (size_t i = 0; i < children.size; i++) {
        render_mob(&children.mobs[i].sprite, player_sprites, ticks);
    }
    for (size_t i = 0; i < females.size; i++) {
        render_mob(&females.mobs[i].sprite, female_sprites, ticks);
    }
    for (size_t i = 0; i < virgin_females.size; i++) {
        render_mob(&virgin_females.mobs[i].sprite, virgin_female_sprites, ticks);
    }
    render_mob(&player, player_sprites, ticks);

    for (int y = 0; y < tile_map.height; y++) {
        for (int x = 0; x < tile_map.width; x++) {
            uint16_t tile = tile_map.tiles[(y * tile_map.width) + x];
            if (tile == TILE_TREE) {
                SDL_FRect tree_top;
                tree_top.x = ((float)x * (float)TILE_SIZE) - player.x + (WORLD_WIDTH * 0.5f);
                tree_top.y = ((float)y * (float)TILE_SIZE) - player.y + (WORLD_HEIGHT * 0.5f);
                tree_top.w = TILE_SIZE * 2.0f;
                tree_top.h = TILE_SIZE;
                SDL_RenderCopyF(renderer.sdl, sprite_texture, &world_sprites[SPRITE_TREE_TOP], &tree_top);
            }
        }
    }
}

void render_overlay(int64_t ticks)
{
    int timer = 300 - ((ticks - start_ticks) / 1000);
    int minutes = timer / 60;
    int seconds = timer % 60;
    char string_buffer[32];
    snprintf(string_buffer, sizeof(string_buffer), "%d:%02d", minutes, seconds);
    render_string_centered(string_buffer, renderer.width / 2, renderer.height - 15);

    snprintf(string_buffer, sizeof(string_buffer), "Population: %d", (int)population);
    render_string_right(string_buffer, renderer.width, 30);
}
