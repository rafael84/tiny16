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
}

void tiny16_apu_mmio_write(Tiny16APU* apu, uint16_t addr, uint8_t value) {
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
    }

    if (addr >= TINY16_MMIO_APU_WAVE_RAM && addr <= (TINY16_MMIO_APU_WAVE_RAM + 0x1F)) {
        apu->wave.wave[addr - TINY16_MMIO_APU_WAVE_RAM] = value & 0x0F;
    }
}

uint8_t tiny16_apu_mmio_read(Tiny16APU* apu, uint16_t addr) {
    switch (addr) {
    case TINY16_MMIO_APU_CTRL: return (apu->master_volume << 4) | (apu->enabled ? 0x01 : 0x00);

    case TINY16_MMIO_APU_STATUS: {
        uint8_t status = 0;
        if (apu->pulse1.enabled) status |= 0x01;
        if (apu->pulse2.enabled) status |= 0x02;
        if (apu->triangle.enabled) status |= 0x04;
        if (apu->noise.enabled) status |= 0x08;
        if (apu->wave.enabled) status |= 0x10;
        return status;
    }

    case TINY16_MMIO_APU_CH0_VOL: return (apu->pulse1.duty << 4) | apu->pulse1.volume;
    case TINY16_MMIO_APU_CH0_CTRL: return apu->pulse1.enabled ? 0x01 : 0x00;

    case TINY16_MMIO_APU_CH1_VOL: return (apu->pulse2.duty << 4) | apu->pulse2.volume;
    case TINY16_MMIO_APU_CH1_CTRL: return apu->pulse2.enabled ? 0x01 : 0x00;

    case TINY16_MMIO_APU_CH2_VOL: return apu->triangle.volume;
    case TINY16_MMIO_APU_CH2_CTRL: return apu->triangle.enabled ? 0x01 : 0x00;

    case TINY16_MMIO_APU_CH3_PERIOD: return apu->noise.period;
    case TINY16_MMIO_APU_CH3_VOL: return apu->noise.volume;
    case TINY16_MMIO_APU_CH3_CTRL:
        return (apu->noise.short_mode ? 0x04 : 0x00) | (apu->noise.enabled ? 0x01 : 0x00);

    case TINY16_MMIO_APU_CH0_ENV_AD: return (apu->pulse1.env.attack << 4) | apu->pulse1.env.decay;
    case TINY16_MMIO_APU_CH0_ENV_SR:
        return (apu->pulse1.env.sustain << 4) | apu->pulse1.env.release;
    case TINY16_MMIO_APU_CH1_ENV_AD: return (apu->pulse2.env.attack << 4) | apu->pulse2.env.decay;
    case TINY16_MMIO_APU_CH1_ENV_SR:
        return (apu->pulse2.env.sustain << 4) | apu->pulse2.env.release;
    case TINY16_MMIO_APU_CH2_ENV_AD:
        return (apu->triangle.env.attack << 4) | apu->triangle.env.decay;
    case TINY16_MMIO_APU_CH2_ENV_SR:
        return (apu->triangle.env.sustain << 4) | apu->triangle.env.release;
    case TINY16_MMIO_APU_CH3_ENV_AD: return (apu->noise.env.attack << 4) | apu->noise.env.decay;
    case TINY16_MMIO_APU_CH3_ENV_SR: return (apu->noise.env.sustain << 4) | apu->noise.env.release;

    case TINY16_MMIO_APU_CH0_SWEEP:
        return (apu->pulse1.sweep_rate << 4) | (apu->pulse1.sweep_down ? 0x08 : 0x00) |
               (apu->pulse1.sweep_shift & 0x07);
    case TINY16_MMIO_APU_CH1_SWEEP:
        return (apu->pulse2.sweep_rate << 4) | (apu->pulse2.sweep_down ? 0x08 : 0x00) |
               (apu->pulse2.sweep_shift & 0x07);
    case TINY16_MMIO_APU_CH0_LEN: return apu->pulse1.length;
    case TINY16_MMIO_APU_CH1_LEN: return apu->pulse2.length;
    case TINY16_MMIO_APU_CH2_LEN: return apu->triangle.length;
    case TINY16_MMIO_APU_CH3_LEN: return apu->noise.length;

    case TINY16_MMIO_APU_WAVE_FREQ_LO: return apu->wave.freq & 0xFF;
    case TINY16_MMIO_APU_WAVE_FREQ_HI: return (apu->wave.freq >> 8) & 0x07;
    case TINY16_MMIO_APU_WAVE_VOL: return apu->wave.volume;
    case TINY16_MMIO_APU_WAVE_CTRL: return apu->wave.enabled ? 0x01 : 0x00;
    case TINY16_MMIO_APU_WAVE_LEN: return apu->wave.length;
    case TINY16_MMIO_APU_WAVE_ENV_AD: return (apu->wave.env.attack << 4) | apu->wave.env.decay;
    case TINY16_MMIO_APU_WAVE_ENV_SR: return (apu->wave.env.sustain << 4) | apu->wave.env.release;
    }
    if (addr >= TINY16_MMIO_APU_WAVE_RAM && addr <= (TINY16_MMIO_APU_WAVE_RAM + 0x1F)) {
        return apu->wave.wave[addr - TINY16_MMIO_APU_WAVE_RAM] & 0x0F;
    }
    return 0;
}

uint32_t tiny16_apu_samples_for_cycles(Tiny16APU* apu, uint32_t cpu_cycles, uint32_t cpu_hz) {
    if (cpu_hz == 0) return 0;
    uint64_t add = (uint64_t)cpu_cycles * (uint64_t)TINY16_APU_SAMPLE_RATE;
    uint64_t accum = apu->sample_accum + add;
    uint32_t frames = (uint32_t)(accum / cpu_hz);
    apu->sample_accum = accum - (uint64_t)frames * cpu_hz;
    return frames;
}

void tiny16_apu_generate_samples(Tiny16APU* apu, float* buffer, unsigned int frames) {
    for (unsigned int i = 0; i < frames; i++) {
        float mix = 0.0f;
        int active_channels = 0;

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
}
