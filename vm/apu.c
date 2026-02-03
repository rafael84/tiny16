#include "apu.h"
#include "memory.h"

#define LFSR_SEED 0xACE1

// Duty cycle thresholds (normalized 0-1 scaled to 32-bit phase)
static const uint32_t duty_thresholds[] = {
    0x20000000, // 12.5% = 0.125 * 2^32
    0x40000000, // 25%   = 0.25 * 2^32
    0x80000000, // 50%   = 0.5 * 2^32
    0xC0000000, // 75%   = 0.75 * 2^32
};

// Noise period divisors (period 0-15 maps to divider)
static const uint32_t noise_periods[] = {1,  2,  4,   8,   16,  32,  48,  64,
                                         80, 96, 112, 128, 160, 202, 254, 380};

static const uint32_t samples_per_frame = TINY16_APU_SAMPLE_RATE / 60;
static const uint32_t sweep_base_samples = TINY16_APU_SAMPLE_RATE / 120;

// Envelope rate table in seconds (index 0 = immediate)
static const float env_rate_seconds[] = {
    0.0f,  0.005f, 0.01f, 0.02f, 0.04f, 0.06f, 0.08f, 0.10f,
    0.15f, 0.20f,  0.30f, 0.40f, 0.60f, 0.80f, 1.00f, 1.50f,
};

static inline void apu_lock(Tiny16APU* apu) {
    while (__sync_lock_test_and_set(&apu->lock, 1)) {
    }
}

static inline void apu_unlock(Tiny16APU* apu) { __sync_lock_release(&apu->lock); }

static inline uint32_t calc_phase_inc(uint16_t freq_value) {
    // freq_hz = 44100 / (2048 - freq_value)
    // phase_inc = freq_hz / sample_rate * 2^32
    // Simplified: phase_inc = 2^32 / (2048 - freq_value)
    uint32_t divisor = 2048 - freq_value;
    if (divisor == 0) divisor = 1;
    return (uint32_t)(4294967296.0 / divisor);
}

static inline float generate_pulse(uint32_t phase, uint8_t duty) {
    uint32_t threshold = duty_thresholds[duty & 0x03];
    return (phase < threshold) ? 1.0f : -1.0f;
}

static inline float generate_triangle(uint32_t phase) {
    // Normalize phase to 0-1 range
    float t = (float)phase / 4294967296.0f;

    // Triangle wave: ramp up, then down
    if (t < 0.25f) {
        return t * 4.0f;
    } else if (t < 0.75f) {
        return 2.0f - t * 4.0f;
    } else {
        return t * 4.0f - 4.0f;
    }
}

static inline void noise_step_long(uint16_t* lfsr) {
    uint16_t bit = ((*lfsr >> 0) ^ (*lfsr >> 1) ^ (*lfsr >> 3) ^ (*lfsr >> 12)) & 1;
    *lfsr = (*lfsr >> 1) | (bit << 15);
}

static inline void noise_step_short(uint16_t* lfsr) {
    uint16_t bit = ((*lfsr >> 0) ^ (*lfsr >> 6)) & 1;
    *lfsr = (*lfsr >> 1) | (bit << 15);
}

static inline float generate_noise(uint16_t lfsr) {
    // Convert to float -1.0 to 1.0
    return ((float)lfsr / 32768.0f) - 1.0f;
}

static inline float generate_wave(const uint8_t* wave, uint32_t phase) {
    uint8_t index = (uint8_t)(phase >> 27); // 32 samples
    float v = (float)(wave[index] & 0x0F);
    return (v / 7.5f) - 1.0f;
}

static inline void env_start(Tiny16APUEnvelope* env) {
    env->level = 0.0f;
    env->phase = TINY16_APU_ENV_ATTACK;
}

static inline void env_release(Tiny16APUEnvelope* env) {
    if (env->phase != TINY16_APU_ENV_OFF) {
        env->phase = TINY16_APU_ENV_RELEASE;
    }
}

static inline float env_update(Tiny16APUEnvelope* env) {
    float sustain_level = (float)env->sustain / 15.0f;

    switch (env->phase) {
    case TINY16_APU_ENV_OFF: env->level = 0.0f; break;
    case TINY16_APU_ENV_ATTACK: {
        float secs = env_rate_seconds[env->attack & 0x0F];
        if (secs <= 0.0f) {
            env->level = 1.0f;
            env->phase = TINY16_APU_ENV_DECAY;
            break;
        }
        env->level += 1.0f / (secs * (float)TINY16_APU_SAMPLE_RATE);
        if (env->level >= 1.0f) {
            env->level = 1.0f;
            env->phase = TINY16_APU_ENV_DECAY;
        }
        break;
    }
    case TINY16_APU_ENV_DECAY: {
        float secs = env_rate_seconds[env->decay & 0x0F];
        if (secs <= 0.0f || sustain_level >= 1.0f) {
            env->level = sustain_level;
            env->phase = TINY16_APU_ENV_SUSTAIN;
            break;
        }
        env->level -= (1.0f - sustain_level) / (secs * (float)TINY16_APU_SAMPLE_RATE);
        if (env->level <= sustain_level) {
            env->level = sustain_level;
            env->phase = TINY16_APU_ENV_SUSTAIN;
        }
        break;
    }
    case TINY16_APU_ENV_SUSTAIN: env->level = sustain_level; break;
    case TINY16_APU_ENV_RELEASE: {
        float secs = env_rate_seconds[env->release & 0x0F];
        if (secs <= 0.0f) {
            env->level = 0.0f;
            env->phase = TINY16_APU_ENV_OFF;
            break;
        }
        env->level -= 1.0f / (secs * (float)TINY16_APU_SAMPLE_RATE);
        if (env->level <= 0.0f) {
            env->level = 0.0f;
            env->phase = TINY16_APU_ENV_OFF;
        }
        break;
    }
    }

    return env->level;
}

static inline uint32_t length_samples(uint8_t length_frames) {
    if (length_frames == 0) return 0;
    return (uint32_t)length_frames * samples_per_frame;
}

static inline void sweep_init(Tiny16APUPulseChannel* ch) {
    ch->sweep_timer = (ch->sweep_rate == 0) ? 0 : (uint32_t)ch->sweep_rate * sweep_base_samples;
}

static inline void sweep_step(Tiny16APUPulseChannel* ch) {
    if (ch->sweep_rate == 0 || ch->sweep_shift == 0) return;
    if (ch->sweep_timer > 0) {
        ch->sweep_timer--;
        return;
    }

    uint16_t delta = ch->freq >> ch->sweep_shift;
    if (ch->sweep_down) {
        if (ch->freq > delta) {
            ch->freq -= delta;
        } else {
            ch->freq = 0;
        }
    } else {
        ch->freq += delta;
        if (ch->freq > 2047) ch->freq = 2047;
    }
    ch->phase_inc = calc_phase_inc(ch->freq);
    sweep_init(ch);
}

void tiny16_apu_reset(Tiny16APU* apu) {
    apu->lock = 0;
    apu->enabled = false;
    apu->master_volume = 0;
    apu->sample_accum = 0;

    apu->pulse1.freq = 0;
    apu->pulse1.volume = 0;
    apu->pulse1.duty = TINY16_APU_DUTY_50;
    apu->pulse1.enabled = false;
    apu->pulse1.trigger = false;
    apu->pulse1.phase = 0;
    apu->pulse1.phase_inc = 0;
    apu->pulse1.env.attack = 0;
    apu->pulse1.env.decay = 0;
    apu->pulse1.env.sustain = 15;
    apu->pulse1.env.release = 0;
    apu->pulse1.env.phase = TINY16_APU_ENV_OFF;
    apu->pulse1.env.level = 0.0f;
    apu->pulse1.sweep_rate = 0;
    apu->pulse1.sweep_shift = 0;
    apu->pulse1.sweep_down = false;
    apu->pulse1.sweep_timer = 0;
    apu->pulse1.length = 0;
    apu->pulse1.length_left = 0;

    apu->pulse2.freq = 0;
    apu->pulse2.volume = 0;
    apu->pulse2.duty = TINY16_APU_DUTY_50;
    apu->pulse2.enabled = false;
    apu->pulse2.trigger = false;
    apu->pulse2.phase = 0;
    apu->pulse2.phase_inc = 0;
    apu->pulse2.env.attack = 0;
    apu->pulse2.env.decay = 0;
    apu->pulse2.env.sustain = 15;
    apu->pulse2.env.release = 0;
    apu->pulse2.env.phase = TINY16_APU_ENV_OFF;
    apu->pulse2.env.level = 0.0f;
    apu->pulse2.sweep_rate = 0;
    apu->pulse2.sweep_shift = 0;
    apu->pulse2.sweep_down = false;
    apu->pulse2.sweep_timer = 0;
    apu->pulse2.length = 0;
    apu->pulse2.length_left = 0;

    apu->triangle.freq = 0;
    apu->triangle.volume = 0;
    apu->triangle.enabled = false;
    apu->triangle.trigger = false;
    apu->triangle.phase = 0;
    apu->triangle.phase_inc = 0;
    apu->triangle.env.attack = 0;
    apu->triangle.env.decay = 0;
    apu->triangle.env.sustain = 15;
    apu->triangle.env.release = 0;
    apu->triangle.env.phase = TINY16_APU_ENV_OFF;
    apu->triangle.env.level = 0.0f;
    apu->triangle.length = 0;
    apu->triangle.length_left = 0;

    apu->noise.period = 0;
    apu->noise.volume = 0;
    apu->noise.enabled = false;
    apu->noise.trigger = false;
    apu->noise.short_mode = false;
    apu->noise.lfsr = LFSR_SEED;
    apu->noise.timer = 0;
    apu->noise.env.attack = 0;
    apu->noise.env.decay = 0;
    apu->noise.env.sustain = 15;
    apu->noise.env.release = 0;
    apu->noise.env.phase = TINY16_APU_ENV_OFF;
    apu->noise.env.level = 0.0f;
    apu->noise.length = 0;
    apu->noise.length_left = 0;

    apu->wave.freq = 0;
    apu->wave.volume = 0;
    apu->wave.enabled = false;
    apu->wave.trigger = false;
    apu->wave.phase = 0;
    apu->wave.phase_inc = 0;
    apu->wave.env.attack = 0;
    apu->wave.env.decay = 0;
    apu->wave.env.sustain = 15;
    apu->wave.env.release = 0;
    apu->wave.env.phase = TINY16_APU_ENV_OFF;
    apu->wave.env.level = 0.0f;
    apu->wave.length = 0;
    apu->wave.length_left = 0;
    for (int i = 0; i < 32; i++)
        apu->wave.wave[i] = 8;

    // SFX system reset
    apu->sfx.table_addr = 0;
    apu->sfx.count = 0;
    for (int i = 0; i < 4; i++) {
        apu->sfx.ch[i].active = false;
        apu->sfx.ch[i].duration_left = 0;
        apu->sfx.ch[i].sfx_id = 0;
    }

    // Music system reset
    apu->music.enabled = false;
    apu->music.looping = false;
    apu->music.song_addr = 0;
    apu->music.song_length = 0;
    apu->music.current_note = 0;
    apu->music.frames_left = 0;
    apu->music.channel = 1; // default to pulse2

    apu->frame_samples = 0;
    apu->memory = NULL;
}

// =============================================================================
// SFX System Implementation
// =============================================================================

// SFX entry format (9 bytes):
// [0] channel (0-3: pulse1/pulse2/tri/noise)
// [1] freq_lo
// [2] freq_hi
// [3] period (noise only)
// [4] volume
// [5] duty (pulse only)
// [6] env_ad (attack << 4 | decay)
// [7] env_sr (sustain << 4 | release)
// [8] duration (frames)

static void tiny16_apu_play_sfx(Tiny16APU* apu, uint8_t sfx_id) {
    if (!apu->memory || sfx_id >= apu->sfx.count) return;

    Tiny16Memory* mem = (Tiny16Memory*)apu->memory;
    uint16_t addr = apu->sfx.table_addr + (uint16_t)sfx_id * TINY16_APU_SFX_ENTRY_SIZE;

    // Read SFX entry from memory
    uint8_t channel = mem->bytes[addr + 0] & 0x03;
    uint8_t freq_lo = mem->bytes[addr + 1];
    uint8_t freq_hi = mem->bytes[addr + 2];
    uint8_t period = mem->bytes[addr + 3];
    uint8_t volume = mem->bytes[addr + 4] & 0x0F;
    uint8_t duty = mem->bytes[addr + 5] & 0x03;
    uint8_t env_ad = mem->bytes[addr + 6];
    uint8_t env_sr = mem->bytes[addr + 7];
    uint8_t duration = mem->bytes[addr + 8];

    uint16_t freq = ((uint16_t)(freq_hi & 0x07) << 8) | freq_lo;

    // Set up channel based on type
    switch (channel) {
    case 0: // Pulse 1
        apu->pulse1.freq = freq;
        apu->pulse1.phase_inc = calc_phase_inc(freq);
        apu->pulse1.volume = volume;
        apu->pulse1.duty = duty;
        apu->pulse1.env.attack = (env_ad >> 4) & 0x0F;
        apu->pulse1.env.decay = env_ad & 0x0F;
        apu->pulse1.env.sustain = (env_sr >> 4) & 0x0F;
        apu->pulse1.env.release = env_sr & 0x0F;
        apu->pulse1.length = duration;
        apu->pulse1.length_left = length_samples(duration);
        apu->pulse1.phase = 0;
        apu->pulse1.enabled = true;
        env_start(&apu->pulse1.env);
        sweep_init(&apu->pulse1);
        break;

    case 1: // Pulse 2
        apu->pulse2.freq = freq;
        apu->pulse2.phase_inc = calc_phase_inc(freq);
        apu->pulse2.volume = volume;
        apu->pulse2.duty = duty;
        apu->pulse2.env.attack = (env_ad >> 4) & 0x0F;
        apu->pulse2.env.decay = env_ad & 0x0F;
        apu->pulse2.env.sustain = (env_sr >> 4) & 0x0F;
        apu->pulse2.env.release = env_sr & 0x0F;
        apu->pulse2.length = duration;
        apu->pulse2.length_left = length_samples(duration);
        apu->pulse2.phase = 0;
        apu->pulse2.enabled = true;
        env_start(&apu->pulse2.env);
        sweep_init(&apu->pulse2);
        break;

    case 2: // Triangle
        apu->triangle.freq = freq;
        apu->triangle.phase_inc = calc_phase_inc(freq);
        apu->triangle.volume = volume;
        apu->triangle.env.attack = (env_ad >> 4) & 0x0F;
        apu->triangle.env.decay = env_ad & 0x0F;
        apu->triangle.env.sustain = (env_sr >> 4) & 0x0F;
        apu->triangle.env.release = env_sr & 0x0F;
        apu->triangle.length = duration;
        apu->triangle.length_left = length_samples(duration);
        apu->triangle.phase = 0;
        apu->triangle.enabled = true;
        env_start(&apu->triangle.env);
        break;

    case 3: // Noise
        apu->noise.period = period & 0x0F;
        apu->noise.volume = volume;
        apu->noise.env.attack = (env_ad >> 4) & 0x0F;
        apu->noise.env.decay = env_ad & 0x0F;
        apu->noise.env.sustain = (env_sr >> 4) & 0x0F;
        apu->noise.env.release = env_sr & 0x0F;
        apu->noise.length = duration;
        apu->noise.length_left = length_samples(duration);
        apu->noise.lfsr = LFSR_SEED;
        apu->noise.timer = 0;
        apu->noise.enabled = true;
        env_start(&apu->noise.env);
        break;
    }

    // Update SFX state
    apu->sfx.ch[channel].active = true;
    apu->sfx.ch[channel].duration_left = duration;
    apu->sfx.ch[channel].sfx_id = sfx_id;
}

static void tiny16_apu_stop_sfx(Tiny16APU* apu, uint8_t channel) {
    if (channel > 3) return;

    apu->sfx.ch[channel].active = false;
    apu->sfx.ch[channel].duration_left = 0;

    // Release the channel
    switch (channel) {
    case 0:
        apu->pulse1.enabled = false;
        env_release(&apu->pulse1.env);
        break;
    case 1:
        apu->pulse2.enabled = false;
        env_release(&apu->pulse2.env);
        break;
    case 2:
        apu->triangle.enabled = false;
        env_release(&apu->triangle.env);
        break;
    case 3:
        apu->noise.enabled = false;
        env_release(&apu->noise.env);
        break;
    }
}

static uint8_t tiny16_apu_get_sfx_status(Tiny16APU* apu) {
    uint8_t status = 0;
    for (int i = 0; i < 4; i++) {
        if (apu->sfx.ch[i].active) status |= (1 << i);
    }
    return status;
}

// Called once per frame to update SFX durations
static void tiny16_apu_update_sfx(Tiny16APU* apu) {
    for (int i = 0; i < 4; i++) {
        if (apu->sfx.ch[i].active && apu->sfx.ch[i].duration_left > 0) {
            apu->sfx.ch[i].duration_left--;
            if (apu->sfx.ch[i].duration_left == 0) {
                apu->sfx.ch[i].active = false;
            }
        }
    }
}

// =============================================================================
// Music Sequencer Implementation
// =============================================================================

// Music note format (4 bytes):
// [0] note (0=rest, 1-96=notes C1-B8, using MIDI-like indexing)
// [1] volume (4-bit)
// [2] duration (frames)
// [3] reserved (for future use: instrument, effects, etc.)

// Note frequency table (C3 to B5, 3 octaves = 36 notes)
// Frequencies are pre-calculated for the APU's freq_value format
static const uint16_t music_note_freq[] = {
    // Octave 3: C3, C#3, D3, D#3, E3, F3, F#3, G3, G#3, A3, A#3, B3
    1711,
    1730,
    1748,
    1765,
    1780,
    1795,
    1810,
    1823,
    1836,
    1848,
    1859,
    1869,
    // Octave 4: C4, C#4, D4, D#4, E4, F4, F#4, G4, G#4, A4, A#4, B4
    1879,
    1889,
    1898,
    1906,
    1914,
    1922,
    1929,
    1935,
    1942,
    1948,
    1953,
    1959,
    // Octave 5: C5, C#5, D5, D#5, E5, F5, F#5, G5, G#5, A5, A#5, B5
    1964,
    1968,
    1973,
    1977,
    1981,
    1985,
    1988,
    1992,
    1995,
    1998,
    2001,
    2003,
};
#define MUSIC_NOTE_TABLE_SIZE (sizeof(music_note_freq) / sizeof(music_note_freq[0]))

static void tiny16_apu_play_music_note(Tiny16APU* apu, uint8_t note, uint8_t volume) {
    if (note == 0) {
        // Rest - silence the music channel
        if (apu->music.channel == 1) {
            apu->pulse2.enabled = false;
            env_release(&apu->pulse2.env);
        }
        return;
    }

    // Convert note index (1-based) to frequency table index (0-based)
    uint8_t note_idx = note - 1;
    if (note_idx >= MUSIC_NOTE_TABLE_SIZE) return;

    uint16_t freq = music_note_freq[note_idx];

    // Play on the music channel (default: pulse2)
    // Envelope: instant attack, no decay, sustain at requested volume, quick release
    uint8_t vol = volume & 0x0F;

    switch (apu->music.channel) {
    case 0: // Pulse 1
        apu->pulse1.freq = freq;
        apu->pulse1.phase_inc = calc_phase_inc(freq);
        apu->pulse1.volume = vol;
        apu->pulse1.duty = TINY16_APU_DUTY_25;
        apu->pulse1.env.attack = 0;
        apu->pulse1.env.decay = 0;
        apu->pulse1.env.sustain = vol;
        apu->pulse1.env.release = 1;
        apu->pulse1.length = 0;
        apu->pulse1.length_left = 0;
        apu->pulse1.phase = 0;
        apu->pulse1.enabled = true;
        env_start(&apu->pulse1.env);
        break;

    case 1: // Pulse 2 (default)
        apu->pulse2.freq = freq;
        apu->pulse2.phase_inc = calc_phase_inc(freq);
        apu->pulse2.volume = vol;
        apu->pulse2.duty = TINY16_APU_DUTY_25;
        apu->pulse2.env.attack = 0;
        apu->pulse2.env.decay = 0;
        apu->pulse2.env.sustain = vol;
        apu->pulse2.env.release = 1;
        apu->pulse2.length = 0;
        apu->pulse2.length_left = 0;
        apu->pulse2.phase = 0;
        apu->pulse2.enabled = true;
        env_start(&apu->pulse2.env);
        break;

    case 2: // Triangle
        apu->triangle.freq = freq;
        apu->triangle.phase_inc = calc_phase_inc(freq);
        apu->triangle.volume = vol;
        apu->triangle.env.attack = 0;
        apu->triangle.env.decay = 0;
        apu->triangle.env.sustain = vol;
        apu->triangle.env.release = 1;
        apu->triangle.length = 0;
        apu->triangle.length_left = 0;
        apu->triangle.phase = 0;
        apu->triangle.enabled = true;
        env_start(&apu->triangle.env);
        break;
    }
}

static void tiny16_apu_music_start(Tiny16APU* apu) {
    apu->music.enabled = true;
    apu->music.current_note = 0;
    apu->music.frames_left = 0;
}

static void tiny16_apu_music_stop(Tiny16APU* apu) {
    apu->music.enabled = false;

    // Release the music channel
    switch (apu->music.channel) {
    case 0:
        apu->pulse1.enabled = false;
        env_release(&apu->pulse1.env);
        break;
    case 1:
        apu->pulse2.enabled = false;
        env_release(&apu->pulse2.env);
        break;
    case 2:
        apu->triangle.enabled = false;
        env_release(&apu->triangle.env);
        break;
    }
}

// Called once per frame to update music sequencer
static void tiny16_apu_update_music(Tiny16APU* apu) {
    if (!apu->music.enabled || !apu->memory) return;
    if (apu->music.song_length == 0) return;

    Tiny16Memory* mem = (Tiny16Memory*)apu->memory;

    // Check if it's time for the next note
    if (apu->music.frames_left > 0) {
        apu->music.frames_left--;
        return;
    }

    // Read current note from memory
    uint16_t addr = apu->music.song_addr + (apu->music.current_note * TINY16_APU_MUSIC_NOTE_SIZE);
    uint8_t note = mem->bytes[addr + 0];
    uint8_t volume = mem->bytes[addr + 1];
    uint8_t duration = mem->bytes[addr + 2];
    // byte 3 is reserved

    // Play the note
    tiny16_apu_play_music_note(apu, note, volume);
    apu->music.frames_left = duration;

    // Advance to next note
    apu->music.current_note++;
    if (apu->music.current_note >= apu->music.song_length) {
        if (apu->music.looping) {
            apu->music.current_note = 0;
        } else {
            tiny16_apu_music_stop(apu);
        }
    }
}

void tiny16_apu_mmio_write(Tiny16APU* apu, uint16_t addr, uint8_t value) {
    apu_lock(apu);
    switch (addr) {
    // Master control
    case TINY16_MMIO_APU_CTRL:
        apu->enabled = (value & 0x01) != 0;
        apu->master_volume = (value >> 4) & 0x0F;
        break;

    // Channel 0 - Pulse 1
    case TINY16_MMIO_APU_CH0_FREQ_LO:
        apu->pulse1.freq = (apu->pulse1.freq & 0x700) | value;
        apu->pulse1.phase_inc = calc_phase_inc(apu->pulse1.freq);
        break;
    case TINY16_MMIO_APU_CH0_FREQ_HI:
        apu->pulse1.freq = (apu->pulse1.freq & 0x0FF) | ((value & 0x07) << 8);
        apu->pulse1.phase_inc = calc_phase_inc(apu->pulse1.freq);
        break;
    case TINY16_MMIO_APU_CH0_VOL:
        apu->pulse1.volume = value & 0x0F;
        apu->pulse1.duty = (value >> 4) & 0x03;
        break;
    case TINY16_MMIO_APU_CH0_CTRL: {
        bool prev_enabled = apu->pulse1.enabled;
        apu->pulse1.enabled = (value & 0x01) != 0;
        if (apu->pulse1.enabled && !prev_enabled) {
            env_start(&apu->pulse1.env);
            if (apu->pulse1.length > 0) {
                apu->pulse1.length_left = length_samples(apu->pulse1.length);
            }
        }
        if (!apu->pulse1.enabled && prev_enabled) {
            env_release(&apu->pulse1.env);
        }
    }
        if (value & 0x02) { // trigger
            apu->pulse1.phase = 0;
            env_start(&apu->pulse1.env);
            if (apu->pulse1.length > 0) {
                apu->pulse1.length_left = length_samples(apu->pulse1.length);
            }
            sweep_init(&apu->pulse1);
        }
        break;

    // Channel 1 - Pulse 2
    case TINY16_MMIO_APU_CH1_FREQ_LO:
        apu->pulse2.freq = (apu->pulse2.freq & 0x700) | value;
        apu->pulse2.phase_inc = calc_phase_inc(apu->pulse2.freq);
        break;
    case TINY16_MMIO_APU_CH1_FREQ_HI:
        apu->pulse2.freq = (apu->pulse2.freq & 0x0FF) | ((value & 0x07) << 8);
        apu->pulse2.phase_inc = calc_phase_inc(apu->pulse2.freq);
        break;
    case TINY16_MMIO_APU_CH1_VOL:
        apu->pulse2.volume = value & 0x0F;
        apu->pulse2.duty = (value >> 4) & 0x03;
        break;
    case TINY16_MMIO_APU_CH1_CTRL: {
        bool prev_enabled = apu->pulse2.enabled;
        apu->pulse2.enabled = (value & 0x01) != 0;
        if (apu->pulse2.enabled && !prev_enabled) {
            env_start(&apu->pulse2.env);
            if (apu->pulse2.length > 0) {
                apu->pulse2.length_left = length_samples(apu->pulse2.length);
            }
        }
        if (!apu->pulse2.enabled && prev_enabled) {
            env_release(&apu->pulse2.env);
        }
    }
        if (value & 0x02) {
            apu->pulse2.phase = 0;
            env_start(&apu->pulse2.env);
            if (apu->pulse2.length > 0) {
                apu->pulse2.length_left = length_samples(apu->pulse2.length);
            }
            sweep_init(&apu->pulse2);
        }
        break;

    // Channel 2 - Triangle
    case TINY16_MMIO_APU_CH2_FREQ_LO:
        apu->triangle.freq = (apu->triangle.freq & 0x700) | value;
        apu->triangle.phase_inc = calc_phase_inc(apu->triangle.freq);
        break;
    case TINY16_MMIO_APU_CH2_FREQ_HI:
        apu->triangle.freq = (apu->triangle.freq & 0x0FF) | ((value & 0x07) << 8);
        apu->triangle.phase_inc = calc_phase_inc(apu->triangle.freq);
        break;
    case TINY16_MMIO_APU_CH2_VOL: apu->triangle.volume = value & 0x0F; break;
    case TINY16_MMIO_APU_CH2_CTRL: {
        bool prev_enabled = apu->triangle.enabled;
        apu->triangle.enabled = (value & 0x01) != 0;
        if (apu->triangle.enabled && !prev_enabled) {
            env_start(&apu->triangle.env);
            if (apu->triangle.length > 0) {
                apu->triangle.length_left = length_samples(apu->triangle.length);
            }
        }
        if (!apu->triangle.enabled && prev_enabled) {
            env_release(&apu->triangle.env);
        }
    }
        if (value & 0x02) {
            apu->triangle.phase = 0;
            env_start(&apu->triangle.env);
            if (apu->triangle.length > 0) {
                apu->triangle.length_left = length_samples(apu->triangle.length);
            }
        }
        break;

    // Channel 3 - Noise
    case TINY16_MMIO_APU_CH3_PERIOD: apu->noise.period = value & 0x0F; break;
    case TINY16_MMIO_APU_CH3_VOL: apu->noise.volume = value & 0x0F; break;
    case TINY16_MMIO_APU_CH3_CTRL: {
        bool prev_enabled = apu->noise.enabled;
        apu->noise.enabled = (value & 0x01) != 0;
        if (apu->noise.enabled && !prev_enabled) {
            env_start(&apu->noise.env);
            if (apu->noise.length > 0) {
                apu->noise.length_left = length_samples(apu->noise.length);
            }
        }
        if (!apu->noise.enabled && prev_enabled) {
            env_release(&apu->noise.env);
        }
    }
        if (value & 0x02) { // trigger - reseed LFSR
            apu->noise.lfsr = LFSR_SEED;
            apu->noise.timer = 0;
            env_start(&apu->noise.env);
            if (apu->noise.length > 0) {
                apu->noise.length_left = length_samples(apu->noise.length);
            }
        }
        apu->noise.short_mode = (value & 0x04) != 0;
        break;

    case TINY16_MMIO_APU_CH0_ENV_AD:
        apu->pulse1.env.attack = (value >> 4) & 0x0F;
        apu->pulse1.env.decay = value & 0x0F;
        break;
    case TINY16_MMIO_APU_CH0_ENV_SR:
        apu->pulse1.env.sustain = (value >> 4) & 0x0F;
        apu->pulse1.env.release = value & 0x0F;
        break;
    case TINY16_MMIO_APU_CH1_ENV_AD:
        apu->pulse2.env.attack = (value >> 4) & 0x0F;
        apu->pulse2.env.decay = value & 0x0F;
        break;
    case TINY16_MMIO_APU_CH1_ENV_SR:
        apu->pulse2.env.sustain = (value >> 4) & 0x0F;
        apu->pulse2.env.release = value & 0x0F;
        break;
    case TINY16_MMIO_APU_CH2_ENV_AD:
        apu->triangle.env.attack = (value >> 4) & 0x0F;
        apu->triangle.env.decay = value & 0x0F;
        break;
    case TINY16_MMIO_APU_CH2_ENV_SR:
        apu->triangle.env.sustain = (value >> 4) & 0x0F;
        apu->triangle.env.release = value & 0x0F;
        break;
    case TINY16_MMIO_APU_CH3_ENV_AD:
        apu->noise.env.attack = (value >> 4) & 0x0F;
        apu->noise.env.decay = value & 0x0F;
        break;
    case TINY16_MMIO_APU_CH3_ENV_SR:
        apu->noise.env.sustain = (value >> 4) & 0x0F;
        apu->noise.env.release = value & 0x0F;
        break;

    case TINY16_MMIO_APU_CH0_SWEEP:
        apu->pulse1.sweep_rate = (value >> 4) & 0x0F;
        apu->pulse1.sweep_down = (value & 0x08) != 0;
        apu->pulse1.sweep_shift = value & 0x07;
        sweep_init(&apu->pulse1);
        break;
    case TINY16_MMIO_APU_CH1_SWEEP:
        apu->pulse2.sweep_rate = (value >> 4) & 0x0F;
        apu->pulse2.sweep_down = (value & 0x08) != 0;
        apu->pulse2.sweep_shift = value & 0x07;
        sweep_init(&apu->pulse2);
        break;
    case TINY16_MMIO_APU_CH0_LEN:
        apu->pulse1.length = value;
        apu->pulse1.length_left = length_samples(value);
        break;
    case TINY16_MMIO_APU_CH1_LEN:
        apu->pulse2.length = value;
        apu->pulse2.length_left = length_samples(value);
        break;
    case TINY16_MMIO_APU_CH2_LEN:
        apu->triangle.length = value;
        apu->triangle.length_left = length_samples(value);
        break;
    case TINY16_MMIO_APU_CH3_LEN:
        apu->noise.length = value;
        apu->noise.length_left = length_samples(value);
        break;

    case TINY16_MMIO_APU_WAVE_FREQ_LO:
        apu->wave.freq = (apu->wave.freq & 0x700) | value;
        apu->wave.phase_inc = calc_phase_inc(apu->wave.freq);
        break;
    case TINY16_MMIO_APU_WAVE_FREQ_HI:
        apu->wave.freq = (apu->wave.freq & 0x0FF) | ((value & 0x07) << 8);
        apu->wave.phase_inc = calc_phase_inc(apu->wave.freq);
        break;
    case TINY16_MMIO_APU_WAVE_VOL: apu->wave.volume = value & 0x0F; break;
    case TINY16_MMIO_APU_WAVE_CTRL: {
        bool prev_enabled = apu->wave.enabled;
        apu->wave.enabled = (value & 0x01) != 0;
        if (apu->wave.enabled && !prev_enabled) {
            env_start(&apu->wave.env);
            if (apu->wave.length > 0) {
                apu->wave.length_left = length_samples(apu->wave.length);
            }
        }
        if (!apu->wave.enabled && prev_enabled) {
            env_release(&apu->wave.env);
        }
    }
        if (value & 0x02) {
            apu->wave.phase = 0;
            env_start(&apu->wave.env);
            if (apu->wave.length > 0) {
                apu->wave.length_left = length_samples(apu->wave.length);
            }
        }
        break;
    case TINY16_MMIO_APU_WAVE_LEN:
        apu->wave.length = value;
        apu->wave.length_left = length_samples(value);
        break;
    case TINY16_MMIO_APU_WAVE_ENV_AD:
        apu->wave.env.attack = (value >> 4) & 0x0F;
        apu->wave.env.decay = value & 0x0F;
        break;
    case TINY16_MMIO_APU_WAVE_ENV_SR:
        apu->wave.env.sustain = (value >> 4) & 0x0F;
        apu->wave.env.release = value & 0x0F;
        break;

    // SFX System
    case TINY16_MMIO_APU_SFX_PLAY: tiny16_apu_play_sfx(apu, value); break;
    case TINY16_MMIO_APU_SFX_STOP: tiny16_apu_stop_sfx(apu, value); break;
    case TINY16_MMIO_APU_SFX_TABLE_HI:
        apu->sfx.table_addr = (apu->sfx.table_addr & 0x00FF) | ((uint16_t)value << 8);
        break;
    case TINY16_MMIO_APU_SFX_TABLE_LO:
        apu->sfx.table_addr = (apu->sfx.table_addr & 0xFF00) | value;
        break;
    case TINY16_MMIO_APU_SFX_COUNT: apu->sfx.count = value; break;

    // Music System
    case TINY16_MMIO_APU_MUSIC_CTRL:
        if (value & 0x01) {
            apu->music.looping = (value & 0x04) != 0;
            tiny16_apu_music_start(apu);
        }
        if (value & 0x02) {
            tiny16_apu_music_stop(apu);
        }
        break;
    case TINY16_MMIO_APU_MUSIC_ADDR_HI:
        apu->music.song_addr = (apu->music.song_addr & 0x00FF) | ((uint16_t)value << 8);
        break;
    case TINY16_MMIO_APU_MUSIC_ADDR_LO:
        apu->music.song_addr = (apu->music.song_addr & 0xFF00) | value;
        break;
    case TINY16_MMIO_APU_MUSIC_LEN_HI:
        apu->music.song_length = (apu->music.song_length & 0x00FF) | ((uint16_t)value << 8);
        break;
    case TINY16_MMIO_APU_MUSIC_LEN_LO:
        apu->music.song_length = (apu->music.song_length & 0xFF00) | value;
        break;
    }

    if (addr >= TINY16_MMIO_APU_WAVE_RAM && addr <= (TINY16_MMIO_APU_WAVE_RAM + 0x1F)) {
        apu->wave.wave[addr - TINY16_MMIO_APU_WAVE_RAM] = value & 0x0F;
    }
    apu_unlock(apu);
}

uint8_t tiny16_apu_mmio_read(Tiny16APU* apu, uint16_t addr) {
    uint8_t value = 0;
    apu_lock(apu);
    switch (addr) {
    case TINY16_MMIO_APU_CTRL:
        value = (apu->master_volume << 4) | (apu->enabled ? 0x01 : 0x00);
        break;

    case TINY16_MMIO_APU_STATUS: {
        uint8_t status = 0;
        if (apu->pulse1.enabled) status |= 0x01;
        if (apu->pulse2.enabled) status |= 0x02;
        if (apu->triangle.enabled) status |= 0x04;
        if (apu->noise.enabled) status |= 0x08;
        if (apu->wave.enabled) status |= 0x10;
        value = status;
        break;
    }

    case TINY16_MMIO_APU_CH0_VOL: value = (apu->pulse1.duty << 4) | apu->pulse1.volume; break;
    case TINY16_MMIO_APU_CH0_CTRL: value = apu->pulse1.enabled ? 0x01 : 0x00; break;

    case TINY16_MMIO_APU_CH1_VOL: value = (apu->pulse2.duty << 4) | apu->pulse2.volume; break;
    case TINY16_MMIO_APU_CH1_CTRL: value = apu->pulse2.enabled ? 0x01 : 0x00; break;

    case TINY16_MMIO_APU_CH2_VOL: value = apu->triangle.volume; break;
    case TINY16_MMIO_APU_CH2_CTRL: value = apu->triangle.enabled ? 0x01 : 0x00; break;

    case TINY16_MMIO_APU_CH3_PERIOD: value = apu->noise.period; break;
    case TINY16_MMIO_APU_CH3_VOL: value = apu->noise.volume; break;
    case TINY16_MMIO_APU_CH3_CTRL:
        value = (apu->noise.short_mode ? 0x04 : 0x00) | (apu->noise.enabled ? 0x01 : 0x00);
        break;

    case TINY16_MMIO_APU_CH0_ENV_AD:
        value = (apu->pulse1.env.attack << 4) | apu->pulse1.env.decay;
        break;
    case TINY16_MMIO_APU_CH0_ENV_SR:
        value = (apu->pulse1.env.sustain << 4) | apu->pulse1.env.release;
        break;
    case TINY16_MMIO_APU_CH1_ENV_AD:
        value = (apu->pulse2.env.attack << 4) | apu->pulse2.env.decay;
        break;
    case TINY16_MMIO_APU_CH1_ENV_SR:
        value = (apu->pulse2.env.sustain << 4) | apu->pulse2.env.release;
        break;
    case TINY16_MMIO_APU_CH2_ENV_AD:
        value = (apu->triangle.env.attack << 4) | apu->triangle.env.decay;
        break;
    case TINY16_MMIO_APU_CH2_ENV_SR:
        value = (apu->triangle.env.sustain << 4) | apu->triangle.env.release;
        break;
    case TINY16_MMIO_APU_CH3_ENV_AD:
        value = (apu->noise.env.attack << 4) | apu->noise.env.decay;
        break;
    case TINY16_MMIO_APU_CH3_ENV_SR:
        value = (apu->noise.env.sustain << 4) | apu->noise.env.release;
        break;

    case TINY16_MMIO_APU_CH0_SWEEP:
        value = (apu->pulse1.sweep_rate << 4) | (apu->pulse1.sweep_down ? 0x08 : 0x00) |
                (apu->pulse1.sweep_shift & 0x07);
        break;
    case TINY16_MMIO_APU_CH1_SWEEP:
        value = (apu->pulse2.sweep_rate << 4) | (apu->pulse2.sweep_down ? 0x08 : 0x00) |
                (apu->pulse2.sweep_shift & 0x07);
        break;
    case TINY16_MMIO_APU_CH0_LEN: value = apu->pulse1.length; break;
    case TINY16_MMIO_APU_CH1_LEN: value = apu->pulse2.length; break;
    case TINY16_MMIO_APU_CH2_LEN: value = apu->triangle.length; break;
    case TINY16_MMIO_APU_CH3_LEN: value = apu->noise.length; break;

    case TINY16_MMIO_APU_WAVE_FREQ_LO: value = apu->wave.freq & 0xFF; break;
    case TINY16_MMIO_APU_WAVE_FREQ_HI: value = (apu->wave.freq >> 8) & 0x07; break;
    case TINY16_MMIO_APU_WAVE_VOL: value = apu->wave.volume; break;
    case TINY16_MMIO_APU_WAVE_CTRL: value = apu->wave.enabled ? 0x01 : 0x00; break;
    case TINY16_MMIO_APU_WAVE_LEN: value = apu->wave.length; break;
    case TINY16_MMIO_APU_WAVE_ENV_AD:
        value = (apu->wave.env.attack << 4) | apu->wave.env.decay;
        break;
    case TINY16_MMIO_APU_WAVE_ENV_SR:
        value = (apu->wave.env.sustain << 4) | apu->wave.env.release;
        break;

    // SFX System
    case TINY16_MMIO_APU_SFX_STATUS: value = tiny16_apu_get_sfx_status(apu); break;
    case TINY16_MMIO_APU_SFX_TABLE_HI: value = (apu->sfx.table_addr >> 8) & 0xFF; break;
    case TINY16_MMIO_APU_SFX_TABLE_LO: value = apu->sfx.table_addr & 0xFF; break;
    case TINY16_MMIO_APU_SFX_COUNT: value = apu->sfx.count; break;

    // Music System
    case TINY16_MMIO_APU_MUSIC_STATUS: value = apu->music.enabled ? 0x01 : 0x00; break;
    case TINY16_MMIO_APU_MUSIC_ADDR_HI: value = (apu->music.song_addr >> 8) & 0xFF; break;
    case TINY16_MMIO_APU_MUSIC_ADDR_LO: value = apu->music.song_addr & 0xFF; break;
    case TINY16_MMIO_APU_MUSIC_LEN_HI: value = (apu->music.song_length >> 8) & 0xFF; break;
    case TINY16_MMIO_APU_MUSIC_LEN_LO: value = apu->music.song_length & 0xFF; break;
    }
    if (addr >= TINY16_MMIO_APU_WAVE_RAM && addr <= (TINY16_MMIO_APU_WAVE_RAM + 0x1F)) {
        value = apu->wave.wave[addr - TINY16_MMIO_APU_WAVE_RAM] & 0x0F;
    }
    apu_unlock(apu);
    return value;
}

uint32_t tiny16_apu_samples_for_cycles(Tiny16APU* apu, uint32_t cpu_cycles, uint32_t cpu_hz) {
    apu_lock(apu);
    if (cpu_hz == 0) {
        apu_unlock(apu);
        return 0;
    }
    uint64_t add = (uint64_t)cpu_cycles * (uint64_t)TINY16_APU_SAMPLE_RATE;
    uint64_t accum = apu->sample_accum + add;
    uint32_t frames = (uint32_t)(accum / cpu_hz);
    apu->sample_accum = accum - (uint64_t)frames * cpu_hz;
    apu_unlock(apu);
    return frames;
}

void tiny16_apu_generate_samples(Tiny16APU* apu, float* buffer, unsigned int frames) {
    apu_lock(apu);
    for (unsigned int i = 0; i < frames; i++) {
        float mix = 0.0f;
        int active_channels = 0;

        // Update SFX/music once per frame (every samples_per_frame samples)
        apu->frame_samples++;
        if (apu->frame_samples >= samples_per_frame) {
            apu->frame_samples = 0;
            tiny16_apu_update_sfx(apu);
            tiny16_apu_update_music(apu);
        }

        if (!apu->enabled) {
            buffer[i] = 0.0f;
            continue;
        }

        if (apu->pulse1.length_left > 0) {
            apu->pulse1.length_left--;
            if (apu->pulse1.length_left == 0) {
                apu->pulse1.enabled = false;
                env_release(&apu->pulse1.env);
            }
        }
        if ((apu->pulse1.enabled || apu->pulse1.env.phase != TINY16_APU_ENV_OFF) &&
            apu->pulse1.volume > 0) {
            float env = env_update(&apu->pulse1.env);
            float sample = generate_pulse(apu->pulse1.phase, apu->pulse1.duty);
            sample *= ((float)apu->pulse1.volume / 15.0f) * env;
            if (env > 0.0f) {
                mix += sample;
                active_channels++;
            }
            apu->pulse1.phase += apu->pulse1.phase_inc;
            sweep_step(&apu->pulse1);
        }

        if (apu->pulse2.length_left > 0) {
            apu->pulse2.length_left--;
            if (apu->pulse2.length_left == 0) {
                apu->pulse2.enabled = false;
                env_release(&apu->pulse2.env);
            }
        }
        if ((apu->pulse2.enabled || apu->pulse2.env.phase != TINY16_APU_ENV_OFF) &&
            apu->pulse2.volume > 0) {
            float env = env_update(&apu->pulse2.env);
            float sample = generate_pulse(apu->pulse2.phase, apu->pulse2.duty);
            sample *= ((float)apu->pulse2.volume / 15.0f) * env;
            if (env > 0.0f) {
                mix += sample;
                active_channels++;
            }
            apu->pulse2.phase += apu->pulse2.phase_inc;
            sweep_step(&apu->pulse2);
        }

        if (apu->triangle.length_left > 0) {
            apu->triangle.length_left--;
            if (apu->triangle.length_left == 0) {
                apu->triangle.enabled = false;
                env_release(&apu->triangle.env);
            }
        }
        if ((apu->triangle.enabled || apu->triangle.env.phase != TINY16_APU_ENV_OFF) &&
            apu->triangle.volume > 0) {
            float env = env_update(&apu->triangle.env);
            float sample = generate_triangle(apu->triangle.phase);
            sample *= ((float)apu->triangle.volume / 15.0f) * env;
            if (env > 0.0f) {
                mix += sample;
                active_channels++;
            }
            apu->triangle.phase += apu->triangle.phase_inc;
        }

        if (apu->noise.length_left > 0) {
            apu->noise.length_left--;
            if (apu->noise.length_left == 0) {
                apu->noise.enabled = false;
                env_release(&apu->noise.env);
            }
        }
        if ((apu->noise.enabled || apu->noise.env.phase != TINY16_APU_ENV_OFF) &&
            apu->noise.volume > 0) {
            float env = env_update(&apu->noise.env);
            apu->noise.timer++;
            if (apu->noise.timer >= noise_periods[apu->noise.period]) {
                apu->noise.timer = 0;
                if (apu->noise.short_mode) {
                    noise_step_short(&apu->noise.lfsr);
                } else {
                    noise_step_long(&apu->noise.lfsr);
                }
            }
            float sample = generate_noise(apu->noise.lfsr);
            sample *= ((float)apu->noise.volume / 15.0f) * env;
            if (env > 0.0f) {
                mix += sample;
                active_channels++;
            }
        }

        if (apu->wave.length_left > 0) {
            apu->wave.length_left--;
            if (apu->wave.length_left == 0) {
                apu->wave.enabled = false;
                env_release(&apu->wave.env);
            }
        }
        if ((apu->wave.enabled || apu->wave.env.phase != TINY16_APU_ENV_OFF) &&
            apu->wave.volume > 0) {
            float env = env_update(&apu->wave.env);
            float sample = generate_wave(apu->wave.wave, apu->wave.phase);
            sample *= ((float)apu->wave.volume / 15.0f) * env;
            if (env > 0.0f) {
                mix += sample;
                active_channels++;
            }
            apu->wave.phase += apu->wave.phase_inc;
        }

        // Normalize by active channel count
        if (active_channels > 0) {
            mix /= (float)active_channels;
        }

        mix *= (float)apu->master_volume / 15.0f;

        if (mix > 1.0f) mix = 1.0f;
        if (mix < -1.0f) mix = -1.0f;

        buffer[i] = mix;
    }
    apu_unlock(apu);
}
