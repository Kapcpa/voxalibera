# Voxalibera

A DAW for Linux, in C++20 on JUCE 8 with its own audio engine. Pre-alpha: it records and plays back, but can't export a song yet.

## What works

- JACK (PipeWire) and direct ALSA at low latency, hardware mixer controls per interface
- mono and stereo audio tracks, recording with count-in and metronome, playback from disk
- timeline with waveforms, moving and deleting clips, mixer
- VST3 and LV2 effects: scanning, browser, per-track chains, plugin windows, bypass, latency compensation
- projects saved as JSON folders, undo and redo, autosave and crash recovery

Next up: importing audio, editing clips and exporting. See [docs/plan.md](docs/plan.md).

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
