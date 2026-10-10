# Voxalibera

Working name for an open-source DAW for Linux, built in C++20 on JUCE 8 with our own engine.

## Build and run

```
./compile.sh                  configures on first use, builds, then runs
./run.sh                      runs the last build
./test.sh                     builds and runs the base tests
./run.sh --list-devices       arguments after the options go to the app
./compile.sh --debug          same scripts, debug build in build-debug/ with address and undefined behaviour sanitizers
```

Shared script settings live in `build_options.sh`. Use the `--debug` build when chasing crashes or odd behaviour; sanitizer reports print to the terminal.

Needs `cmake`, `ninja`, `alsa-lib`, `libsystemd` and `pipewire-jack`. JUCE and Catch2 are fetched by CMake.

## Code rules

- No comments in code. If something needs explaining, rename it or split it until it doesn't. Explanations live in this file or in docs, not next to code.
- Formatting is whatever `.clang-format` produces. Run `clang-format -i` on changed files. In short:
  - tabs, no line limit;
  - `{` on the same line for `if`/`for`/`while`/`switch`, on its own line for functions, classes and namespaces;
  - namespace contents indented;
  - constructor initializers on the signature line;
  - lists spanning several lines end with a trailing comma, which keeps the closing `};` on its own line.
- Files and folders: lowercase `snake_case` (`device_session.cpp`).
- One file is one thing someone would look for by name ("where's the recording?", "where's the mixer?"), not one class. A header and its `.cpp` together should land between roughly 50 and 300 lines.
  - Merge into another file when it has a single user and is small (under ~50 lines), when it's plain data that always travels together (the project model), or when it's small building blocks of one screen.
  - Keep separate when it runs on another thread or belongs to another layer, when several places use it, or when merging would pass ~300 lines.
  - Past ~300 lines, split along a natural seam.
- Types `PascalCase`, functions and variables `snake_case`, constants `UPPER_SNAKE` (in an anonymous namespace at the top of the `.cpp`), enum values `snake_case`. Overrides of JUCE virtuals keep JUCE's names (`paint`, `resized`, `audioDeviceIOCallbackWithContext`).
- Full words, no abbreviations except in math.
- No magic numbers in logic or layout: name them as constants.
- Short functions with guard clauses and early returns, shallow nesting.
- Composition over inheritance. Inherit only from JUCE base classes or thin interfaces.
- Plain `struct` for data, members public by default; `private` only for what would break invariants.
- Dependencies are passed explicitly (constructor references). No singletons, no globals. `AppContext` is the one shared object.
- Settings and saved state are JSON with `kebab-case` keys, written with `juce::File::replaceWithText` (temporary file then rename, so a crash can't leave a half-written file).
- Log notable events with `log_event()` (`core/log.h`), never from the audio thread. The log is `~/.local/state/voxalibera/voxalibera.log`, capped at 1 MB.

## Architecture

A file-by-file map with the most useful functions is in [docs/code.md](docs/code.md). Adding, removing or renaming a file means updating its one line there.

```
source/
  main.cpp     starts the application
  app/         application lifecycle, windows, commands and menus, command-line tools
  ui/          views; read state every frame, call into core/engine on user input
  core/        app state: AppContext, Settings, DeviceSession (device choice, reservations, PipeWire setup, restoring hardware controls)
  engine/      audio processing; everything the audio thread touches
  platform/    Linux specifics: ALSA cards and controls, D-Bus device reservation, PipeWire properties, thread priority
tests/
```

Dependencies only point down: `app -> ui -> core -> engine -> platform`. Nothing outside `platform/` includes ALSA, sd-bus or PipeWire headers.

### Frame loop

`MainWindow` runs a 30 Hz timer, the equivalent of a game loop: `context.update()` first, then the views that need it (the audio settings view while its overlay is open). Views poll state (meters, device starts, active card) instead of subscribing to events. There is no event bus. Anything that must happen regardless of which windows are open (like restoring hardware controls after a device starts) belongs in `core/`, not in a view.

### Dialogs

Settings and anything else that shouldn't be touched mid-session open as a `ModalOverlay` inside the main window: the window dims, the dialog sits centered, clicks outside it, the menu bar and every shortcut except Quit are blocked, and Escape closes it. No separate OS windows for this, since Wayland compositors decide where windows go. Messages and questions use `MessageDialog` through `MainWindow::show_message`, never `NativeMessageBox`, so they look like the rest of the app.

### Theme

All colours live in `ui/theme.h`; `ThemeLookAndFeel` applies them to every JUCE widget. Views use `theme::` constants instead of their own colours.

### Commands

Keyboard shortcuts always win in the main window: only a text field being typed into may hold keyboard focus there. `MainWindow` takes focus away from any other control (sliders, lists, buttons) each frame, so arrow keys, Space and Enter never get captured by whatever was clicked last. Dialogs in the overlay keep their own focus.

Every user action is a command: an id in `app/commands.h`, one line in the definitions table (name, menu category, default key), and a handler in `MainWindow`'s `actions` map. Menus, keyboard shortcuts and later a command palette and scripts all go through JUCE's `ApplicationCommandManager`. A command without a handler shows up greyed out, so planned features can sit in the menus before they exist.

### Real-time rules

The audio callback (`AudioEngine::audioDeviceIOCallbackWithContext`) and everything it calls:
- never allocates, locks, logs, or touches files;
- reads settings only through `std::atomic` members (`Passthrough::monitoring`, `Passthrough::routing`);
- publishes results only through atomics (`LevelMeter`, `running_realtime`, `device_starts`).

The UI writes those atomics and saves `Settings`; the engine never sees `Settings`.

### Project model and edits

- `core/project/` holds the model as plain data: `Project` has `Track`s; ids (`TrackId`, later `ClipId`...) are separate types so they can't be mixed up, never reused, and are how everything refers to everything.
- The model is only changed through `EditHistory`: `auto edit = context.edits.begin("Rename Track"); edit.set(id, &Track::name, name);`. Every operation records how to undo itself; everything in one `edit` is one undo step; edits with the same merge key (e.g. `"volume:7"`) within 1.5 s become one step (fader drags).
- Adding a property to a track or the project: add the field. It's immediately editable and undoable with `edit.set`, nothing else to write. Structural changes (add/remove things) get one pair of operations in `Edit`, written once.
- Higher-level actions live in editors like `TrackEditor` (`add`, `remove_selected`, `toggle_selected`...), which commands and views call. Views never touch the model directly.
- Selection lives in `core/Selection`, outside the model and outside undo.
- `project.revision` and `selection.revision` change with every edit; views compare them each frame and refresh only when they changed. Per-object views (track headers) are kept by id, so they survive reordering and keep their state.

### Project files

- `core/project/project_file` writes a `Project` as JSON (`.vxl`, kebab-case keys, a `format` number). Reading never refuses a file it can parse: broken tracks or clips are skipped and reported. A newer `format` opens read-only (Save becomes Save As).
- A new field in the model needs a line in `write_*` and `read_*` there, with a default for older files. Bump `PROJECT_FORMAT` only when old versions would misread the file.
- `ProjectSession` owns the file: open replaces the whole `Project` (history and selection cleared, tracks disarmed), save writes with `replaceWithText` and keeps the last 10 copies in `backups/`. Unsaved changes are `project.revision != saved_revision`.
- The first Save As moves the temporary recordings folder into the project; Save As of a saved project copies its `audio/`. "Don't Save" on a never-saved project deletes its temporary folder, and the question says so.
- While a project has a folder, `ProjectRecovery` keeps a `.lock` with our process id in it; a lock whose process isn't a running `voxalibera` means a crash. Autosave runs every 5 minutes when there are changes: the JSON is built on the message thread and written to `autosave/autosave.vxl` on the disk thread, so it never touches the audio thread or the real `.vxl`. Saving or "Don't Save" removes the autosave.
- At startup `ProjectDialogs::check_recovery` offers to reopen a crashed project (restoring its autosave if it's newer than the saved file), then each leftover unsaved session (Recover / Delete / Later). A recovered session without an autosave gets one track per take at the start.
- File > Clean Up Project deletes takes no clip uses, after listing them; it clears the undo history (undo could bring back a clip whose file is gone) and asks to save first so the saved file never points at deleted takes. Closing a saved project with over 100 MB of unused takes hints once, then again after another 100 MB.
- A project's sample rate is the rate of its first recording. In JACK mode `DeviceSession::use_project_rate` asks PipeWire for it (`node.force-rate`) when the card can run it (the UX1 only runs 44.1 kHz); otherwise, and in ALSA mode, a different device rate leaves clips silent and the transport bar says why.

### Engine snapshots and transport

- `EnginePublisher` notices a new revision once per frame, builds an `EngineSnapshot` (a read-only copy of what the audio thread needs) and publishes it through `SnapshotExchange`: an atomic pointer swap. Replaced snapshots are freed on the message thread once the audio thread has finished a block that started after the swap, never on the audio thread.
- `Transport` takes play/stop/locate from the UI through a `LockFreeQueue` and publishes its state and position through atomics. Positions inside the engine are in samples; `engine/musical_time` converts to bars, beats and ticks.

### Recording

- `core/Recorder` runs a take from the message thread: it arms the selected track if nothing is armed, creates one WAV writer per armed track (`juce::AudioFormatWriter::ThreadedWriter` on the `disk_thread`), and hands a `RecordingSession` to the engine through `SnapshotExchange`, so it's only freed after the audio thread is done with it.
- The audio thread only copies input samples into each writer's lock-free buffer and collects waveform peaks into a `LockFreeQueue`; the disk thread writes the files and refreshes the WAV header every couple of seconds, so a crash leaves a valid file.
- On stop, `Recorder` waits until the engine has really stopped recording, then adds all takes as clips in one "Record" edit, shifted by the device's reported latency (`source_offset`).
- Takes go to the project's folder; before the first save that's a temporary folder under `~/.local/share/voxalibera/unsaved/`. Files are never deleted silently (see `docs/projects.md`).
- Clip `file` paths are relative to the project folder; never build a `juce::File` from them directly, resolve them against `project.folder`.

### Playback and mixing

- `EnginePublisher` builds each track's playback data for the snapshot: linear gains with pan already applied, whether it's audible (solo resolved here, not on the audio thread), whether it monitors its input (armed tracks do: arming is monitoring), and its clips with their opened files.
- Audio files are memory-mapped (`engine/AudioFile`); the audio thread copies samples straight from mapped memory. `ReadAhead` on the disk thread touches the next seconds of every clip so the pages are in memory before they're played.
- `MixRenderer` renders in chunks of up to 2048 samples into preallocated buffers: clips and monitored input per track, then gain and pan, then the master gain. Mono clips on stereo tracks play on both sides; stereo clips on mono tracks are summed.
- Track meters live in `engine.mixer.track_meters`; the publisher gives each track a slot (`publisher.meter_of(id)`).
- Clips whose file sample rate differs from the device stay silent, are logged once and shown in the transport bar (`publisher.mismatched_file_rate()`). Real-time resampling is a planned task.

### Devices

- Views never change the `AudioDeviceManager` themselves; they call `DeviceSession::choose_*`. Only those user choices are saved, so a fallback device (the saved one is unplugged) is used without overwriting the user's choice.
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
