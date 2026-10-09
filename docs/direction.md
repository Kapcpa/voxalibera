# Direction

Decisions so far. Background in [research.md](research.md).

## Identity

- A fusion of Logic's clarity and recording workflow with Ableton's clip-based creativity.
- Plus the gap nobody fills on Linux: strong MIDI, crash-proof plugin hosting, a readable and versionable project format, and first-class Linux audio (PipeWire/JACK, direct ALSA, device quirks handled for the user).
- Open source under AGPLv3 (required by JUCE's free license).

## Recording first

The core loop is: plug in an interface, arm a track, record, listen back, record again, keep the best takes. Everything else is built around that. Low-latency monitoring and reliable device handling (already working) are the foundation.

## Layout to start with

```
+-------------------------------------------------------------+
| transport: record, play, stop, loop, metronome, tempo, time |
+-------------------------------------------------------------+
| track headers |  timeline with clips                        |
|               |                                             |
+-------------------------------------------------------------+
| mixer: one channel strip per track                          |
+-------------------------------------------------------------+
```

- Single window, zones that can be toggled, like Logic.
- Effect chains: **not** Logic's sidebar inspector. A different design comes later.

## Later, planned for in the model

- Clip launcher (Ableton's session grid): clips are independent objects, not just positions on a timeline, so a launcher grid can be added without rebuilding the model. For a guitarist it works like a looper pedal.
- Take lanes with swipe comping, for audio and MIDI.
- Macros and modulation on any parameter.
