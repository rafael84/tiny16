#include "apu.h"
#include "memory.h"

// =============================================================================
// Constants
// =============================================================================

#define LFSR_SEED         0xACE1
#define SAMPLES_PER_FRAME (TINY16_APU_SAMPLE_RATE / 60)

static const uint32_t noise_periods[] = {1,  2,  4,   8,   16,  32,  48,  64,
                                         80, 96, 112, 128, 160, 202, 254, 380};

static const float env_rate_seconds[] = {
    0.0f,  0.005f, 0.01f, 0.02f, 0.04f, 0.06f, 0.08f, 0.10f,
    0.15f, 0.20f,  0.30f, 0.40f, 0.60f, 0.80f, 1.00f, 1.50f,
};

// Note frequency table (C2-B6, 60 notes)
static const uint16_t note_freq[] = {
    // Octave 2
    1374,
    1412,
    1447,
    1481,
    1513,
    1543,
    1571,
    1598,
    1623,
    1647,
    1670,
    1691,
    // Octave 3
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
    // Octave 4
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
    // Octave 5
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
    // Octave 6
    2006,
    2008,
    2010,
    2013,
    2015,
    2016,
    2018,
    2020,
    2021,
    2023,
    2024,
    2026,
};
#define NOTE_TABLE_SIZE (sizeof(note_freq) / sizeof(note_freq[0]))

// =============================================================================
// Locking
// =============================================================================

static inline void apu_lock(Tiny16APU* apu) {
    while (__sync_lock_test_and_set(&apu->lock, 1)) {
    }
}

static inline void apu_unlock(Tiny16APU* apu) { __sync_lock_release(&apu->lock); }

// =============================================================================
// Waveform Generators
// =============================================================================

static inline uint32_t calc_phase_inc(uint16_t freq_value) {
    uint32_t divisor = 2048 - freq_value;
    if (divisor == 0) divisor = 1;
    return (uint32_t)(4294967296.0 / divisor);
}

static inline float generate_triangle(uint32_t phase) {
    float t = (float)phase / 4294967296.0f;
    if (t < 0.25f) return t * 4.0f;
    if (t < 0.75f) return 2.0f - t * 4.0f;
    return t * 4.0f - 4.0f;
}

static inline float generate_saw(uint32_t phase) {
    return ((float)phase / 4294967296.0f) * 2.0f - 1.0f;
}

static inline float generate_tsaw(uint32_t phase) {
    float t = (float)phase / 4294967296.0f;
    if (t < 0.125f) return t * 8.0f;
    return 1.0f - (t - 0.125f) * (2.0f / 0.875f);
}

static inline float generate_pulse(uint32_t phase, uint8_t duty) {
    // duty: 0=12.5%, 1=25%, 2=50%, 3=75%
    static const uint32_t thresholds[] = {0x20000000, 0x40000000, 0x80000000, 0xC0000000};
    return (phase < thresholds[duty & 0x03]) ? 1.0f : -1.0f;
}

static inline float generate_organ(uint32_t phase) {
    float t = (float)phase / 4294967296.0f;
    // Fundamental triangle
    float tri = (t < 0.25f) ? t * 4.0f : (t < 0.75f) ? 2.0f - t * 4.0f : t * 4.0f - 4.0f;
    // 2nd harmonic
    float t2 = t * 2.0f;
    t2 = t2 - (int)t2;
    float h2 = (t2 < 0.25f) ? t2 * 4.0f : (t2 < 0.75f) ? 2.0f - t2 * 4.0f : t2 * 4.0f - 4.0f;
    return tri * 0.7f + h2 * 0.3f;
}

static inline float generate_phaser(uint32_t phase) {
    float p1 = (phase < 0x40000000) ? 1.0f : -1.0f;
    uint32_t phase2 = phase + 0x08000000;
    float p2 = (phase2 < 0x40000000) ? 1.0f : -1.0f;
    return (p1 + p2) * 0.5f;
}

static inline void noise_step(uint16_t* lfsr) {
    uint16_t bit = ((*lfsr >> 0) ^ (*lfsr >> 1) ^ (*lfsr >> 3) ^ (*lfsr >> 12)) & 1;
    *lfsr = (*lfsr >> 1) | (bit << 15);
}

// =============================================================================
// Envelope
// =============================================================================

static inline void env_start(Tiny16APUEnvelope* env) {
    env->level = 0.01f; // Start slightly above 0 for immediate sound
    env->phase = TINY16_APU_ENV_ATTACK;
}

static inline void env_release(Tiny16APUEnvelope* env) {
    if (env->phase != TINY16_APU_ENV_OFF) {
        env->phase = TINY16_APU_ENV_RELEASE;
    }
}

static inline float env_update(Tiny16APUEnvelope* env) {
    float sustain = (float)env->sustain / 15.0f;
    float rate;

    switch (env->phase) {
    case TINY16_APU_ENV_OFF: return 0.0f;

    case TINY16_APU_ENV_ATTACK:
        rate = env_rate_seconds[env->attack & 0x0F];
        if (rate <= 0.0f) {
            env->level = 1.0f;
            env->phase = TINY16_APU_ENV_DECAY;
        } else {
            env->level += 1.0f / (rate * TINY16_APU_SAMPLE_RATE);
            if (env->level >= 1.0f) {
                env->level = 1.0f;
                env->phase = TINY16_APU_ENV_DECAY;
            }
        }
        break;

    case TINY16_APU_ENV_DECAY:
        rate = env_rate_seconds[env->decay & 0x0F];
        if (rate <= 0.0f || sustain >= 1.0f) {
            env->level = sustain;
            env->phase = TINY16_APU_ENV_SUSTAIN;
        } else {
            env->level -= (1.0f - sustain) / (rate * TINY16_APU_SAMPLE_RATE);
            if (env->level <= sustain) {
                env->level = sustain;
                env->phase = TINY16_APU_ENV_SUSTAIN;
            }
        }
        break;

    case TINY16_APU_ENV_SUSTAIN: env->level = sustain; break;

    case TINY16_APU_ENV_RELEASE:
        rate = env_rate_seconds[env->release & 0x0F];
        if (rate <= 0.0f) {
            env->level = 0.0f;
            env->phase = TINY16_APU_ENV_OFF;
        } else {
            env->level -= 1.0f / (rate * TINY16_APU_SAMPLE_RATE);
            if (env->level <= 0.0f) {
                env->level = 0.0f;
                env->phase = TINY16_APU_ENV_OFF;
            }
        }
        break;
    }

    return env->level;
}

// =============================================================================
// Channel Sample Generation
// =============================================================================

static inline float generate_channel_sample(Tiny16APUChannel* ch) {
    if (ch->volume == 0) return 0.0f;

    float sample = 0.0f;

    switch (ch->waveform) {
    case TINY16_APU_WAVE_TRIANGLE:
        sample = generate_triangle(ch->phase);
        ch->phase += ch->phase_inc;
        break;
    case TINY16_APU_WAVE_TSAW:
        sample = generate_tsaw(ch->phase);
        ch->phase += ch->phase_inc;
        break;
    case TINY16_APU_WAVE_SAW:
        sample = generate_saw(ch->phase);
        ch->phase += ch->phase_inc;
        break;
    case TINY16_APU_WAVE_SQUARE:
        sample = generate_pulse(ch->phase, 2);
        ch->phase += ch->phase_inc;
        break;
    case TINY16_APU_WAVE_PULSE:
        sample = generate_pulse(ch->phase, 1);
        ch->phase += ch->phase_inc;
        break;
    case TINY16_APU_WAVE_ORGAN:
        sample = generate_organ(ch->phase);
        ch->phase += ch->phase_inc;
        break;
    case TINY16_APU_WAVE_NOISE:
        ch->noise_timer++;
        if (ch->noise_timer >= noise_periods[ch->freq & 0x0F]) {
            ch->noise_timer = 0;
            noise_step(&ch->lfsr);
        }
        sample = ((float)ch->lfsr / 32768.0f) - 1.0f;
        break;
    case TINY16_APU_WAVE_PHASER:
        sample = generate_phaser(ch->phase);
        ch->phase += ch->phase_inc;
        break;
    }

    return sample * ((float)ch->volume / 15.0f);
}

// Same generator for music channels
static inline float generate_music_sample(Tiny16MusicChannel* mch) {
    if (mch->volume == 0) return 0.0f;

    float sample = 0.0f;

    switch (mch->waveform) {
    case TINY16_APU_WAVE_TRIANGLE:
        sample = generate_triangle(mch->phase);
        mch->phase += mch->phase_inc;
        break;
    case TINY16_APU_WAVE_TSAW:
        sample = generate_tsaw(mch->phase);
        mch->phase += mch->phase_inc;
        break;
    case TINY16_APU_WAVE_SAW:
        sample = generate_saw(mch->phase);
        mch->phase += mch->phase_inc;
        break;
    case TINY16_APU_WAVE_SQUARE:
        sample = generate_pulse(mch->phase, 2);
        mch->phase += mch->phase_inc;
        break;
    case TINY16_APU_WAVE_PULSE:
        sample = generate_pulse(mch->phase, 1);
        mch->phase += mch->phase_inc;
        break;
    case TINY16_APU_WAVE_ORGAN:
        sample = generate_organ(mch->phase);
        mch->phase += mch->phase_inc;
        break;
    case TINY16_APU_WAVE_NOISE:
        mch->noise_timer++;
        if (mch->noise_timer >= noise_periods[(mch->freq >> 7) & 0x0F]) {
            mch->noise_timer = 0;
            noise_step(&mch->lfsr);
        }
        sample = ((float)mch->lfsr / 32768.0f) - 1.0f;
        break;
    case TINY16_APU_WAVE_PHASER:
        sample = generate_phaser(mch->phase);
        mch->phase += mch->phase_inc;
        break;
    }

    return sample * ((float)mch->volume / 15.0f);
}

// =============================================================================
// Reset
// =============================================================================

static inline void reset_channel(Tiny16APUChannel* ch) {
    ch->enabled = false;
    ch->waveform = TINY16_APU_WAVE_PULSE;
    ch->freq = 0;
    ch->volume = 0;
    ch->phase = 0;
    ch->phase_inc = 0;
    ch->lfsr = LFSR_SEED;
    ch->noise_timer = 0;
    ch->env.attack = 0;
    ch->env.decay = 0;
    ch->env.sustain = 15;
    ch->env.release = 0;
    ch->env.phase = TINY16_APU_ENV_OFF;
    ch->env.level = 0.0f;
    ch->length = 0;
    ch->length_left = 0;
}

static inline void reset_music_channel(Tiny16MusicChannel* mch) {
    mch->enabled = false;
    mch->looping = false;
    mch->track_addr = 0;
    mch->track_length = 0;
    mch->current_note = 0;
    mch->frames_left = 0;
    mch->waveform = TINY16_APU_WAVE_PULSE;
    mch->freq = 0;
    mch->volume = 0;
    mch->phase = 0;
    mch->phase_inc = 0;
    mch->lfsr = LFSR_SEED;
    mch->noise_timer = 0;
}

void tiny16_apu_reset(Tiny16APU* apu) {
    apu->lock = 0;
    apu->enabled = false;
    apu->master_volume = 0;
    apu->sample_accum = 0;
    apu->frame_samples = 0;
    apu->memory = NULL;

    for (int i = 0; i < TINY16_APU_NUM_CHANNELS; i++) {
        reset_channel(&apu->ch[i]);
        apu->sfx.ch[i].active = false;
        apu->sfx.ch[i].duration_left = 0;
        apu->sfx.ch[i].sfx_id = 0;
        reset_music_channel(&apu->music.ch[i]);
    }

    apu->sfx.table_addr = 0;
    apu->sfx.count = 0;
    apu->music.enabled = false;
}

// =============================================================================
// SFX System
// =============================================================================

// SFX entry format (6 bytes):
// [0] channel (0-3)
// [1] freq_lo
// [2] freq_hi (bits 0-2) | waveform (bits 4-6)
// [3] volume (bits 0-3)
// [4] env_ad (attack << 4 | decay)
// [5] duration (frames)

static void play_sfx(Tiny16APU* apu, uint8_t sfx_id) {
    if (!apu->memory || sfx_id >= apu->sfx.count) return;

    Tiny16Memory* mem = (Tiny16Memory*)apu->memory;
    uint16_t addr = apu->sfx.table_addr + (uint16_t)sfx_id * TINY16_APU_SFX_ENTRY_SIZE;

    uint8_t channel = mem->bytes[addr + 0] & 0x03;
    uint8_t freq_lo = mem->bytes[addr + 1];
    uint8_t freq_hi_wave = mem->bytes[addr + 2];
    uint8_t volume = mem->bytes[addr + 3] & 0x0F;
    uint8_t env_ad = mem->bytes[addr + 4];
    uint8_t duration = mem->bytes[addr + 5];

    Tiny16APUChannel* ch = &apu->ch[channel];

    ch->freq = ((uint16_t)(freq_hi_wave & 0x07) << 8) | freq_lo;
    ch->waveform = (freq_hi_wave >> 4) & 0x07;
    ch->volume = volume;
    ch->phase = 0;
    ch->phase_inc = calc_phase_inc(ch->freq);
    ch->lfsr = LFSR_SEED;
    ch->noise_timer = 0;
    ch->env.attack = (env_ad >> 4) & 0x0F;
    ch->env.decay = env_ad & 0x0F;
    ch->env.sustain = 15;
    ch->env.release = 2;
    ch->length = duration;
    ch->length_left = (uint32_t)duration * SAMPLES_PER_FRAME;
    ch->enabled = true;
    env_start(&ch->env);

    apu->sfx.ch[channel].active = true;
    apu->sfx.ch[channel].duration_left = duration;
    apu->sfx.ch[channel].sfx_id = sfx_id;
}

static void stop_sfx(Tiny16APU* apu, uint8_t channel) {
    if (channel >= TINY16_APU_NUM_CHANNELS) return;

    apu->sfx.ch[channel].active = false;
    apu->sfx.ch[channel].duration_left = 0;
    apu->ch[channel].enabled = false;
    env_release(&apu->ch[channel].env);
}

static void update_sfx(Tiny16APU* apu) {
    for (int i = 0; i < TINY16_APU_NUM_CHANNELS; i++) {
        if (apu->sfx.ch[i].active && apu->sfx.ch[i].duration_left > 0) {
            apu->sfx.ch[i].duration_left--;
            if (apu->sfx.ch[i].duration_left == 0) {
                apu->sfx.ch[i].active = false;
            }
        }
    }
}

// =============================================================================
// Music System
// =============================================================================

// Music note format (4 bytes):
// [0] note (0=rest, 1-60=C2-B6)
// [1] volume (0-15)
// [2] duration (frames)
// [3] waveform (0-7)

static void play_music_note(Tiny16MusicChannel* mch, uint8_t note, uint8_t volume,
                            uint8_t waveform) {
    if (note == 0) {
        mch->volume = 0;
        return;
    }

    mch->volume = volume & 0x0F;
    mch->waveform = waveform & 0x07;

    if (waveform == TINY16_APU_WAVE_NOISE) {
        uint8_t period = (note <= 15) ? note : 8;
        mch->freq = period << 7;
        mch->lfsr = LFSR_SEED;
        mch->noise_timer = 0;
    } else {
        uint8_t note_idx = note - 1;
        if (note_idx >= NOTE_TABLE_SIZE) {
            mch->volume = 0;
            return;
        }
        mch->freq = note_freq[note_idx];
        mch->phase_inc = calc_phase_inc(mch->freq);
        mch->phase = 0;
    }
}

static void music_channel_start(Tiny16APU* apu, int ch) {
    if (ch < 0 || ch >= TINY16_APU_NUM_CHANNELS) return;

    Tiny16MusicChannel* mch = &apu->music.ch[ch];
    mch->enabled = true;
    mch->current_note = 0;
    mch->frames_left = 0;
    mch->volume = 0;
    mch->waveform = TINY16_APU_WAVE_PULSE;
    mch->phase = 0;
    mch->phase_inc = 0;
    mch->lfsr = LFSR_SEED;
    mch->noise_timer = 0;
    apu->music.enabled = true;
}

static void music_channel_stop(Tiny16APU* apu, int ch) {
    if (ch < 0 || ch >= TINY16_APU_NUM_CHANNELS) return;

    apu->music.ch[ch].enabled = false;
    apu->music.ch[ch].volume = 0;

    // Check if any channel still playing
    apu->music.enabled = false;
    for (int i = 0; i < TINY16_APU_NUM_CHANNELS; i++) {
        if (apu->music.ch[i].enabled) {
            apu->music.enabled = true;
            break;
        }
    }
}

static void update_music_channel(Tiny16APU* apu, int ch) {
    Tiny16MusicChannel* mch = &apu->music.ch[ch];
    if (!mch->enabled || mch->track_length == 0) return;

    Tiny16Memory* mem = (Tiny16Memory*)apu->memory;

    if (mch->frames_left > 0) {
        mch->frames_left--;
        return;
    }

    // Read next note
    uint16_t addr = mch->track_addr + (mch->current_note * TINY16_APU_MUSIC_NOTE_SIZE);
    uint8_t note = mem->bytes[addr + 0];
    uint8_t volume = mem->bytes[addr + 1];
    uint8_t duration = mem->bytes[addr + 2];
    uint8_t waveform = mem->bytes[addr + 3];

    play_music_note(mch, note, volume, waveform);
    mch->frames_left = duration;

    mch->current_note++;
    if (mch->current_note >= mch->track_length) {
        if (mch->looping) {
            mch->current_note = 0;
        } else {
            music_channel_stop(apu, ch);
        }
    }
}

static void update_music(Tiny16APU* apu) {
    if (!apu->music.enabled || !apu->memory) return;

    for (int ch = 0; ch < TINY16_APU_NUM_CHANNELS; ch++) {
        update_music_channel(apu, ch);
    }
}

// =============================================================================
// MMIO
// =============================================================================

void tiny16_apu_mmio_write(Tiny16APU* apu, uint16_t addr, uint8_t value) {
    apu_lock(apu);

    // Master control
    if (addr == TINY16_MMIO_APU_CTRL) {
        apu->enabled = (value & 0x01) != 0;
        apu->master_volume = (value >> 4) & 0x0F;
        apu_unlock(apu);
        return;
    }

    // Channel registers (0xBF42 - 0xBF59)
    if (addr >= TINY16_MMIO_APU_CH_BASE &&
        addr < TINY16_MMIO_APU_CH_BASE + 4 * TINY16_MMIO_APU_CH_STRIDE) {
        int ch_idx = (addr - TINY16_MMIO_APU_CH_BASE) / TINY16_MMIO_APU_CH_STRIDE;
        int reg = (addr - TINY16_MMIO_APU_CH_BASE) % TINY16_MMIO_APU_CH_STRIDE;
        Tiny16APUChannel* ch = &apu->ch[ch_idx];

        switch (reg) {
        case 0: // FREQ_LO
            ch->freq = (ch->freq & 0x700) | value;
            ch->phase_inc = calc_phase_inc(ch->freq);
            break;
        case 1: // FREQ_HI + waveform
            ch->freq = (ch->freq & 0x0FF) | ((value & 0x07) << 8);
            ch->waveform = (value >> 4) & 0x07;
            ch->phase_inc = calc_phase_inc(ch->freq);
            break;
        case 2: // VOL
            ch->volume = value & 0x0F;
            break;
        case 3: // CTRL
        {
            bool prev = ch->enabled;
            ch->enabled = (value & 0x01) != 0;
            if (ch->enabled && !prev) env_start(&ch->env);
            if (!ch->enabled && prev) env_release(&ch->env);
            if (value & 0x02) { // trigger
                ch->phase = 0;
                ch->lfsr = LFSR_SEED;
                ch->noise_timer = 0;
                env_start(&ch->env);
                if (ch->length > 0) {
                    ch->length_left = (uint32_t)ch->length * SAMPLES_PER_FRAME;
                }
            }
        } break;
        case 4: // ENV_AD
            ch->env.attack = (value >> 4) & 0x0F;
            ch->env.decay = value & 0x0F;
            break;
        case 5: // ENV_SR
            ch->env.sustain = (value >> 4) & 0x0F;
            ch->env.release = value & 0x0F;
            break;
        }
        apu_unlock(apu);
        return;
    }

    // Length registers (0xBF5A - 0xBF5D)
    if (addr >= TINY16_MMIO_APU_CH0_LEN && addr <= TINY16_MMIO_APU_CH3_LEN) {
        int ch_idx = addr - TINY16_MMIO_APU_CH0_LEN;
        apu->ch[ch_idx].length = value;
        apu->ch[ch_idx].length_left = (uint32_t)value * SAMPLES_PER_FRAME;
        apu_unlock(apu);
        return;
    }

    // Music channel registers (0xBF60 - 0xBF73)
    if (addr >= TINY16_MMIO_APU_MUSIC_BASE && addr <= TINY16_MMIO_APU_MUSIC_CH3_CTRL) {
        int offset = addr - TINY16_MMIO_APU_MUSIC_BASE;
        int ch_idx = offset / TINY16_MMIO_APU_MUSIC_STRIDE;
        int reg = offset % TINY16_MMIO_APU_MUSIC_STRIDE;
        Tiny16MusicChannel* mch = &apu->music.ch[ch_idx];

        switch (reg) {
        case 0: // ADDR_HI
            mch->track_addr = (mch->track_addr & 0x00FF) | ((uint16_t)value << 8);
            break;
        case 1: // ADDR_LO
            mch->track_addr = (mch->track_addr & 0xFF00) | value;
            break;
        case 2: // LEN_HI
            mch->track_length = (mch->track_length & 0x00FF) | ((uint16_t)value << 8);
            break;
        case 3: // LEN_LO
            mch->track_length = (mch->track_length & 0xFF00) | value;
            break;
        case 4: // CTRL
            mch->looping = (value & 0x04) != 0;
            if (value & 0x01) music_channel_start(apu, ch_idx);
            if (value & 0x02) music_channel_stop(apu, ch_idx);
            break;
        }
        apu_unlock(apu);
        return;
    }

    // SFX system (0xBF90 - 0xBF95)
    switch (addr) {
    case TINY16_MMIO_APU_SFX_PLAY: play_sfx(apu, value); break;
    case TINY16_MMIO_APU_SFX_STOP: stop_sfx(apu, value); break;
    case TINY16_MMIO_APU_SFX_TABLE_HI:
        apu->sfx.table_addr = (apu->sfx.table_addr & 0x00FF) | ((uint16_t)value << 8);
        break;
    case TINY16_MMIO_APU_SFX_TABLE_LO:
        apu->sfx.table_addr = (apu->sfx.table_addr & 0xFF00) | value;
        break;
    case TINY16_MMIO_APU_SFX_COUNT: apu->sfx.count = value; break;
    }

    apu_unlock(apu);
}

uint8_t tiny16_apu_mmio_read(Tiny16APU* apu, uint16_t addr) {
    uint8_t value = 0;
    apu_lock(apu);

    if (addr == TINY16_MMIO_APU_CTRL) {
        value = (apu->master_volume << 4) | (apu->enabled ? 0x01 : 0x00);
    } else if (addr == TINY16_MMIO_APU_STATUS) {
        for (int i = 0; i < TINY16_APU_NUM_CHANNELS; i++) {
            if (apu->ch[i].enabled) value |= (1 << i);
        }
    } else if (addr >= TINY16_MMIO_APU_CH_BASE &&
               addr < TINY16_MMIO_APU_CH_BASE + 4 * TINY16_MMIO_APU_CH_STRIDE) {
        int ch_idx = (addr - TINY16_MMIO_APU_CH_BASE) / TINY16_MMIO_APU_CH_STRIDE;
        int reg = (addr - TINY16_MMIO_APU_CH_BASE) % TINY16_MMIO_APU_CH_STRIDE;
        Tiny16APUChannel* ch = &apu->ch[ch_idx];

        switch (reg) {
        case 0: value = ch->freq & 0xFF; break;
        case 1: value = ((ch->freq >> 8) & 0x07) | ((ch->waveform & 0x07) << 4); break;
        case 2: value = ch->volume; break;
        case 3: value = ch->enabled ? 0x01 : 0x00; break;
        case 4: value = (ch->env.attack << 4) | ch->env.decay; break;
        case 5: value = (ch->env.sustain << 4) | ch->env.release; break;
        }
    } else if (addr >= TINY16_MMIO_APU_CH0_LEN && addr <= TINY16_MMIO_APU_CH3_LEN) {
        int ch_idx = addr - TINY16_MMIO_APU_CH0_LEN;
        value = apu->ch[ch_idx].length;
    } else if (addr == TINY16_MMIO_APU_SFX_STATUS) {
        for (int i = 0; i < TINY16_APU_NUM_CHANNELS; i++) {
            if (apu->sfx.ch[i].active) value |= (1 << i);
        }
    } else if (addr == TINY16_MMIO_APU_SFX_TABLE_HI) {
        value = (apu->sfx.table_addr >> 8) & 0xFF;
    } else if (addr == TINY16_MMIO_APU_SFX_TABLE_LO) {
        value = apu->sfx.table_addr & 0xFF;
    } else if (addr == TINY16_MMIO_APU_SFX_COUNT) {
        value = apu->sfx.count;
    } else if (addr == TINY16_MMIO_APU_MUSIC_STATUS) {
        for (int i = 0; i < TINY16_APU_NUM_CHANNELS; i++) {
            if (apu->music.ch[i].enabled) value |= (1 << i);
        }
    }

    apu_unlock(apu);
    return value;
}

// =============================================================================
// Sample Generation
// =============================================================================

uint32_t tiny16_apu_samples_for_cycles(Tiny16APU* apu, uint32_t cpu_cycles, uint32_t cpu_hz) {
    if (cpu_hz == 0) return 0;

    apu_lock(apu);
    uint64_t add = (uint64_t)cpu_cycles * TINY16_APU_SAMPLE_RATE;
    uint64_t accum = apu->sample_accum + add;
    uint32_t frames = (uint32_t)(accum / cpu_hz);
    apu->sample_accum = accum - (uint64_t)frames * cpu_hz;
    apu_unlock(apu);

    return frames;
}

void tiny16_apu_generate_samples(Tiny16APU* apu, float* buffer, unsigned int frames) {
    apu_lock(apu);

    for (unsigned int i = 0; i < frames; i++) {
        // Per-frame updates
        apu->frame_samples++;
        if (apu->frame_samples >= SAMPLES_PER_FRAME) {
            apu->frame_samples = 0;
            update_sfx(apu);
            update_music(apu);
        }

        if (!apu->enabled) {
            buffer[i] = 0.0f;
            continue;
        }

        float mix = 0.0f;
        int active = 0;

        // Hardware channels
        for (int c = 0; c < TINY16_APU_NUM_CHANNELS; c++) {
            Tiny16APUChannel* ch = &apu->ch[c];

            // Length countdown
            if (ch->length_left > 0) {
                ch->length_left--;
                if (ch->length_left == 0) {
                    ch->enabled = false;
                    env_release(&ch->env);
                }
            }

            // Generate sample if active or releasing
            bool is_active = ch->enabled || ch->env.phase != TINY16_APU_ENV_OFF;
            if (is_active && ch->volume > 0) {
                float env = env_update(&ch->env);
                if (env > 0.0f) {
                    mix += generate_channel_sample(ch) * env;
                    active++;
                }
            }
        }

        // Music channels (virtual)
        for (int c = 0; c < TINY16_APU_NUM_CHANNELS; c++) {
            Tiny16MusicChannel* mch = &apu->music.ch[c];
            if (mch->enabled && mch->volume > 0) {
                mix += generate_music_sample(mch);
                active++;
            }
        }

        // Normalize and apply master volume
        if (active > 0) mix /= (float)active;
        mix *= (float)apu->master_volume / 15.0f;

        // Clamp
        if (mix > 1.0f) mix = 1.0f;
        if (mix < -1.0f) mix = -1.0f;

        buffer[i] = mix;
    }

    apu_unlock(apu);
}
