#include "SDL.h"
#include <samplerate.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "audio.h"

#define MAX_SFX 8

typedef struct AudioStream
{
    AudioData *audio_data;
    uint32_t position;
} AudioStream;

AudioData breed;
AudioData game_over;
AudioData menu;
AudioData menu_cycle;
AudioData menu_theme;
AudioData start;
AudioData theme;

static SDL_AudioDeviceID audio_device;
static AudioStream music = {&theme, 0};
static AudioStream sound_effects[MAX_SFX];

void play_sound(AudioData *audio_data)
{
    static int index = 0;
    SDL_LockAudioDevice(audio_device);
    sound_effects[index].audio_data = audio_data;
    sound_effects[index].position = 0;
    index++;
    if (index >= MAX_SFX) {
        index = 0;
    }
    SDL_UnlockAudioDevice(audio_device);
}

static void audio_callback(void *userdata, Uint8 *stream, int len)
{
    int requested_samples = (len / sizeof(float)) / 2;  // Output is always 2 channel
    float *output = (float *)stream;
    for (int s = 0; s < requested_samples; s++) {
        *output++ = music.audio_data->data[music.position * music.audio_data->channels];
        // For mono sound input, write the same data to both channels
        *output++ = music.audio_data->data[(music.position * music.audio_data->channels) + (music.audio_data->channels - 1)];
        music.position += 1;
        if (music.position >= music.audio_data->samples) {
            music.position = 0;
        }
    }

    for (int i = 0; i < MAX_SFX; i++) {
        if (sound_effects[i].audio_data) {
            output = (float *)stream;
            for (int s = 0; s < requested_samples; s++) {
                *output++ += sound_effects[i].audio_data->data[sound_effects[i].position * sound_effects[i].audio_data->channels];
                // For mono sound input, write the same data to both channels
                *output++ += sound_effects[i].audio_data->data[(sound_effects[i].position * sound_effects[i].audio_data->channels) + (sound_effects[i].audio_data->channels - 1)];
                sound_effects[i].position += 1;
                if (sound_effects[i].position >= sound_effects[i].audio_data->samples) {
                    memset(sound_effects + i, 0, sizeof(AudioStream));
                    break;
                }
            }
        }
    }
}

static void load_wav(const char *filename, AudioData *audio_data, int frequency)
{
    SDL_AudioSpec audio_spec;
    int16_t *wav_data;
    Uint32 audio_len;
    if (SDL_LoadWAV(filename, &audio_spec, (Uint8 **)&wav_data, &audio_len) != &audio_spec) {
        fprintf(stderr, "SDL_LoadWAV failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    if (audio_spec.format != AUDIO_S16LSB) {
        fprintf(stderr, "%s: Unsupported format: %hu\n", filename, audio_spec.format);
        exit(EXIT_FAILURE);
    }
    if (audio_spec.channels != 1 && audio_spec.channels != 2) {
        fprintf(stderr, "%s: Unsupported number of channels: %hhu\n", filename, audio_spec.channels);
        exit(EXIT_FAILURE);
    }
    audio_data->channels = audio_spec.channels;
    audio_data->samples = (audio_len / sizeof(int16_t)) / audio_data->channels;
    float *float_data = malloc(audio_data->samples * audio_data->channels * sizeof(float));
    if (float_data == NULL) {
        fprintf(stderr, "malloc failed\n");
        exit(EXIT_FAILURE);
    }
    for (uint32_t i = 0; i < audio_data->samples * audio_data->channels; i++) {
        float_data[i] = wav_data[i] * (1.0f / 32768.0f);
    }
    SDL_FreeWAV((Uint8 *)wav_data);
    if (audio_spec.freq == frequency) {
        audio_data->data = float_data;
    } else {
        /* 
         * Use libsample rate to convert if the frequency doesn't match what the system can support.
         * This converter is slow for large files. We ask SDL for a 48000 frequency which is what the large music files are.
         * This should be supported by most systems natively.  However, if it's not, SDL can give us back a different frequency.
         * If the music files need converted, it can take ~30 seoncds.  Even though this only happens once at the start of the game, it's still a long load time.
         * The sound effects go through this path but they only take 1 second or so.
         * If this is a problem, maybe look for an alternative.  I wanted to avoid using SDL's resampler as it is somewhat buggy:
         * https://github.com/libsdl-org/SDL/issues/6391
         * https://github.com/libsdl-org/SDL/issues/7358
         */
        SRC_DATA src_data;
        memset(&src_data, 0, sizeof(SRC_DATA));
        uint32_t allocated_samples = audio_data->samples * 2;
        src_data.input_frames = audio_data->samples;
        src_data.output_frames = allocated_samples;
        src_data.src_ratio = (double)frequency / (double)audio_spec.freq;
        src_data.data_in = float_data;
        src_data.data_out = malloc(allocated_samples * audio_data->channels * sizeof(float));
        if (src_data.data_out == NULL) {
            fprintf(stderr, "malloc failed\n");
            exit(EXIT_FAILURE);
        }
        int error = src_simple(&src_data, SRC_SINC_BEST_QUALITY, audio_data->channels);
        if (error) {
            fprintf(stderr, "src_simple failed: %s\n", src_strerror(error));
            exit(EXIT_FAILURE);
        }
        free(float_data);
        // The API doesn't tell you ahead of times how many samples it will generate.
        // We ask for twice as many and then free the un-used memory with realloc.
        audio_data->samples = src_data.output_frames_gen;
        if (audio_data->samples < allocated_samples) {
            src_data.data_out = realloc(src_data.data_out, audio_data->samples * audio_data->channels * sizeof(float));
            if (src_data.data_out == NULL) {
                fprintf(stderr, "realloc failed\n");
                exit(EXIT_FAILURE);
            }
        }
        audio_data->data = src_data.data_out;
    }
}

void init_audio(void)
{
    SDL_AudioSpec desired;
    memset(&desired, 0, sizeof(SDL_AudioSpec));
    desired.freq = 48000;
    desired.format = AUDIO_F32LSB;
    desired.channels = 2;
    desired.samples = 4096;
    desired.callback = audio_callback;

    SDL_AudioSpec obtained;
    audio_device = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
    if (audio_device == 0) {
        fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    load_wav("res/sound/breed.wav", &breed, obtained.freq);
    load_wav("res/sound/gameover.wav", &game_over, obtained.freq);
    load_wav("res/sound/menu.wav", &menu, obtained.freq);
    load_wav("res/sound/menucycle.wav", &menu_cycle, obtained.freq);
    load_wav("res/sound/menutheme.wav", &menu_theme, obtained.freq);
    load_wav("res/sound/start.wav", &start, obtained.freq);
    load_wav("res/sound/theme.wav", &theme, obtained.freq);

    SDL_PauseAudioDevice(audio_device, 0);
}
