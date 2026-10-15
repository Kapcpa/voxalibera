# Voxalibera

Working name for an open-source DAW for Linux, built in C++20 on JUCE 8 with our own engine.

## Build and run

```
./compile.sh                  configures on first use and builds
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
  - Folders group files by topic, once a topic has three or more files; a file goes in the folder of what it's about, not of who uses it.
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
    tracks/ mixer/ settings/ plugins/   one folder per screen area
    parts/                     pieces several screens reuse (meters, overlay, message dialog)
  core/        app state: AppContext, Settings, editors, selection, log
    project/   the song document and its files: model, edits, .vxl format, session, recovery, cleanup
    audio/     talks to the engine and the devices: device session, engine publisher, recorder
    media/     audio files and waveforms
    plugins/   effects: the list of installed plugins, the scanner, running instances
  engine/      audio processing; everything the audio thread touches
  platform/    Linux specifics: ALSA cards and controls, PipeWire (device reservation, client properties), app folders, thread priority
tests/
```

Dependencies only point down: `app -> ui -> core -> engine -> platform`. Nothing outside `platform/` includes ALSA, sd-bus or PipeWire headers.

### Frame loop

`MainWindow` runs a 30 Hz timer, the equivalent of a game loop: `context.update()` first, then the views that need it (the audio settings view while its overlay is open). Views poll state (meters, device starts, active card) instead of subscribing to events. There is no event bus. Anything that must happen regardless of which windows are open (like restoring hardware controls after a device starts) belongs in `core/`, not in a view. Views redraw only when something they show changed, so any state they draw needs a `revision` counter they compare each frame; that includes data arriving from background threads (`peaks.revision` for waveforms built after opening a project).

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
- Switches that aren't part of the song (metronome, count-in) use `edits.set_without_undo(&Project::field, value)`: saved and marked as a change, but no undo step.
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
- A project's sample rate is the rate of its first recording. In JACK mode `DeviceSession::use_project_rate` asks PipeWire for it (`node.force-rate`) when the card can run it (the UX1 only runs 44.1 kHz); otherwise, and in ALSA mode, clips are resampled in real time.

### Engine snapshots and transport

- `EnginePublisher` notices a new revision once per frame, builds an `EngineSnapshot` (a read-only copy of what the audio thread needs) and publishes it through `SnapshotExchange`: an atomic pointer swap. Replaced snapshots are freed on the message thread once the audio thread has finished a block that started after the swap, never on the audio thread.
- `Transport` takes play/stop/locate from the UI through a `LockFreeQueue` and publishes its state and position through atomics. Positions inside the engine are in samples; `engine/musical_time` converts to bars, beats and ticks.

### Recording

- `core/Recorder` runs a take from the message thread: it arms the selected track if nothing is armed, creates one WAV writer per armed track (`juce::AudioFormatWriter::ThreadedWriter` on the `disk_thread`), and hands a `RecordingSession` to the engine through `SnapshotExchange`, so it's only freed after the audio thread is done with it.
- The audio thread only copies input samples into each writer's lock-free buffer and collects waveform peaks into a `LockFreeQueue`; the disk thread writes the files and refreshes the WAV header every couple of seconds, so a crash leaves a valid file.
- Count-in (`project.count_in`, only when starting from a stop): `transport.record(count_in_samples)` moves the playhead one bar back and keeps `record_from` at the start point. Blocks before it are pre-roll: clips play, the metronome clicks even when it's off, and `RecordingSession::write` skips everything before `record_from`, to the sample.
- On stop, `Recorder` waits until the engine has really stopped recording, then adds all takes as clips in one "Record" edit, shifted by the device's reported latency (`source_offset`).
- Takes go to the project's folder; before the first save that's a temporary folder under `~/.local/share/voxalibera/unsaved/`. Files are never deleted silently (see `docs/projects.md`).
- Clip `file` paths are relative to the project folder; never build a `juce::File` from them directly, resolve them against `project.folder`.

### Playback and mixing

- `EnginePublisher` builds each track's playback data for the snapshot: linear gains with pan already applied, whether it's audible (solo resolved here, not on the audio thread), whether it monitors its input (armed tracks do: arming is monitoring), and its clips with their opened files.
- Audio files are memory-mapped (`engine/AudioFile`); the audio thread copies samples straight from mapped memory. `ReadAhead` on the disk thread touches the next seconds of every clip so the pages are in memory before they're played. Starting playback or recording and jumping (`context.locate`) first load the next half second right away (`load_ahead_of`), so the first block never waits for the disk. Recording writes on its own `recording_thread`, so slow reads can't hold up a take.
- `MixRenderer` renders in chunks of up to 2048 samples into preallocated buffers: clips and monitored input per track, then gain and pan, then the master gain. Mono clips on stereo tracks play on both sides; stereo clips on mono tracks are summed.
- Track meters live in `engine.mixer.track_meters`; the publisher gives each track a slot (`publisher.meter_of(id)`).
- Positions in the project (clip start, length, source offset) are samples at the project's rate (`timeline_rate(project, device_rate)`); the transport, the playhead and recording work in device samples. Convert at the edges with `convert_samples`: the publisher for clips, the timeline for the playhead and live takes, the recorder for new clips. Peaks carry their file's rate.
- A clip whose file rate differs from the device plays through `ResamplingKernel` (32-tap windowed sinc, built once per rate ratio on the message thread and shared through the snapshot); matching files keep the direct path. Ratios beyond 4x stay silent, logged and shown in the transport bar (`publisher.mismatched_file_rate()`).

### Plugins

- A track's effects are `Track::plugins` in the model: identity (`format`, `identifier`, name, manufacturer, version), `bypass` and the last stored `state`. They change through edits like everything else (`context.tracks.add_plugin`, `insert_plugin`, `move_plugin` within or across tracks, `remove_plugin`, `toggle_plugin_bypass`; `edit.set(plugin_id, &Plugin::field, value)`). Positions are indexes in the chain; moving to a position counts it after the plugin has left its old place.
- `PluginPool` (`core/plugins/`) owns the running instances, one `PluginProcessor` per `PluginId`, created and prepared on the message thread at the device rate with `MixRenderer::CHUNK_SAMPLES` as the largest block. A plugin that can't be created has no instance: its track plays without it and its data stays in the model. `is_missing(id)` tells that apart from a plugin that simply isn't running yet (no device), when `find_instance` also returns `nullptr`.
- The publisher asks the pool for each track's `PluginChain`. A chain is immutable once built (instances plus bypass flags); the pool hands out the same chain while nothing about it changed and builds a new one when it did, so the audio thread never sees a chain change under it and old chains die with their snapshot. Only the chain's own tail counter changes, on the audio thread.
- When the device rate changes, every instance is retired and recreated at the new rate instead of being prepared again while the audio thread may still be using it. Retiring keeps the instance's state, which is also what brings a plugin back with its sound after undoing its removal.
- A plugin's own settings (its knobs, its preset) live in the instance, not in the model, and are outside undo. Whatever changes them calls `mark_changed()` on the processor (from any thread: it's an atomic counter); each frame `PluginPool::take_changes` notices it and marks the project changed without an undo step, which is what drives the unsaved mark, the save question and autosave. Restoring a state when an instance is created doesn't count as a change.
- Plugin windows: `context.plugins.find_instance(id)` gives the running `PluginProcessor`, `create_editor()` its window contents (the plugin's own editor for VST3/LV2, our component for builtin effects, `nullptr` when there is none). Only on the message thread. Whoever shows it holds the processor's `shared_ptr` declared before the editor, so the editor is destroyed first, and closes the window when `find_instance(id)` no longer returns the same processor (plugin removed, rate change, project replaced).
- Before every save and autosave `PluginPool::store_states` copies the live states into the model. Opening, New and recovery clear the pool, since plugin ids are only unique within one project.
- `engine/plugins/builtin_plugins.h` lists our own effects (format `builtin`). VST3 and LV2 run through `HostedPlugin` (`engine/plugins/hosted_plugin`), which `PluginPool::create_hosted` makes from the `juce::PluginDescription` that `PluginLibrary::find(plugin)` returns. It asks the plugin for stereo in and out, then mono in and stereo out, then mono; plugins needing more channels are treated as missing. Plugins without their own editor get JUCE's generic one.
- The app scans for new or changed plugins at startup (`scanner.start(false)` in `AppContext`); it runs in the background and the browser shows its progress.
- A track's chain always gets two channels: a mono track copies left to right before its effects, so stereo effects (amp simulators, reverbs) keep their stereo output; pan on a mono track then works on both sides like balance.
- Effects get a `PluginPlayhead` with every block (playing, recording, position in device samples, tempo) for tempo-synced effects.
- `PluginLibrary` holds JUCE's format manager (VST3, LV2) and the list of installed plugins with the blacklist, cached in `~/.cache/voxalibera/plugins.xml`. `search(text)` gives the browser our effects plus installed ones (no instruments yet) as ready `Plugin` values for `add_plugin`.
- `PluginScanner` scans on its own thread. It runs the app binary again as `voxalibera --scan-plugins <format> <list file> <output file>`; that child writes a line before and after each plugin, so when it crashes or stops responding for 30 s the parent knows which plugin did it, blacklists it and starts a new child with the rest. Only new or changed plugins are scanned, unless `start(true)` rescans everything (which also retries the blacklist). Default folders per format plus `plugin-folders` from settings. UI polls `scanner.progress()` and reads `scanner.failures` for the report; when new plugins arrive, plugins that were missing get another try (`PluginPool::retry_missing`).

### Effect chain panel

- Above the mixer (View > Plugin Chain, `V`), for the first selected track: the track -> effects -> Output, scrolling sideways. Selecting a track elsewhere clears what's selected in the chain.
- What's selected in the chain lives in `Selection` like tracks and clips: `selection.plugin` (an effect) or `selection.gap` (the wire before position N).
- An effect: click selects, clicking it again opens its window; drag it onto a wire to move it. A wire: click selects it, clicking again or on its `+` opens the effect browser for that place. Enter opens whatever is selected in the chain (otherwise it still returns to start); Delete removes the selected effect and selects the wire where it was. The side menu does the same: Add (at the selected wire, after the selected effect, or at the end), Open, Remove.
- The effect browser is a dialog in the overlay: typing filters, arrows move, Enter or a double click inserts, and the new effect is selected so Enter opens it.
- Plugin windows are the one exception to "no separate OS windows": a plugin's editor is its own window. `PluginWindows` keeps them by `PluginId`, holds the processor so the editor dies first, and closes a window when its instance goes away (removed, rate change, other project).

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
