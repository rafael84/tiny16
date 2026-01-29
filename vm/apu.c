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

//==============================================================================
// Phase increment calculation
//==============================================================================

static inline uint32_t calc_phase_inc(uint16_t freq_value) {
    // freq_hz = 44100 / (2048 - freq_value)
    // phase_inc = freq_hz / sample_rate * 2^32
    // Simplified: phase_inc = 2^32 / (2048 - freq_value)
    uint32_t divisor = 2048 - freq_value;
    if (divisor == 0) divisor = 1;
    return (uint32_t)(4294967296.0 / divisor);
}

//==============================================================================
// Waveform generation
//==============================================================================

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

static inline float generate_noise(uint16_t* lfsr) {
    // Galois LFSR with taps at bits 0, 1, 3, 12
    uint16_t bit = ((*lfsr >> 0) ^ (*lfsr >> 1) ^ (*lfsr >> 3) ^ (*lfsr >> 12)) & 1;
    *lfsr = (*lfsr >> 1) | (bit << 15);
    // Convert to float -1.0 to 1.0
    return ((float)*lfsr / 32768.0f) - 1.0f;
}

//==============================================================================
// Reset
//==============================================================================

void tiny16_apu_reset(Tiny16APU* apu) {
    apu->enabled = false;
    apu->master_volume = 0;

    // Pulse 1
    apu->pulse1.freq = 0;
    apu->pulse1.volume = 0;
    apu->pulse1.duty = TINY16_APU_DUTY_50;
    apu->pulse1.enabled = false;
    apu->pulse1.trigger = false;
    apu->pulse1.phase = 0;
    apu->pulse1.phase_inc = 0;

    // Pulse 2
    apu->pulse2.freq = 0;
    apu->pulse2.volume = 0;
    apu->pulse2.duty = TINY16_APU_DUTY_50;
    apu->pulse2.enabled = false;
    apu->pulse2.trigger = false;
    apu->pulse2.phase = 0;
    apu->pulse2.phase_inc = 0;

    // Triangle
    apu->triangle.freq = 0;
    apu->triangle.volume = 0;
    apu->triangle.enabled = false;
    apu->triangle.trigger = false;
    apu->triangle.phase = 0;
    apu->triangle.phase_inc = 0;

    // Noise
    apu->noise.period = 0;
    apu->noise.volume = 0;
    apu->noise.enabled = false;
    apu->noise.trigger = false;
    apu->noise.short_mode = false;
    apu->noise.lfsr = LFSR_SEED;
    apu->noise.timer = 0;
}

//==============================================================================
// MMIO Write
//==============================================================================

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
    case TINY16_MMIO_APU_CH0_CTRL:
        apu->pulse1.enabled = (value & 0x01) != 0;
        if (value & 0x02) { // trigger
            apu->pulse1.phase = 0;
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
    case TINY16_MMIO_APU_CH1_CTRL:
        apu->pulse2.enabled = (value & 0x01) != 0;
        if (value & 0x02) {
            apu->pulse2.phase = 0;
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
    case TINY16_MMIO_APU_CH2_CTRL:
        apu->triangle.enabled = (value & 0x01) != 0;
        if (value & 0x02) {
            apu->triangle.phase = 0;
        }
        break;

    // Channel 3 - Noise
    case TINY16_MMIO_APU_CH3_PERIOD: apu->noise.period = value & 0x0F; break;
    case TINY16_MMIO_APU_CH3_VOL: apu->noise.volume = value & 0x0F; break;
    case TINY16_MMIO_APU_CH3_CTRL:
        apu->noise.enabled = (value & 0x01) != 0;
        if (value & 0x02) { // trigger - reseed LFSR
            apu->noise.lfsr = LFSR_SEED;
            apu->noise.timer = 0;
        }
        apu->noise.short_mode = (value & 0x04) != 0;
        break;
    }
}

//==============================================================================
// MMIO Read
//==============================================================================

uint8_t tiny16_apu_mmio_read(Tiny16APU* apu, uint16_t addr) {
    switch (addr) {
    case TINY16_MMIO_APU_CTRL: return (apu->master_volume << 4) | (apu->enabled ? 0x01 : 0x00);

    case TINY16_MMIO_APU_STATUS: {
        uint8_t status = 0;
        if (apu->pulse1.enabled) status |= 0x01;
        if (apu->pulse2.enabled) status |= 0x02;
        if (apu->triangle.enabled) status |= 0x04;
        if (apu->noise.enabled) status |= 0x08;
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
    }
    return 0;
}

//==============================================================================
// Sample generation (called from audio callback)
//==============================================================================

void tiny16_apu_generate_samples(Tiny16APU* apu, float* buffer, unsigned int frames) {
    for (unsigned int i = 0; i < frames; i++) {
        float mix = 0.0f;
        int active_channels = 0;

        if (!apu->enabled) {
            buffer[i] = 0.0f;
            continue;
        }

        // Pulse 1
        if (apu->pulse1.enabled && apu->pulse1.volume > 0) {
            float sample = generate_pulse(apu->pulse1.phase, apu->pulse1.duty);
            sample *= (float)apu->pulse1.volume / 15.0f;
            mix += sample;
            active_channels++;
            apu->pulse1.phase += apu->pulse1.phase_inc;
        }

        // Pulse 2
        if (apu->pulse2.enabled && apu->pulse2.volume > 0) {
            float sample = generate_pulse(apu->pulse2.phase, apu->pulse2.duty);
            sample *= (float)apu->pulse2.volume / 15.0f;
            mix += sample;
            active_channels++;
            apu->pulse2.phase += apu->pulse2.phase_inc;
        }

        // Triangle
        if (apu->triangle.enabled && apu->triangle.volume > 0) {
            float sample = generate_triangle(apu->triangle.phase);
            sample *= (float)apu->triangle.volume / 15.0f;
            mix += sample;
            active_channels++;
            apu->triangle.phase += apu->triangle.phase_inc;
        }

        // Noise
        if (apu->noise.enabled && apu->noise.volume > 0) {
            // Advance noise based on period
            apu->noise.timer++;
            if (apu->noise.timer >= noise_periods[apu->noise.period]) {
                apu->noise.timer = 0;
                // For short mode, use different taps (simplified)
                if (apu->noise.short_mode) {
                    uint16_t bit = ((apu->noise.lfsr >> 0) ^ (apu->noise.lfsr >> 6)) & 1;
                    apu->noise.lfsr = (apu->noise.lfsr >> 1) | (bit << 15);
                }
            }
            float sample = generate_noise(&apu->noise.lfsr);
            sample *= (float)apu->noise.volume / 15.0f;
            mix += sample;
            active_channels++;
        }

        // Normalize by active channel count
        if (active_channels > 0) {
            mix /= (float)active_channels;
        }

        // Apply master volume
        mix *= (float)apu->master_volume / 15.0f;

        // Clamp
        if (mix > 1.0f) mix = 1.0f;
        if (mix < -1.0f) mix = -1.0f;

        buffer[i] = mix;
    }
}
