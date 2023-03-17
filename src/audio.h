#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

typedef struct AudioData
{
    uint8_t channels;
    uint32_t samples;
    float *data;
} AudioData;

extern AudioData breed;
extern AudioData game_over;
extern AudioData menu;
extern AudioData menu_cycle;
extern AudioData menu_theme;
extern AudioData start;
extern AudioData theme;

void init_audio(void);
void play_sound(AudioData *audio_data);

#endif
