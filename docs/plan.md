# Plan

Rough development plan. Background in [research.md](research.md) and [direction.md](direction.md). Each milestone ends with something usable, so the app is worth opening at every step.

## Where we are

- Low-latency audio I/O through JACK (PipeWire) or direct ALSA, with device borrowing, device quirks handled, hardware controls per interface.
- Main window skeleton (transport, tracks, mixer zones), menu bar driven by commands, settings dialog, shared theme.
- Logging, sanitizer debug build, base tests, scripts.

## Architecture to settle before milestone 1

Short design notes, decided once and then followed everywhere.

### Threads

| Thread | Does | Never does |
|---|---|---|
| Message (UI) | owns the project model, runs commands and undo, builds engine snapshots, saves | blocks on audio or disk |
| Audio | renders the current engine snapshot | allocates, locks, logs, touches files |
| Disk read | reads audio files ahead of the playhead into ring buffers | touches the model |
| Disk write | writes recorded audio from ring buffers to files | touches the model |
| Plugin scanner | separate process, so a crashing plugin can't take the app down | |

### Project model (`core/`)

- `Project` holds `Track`s; a track holds `Clip`s; a clip references a media file plus a range. Takes are clips on the same track in separate lanes.
- A track is created as **mono** or **stereo**. Mono tracks record one input channel and play it on both sides, placed by the pan control. Stereo tracks record two channels.
- Plain structs with stable ids, owned by the message thread. Nothing in the model is touched by the audio thread.
- Every change is a command (`add_track`, `move_clip`, `set_volume`...) that knows how to undo itself. Views call commands, never mutate the model directly.
- The model keeps a change counter; views redraw when it changes (polling, as everywhere else).

### Engine (`engine/`)

- After a model change, the message thread compiles an **engine snapshot**: a flat, read-only description of what to play (tracks, clips with sample positions, gains, routing).
- The snapshot is handed to the audio thread through an atomic pointer swap. The old one is released later on the message thread, never on the audio thread.
- Transport commands (play, stop, record, locate) go through a lock-free queue; the audio thread publishes position and state through atomics.
- Time inside the engine is in samples. Bars and beats exist only in the model and UI, through a tempo map (constant tempo first).
- Latency compensation and multi-core rendering are designed into the snapshot format from the start, even if the first version renders on one thread.

### Recording

- The audio thread copies armed inputs into lock-free ring buffers; the disk write thread turns them into WAV files in the project's `audio/` folder.
- Recorded clips are placed compensating for the device's reported input and output latency, so takes line up with what was heard.
- Monitoring follows the armed track: arming a track means hearing its input through that track's own effect chain; arming a different track switches to its chain. The global monitoring switch in settings is temporary (it may stay as a test or direct monitor).

### Project files

Full design: [projects.md](projects.md). Where the installed app and the user's files live: [distribution.md](distribution.md).

- A project is a folder: `<name>.vxl` (readable JSON, kebab-case keys) plus `audio/`, `plugins/`, `backups/`, `autosave/`, `cache/`.
- Saving uses temporary-file-then-rename (already the rule). Autosave every few minutes to `project.autosave.json`.
- Opening never fails because something is missing: missing audio files and plugins become placeholders that keep their data.

### Plugins (milestone 2, but shapes the engine)

- VST3 and LV2 through JUCE, CLAP later with our own host code.
- Scanning in a separate process with a blacklist and a readable report.
- Every plugin parameter, track volume, pan and send is addressed the same way (`ParameterAddress`), which automation, macros and controllers will all use.

## Milestones

### 1. Record a take

The core loop: arm a track, record, hear it back, record again.

- Transport bar: play, stop, record, return to start, position (bars:beats and time), tempo, metronome toggle with count-in.
- Constant tempo and time signature; the ruler shows bars and beats.
- Audio tracks, mono or stereo: add, remove, rename, color; header with arm, monitor, mute, solo, volume.
- Recording to WAV, with latency-compensated placement.
- Playback of recorded clips from disk.
- Timeline: ruler, track lanes, clips with waveforms, playhead, click to set position, scroll and zoom.
- Select, move and delete clips, so a bad take can be thrown away or nudged before full editing exists.
- Mixer: one strip per track with volume, pan, mute, solo, meter, plus a master strip.
- New, open, save, save as, recent projects. Undo and redo for everything above.

### 2. Effects

- Plugin hosting (VST3, LV2), scanner process, plugin browser with search.
- Insert chains per track, in a new design (not Logic's sidebar). Ideas to try: the chain expanding under the track header, a chain row in the mixer strip, a dedicated chain view in the bottom panel.
- Plugin windows, bypass, presets.
- Sends, buses, groups; latency compensation across all of them.
- Monitoring through effects while recording (amp simulators for guitar).
- CPU: effect chains run per track inside `MixRenderer::render_track` (after clips and input, before volume and pan). From the start, chains sleep when the track is silent and their tail has ended. Then, in order of need:
  - freeze: render a track with its effects to a file in the background and play that instead, until it's unfrozen;
  - spread independent tracks across processor cores;
  - anticipative processing: render tracks that aren't live ahead of time on worker threads, keeping only live tracks at low latency.

### 3. Edit and arrange

- Trim, split, duplicate clips; fades and clip gain.
- Channel operations on clips: left only, right only, swap, mix down to mono.
- Snap and grid, loop range, markers.
- Takes: cycle recording into take lanes, pick or swipe-comp the best parts.
- Selection across tracks, copy and paste.
- Export the mix to WAV (and later FLAC/MP3).

### 4. MIDI

- Instrument tracks, MIDI input devices, MIDI recording.
- Piano roll with velocity, quantize, basic transforms.
- Takes and comping for MIDI as well.

### 5. Automation and control

- Automation lanes for any parameter (read, write, touch, latch), curve editing.
- Macros and modulators (LFO, envelope follower) on any parameter.
- MIDI controller mapping.

### 6. Ready to open up

- Key remapping UI and command palette.
- A proper notification system for errors and warnings (non-blocking, with details on demand) instead of the plain message dialog and the transport bar note.
- Crash recovery from autosave, plugin crash isolation report.
- Packaging (AUR, Flatpak or AppImage), a website, contributor docs.
- Make the GitHub repository public.

### Later

Clip launcher (session grid), CLAP, out-of-process plugin sandboxing, time-stretch (Rubber Band), stem separation (offline, open models), DAWproject import/export, scripting (Lua), tempo changes and time-signature changes, video.

## Decisions

- Effects come before full editing: recording guitar through an amp simulator matters more early on than trimming clips. Selecting, moving and deleting clips is already in milestone 1.
- Tempo, bars and a metronome are part of milestone 1; recording to a click is the normal case.
- Tracks are mono or stereo, chosen when created. Splitting or swapping channels happens later as clip operations.
- Recording: Ctrl+R records (R arms the selected track). If no track is armed, recording arms the selected one. A count-in toggle comes later.
- The app starts with an empty project; File > Open Recent lists the last ones (a start screen comes later). Projects default to `~/Music/Voxalibera/<name>/<name>.vxl`.
- Arming is saved in the project, but tracks open disarmed (arming means monitoring, so an armed track with a laptop microphone could feed back). A setting to restore it can come later.
- Sample rate: a project keeps the rate of its first recording. In JACK mode the app asks PipeWire to run at the project's rate; when the card can't run it (the UX1 only runs 44.1 kHz), and in ALSA mode, clips at another rate stay silent with a clear message. Real-time resampling in the engine is a firm later task (with the export dialog), so ALSA and any device work too.
- Projects use the `.vxl` extension. Imported audio is copied into the project. A project keeps the sample rate it was created at; export can convert to another rate.
