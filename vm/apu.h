#pragma once

#include <stdbool.h>
#include <stdint.h>

#define TINY16_APU_SAMPLE_RATE 44100
#define TINY16_APU_CHANNELS    5

typedef enum {
    TINY16_APU_CH_PULSE1 = 0,
    TINY16_APU_CH_PULSE2 = 1,
    TINY16_APU_CH_TRIANGLE = 2,
    TINY16_APU_CH_NOISE = 3,
    TINY16_APU_CH_WAVE = 4,
} Tiny16APUChannelType;

typedef enum {
    TINY16_APU_DUTY_12_5 = 0, // 12.5%
    TINY16_APU_DUTY_25 = 1,   // 25%
    TINY16_APU_DUTY_50 = 2,   // 50%
    TINY16_APU_DUTY_75 = 3,   // 75%
} Tiny16APUDutyCycle;

typedef enum {
    TINY16_APU_ENV_OFF = 0,
    TINY16_APU_ENV_ATTACK,
    TINY16_APU_ENV_DECAY,
    TINY16_APU_ENV_SUSTAIN,
    TINY16_APU_ENV_RELEASE,
} Tiny16APUEnvPhase;

typedef struct {
    uint8_t attack;  // 4-bit
    uint8_t decay;   // 4-bit
    uint8_t sustain; // 4-bit
    uint8_t release; // 4-bit
    Tiny16APUEnvPhase phase;
    float level; // 0.0 - 1.0
} Tiny16APUEnvelope;

typedef struct {
    uint16_t freq;      // 11-bit frequency divisor (0-2047)
    uint8_t volume;     // 4-bit volume (0-15)
    uint8_t duty;       // 2-bit duty cycle
    bool enabled;       // channel enable
    bool trigger;       // trigger flag (resets phase)
    uint32_t phase;     // 32-bit phase accumulator
    uint32_t phase_inc; // phase increment per sample
    Tiny16APUEnvelope env;
    uint8_t sweep_rate;   // 4-bit
    uint8_t sweep_shift;  // 3-bit
    bool sweep_down;      // direction
    uint32_t sweep_timer; // samples until next sweep
    uint8_t length;       // length in frames
    uint32_t length_left; // samples remaining
} Tiny16APUPulseChannel;

typedef struct {
    uint16_t freq;      // 11-bit frequency divisor (0-2047)
    uint8_t volume;     // 4-bit volume (0-15)
    bool enabled;       // channel enable
    bool trigger;       // trigger flag (resets phase)
    uint32_t phase;     // 32-bit phase accumulator
    uint32_t phase_inc; // phase increment per sample
    Tiny16APUEnvelope env;
    uint8_t length;       // length in frames
    uint32_t length_left; // samples remaining
} Tiny16APUTriangleChannel;

typedef struct {
    uint8_t period;  // 4-bit period (0-15)
    uint8_t volume;  // 4-bit volume (0-15)
    bool enabled;    // channel enable
    bool trigger;    // trigger flag (reseeds LFSR)
    bool short_mode; // noise mode: false=long, true=short
    uint16_t lfsr;   // 16-bit Linear Feedback Shift Register
    uint32_t timer;  // timer counter for period
    Tiny16APUEnvelope env;
    uint8_t length;       // length in frames
    uint32_t length_left; // samples remaining
} Tiny16APUNoiseChannel;

typedef struct {
    uint16_t freq;      // 11-bit frequency divisor (0-2047)
    uint8_t volume;     // 4-bit volume (0-15)
    bool enabled;       // channel enable
    bool trigger;       // trigger flag (resets phase)
    uint32_t phase;     // 32-bit phase accumulator
    uint32_t phase_inc; // phase increment per sample
    Tiny16APUEnvelope env;
    uint8_t length;       // length in frames
    uint32_t length_left; // samples remaining
    uint8_t wave[32];     // 4-bit samples
} Tiny16APUWaveChannel;

typedef struct {
    volatile int lock;
    bool enabled;          // APU master enable
    uint8_t master_volume; // 4-bit master volume (0-15)
    uint64_t sample_accum; // fractional samples in cpu_hz units

    Tiny16APUPulseChannel pulse1;      // CH0
    Tiny16APUPulseChannel pulse2;      // CH1
    Tiny16APUTriangleChannel triangle; // CH2
    Tiny16APUNoiseChannel noise;       // CH3
    Tiny16APUWaveChannel wave;         // CH4
} Tiny16APU;

void tiny16_apu_reset(Tiny16APU* apu);

void tiny16_apu_mmio_write(Tiny16APU* apu, uint16_t addr, uint8_t value);
uint8_t tiny16_apu_mmio_read(Tiny16APU* apu, uint16_t addr);

uint32_t tiny16_apu_samples_for_cycles(Tiny16APU* apu, uint32_t cpu_cycles, uint32_t cpu_hz);
void tiny16_apu_generate_samples(Tiny16APU* apu, float* buffer, unsigned int frames);
