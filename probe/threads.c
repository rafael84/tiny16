/*
 * Threading Concepts for tiny16 APU Development
 *
 * This program demonstrates the threading model used by raylib's audio system.
 * It builds up from simple concepts to the actual pattern you'll use.
 *
 * Build: cc -o threads probe/threads.c -pthread
 * Run: ./threads
 *
 * Press ENTER to cycle through examples
 * Press 'q' + ENTER to quit
 */

#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

//==============================================================================
// EXAMPLE 1: Basic Thread Creation
//==============================================================================

void* simple_thread_function(void* arg) {
    const char* message = (const char*)arg;

    for (int i = 0; i < 5; i++) {
        printf("  [Thread] %s (count: %d)\n", message, i);
        usleep(500000); // Sleep 0.5 seconds
    }

    printf("  [Thread] Finished!\n");
    return NULL;
}

void example1_basic_thread(void) {
    printf("\n=== EXAMPLE 1: Basic Thread Creation ===\n");
    printf("Creates a separate thread that runs alongside main program.\n\n");

    pthread_t thread;
    const char* message = "Hello from separate thread!";

    // Create thread
    printf("[Main] Creating thread...\n");
    pthread_create(&thread, NULL, simple_thread_function, (void*)message);

    // Main thread continues running
    for (int i = 0; i < 5; i++) {
        printf("[Main] Doing work in main thread (count: %d)\n", i);
        usleep(300000); // Sleep 0.3 seconds
    }

    // Wait for thread to finish
    printf("[Main] Waiting for thread to finish...\n");
    pthread_join(thread, NULL);
    printf("[Main] Thread finished, exiting.\n");

    printf("\nKey concept: Two threads run SIMULTANEOUSLY\n");
}

//==============================================================================
// EXAMPLE 2: Shared Data (The Problem)
//==============================================================================

int shared_counter = 0; // Both threads will access this

void* incrementing_thread(void* arg) {
    int increments = *(int*)arg;

    for (int i = 0; i < increments; i++) {
        shared_counter++; // DANGER: Race condition!
        usleep(1000);     // Simulate some work
    }

    return NULL;
}

void example2_shared_data_problem(void) {
    printf("\n=== EXAMPLE 2: Shared Data Problem ===\n");
    printf("Shows what happens when threads access shared data without protection.\n\n");

    shared_counter = 0;
    int increments = 100;

    pthread_t thread1, thread2;

    printf("[Main] Starting two threads, each incrementing counter 100 times...\n");
    pthread_create(&thread1, NULL, incrementing_thread, &increments);
    pthread_create(&thread2, NULL, incrementing_thread, &increments);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("[Main] Expected result: 200\n");
    printf("[Main] Actual result: %d\n", shared_counter);

    if (shared_counter != 200) {
        printf("\nBUG! Race condition caused lost updates!\n");
    } else {
        printf("\nLucky! This time it worked, but it's not guaranteed.\n");
    }

    printf("\nKey concept: Simultaneous access to shared data = problems\n");
}

//==============================================================================
// EXAMPLE 3: Lock-Free Pattern (Producer-Consumer)
//==============================================================================

typedef struct {
    int frequency;
    int volume;
    bool enabled;
} AudioState;

AudioState audio_state = {440, 10, false};

void* audio_consumer_thread(void* arg) {
    (void)arg; // Unused
    printf("  [Audio] Starting audio thread...\n");

    for (int i = 0; i < 20; i++) {
        // Read current state (consumer)
        int freq = audio_state.frequency;
        int vol = audio_state.volume;
        bool en = audio_state.enabled;

        if (en) {
            printf("  [Audio] Playing: %d Hz at volume %d\n", freq, vol);
        } else {
            printf("  [Audio] Silence (disabled)\n");
        }

        usleep(200000); // Simulate generating audio samples
    }

    printf("  [Audio] Audio thread finished\n");
    return NULL;
}

void example3_lockfree_pattern(void) {
    printf("\n=== EXAMPLE 3: Lock-Free Producer-Consumer ===\n");
    printf("Main thread writes state, audio thread reads state.\n");
    printf("This is the pattern raylib uses!\n\n");

    audio_state.frequency = 440;
    audio_state.volume = 10;
    audio_state.enabled = false;

    pthread_t audio_thread;
    pthread_create(&audio_thread, NULL, audio_consumer_thread, NULL);

    usleep(400000);
    printf("[Main] Enabling audio...\n");
    audio_state.enabled = true; // Producer writes

    usleep(800000);
    printf("[Main] Changing frequency to 880 Hz...\n");
    audio_state.frequency = 880; // Producer writes

    usleep(800000);
    printf("[Main] Changing volume to 15...\n");
    audio_state.volume = 15; // Producer writes

    usleep(800000);
    printf("[Main] Disabling audio...\n");
    audio_state.enabled = false; // Producer writes

    pthread_join(audio_thread, NULL);

    printf("\nKey concept: One thread WRITES, other thread READS\n");
    printf("No locks needed for simple state reads!\n");
}

//==============================================================================
// EXAMPLE 4: Audio Callback Pattern (Raylib Style)
//==============================================================================

typedef struct {
    float phase;
    float frequency;
    float volume;
    bool playing;
} SimpleOscillator;

SimpleOscillator osc = {0.0f, 440.0f, 0.5f, false};

// This simulates raylib's audio callback
void audio_callback_simulation(float* buffer, int frames) {
    for (int i = 0; i < frames; i++) {
        if (osc.playing) {
            // Generate simple sine wave sample
            float sample = osc.volume * sinf(osc.phase * 2.0f * 3.14159f);
            buffer[i] = sample;

            // Advance phase
            osc.phase += osc.frequency / 44100.0f;
            if (osc.phase >= 1.0f) osc.phase -= 1.0f;
        } else {
            buffer[i] = 0.0f; // Silence
        }
    }
}

void* audio_callback_thread(void* arg) {
    (void)arg; // Unused
    printf("  [Audio] Audio callback thread started\n");

    float buffer[128]; // Small audio buffer
    int callback_count = 0;

    for (int i = 0; i < 50; i++) {
        // Simulate audio system requesting samples
        audio_callback_simulation(buffer, 128);
        callback_count++;

        // Show what we generated (occasionally)
        if (callback_count % 10 == 0) {
            if (osc.playing) {
                printf("  [Audio] Callback #%d: Playing %.0f Hz\n", callback_count, osc.frequency);
            } else {
                printf("  [Audio] Callback #%d: Silence\n", callback_count);
            }
        }

        usleep(50000); // Simulate time between callbacks (50ms = ~50 FPS for this demo)
    }

    printf("  [Audio] Audio callback thread finished\n");
    return NULL;
}

void example4_callback_pattern(void) {
    printf("\n=== EXAMPLE 4: Audio Callback Pattern ===\n");
    printf("This is EXACTLY how raylib audio works!\n");
    printf("Audio thread calls your function repeatedly to generate samples.\n\n");

    osc.phase = 0.0f;
    osc.frequency = 440.0f;
    osc.volume = 0.3f;
    osc.playing = false;

    pthread_t audio_thread;
    pthread_create(&audio_thread, NULL, audio_callback_thread, NULL);

    usleep(300000);
    printf("[Main] Starting playback at 440 Hz...\n");
    osc.playing = true;

    usleep(800000);
    printf("[Main] Changing to 880 Hz...\n");
    osc.frequency = 880.0f;

    usleep(800000);
    printf("[Main] Changing to 220 Hz...\n");
    osc.frequency = 220.0f;

    usleep(500000);
    printf("[Main] Stopping playback...\n");
    osc.playing = false;

    pthread_join(audio_thread, NULL);

    printf("\nKey concept: Audio thread calls YOUR function repeatedly\n");
    printf("Your function reads state and generates samples\n");
    printf("Main thread updates state whenever it wants\n");
}

//==============================================================================
// EXAMPLE 5: APU-Style Pattern (What You'll Actually Build)
//==============================================================================

typedef struct {
    uint16_t frequency_reg; // 11-bit frequency value
    uint8_t control;        // Enable + volume
    uint32_t phase_acc;     // Internal state (not visible to CPU)
} APUChannel;

typedef struct {
    APUChannel channels[4];
    uint8_t master_ctrl;
} APU;

APU apu = {0};

// This is what your APU audio callback will look like
void apu_audio_callback(float* buffer, int frames) {
    for (int i = 0; i < frames; i++) {
        float mix = 0.0f;
        int active = 0;

        // Mix all enabled channels
        for (int ch = 0; ch < 4; ch++) {
            APUChannel* channel = &apu.channels[ch];

            // Read control register (CPU writes this)
            bool enabled = (channel->control & 0x80) != 0;
            int volume = (channel->control & 0x0F);

            if (enabled && volume > 0) {
                // Read frequency register (CPU writes this)
                float freq = channel->frequency_reg / 10.0f;

                // Generate sample (simplified)
                float sample = (channel->phase_acc & 0x8000) ? 1.0f : -1.0f;
                sample *= volume / 15.0f;
                mix += sample;
                active++;

                // Update internal state (CPU never touches this)
                channel->phase_acc += (uint16_t)(freq * 10);
            }
        }

        // Average to prevent clipping
        if (active > 0) {
            mix /= active;
        }

        // Apply master volume
        int master_vol = (apu.master_ctrl & 0xF0) >> 4;
        mix *= master_vol / 15.0f;

        buffer[i] = mix;
    }
}

void* apu_audio_thread(void* arg) {
    (void)arg; // Unused
    printf("  [APU] APU audio thread started\n");

    float buffer[256];

    for (int i = 0; i < 30; i++) {
        apu_audio_callback(buffer, 256);

        // Show status occasionally
        if (i % 10 == 0) {
            printf("  [APU] Generating samples... ");
            for (int ch = 0; ch < 4; ch++) {
                if (apu.channels[ch].control & 0x80) {
                    printf("CH%d:ON ", ch);
                } else {
                    printf("CH%d:off ", ch);
                }
            }
            printf("\n");
        }

        usleep(50000); // Simulate callback rate
    }

    printf("  [APU] APU audio thread finished\n");
    return NULL;
}

void example5_apu_pattern(void) {
    printf("\n=== EXAMPLE 5: APU Pattern (tiny16 Implementation) ===\n");
    printf("Shows how your actual APU will work.\n\n");

    // Initialize APU
    memset(&apu, 0, sizeof(apu));

    pthread_t audio_thread;
    pthread_create(&audio_thread, NULL, apu_audio_thread, NULL);

    usleep(300000);
    printf("[CPU] Writing to APU registers...\n");
    printf("[CPU] Enabling master APU (0xBF40 = 0xF1)\n");
    apu.master_ctrl = 0xF1; // Enable + max volume

    usleep(300000);
    printf("[CPU] Starting Channel 0: 440 Hz\n");
    printf("[CPU] - Write 0xBF41 = 0x88 (freq low)\n");
    printf("[CPU] - Write 0xBF42 = 0x01 (freq high)\n");
    printf("[CPU] - Write 0xBF43 = 0x8C (enable + vol 12)\n");
    apu.channels[0].frequency_reg = 440;
    apu.channels[0].control = 0x8C;

    usleep(500000);
    printf("[CPU] Starting Channel 1: 550 Hz\n");
    apu.channels[1].frequency_reg = 550;
    apu.channels[1].control = 0x88;

    usleep(500000);
    printf("[CPU] Stopping Channel 0\n");
    apu.channels[0].control = 0x00;

    usleep(300000);
    printf("[CPU] Stopping Channel 1\n");
    apu.channels[1].control = 0x00;

    pthread_join(audio_thread, NULL);

    printf("\nKey concept: This is YOUR APU!\n");
    printf("- CPU writes to registers (apu.channels[].control, etc.)\n");
    printf("- Audio thread reads registers and generates samples\n");
    printf("- No synchronization needed - just read/write simple values\n");
}

//==============================================================================
// MAIN PROGRAM
//==============================================================================

int main(void) {
    printf("========================================\n");
    printf("   Threading for tiny16 APU Tutorial\n");
    printf("========================================\n");
    printf("\nThis teaches you threads step-by-step.\n");
    printf("Each example builds on the previous one.\n");

    char input[10];

    printf("\nPress ENTER to start Example 1...");
    fgets(input, sizeof(input), stdin);
    example1_basic_thread();

    printf("\nPress ENTER for Example 2...");
    fgets(input, sizeof(input), stdin);
    example2_shared_data_problem();

    printf("\nPress ENTER for Example 3...");
    fgets(input, sizeof(input), stdin);
    example3_lockfree_pattern();

    printf("\nPress ENTER for Example 4...");
    fgets(input, sizeof(input), stdin);
    example4_callback_pattern();

    printf("\nPress ENTER for Example 5 (APU Pattern)...");
    fgets(input, sizeof(input), stdin);
    example5_apu_pattern();

    printf("\n========================================\n");
    printf("            Tutorial Complete!\n");
    printf("========================================\n");
    printf("\nSummary:\n");
    printf("1. Threads run simultaneously\n");
    printf("2. Shared data needs protection (or careful design)\n");
    printf("3. Lock-free pattern: One writes, one reads\n");
    printf("4. Audio callback: Runs on separate thread\n");
    printf("5. APU pattern: CPU writes registers, audio reads them\n");
    printf("\nFor tiny16 APU:\n");
    printf("- Audio callback runs on raylib's audio thread\n");
    printf("- CPU (your program) writes to APU registers via STORE\n");
    printf("- Audio callback reads registers and generates samples\n");
    printf("- No locks, no sync - just simple reads/writes!\n");
    printf("\n");

    return 0;
}
