# Probe Programs

Reference implementations and learning tools for tiny16 development.

## threads.c

**Interactive tutorial teaching threading concepts for APU development.**

Progressively builds understanding through 5 examples:

### Example 1: Basic Thread Creation
- How to create and join threads
- Demonstrates simultaneous execution
- Shows main thread and worker thread running in parallel

### Example 2: Shared Data Problem
- Demonstrates race conditions
- Shows what happens without proper synchronization
- Explains why careful design is needed

### Example 3: Lock-Free Producer-Consumer
- One thread writes (producer)
- Another thread reads (consumer)
- No locks needed for simple state
- **This is the pattern raylib uses!**

### Example 4: Audio Callback Pattern
- How raylib's audio system works
- Callback function called repeatedly by audio thread
- Main thread updates state, callback reads it
- Demonstrates the actual pattern you'll use

### Example 5: APU Pattern
- Complete tiny16 APU threading model
- CPU writes to registers via STORE
- Audio callback reads registers and generates samples
- Shows exactly how your APU will work

**Build:**
```bash
make probe
```

**Run:**
```bash
bin/probe-threads
```

**Interactive:** Press ENTER to advance through examples.

**Key Takeaways:**
- Audio runs on separate thread (automatic in raylib)
- Your CPU code writes to APU registers
- Audio callback reads registers and generates samples
- No locks or complex synchronization needed
- Just simple reads and writes!

---

## audio.c

**Comprehensive audio reference demonstrating concepts for APU development.**

Features:
- **Waveform generation**: sine, square, triangle, sawtooth, noise
- **Phase accumulator**: smooth frequency control technique
- **Duty cycle**: pulse width modulation for square waves
- **LFSR**: Linear Feedback Shift Register for noise generation
- **Multi-channel mixing**: combining audio sources
- **Audio streaming**: raylib's callback-based audio system
- **Musical notes**: frequency table and melody playback demo
- **Sound effects**: practical examples of noise channel usage

**Build:**
```bash
make probe
```

**Run:**
```bash
bin/probe-audio
```

**Controls:**

*Channel Control:*
- `1-5`: Toggle individual channels (Sine, Square, Triangle, Sawtooth, Noise)
- `6`: Toggle all channels
- `Up/Down`: Increase/decrease frequency
- `Left/Right`: Adjust duty cycle (square wave only)
- `Q/A`: Master volume up/down
- `Space`: Pause/resume

*Music:*
- `M`: Play/stop melody demo (Twinkle Twinkle Little Star)
- `N`: Change melody waveform (cycles through Sine → Square → Triangle → Sawtooth)

*Sound Effects (demonstrates noise channel usage):*
- `E`: Explosion (noise fade-out with frequency sweep)
- `L`: Laser (square wave pitch sweep)
- `H`: Hit (short noise burst)
- `K`: Drum (noise + bass thump)

*Other:*
- `ESC`: Exit

**Sound Effects Examples:**

The probe includes interactive demonstrations:
- **Explosion (E)**: Noise with volume fade and frequency sweep - mimics explosion decay
- **Laser (L)**: Square wave with rapid frequency sweep - classic sci-fi laser
- **Hit (H)**: Short, loud noise burst - impact/damage sound
- **Drum (K)**: Noise + low triangle wave - percussion with bass punch

Watch the red progress bar in the top-right when effects play!

---

## Purpose

These programs serve as:
1. **Learning resources** - Understand audio and threading concepts
2. **Reference implementations** - Consult during APU development
3. **Testing grounds** - Try algorithms before VM integration
4. **Debugging tools** - Isolate and test specific features

## Recommended Learning Path

1. **Start with threads.c** - Understand threading model (15 minutes)
2. **Run audio.c** - Hear waveforms and effects (play around!)
3. **Read specs/apu.txt** - Study the full APU specification
4. **Implement APU** - Build it into your VM using these references

The threading knowledge from `threads.c` combined with the audio concepts from `audio.c` gives you everything needed to implement the tiny16 APU!
