# Voxalibera

A DAW for Linux, in C++20 on JUCE 8 with its own audio engine. Pre-alpha: a song can be recorded, edited, mixed and exported, audio only for now.

## What works

- JACK (PipeWire) and direct ALSA at low latency, hardware mixer controls per interface
- mono and stereo audio tracks, recording with count-in and metronome, playback from disk
- audio import (WAV, FLAC, Ogg, MP3, AIFF), drag and drop
- clip editing: move, trim, split, fades, gain, copy and paste across tracks
- grid and snap, loop range, cycle recording into take lanes with comping
- mixer, reordering tracks
- VST3 and LV2 effects on tracks and the master: scanning, browser, chains, plugin windows, bypass, latency compensation
- our own effects: noise gate, drives, cabinet with IR loading, graphic EQ
- export to WAV (16-bit with dither, 24-bit, 32-bit float), whole song or loop range
- projects saved as JSON folders, undo and redo, autosave and crash recovery

Next up: MIDI and instruments. See [docs/plan.md](docs/plan.md).

## Build

Needs `cmake`, `ninja`, a C++20 compiler, `alsa-lib`, `libsystemd` and `pipewire-jack`. CMake downloads JUCE and Catch2.

```
./compile.sh    build
./run.sh        start the last build
./test.sh       build and run the tests
```

`--debug` on any of them builds into `build-debug/` with sanitizers.

## Docs

- [docs/plan.md](docs/plan.md) - milestones
- [AGENTS.md](AGENTS.md) - code rules and architecture
- [docs/code.md](docs/code.md) - where things are in the source
- [docs/projects.md](docs/projects.md) - the project format

Contributing: [CONTRIBUTING.md](CONTRIBUTING.md).

## License

Copyright (C) 2026 Kapcpa. AGPL-3.0-or-later, see [LICENSE](LICENSE).

Additional terms under section 7 of the AGPL:

- (b) Copies and modified versions must keep the copyright notice and the link to this repository in the About dialog.
- (e) The license grants no rights to the Voxalibera name or logo. Modified versions that are distributed must use a different name.
