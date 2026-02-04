#pragma once

#include <stdbool.h>
#include <stdint.h>

#define TINY16_APU_SAMPLE_RATE     44100
#define TINY16_APU_NUM_CHANNELS    4
#define TINY16_APU_SFX_ENTRY_SIZE  6
#define TINY16_APU_MUSIC_NOTE_SIZE 4

// Forward declaration
struct Tiny16Memory;

// Waveform types
typedef enum {
    TINY16_APU_WAVE_TRIANGLE = 0,
    TINY16_APU_WAVE_TSAW = 1,
    TINY16_APU_WAVE_SAW = 2,
    TINY16_APU_WAVE_SQUARE = 3,
    TINY16_APU_WAVE_PULSE = 4,
    TINY16_APU_WAVE_ORGAN = 5,
    TINY16_APU_WAVE_NOISE = 6,
    TINY16_APU_WAVE_PHASER = 7,
} Tiny16APUWaveform;

// Envelope phases
typedef enum {
    TINY16_APU_ENV_OFF = 0,
    TINY16_APU_ENV_ATTACK,
    TINY16_APU_ENV_DECAY,
    TINY16_APU_ENV_SUSTAIN,
    TINY16_APU_ENV_RELEASE,
} Tiny16APUEnvPhase;

// ADSR envelope
typedef struct {
    uint8_t attack;
    uint8_t decay;
    uint8_t sustain;
    uint8_t release;
    Tiny16APUEnvPhase phase;
    float level;
} Tiny16APUEnvelope;

// Unified audio channel - can produce any waveform
typedef struct {
    bool enabled;
    uint8_t waveform;
    uint16_t freq;
    uint8_t volume;
    uint32_t phase;
    uint32_t phase_inc;
    uint16_t lfsr;
    uint32_t noise_timer;
    Tiny16APUEnvelope env;
    uint8_t length;
    uint32_t length_left;
} Tiny16APUChannel;

// SFX state per channel
typedef struct {
    bool active;
    uint8_t duration_left;
    uint8_t sfx_id;
} Tiny16APUSfxState;

// SFX system
typedef struct {
    uint16_t table_addr;
    uint8_t count;
    Tiny16APUSfxState ch[TINY16_APU_NUM_CHANNELS];
} Tiny16APUSfxSystem;

// Music channel (virtual, mixed separately)
typedef struct {
    bool enabled;
    bool looping;
    uint16_t track_addr;
    uint16_t track_length;
    uint16_t current_note;
    uint8_t frames_left;
    uint8_t waveform;
    uint16_t freq;
    uint8_t volume;
    uint32_t phase;
    uint32_t phase_inc;
    uint16_t lfsr;
    uint32_t noise_timer;
} Tiny16MusicChannel;

// Music system
typedef struct {
    bool enabled;
    Tiny16MusicChannel ch[TINY16_APU_NUM_CHANNELS];
} Tiny16MusicPlayer;

// Main APU structure
typedef struct {
    volatile int lock;
    bool enabled;
    uint8_t master_volume;

    Tiny16APUChannel ch[TINY16_APU_NUM_CHANNELS];

    Tiny16APUSfxSystem sfx;
    Tiny16MusicPlayer music;
    uint32_t frame_samples;

    struct Tiny16Memory* memory;
} Tiny16APU;

// Public API
void tiny16_apu_reset(Tiny16APU* apu);
void tiny16_apu_mmio_write(Tiny16APU* apu, uint16_t addr, uint8_t value);
uint8_t tiny16_apu_mmio_read(Tiny16APU* apu, uint16_t addr);
void tiny16_apu_generate_samples(Tiny16APU* apu, float* buffer, unsigned int frames);
