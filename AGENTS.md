# custom-daw

Working name for an open-source DAW for Linux, built in C++20 on JUCE 8 with our own engine.

## Build and run

```
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/custom_daw_artefacts/Release/custom-daw
./build/custom_daw_artefacts/Release/custom-daw --list-devices
./build/tests/custom_daw_tests_artefacts/Release/custom_daw_tests
```

Needs `cmake`, `ninja`, `alsa-lib`, `libsystemd` and `pipewire-jack`. JUCE and Catch2 are fetched by CMake.

## Code rules

- No comments in code. If something needs explaining, rename it or split it until it doesn't. Explanations live in this file or in docs, not next to code.
- Formatting is whatever `.clang-format` produces: tabs, Allman braces, no line limit. Run `clang-format -i` on changed files.
- Files and folders: lowercase `snake_case` (`device_session.cpp`), one concept per file, aim for under ~300 lines.
- Types `PascalCase`, functions and variables `snake_case`, constants `UPPER_SNAKE` (in an anonymous namespace at the top of the `.cpp`), enum values `snake_case`. Overrides of JUCE virtuals keep JUCE's names (`paint`, `resized`, `audioDeviceIOCallbackWithContext`).
- Full words, no abbreviations except in math.
- No magic numbers in logic or layout: name them as constants.
- Short functions with guard clauses and early returns, shallow nesting.
- Composition over inheritance. Inherit only from JUCE base classes or thin interfaces.
- Plain `struct` for data, members public by default; `private` only for what would break invariants.
- Dependencies are passed explicitly (constructor references). No singletons, no globals. `AppContext` is the one shared object.
- Settings and saved state are JSON with `kebab-case` keys.

## Architecture

```
source/
  main.cpp     starts the application
  app/         application lifecycle, main window, command-line tools
  ui/          views; read state every frame, call into core/engine on user input
  core/        app state: AppContext, Settings, DeviceSession (device choice, reservations, PipeWire setup)
  engine/      audio processing; everything the audio thread touches
  platform/    Linux specifics: ALSA cards and controls, D-Bus device reservation, PipeWire properties, thread priority
tests/
```

Dependencies only point down: `app -> ui -> core -> engine -> platform`. Nothing outside `platform/` includes ALSA, sd-bus or PipeWire headers.

### Frame loop

`MainView` runs a 30 Hz timer, the equivalent of a game loop: `context.update()` first, then every view's `update()`. Views poll state (meters, device starts, active card) instead of subscribing to events. There is no event bus.

### Real-time rules

The audio callback (`AudioEngine::audioDeviceIOCallbackWithContext`) and everything it calls:
- never allocates, locks, logs, or touches files;
- reads settings only through `std::atomic` members (`Passthrough::monitoring`, `Passthrough::routing`);
- publishes results only through atomics (`LevelMeter`, `running_realtime`, `device_starts`).

The UI writes those atomics and saves `Settings`; the engine never sees `Settings`.

### Devices

- **ALSA mode:** `DeviceSession` borrows the card from PipeWire through `org.freedesktop.ReserveDevice1` and hands it back on close. The audio thread promotes itself to `SCHED_FIFO` because JUCE's ALSA backend doesn't.
- **JACK mode (PipeWire):** before JUCE opens its JACK client, `PIPEWIRE_PROPS` sets `node.force-quantum` and, for cards that can't run at 48 kHz, `node.force-rate`. PipeWire drops both when the client disconnects.
- JUCE scans JACK devices only once, so `DeviceSession` rescans after handing a card back, and on the Rescan button.
- Hardware controls (input source, hardware monitor...) are saved per card name and re-applied every time the device starts.

## Tests

`tests/` holds a few base tests for logic that must never silently break (audio routing, device-to-card matching, PipeWire properties), runnable without audio hardware. Tests for a feature in progress may live there while it's being built and are removed afterwards. Don't grow a large suite that has to be updated alongside every change.

## Known hardware: Line 6 POD Studio UX1

- Driver `snd_usb_toneport`, 44.1 kHz only. Its hardware controls reset to Microphone input and full hardware monitor on every replug; the app re-applies saved values.
- JUCE's ALSA list doesn't show it without an alias; `~/.asoundrc` defines `pcm.ux1` (`type empty`, described as "POD Studio UX1 (direct hardware)").
- `~/.config/wireplumber/wireplumber.conf.d/51-ux1-low-latency.conf` makes it usable through PipeWire: 44.1 kHz, period 128 x 4, headroom 128, forced quantum 128, never paused or suspended. Each value was found by testing; the file explains why.
- Needs a few seconds to settle after plugging in or waking from sleep.
