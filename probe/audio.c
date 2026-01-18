#include <math.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Standard CD-quality sample rate (samples per second)
// Higher = better quality but more CPU. Common values: 22050, 44100, 48000
#define SAMPLE_RATE 44100

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Frequency ranges (in Hz)
#define MIN_FREQ  110.0f  // A2 note
#define MAX_FREQ  1760.0f // A6 note
#define FREQ_STEP 10.0f

// Note frequencies (4th octave, standard tuning A4 = 440Hz)
typedef enum {
    NOTE_C4 = 0,
    NOTE_CS4,
    NOTE_D4,
    NOTE_DS4,
    NOTE_E4,
    NOTE_F4,
    NOTE_FS4,
    NOTE_G4,
    NOTE_GS4,
    NOTE_A4,
    NOTE_AS4,
    NOTE_B4,
    NOTE_C5,
    NOTE_REST, // Silence
    NOTE_COUNT
} Note;

static const float note_frequencies[] = {
    261.63f, // C4
    277.18f, // C#4
    293.66f, // D4
    311.13f, // D#4
    329.63f, // E4
    349.23f, // F4
    369.99f, // F#4
    392.00f, // G4
    415.30f, // G#4
    440.00f, // A4
    466.16f, // A#4
    493.88f, // B4
    523.25f, // C5
    0.0f,    // REST
};

static const char* note_names[] = {
    "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4", "C5", "REST",
};

typedef struct {
    Note note;
    float duration;
} MelodyNote;

static const MelodyNote twinkle_melody[] = {
    {NOTE_C4, 0.3f}, {NOTE_C4, 0.3f}, {NOTE_G4, 0.3f}, {NOTE_G4, 0.3f}, {NOTE_A4, 0.3f},
    {NOTE_A4, 0.3f}, {NOTE_G4, 0.6f}, {NOTE_F4, 0.3f}, {NOTE_F4, 0.3f}, {NOTE_E4, 0.3f},
    {NOTE_E4, 0.3f}, {NOTE_D4, 0.3f}, {NOTE_D4, 0.3f}, {NOTE_C4, 0.6f},
};

#define MELODY_LENGTH (sizeof(twinkle_melody) / sizeof(MelodyNote))

typedef enum {
    WAVE_SINE = 0,
    WAVE_SQUARE,
    WAVE_TRIANGLE,
    WAVE_SAWTOOTH,
    WAVE_NOISE,
    WAVE_COUNT
} WaveformType;

typedef struct {
    bool enabled;
    WaveformType type;
    float frequency;     // Frequency in Hz (cycles per second)
    float volume;        // Volume 0.0 to 1.0
    float duty_cycle;    // Duty cycle 0.0 to 1.0 (for square wave)
    uint32_t phase;      // Phase accumulator (32-bit for precision)
    uint32_t phase_inc;  // Phase increment per sample
    uint16_t noise_lfsr; // Linear Feedback Shift Register for noise
    const char* name;
} AudioChannel;

typedef enum {
    SFX_NONE = 0,
    SFX_EXPLOSION,
    SFX_LASER,
    SFX_HIT,
    SFX_DRUM,
} SoundEffect;

typedef struct {
    SoundEffect type;
    float timer;
    float duration;
    float phase;
} SFXState;

typedef struct {
    AudioChannel channels[WAVE_COUNT];
    bool playing;
    float master_volume;

    bool melody_playing;
    int melody_index;
    float melody_timer;
    int melody_waveform; // Which waveform to use for melody

    SFXState sfx;
} AudioState;

static AudioState g_audio = {0};

//==============================================================================
// WAVEFORM GENERATION
//==============================================================================

/*
 * CONCEPT: Phase Accumulator
 *
 * Instead of tracking time directly, we use a 32-bit "phase" counter that
 * wraps around. This gives us smooth frequency control without floating-point
 * precision issues.
 *
 * - Phase: 0x00000000 to 0xFFFFFFFF represents one complete cycle (0° to 360°)
 * - Phase increment = (frequency / sample_rate) * 2^32
 * - Each sample, we add phase_inc to phase (with wraparound)
 * - The high bits of phase tell us where we are in the waveform
 */

static inline uint32_t calc_phase_increment(float freq) {
    // Formula: (freq / sample_rate) * 2^32
    // This tells us how much to advance the phase per sample
    return (uint32_t)((freq / (float)SAMPLE_RATE) * 4294967296.0f);
}

// Generate one sample of a sine wave
// CONCEPT: Sine wave is the purest tone, contains only the fundamental frequency
static inline float generate_sine(uint32_t phase) {
    // Convert 32-bit phase (0-0xFFFFFFFF) to angle (0-2π)
    float angle = ((float)phase / 4294967296.0f) * 2.0f * PI;
    return sinf(angle);
}

// Generate one sample of a square wave
// CONCEPT: Square wave toggles between -1 and +1, contains odd harmonics (1, 3, 5, 7...)
// Creates a "hollow" or "buzzy" sound, good for leads and bass
static inline float generate_square(uint32_t phase, float duty) {
    // Duty cycle: percentage of time the wave is "high"
    // 0.5 = 50% duty (equal high/low time) - classic square wave
    // 0.25 = 25% duty (short pulse) - thinner sound
    // 0.125 = 12.5% duty - very thin, NES-style pulse

    // Check if we're in the "high" portion of the cycle
    float phase_normalized = (float)phase / 4294967296.0f;
    return (phase_normalized < duty) ? 1.0f : -1.0f;
}

// Generate one sample of a triangle wave
// CONCEPT: Triangle wave has softer harmonics than square (odd harmonics 1, 3, 5... with rapid
// falloff) Sounds mellower, good for bass and sub-bass
static inline float generate_triangle(uint32_t phase) {
    // Convert phase to a value from 0 to 1
    float t = (float)phase / 4294967296.0f;

    // Create triangle shape:
    // 0.00 -> 0.25: ramp up from 0 to 1
    // 0.25 -> 0.75: ramp down from 1 to -1
    // 0.75 -> 1.00: ramp up from -1 to 0
    if (t < 0.25f) {
        return t * 4.0f; // 0 to 1
    } else if (t < 0.75f) {
        return 2.0f - (t * 4.0f); // 1 to -1
    } else {
        return -4.0f + (t * 4.0f); // -1 to 0
    }
}

// Generate one sample of a sawtooth wave
// CONCEPT: Sawtooth contains ALL harmonics (1, 2, 3, 4...) with linear falloff
// Brightest, most "buzzy" sound - good for leads and aggressive sounds
static inline float generate_sawtooth(uint32_t phase) {
    // Linear ramp from -1 to 1
    float t = (float)phase / 4294967296.0f;
    return (t * 2.0f) - 1.0f;
}

// Generate one sample of noise
// CONCEPT: White noise contains all frequencies equally, no pitch
// Used for percussion, wind, explosions, etc.
static inline float generate_noise(uint16_t* lfsr) {
    // Linear Feedback Shift Register (LFSR) generates pseudo-random sequence
    // This is how classic game consoles generated noise

    // Galois LFSR with taps at bits 0 and 1 (polynomial: x^16 + x^14 + x^13 + x^11)
    // More info: https://en.wikipedia.org/wiki/Linear-feedback_shift_register

    uint16_t bit = ((*lfsr >> 0) ^ (*lfsr >> 1) ^ (*lfsr >> 3) ^ (*lfsr >> 12)) & 1;
    *lfsr = (*lfsr >> 1) | (bit << 15);

    // Convert to float -1.0 to 1.0
    return ((float)*lfsr / 32768.0f) - 1.0f;
}

//==============================================================================
// MIXING & AUDIO CALLBACK
//==============================================================================

/*
 * CONCEPT: Audio Callback
 *
 * The audio system calls this function periodically to fill a buffer with samples.
 * The callback runs on a separate audio thread, so keep it fast and simple.
 *
 * Parameters:
 * - buffer: Array to fill with audio samples
 * - frames: Number of samples to generate
 *
 * Each "frame" is one sample point in time. For mono audio, frame = sample.
 * For stereo, one frame = two samples (left + right).
 */

static void audio_callback(void* buffer, unsigned int frames) {
    float* out = (float*)buffer;

    for (unsigned int i = 0; i < frames; i++) {
        float mix = 0.0f;
        int active_channels = 0;

        // Mix all enabled channels
        for (int ch = 0; ch < WAVE_COUNT; ch++) {
            AudioChannel* channel = &g_audio.channels[ch];
            if (!channel->enabled) continue;

            float sample = 0.0f;

            // Generate sample based on waveform type
            switch (channel->type) {
            case WAVE_SINE:
                sample = generate_sine(channel->phase);
                break;

            case WAVE_SQUARE:
                sample = generate_square(channel->phase, channel->duty_cycle);
                break;

            case WAVE_TRIANGLE:
                sample = generate_triangle(channel->phase);
                break;

            case WAVE_SAWTOOTH:
                sample = generate_sawtooth(channel->phase);
                break;

            case WAVE_NOISE:
                sample = generate_noise(&channel->noise_lfsr);
                break;

            default:
                sample = 0.0f;
            }

            // Apply channel volume
            sample *= channel->volume;

            // Add to mix
            mix += sample;
            active_channels++;

            // Advance phase for next sample
            channel->phase += channel->phase_inc;
            // Phase automatically wraps around at 0xFFFFFFFF -> 0x00000000
        }

        // CONCEPT: Mixing
        // When mixing multiple channels, we average them to prevent clipping
        // Without averaging, multiple loud channels could exceed [-1.0, 1.0] range
        if (active_channels > 0) {
            mix /= (float)active_channels;
        }

        // Apply master volume
        mix *= g_audio.master_volume;

        // CONCEPT: Clamping
        // Ensure final output stays in valid range [-1.0, 1.0]
        // Values outside this range cause distortion (clipping)
        if (mix > 1.0f) mix = 1.0f;
        if (mix < -1.0f) mix = -1.0f;

        // Write to output buffer
        out[i] = g_audio.playing ? mix : 0.0f;
    }
}

//==============================================================================
// INITIALIZATION & CONTROL
//==============================================================================

static void init_audio_channels(void) {
    const char* names[] = {"Sine", "Square", "Triangle", "Sawtooth", "Noise"};

    for (int i = 0; i < WAVE_COUNT; i++) {
        g_audio.channels[i] =
            (AudioChannel){.enabled = false,
                           .type = i,
                           .frequency = 440.0f, // A4 note
                           .volume = 0.5f,
                           .duty_cycle = 0.5f, // 50% duty for square wave
                           .phase = 0,
                           .phase_inc = calc_phase_increment(440.0f),
                           .noise_lfsr = 0xACE1, // Initial LFSR seed (any non-zero value)
                           .name = names[i]};
    }

    g_audio.playing = true;
    g_audio.master_volume = 0.7f;
    g_audio.melody_waveform = WAVE_SQUARE; // Default to square wave
}

static void update_channel_frequency(int channel_idx, float freq) {
    if (channel_idx < 0 || channel_idx >= WAVE_COUNT) return;

    AudioChannel* ch = &g_audio.channels[channel_idx];
    ch->frequency = freq;
    ch->phase_inc = calc_phase_increment(freq);
}

static void start_melody(void) {
    g_audio.melody_playing = true;
    g_audio.melody_index = 0;
    g_audio.melody_timer = 0.0f;

    // Enable selected waveform channel for melody
    g_audio.channels[g_audio.melody_waveform].enabled = true;
    g_audio.channels[g_audio.melody_waveform].volume = 0.6f;
}

static void stop_melody(void) {
    g_audio.melody_playing = false;
    g_audio.channels[g_audio.melody_waveform].enabled = false;
}

static void update_melody(float delta_time) {
    if (!g_audio.melody_playing) return;

    g_audio.melody_timer += delta_time;

    const MelodyNote* current = &twinkle_melody[g_audio.melody_index];

    // Check if it's time to move to next note
    if (g_audio.melody_timer >= current->duration) {
        g_audio.melody_timer = 0.0f;
        g_audio.melody_index++;

        // Loop or stop at end
        if (g_audio.melody_index >= (int)MELODY_LENGTH) {
            g_audio.melody_index = 0; // Loop
            // Or uncomment to stop: stop_melody(); return;
        }

        // Set frequency for new note
        const MelodyNote* next = &twinkle_melody[g_audio.melody_index];
        if (next->note == NOTE_REST) {
            g_audio.channels[g_audio.melody_waveform].volume = 0.0f;
        } else {
            g_audio.channels[g_audio.melody_waveform].volume = 0.6f;
            update_channel_frequency(g_audio.melody_waveform, note_frequencies[next->note]);
        }
    }
}

static void trigger_sfx(SoundEffect type) {
    g_audio.sfx.type = type;
    g_audio.sfx.timer = 0.0f;
    g_audio.sfx.phase = 0.0f;

    switch (type) {
    case SFX_EXPLOSION:
        g_audio.sfx.duration = 0.8f; // Explosion lasts 0.8s
        g_audio.channels[WAVE_NOISE].enabled = true;
        g_audio.channels[WAVE_NOISE].volume = 0.8f;
        g_audio.channels[WAVE_NOISE].frequency = 800.0f;
        update_channel_frequency(WAVE_NOISE, 800.0f);
        break;

    case SFX_LASER:
        g_audio.sfx.duration = 0.3f; // Laser lasts 0.3s
        g_audio.channels[WAVE_SQUARE].enabled = true;
        g_audio.channels[WAVE_SQUARE].volume = 0.6f;
        g_audio.channels[WAVE_SQUARE].frequency = 1200.0f;
        update_channel_frequency(WAVE_SQUARE, 1200.0f);
        break;

    case SFX_HIT:
        g_audio.sfx.duration = 0.15f; // Hit is very short
        g_audio.channels[WAVE_NOISE].enabled = true;
        g_audio.channels[WAVE_NOISE].volume = 0.9f;
        g_audio.channels[WAVE_NOISE].frequency = 400.0f;
        update_channel_frequency(WAVE_NOISE, 400.0f);
        break;

    case SFX_DRUM:
        g_audio.sfx.duration = 0.1f; // Drum is punchy and short
        g_audio.channels[WAVE_NOISE].enabled = true;
        g_audio.channels[WAVE_NOISE].volume = 0.7f;
        g_audio.channels[WAVE_NOISE].frequency = 200.0f;
        update_channel_frequency(WAVE_NOISE, 200.0f);
        // Also add a low bass thump
        g_audio.channels[WAVE_TRIANGLE].enabled = true;
        g_audio.channels[WAVE_TRIANGLE].volume = 0.8f;
        update_channel_frequency(WAVE_TRIANGLE, 80.0f);
        break;

    default:
        break;
    }
}

static void update_sfx(float delta_time) {
    if (g_audio.sfx.type == SFX_NONE) return;

    g_audio.sfx.timer += delta_time;
    float progress = g_audio.sfx.timer / g_audio.sfx.duration;

    if (progress >= 1.0f) {
        // Sound effect finished
        g_audio.sfx.type = SFX_NONE;
        if (!g_audio.melody_playing) {
            g_audio.channels[WAVE_NOISE].enabled = false;
            g_audio.channels[WAVE_SQUARE].enabled = false;
            g_audio.channels[WAVE_TRIANGLE].enabled = false;
        }
        return;
    }

    // Update sound based on type
    switch (g_audio.sfx.type) {
    case SFX_EXPLOSION:
        // Fade out volume and sweep frequency down
        g_audio.channels[WAVE_NOISE].volume = 0.8f * (1.0f - progress);
        update_channel_frequency(WAVE_NOISE, 800.0f * (1.0f - progress * 0.8f));
        break;

    case SFX_LASER:
        // Sweep frequency down quickly
        g_audio.channels[WAVE_SQUARE].volume = 0.6f * (1.0f - progress * 0.5f);
        update_channel_frequency(WAVE_SQUARE, 1200.0f - (progress * 900.0f));
        break;

    case SFX_HIT:
        // Fast fade out
        g_audio.channels[WAVE_NOISE].volume = 0.9f * (1.0f - progress * progress);
        break;

    case SFX_DRUM: {
        // Quick fade for both noise and bass
        float drum_vol = 1.0f - (progress * progress * 4.0f);
        if (drum_vol < 0.0f) drum_vol = 0.0f;
        g_audio.channels[WAVE_NOISE].volume = 0.7f * drum_vol;
        g_audio.channels[WAVE_TRIANGLE].volume = 0.8f * drum_vol;
        break;
    }

    default:
        break;
    }
}

//==============================================================================
// VISUALIZATION
//==============================================================================

// Draw a simple waveform visualization
static void draw_waveform(AudioChannel* ch, int x, int y, int width, int height) {
    if (!ch->enabled) {
        DrawRectangleLines(x, y, width, height, DARKGRAY);
        return;
    }

    DrawRectangle(x, y, width, height, (Color){20, 20, 30, 255});
    DrawRectangleLines(x, y, width, height, BLUE);

    // Draw waveform
    int center_y = y + height / 2;
    uint32_t phase = 0;
    uint32_t phase_inc = (uint32_t)((4294967296.0 / width) * 2); // 2 cycles across width
    uint16_t noise_lfsr = ch->noise_lfsr;

    for (int i = 0; i < width; i++) {
        float sample = 0.0f;

        switch (ch->type) {
        case WAVE_SINE:
            sample = generate_sine(phase);
            break;
        case WAVE_SQUARE:
            sample = generate_square(phase, ch->duty_cycle);
            break;
        case WAVE_TRIANGLE:
            sample = generate_triangle(phase);
            break;
        case WAVE_SAWTOOTH:
            sample = generate_sawtooth(phase);
            break;
        case WAVE_NOISE:
            sample = generate_noise(&noise_lfsr);
            break;
        default:
            break;
        }

        int sample_y = center_y - (int)(sample * (height / 2 - 5));
        DrawPixel(x + i, sample_y, GREEN);

        phase += phase_inc;
    }

    // Draw center line
    DrawLine(x, center_y, x + width, center_y, DARKGRAY);
}

int main(void) {
    const int screenWidth = 1350;
    const int screenHeight = 1050;

    InitWindow(screenWidth, screenHeight, "Audio Concepts Reference - tiny16 APU Development");
    SetTargetFPS(60);

    // Initialize audio system
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        printf("Failed to initialize audio device\n");
        return 1;
    }

    // Create audio stream
    // Parameters: sample_rate, bits_per_sample (32 = float), channels (1 = mono)
    SetAudioStreamBufferSizeDefault(4096);
    AudioStream stream = LoadAudioStream(SAMPLE_RATE, 32, 1);

    SetAudioStreamCallback(stream, audio_callback);
    PlayAudioStream(stream);
    init_audio_channels();

    g_audio.channels[0].enabled = true;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ONE)) g_audio.channels[0].enabled = !g_audio.channels[0].enabled;
        if (IsKeyPressed(KEY_TWO)) g_audio.channels[1].enabled = !g_audio.channels[1].enabled;
        if (IsKeyPressed(KEY_THREE)) g_audio.channels[2].enabled = !g_audio.channels[2].enabled;
        if (IsKeyPressed(KEY_FOUR)) g_audio.channels[3].enabled = !g_audio.channels[3].enabled;
        if (IsKeyPressed(KEY_FIVE)) g_audio.channels[4].enabled = !g_audio.channels[4].enabled;

        if (IsKeyPressed(KEY_SIX)) {
            bool any_enabled = false;
            for (int i = 0; i < WAVE_COUNT; i++) {
                if (g_audio.channels[i].enabled) {
                    any_enabled = true;
                    break;
                }
            }
            for (int i = 0; i < WAVE_COUNT; i++) {
                g_audio.channels[i].enabled = !any_enabled;
            }
        }

        // Frequency control
        if (IsKeyDown(KEY_UP)) {
            for (int i = 0; i < WAVE_COUNT; i++) {
                if (g_audio.channels[i].enabled) {
                    float new_freq = g_audio.channels[i].frequency + FREQ_STEP;
                    if (new_freq <= MAX_FREQ) {
                        update_channel_frequency(i, new_freq);
                    }
                }
            }
        }
        if (IsKeyDown(KEY_DOWN)) {
            for (int i = 0; i < WAVE_COUNT; i++) {
                if (g_audio.channels[i].enabled) {
                    float new_freq = g_audio.channels[i].frequency - FREQ_STEP;
                    if (new_freq >= MIN_FREQ) {
                        update_channel_frequency(i, new_freq);
                    }
                }
            }
        }

        // Duty cycle control (square wave only)
        if (IsKeyDown(KEY_RIGHT)) {
            g_audio.channels[WAVE_SQUARE].duty_cycle += 0.01f;
            if (g_audio.channels[WAVE_SQUARE].duty_cycle > 1.0f) {
                g_audio.channels[WAVE_SQUARE].duty_cycle = 1.0f;
            }
        }
        if (IsKeyDown(KEY_LEFT)) {
            g_audio.channels[WAVE_SQUARE].duty_cycle -= 0.01f;
            if (g_audio.channels[WAVE_SQUARE].duty_cycle < 0.0f) {
                g_audio.channels[WAVE_SQUARE].duty_cycle = 0.0f;
            }
        }

        if (IsKeyDown(KEY_Q)) {
            g_audio.master_volume += 0.01f;
            if (g_audio.master_volume > 1.0f) g_audio.master_volume = 1.0f;
        }
        if (IsKeyDown(KEY_A)) {
            g_audio.master_volume -= 0.01f;
            if (g_audio.master_volume < 0.0f) g_audio.master_volume = 0.0f;
        }

        if (IsKeyPressed(KEY_SPACE)) {
            g_audio.playing = !g_audio.playing;
        }

        if (IsKeyPressed(KEY_M)) {
            if (g_audio.melody_playing) {
                stop_melody();
            } else {
                start_melody();
            }
        }

        if (IsKeyPressed(KEY_N)) {
            // Cycle through waveforms (skip noise for melody)
            bool was_playing = g_audio.melody_playing;
            if (was_playing) {
                stop_melody();
            }

            g_audio.melody_waveform = (g_audio.melody_waveform + 1) % (WAVE_COUNT - 1);

            if (was_playing) {
                start_melody();
            }
        }

        if (IsKeyPressed(KEY_E)) {
            trigger_sfx(SFX_EXPLOSION);
        }
        if (IsKeyPressed(KEY_L)) {
            trigger_sfx(SFX_LASER);
        }
        if (IsKeyPressed(KEY_H)) {
            trigger_sfx(SFX_HIT);
        }
        if (IsKeyPressed(KEY_K)) {
            trigger_sfx(SFX_DRUM);
        }

        if (g_audio.melody_playing) {
            update_melody(GetFrameTime());
        }

        update_sfx(GetFrameTime());

        //======================================================================
        // DRAWING
        //======================================================================

        BeginDrawing();
        ClearBackground((Color){15, 15, 20, 255});

        DrawText("AUDIO CONCEPTS REFERENCE", 30, 30, 45, WHITE);
        DrawText("Reference for tiny16 APU Development", 30, 82, 24, GRAY);

        int info_y = 135;
        DrawText("Controls:", 30, info_y, 27, YELLOW);
        DrawText("1-5: Toggle channels | 6: Toggle all", 30, info_y + 38, 21, LIGHTGRAY);
        DrawText("UP/DOWN: Change frequency", 30, info_y + 68, 21, LIGHTGRAY);
        DrawText("LEFT/RIGHT: Duty cycle (square only)", 30, info_y + 98, 21, LIGHTGRAY);
        DrawText("Q/A: Master volume up/down", 30, info_y + 128, 21, LIGHTGRAY);
        DrawText("SPACE: Pause/Resume | M: Play melody | N: Change melody waveform", 30,
                 info_y + 158, 21, LIGHTGRAY);

        DrawText("Sound Effects (using noise):", 30, info_y + 198, 21, YELLOW);
        DrawText("E: Explosion | L: Laser | H: Hit | K: Drum", 30, info_y + 228, 18, LIGHTGRAY);

        DrawText(g_audio.playing ? "PLAYING" : "PAUSED", 30, info_y + 268, 27,
                 g_audio.playing ? GREEN : RED);
        DrawText(TextFormat("Master Volume: %.0f%%", g_audio.master_volume * 100), 225,
                 info_y + 268, 27, WHITE);

        if (g_audio.melody_playing) {
            const MelodyNote* current = &twinkle_melody[g_audio.melody_index];
            const char* waveform_name = g_audio.channels[g_audio.melody_waveform].name;
            const char* melody_text =
                TextFormat("MELODY [%s]: %s (note %d/%d)", waveform_name, note_names[current->note],
                           g_audio.melody_index + 1, (int)MELODY_LENGTH);
            int text_width = MeasureText(melody_text, 27);
            DrawText(melody_text, screenWidth - text_width - 30, 30, 27, ORANGE);
        } else {
            // Show current melody waveform even when not playing
            const char* waveform_name = g_audio.channels[g_audio.melody_waveform].name;
            const char* melody_text =
                TextFormat("Melody waveform: %s (press N to change)", waveform_name);
            int text_width = MeasureText(melody_text, 21);
            DrawText(melody_text, screenWidth - text_width - 30, 30, 21, DARKGRAY);
        }

        // Show active sound effect
        if (g_audio.sfx.type != SFX_NONE) {
            const char* sfx_names[] = {"", "EXPLOSION", "LASER", "HIT", "DRUM"};
            const char* sfx_text = TextFormat("SFX: %s", sfx_names[g_audio.sfx.type]);
            int text_width = MeasureText(sfx_text, 24);
            float progress = g_audio.sfx.timer / g_audio.sfx.duration;
            DrawText(sfx_text, screenWidth - text_width - 30, 70, 24, RED);
            // Progress bar
            int bar_width = 200;
            DrawRectangle(screenWidth - bar_width - 30, 100, (int)(bar_width * progress), 8, RED);
            DrawRectangleLines(screenWidth - bar_width - 30, 100, bar_width, 8, DARKGRAY);
        }

        int ch_y = 440;
        int ch_spacing = 120;

        for (int i = 0; i < WAVE_COUNT; i++) {
            AudioChannel* ch = &g_audio.channels[i];
            int y = ch_y + (i * ch_spacing);

            Color label_color = ch->enabled ? GREEN : GRAY;
            DrawText(TextFormat("[%d] %s", i + 1, ch->name), 30, y, 27, label_color);
            DrawText(ch->enabled ? "ON" : "OFF", 225, y, 27, label_color);

            if (ch->enabled) {
                DrawText(TextFormat("%.0f Hz", ch->frequency), 330, y, 24, WHITE);
                DrawText(TextFormat("Vol: %.0f%%", ch->volume * 100), 480, y, 24, WHITE);

                if (ch->type == WAVE_SQUARE) {
                    DrawText(TextFormat("Duty: %.0f%%", ch->duty_cycle * 100), 630, y, 24, WHITE);
                }
            }

            draw_waveform(ch, 825, y - 15, 480, 75);
        }

        DrawText("CONCEPTS:", 30, 975, 21, YELLOW);
        DrawText("Phase Accumulator: Smooth frequency control | "
                 "Mixing: Average channels to prevent clipping | "
                 "LFSR: Pseudo-random noise generation",
                 180, 975, 18, LIGHTGRAY);
        DrawText("Sample Rate: 44.1kHz | Buffer: 4096 samples | "
                 "Format: 32-bit float mono | Press M for melody (note frequencies demo)",
                 180, 1005, 18, DARKGRAY);

        EndDrawing();
    }

    UnloadAudioStream(stream);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
