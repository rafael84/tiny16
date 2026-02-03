// midi2music - Convert MIDI files to tiny16 music format
//
// Usage: midi2music input.mid [options]
//
// Options:
//   -o <file>    Output file (default: stdout)
//   -n <name>    Data label name (default: song_data)
//   -t <track>   Track number to extract (default: 0)
//   -c <channel> MIDI channel to extract (default: all)
//   -v <volume>  Default volume 0-15 (default: 6)
//   -s <scale>   Time scale factor (default: 1.0)
//
// Output format (4 bytes per note):
//   [0] note (0=rest, 1-36=C3-B5)
//   [1] volume (0-15)
//   [2] duration (frames at 60fps)
//   [3] reserved

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NOTES         1024
#define FRAMES_PER_SECOND 60

// MIDI note range: 48 (C3) to 83 (B5) -> tiny16 notes 1-36
#define MIDI_NOTE_MIN 48
#define MIDI_NOTE_MAX 83

typedef struct {
    uint8_t note;     // 0=rest, 1-36=notes
    uint8_t volume;   // 0-15
    uint8_t duration; // frames
    uint8_t reserved;
} MusicNote;

typedef struct {
    MusicNote notes[MAX_NOTES];
    int count;
} MusicSequence;

// Read big-endian values from MIDI file
static uint32_t read_u32_be(FILE* f) {
    uint8_t b[4];
    fread(b, 1, 4, f);
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
}

static uint16_t read_u16_be(FILE* f) {
    uint8_t b[2];
    fread(b, 1, 2, f);
    return ((uint16_t)b[0] << 8) | b[1];
}

// Read variable-length quantity (MIDI delta time)
static uint32_t read_vlq(FILE* f) {
    uint32_t value = 0;
    uint8_t byte;
    do {
        fread(&byte, 1, 1, f);
        value = (value << 7) | (byte & 0x7F);
    } while (byte & 0x80);
    return value;
}

// Convert MIDI note to tiny16 note (0=rest, 1-36=C3-B5)
static uint8_t midi_to_tiny16_note(uint8_t midi_note) {
    if (midi_note < MIDI_NOTE_MIN || midi_note > MIDI_NOTE_MAX) {
        return 0; // Out of range
    }
    return midi_note - MIDI_NOTE_MIN + 1;
}

// Convert MIDI velocity (0-127) to tiny16 volume (0-15)
static uint8_t midi_velocity_to_volume(uint8_t velocity, uint8_t default_vol) {
    if (velocity == 0) return 0;
    uint8_t vol = (velocity * 15) / 127;
    return vol > 0 ? vol : default_vol;
}

// Convert MIDI ticks to frames (60fps)
static uint8_t ticks_to_frames(uint32_t ticks, uint16_t ticks_per_beat, uint32_t tempo_us,
                               float scale) {
    // tempo_us = microseconds per beat
    // frames = ticks * (tempo_us / 1000000) * 60 / ticks_per_beat
    double seconds = (double)ticks * tempo_us / 1000000.0 / ticks_per_beat;
    double frames = seconds * FRAMES_PER_SECOND * scale;
    if (frames < 1) frames = 1;
    if (frames > 255) frames = 255;
    return (uint8_t)frames;
}

typedef struct {
    uint8_t note;
    uint8_t velocity;
    uint32_t start_tick;
} ActiveNote;

#define MAX_ACTIVE_NOTES 16

static int parse_midi_track(FILE* f, uint32_t track_len, MusicSequence* seq, int target_channel,
                            uint16_t ticks_per_beat, uint8_t default_vol, float time_scale) {
    long track_start = ftell(f);
    long track_end = track_start + track_len;

    uint32_t current_tick = 0;
    uint32_t tempo_us = 500000; // Default: 120 BPM
    uint8_t running_status = 0;

    ActiveNote active[MAX_ACTIVE_NOTES];
    int active_count = 0;

    uint32_t last_note_end_tick = 0;

    while (ftell(f) < track_end && seq->count < MAX_NOTES) {
        uint32_t delta = read_vlq(f);
        current_tick += delta;

        uint8_t status;
        fread(&status, 1, 1, f);

        // Handle running status
        if (status < 0x80) {
            fseek(f, -1, SEEK_CUR);
            status = running_status;
        } else {
            running_status = status;
        }

        uint8_t type = status & 0xF0;
        uint8_t channel = status & 0x0F;

        if (type == 0x90 || type == 0x80) {
            // Note On / Note Off
            uint8_t note, velocity;
            fread(&note, 1, 1, f);
            fread(&velocity, 1, 1, f);

            // Note On with velocity 0 = Note Off
            bool is_note_on = (type == 0x90 && velocity > 0);

            if (target_channel >= 0 && channel != target_channel) {
                continue;
            }

            if (is_note_on) {
                // Add rest if there's a gap
                if (last_note_end_tick > 0 && current_tick > last_note_end_tick) {
                    uint32_t gap_ticks = current_tick - last_note_end_tick;
                    uint8_t gap_frames =
                        ticks_to_frames(gap_ticks, ticks_per_beat, tempo_us, time_scale);
                    if (gap_frames > 0 && seq->count < MAX_NOTES) {
                        seq->notes[seq->count].note = 0; // rest
                        seq->notes[seq->count].volume = 0;
                        seq->notes[seq->count].duration = gap_frames;
                        seq->notes[seq->count].reserved = 0;
                        seq->count++;
                    }
                }

                // Start tracking this note
                if (active_count < MAX_ACTIVE_NOTES) {
                    active[active_count].note = note;
                    active[active_count].velocity = velocity;
                    active[active_count].start_tick = current_tick;
                    active_count++;
                }
            } else {
                // Note Off - find and complete the note
                for (int i = 0; i < active_count; i++) {
                    if (active[i].note == note) {
                        uint32_t duration_ticks = current_tick - active[i].start_tick;
                        uint8_t tiny16_note = midi_to_tiny16_note(note);
                        uint8_t vol = midi_velocity_to_volume(active[i].velocity, default_vol);
                        uint8_t frames =
                            ticks_to_frames(duration_ticks, ticks_per_beat, tempo_us, time_scale);

                        if (tiny16_note > 0 && seq->count < MAX_NOTES) {
                            seq->notes[seq->count].note = tiny16_note;
                            seq->notes[seq->count].volume = vol;
                            seq->notes[seq->count].duration = frames;
                            seq->notes[seq->count].reserved = 0;
                            seq->count++;
                        }

                        last_note_end_tick = current_tick;

                        // Remove from active list
                        for (int j = i; j < active_count - 1; j++) {
                            active[j] = active[j + 1];
                        }
                        active_count--;
                        break;
                    }
                }
            }
        } else if (type == 0xA0) {
            // Polyphonic aftertouch
            fseek(f, 2, SEEK_CUR);
        } else if (type == 0xB0) {
            // Control change
            fseek(f, 2, SEEK_CUR);
        } else if (type == 0xC0) {
            // Program change
            fseek(f, 1, SEEK_CUR);
        } else if (type == 0xD0) {
            // Channel aftertouch
            fseek(f, 1, SEEK_CUR);
        } else if (type == 0xE0) {
            // Pitch bend
            fseek(f, 2, SEEK_CUR);
        } else if (status == 0xFF) {
            // Meta event
            uint8_t meta_type;
            fread(&meta_type, 1, 1, f);
            uint32_t meta_len = read_vlq(f);

            if (meta_type == 0x51 && meta_len == 3) {
                // Tempo change
                uint8_t t[3];
                fread(t, 1, 3, f);
                tempo_us = ((uint32_t)t[0] << 16) | ((uint32_t)t[1] << 8) | t[2];
            } else {
                fseek(f, meta_len, SEEK_CUR);
            }
        } else if (status == 0xF0 || status == 0xF7) {
            // SysEx
            uint32_t sysex_len = read_vlq(f);
            fseek(f, sysex_len, SEEK_CUR);
        }
    }

    return 0;
}

static void print_usage(const char* prog) {
    fprintf(stderr, "Usage: %s input.mid [options]\n", prog);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  -o <file>    Output file (default: stdout)\n");
    fprintf(stderr, "  -n <name>    Module/data name (default: song)\n");
    fprintf(stderr, "  -t <track>   Track number to extract (default: 0)\n");
    fprintf(stderr, "  -c <channel> MIDI channel 0-15 (default: all)\n");
    fprintf(stderr, "  -v <volume>  Default volume 0-15 (default: 6)\n");
    fprintf(stderr, "  -s <scale>   Time scale factor (default: 1.0)\n");
    fprintf(stderr, "  -m <max>     Max notes to extract (default: unlimited)\n");
    fprintf(stderr, "\nOutput: S-expression module (.se) for tiny16\n");
    fprintf(stderr, "Use with: (require (mymodule)) then mymodule/data, mymodule/length\n");
    fprintf(stderr, "\nNote range: C3 (MIDI 48) to B5 (MIDI 83)\n");
}

int main(int argc, char** argv) {
    const char* input_file = NULL;
    const char* output_file = NULL;
    const char* module_name = "song";
    int target_track = 0;
    int target_channel = -1; // -1 = all channels
    uint8_t default_volume = 6;
    float time_scale = 1.0f;
    int max_notes = 0; // 0 = no limit (SEC compiler now uses dynamic memory)

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            module_name = argv[++i];
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            target_track = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            target_channel = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-v") == 0 && i + 1 < argc) {
            default_volume = atoi(argv[++i]) & 0x0F;
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            time_scale = atof(argv[++i]);
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            max_notes = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            input_file = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (!input_file) {
        print_usage(argv[0]);
        return 1;
    }

    // Open input file
    FILE* f = fopen(input_file, "rb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open %s\n", input_file);
        return 1;
    }

    // Read MIDI header
    char header[4];
    fread(header, 1, 4, f);
    if (memcmp(header, "MThd", 4) != 0) {
        fprintf(stderr, "Error: Not a valid MIDI file\n");
        fclose(f);
        return 1;
    }

    uint32_t header_len = read_u32_be(f);
    uint16_t midi_format = read_u16_be(f);
    uint16_t num_tracks = read_u16_be(f);
    uint16_t ticks_per_beat = read_u16_be(f);

    (void)header_len;
    (void)midi_format;

    fprintf(stderr, "MIDI: format=%d, tracks=%d, ticks/beat=%d\n", midi_format, num_tracks,
            ticks_per_beat);

    // Handle SMPTE timing (not supported)
    if (ticks_per_beat & 0x8000) {
        fprintf(stderr, "Error: SMPTE timing not supported\n");
        fclose(f);
        return 1;
    }

    MusicSequence seq = {.count = 0};

    // Read tracks
    int track_num = 0;
    while (!feof(f)) {
        char chunk_type[4];
        if (fread(chunk_type, 1, 4, f) != 4) break;

        uint32_t chunk_len = read_u32_be(f);

        if (memcmp(chunk_type, "MTrk", 4) == 0) {
            if (track_num == target_track) {
                fprintf(stderr, "Processing track %d (%u bytes)...\n", track_num, chunk_len);
                parse_midi_track(f, chunk_len, &seq, target_channel, ticks_per_beat, default_volume,
                                 time_scale);
            } else {
                fseek(f, chunk_len, SEEK_CUR);
            }
            track_num++;
        } else {
            fseek(f, chunk_len, SEEK_CUR);
        }
    }

    fclose(f);

    if (seq.count == 0) {
        fprintf(stderr, "Warning: No notes extracted!\n");
        fprintf(stderr, "Try a different track (-t) or channel (-c)\n");
        return 1;
    }

    // Apply max notes limit (if specified)
    if (max_notes > 0 && seq.count > max_notes) {
        fprintf(stderr, "Limiting from %d to %d notes\n", seq.count, max_notes);
        seq.count = max_notes;
    }

    fprintf(stderr, "Extracted %d notes\n", seq.count);

    // Output
    FILE* out = stdout;
    if (output_file) {
        out = fopen(output_file, "w");
        if (!out) {
            fprintf(stderr, "Error: Cannot create %s\n", output_file);
            return 1;
        }
    }

    // Note name table for comments
    const char* note_names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    // S-expression module format
    fprintf(out, "; %s - music module generated by midi2music\n", module_name);
    fprintf(out, "; Source: %s (track %d", input_file, target_track);
    if (target_channel >= 0) fprintf(out, ", channel %d", target_channel);
    fprintf(out, ")\n");
    fprintf(out, "; %d notes, time scale %.2f\n", seq.count, time_scale);
    fprintf(out,
            "; Format: note (0=rest, 1-36=C3-B5), volume (0-15), duration (frames), reserved\n");
    fprintf(out, ";\n");
    fprintf(out, "; Usage:\n");
    fprintf(out, ";   (require (%s))\n", module_name);
    fprintf(out, ";   (apu/music-play (hi %s/data) (lo %s/data) %s/length 1)\n\n", module_name,
            module_name, module_name);

    fprintf(out, "(ns %s)\n\n", module_name);
    fprintf(out, "(def length %d)\n", seq.count);
    fprintf(out, "(def length-hi %d)\n", (seq.count >> 8) & 0xFF);
    fprintf(out, "(def length-lo %d)\n\n", seq.count & 0xFF);
    fprintf(out, "(data data\n");

    for (int i = 0; i < seq.count; i++) {
        MusicNote* n = &seq.notes[i];
        if (n->note == 0) {
            fprintf(out, "  (db 0 0 %d 0)", n->duration);
            fprintf(out, "     ; rest\n");
        } else {
            int octave = 3 + (n->note - 1) / 12;
            int note_in_octave = (n->note - 1) % 12;
            fprintf(out, "  (db %d %d %d 0)", n->note, n->volume, n->duration);
            fprintf(out, "    ; %s%d\n", note_names[note_in_octave], octave);
        }
    }

    fprintf(out, ")\n");

    if (output_file) {
        fclose(out);
        fprintf(stderr, "Output written to %s\n", output_file);
    }

    return 0;
}
