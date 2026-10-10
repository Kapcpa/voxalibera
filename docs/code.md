# Code map

What lives where in `source/`, and the functions worth knowing. Rules and architecture are in [AGENTS.md](../AGENTS.md); this file is for finding your way around. Layers depend only downwards: `app -> ui -> core -> engine -> platform`.

## app/

Starting the program, windows, commands and menus.

- `application` - the JUCE application: creates the log, theme, `AppContext` and the main window.
- `main_window` - the main window; owns commands, the 30 Hz frame loop and the modal overlay.
- `window_content` - lays out the menu bar, the main view and the overlay inside the window.
- `commands` - every command id with its name, menu and default shortcut.
- `main_menu` - builds the menu bar from the command table.
- `shortcut_text` - turns key presses into readable text (`Ctrl+S`, `Space`).
- `device_listing` - `--list-devices`: prints every audio device JUCE can see.

`MainWindow::actions` - map from command id to what it does; a command without an entry shows up greyed out.
`describe_shortcut(key)` - text for a shortcut, used by menus.

## core/

Application state: the project, edits, devices, recording, settings.

- `app_context` - `AppContext`, the one shared object: settings, engine, devices, project, edits, selection, recorder.
- `project/project` - `Project`: tempo, time signature, tracks, the project folder and change counter.
- `project/track` - `Track` and its input and channel helpers.
- `project/clip` - `Clip`: a range of an audio file placed on a track.
- `project/ids` - `TrackId`, `ClipId`: separate id types that can't be mixed up.
- `project/edit_history` - `Edit` and `EditHistory`: every change to the project, with undo and redo.
- `project/track_colours` - the palette new tracks take their colours from.
- `track_editor` - track actions: add, delete, rename, colour, volume, arm, mute, solo.
- `clip_editor` - clip actions: move the selection (in time and across tracks), delete.
- `selection` - which tracks and clips are selected; outside undo.
- `recorder` - runs a recording: arms, creates take files, hands them to the engine, turns them into clips.
- `media/take_files` - where takes go and how they're named.
- `media/peak_store` - waveform overviews per file, for drawing clips.
- `media/audio_file_pool` - opens each audio file once and shares it between snapshots; reopens a take once it's finished.
- `media/read_ahead` - on the disk thread, loads the next few seconds of every clip into memory before the audio thread needs it.
- `device_session` - opening devices, borrowing them from PipeWire, PipeWire properties for JACK.
- `engine_publisher` - turns the project into engine snapshots when it changes: solo, gains, pan, opened files, meter slots.
- `hardware_restore` - re-applies saved interface settings (input source, hardware monitor).
- `settings` - `settings.json`: device, test monitor, hardware controls.
- `log` - the log file under `~/.local/state/voxalibera/`.
- `xrun_watch` - logs dropouts, at most every two seconds.

`context.edits.begin(name, merge_key)` - starts an undoable change; everything until it goes out of scope is one undo step.
`edit.set(id, &Track::field, value)` - changes any track, clip or project field and remembers the old value.
`edit.add_track(track, position)` / `edit.remove_track(id)` - adds or removes a track.
`edit.add_clip(track, clip)` / `edit.remove_clip(id)` / `edit.move_clip(id, track)` - adds, removes or moves a clip to another track.
`context.tracks.add(new_track)` - creates a track below the selected one and selects it.
`context.clips.move_selected(move)` - moves all selected clips by a time and track offset, kept within limits.
`context.recorder.start()` / `stop()` - starts and stops recording on the armed (or selected) tracks.
`context.tracks.set_volume(id, db)` / `set_pan(id, pan)` - fader and pan, merged into one undo step while dragging.
`context.publisher.meter_of(id)` - which meter in `engine.mixer.track_meters` belongs to a track.
`context.devices.choose_output(name)` (and the other `choose_*`) - changes the device and saves it as the user's choice.
`project.find_track(id)` / `find_clip(id)` / `find_track_of(clip)` - lookups; return `nullptr` when missing.
`log_event(message)` - writes a timestamped line to the log; never call it from the audio thread.

## engine/

Everything the audio thread touches. Nothing here allocates, locks or touches files on the audio thread.

- `audio_engine` - `AudioEngine`: the audio callback; owns the device manager, transport, metronome, meters and snapshots.
- `engine_snapshot` - `EngineSnapshot`: the read-only copy of the project the audio thread plays from (tracks, clips, gains).
- `mix_renderer` - plays clips and monitored inputs through each track's gain and pan into the master, fills the meters.
- `audio_file` - a memory-mapped WAV file the audio thread can read from without system calls.
- `gain` - fader decibels to gain, the mono pan law and stereo balance.
- `snapshot_exchange` - swaps snapshots in atomically and frees old ones only after the audio thread is done.
- `transport` - play, stop, record, locate; commands in through a queue, position out through atomics.
- `recording_session` - the audio thread side of recording: writes inputs into each take's writer, collects peaks.
- `metronome` - a sample-accurate synthesized click.
- `musical_time` - samples to bars, beats and ticks and back; time signature parsing.
- `level_meter` - peak meters filled by the audio thread and read by the UI.
- `passthrough` - the test monitor: input straight to output, bypassing tracks.
- `lock_free_queue` - a fixed-size single-producer single-consumer queue.

`transport.play()` / `stop()` / `record()` / `locate(sample)` - safe to call from the UI; applied at the next audio block.
`snapshots.publish(snapshot)` - hands a new snapshot to the audio thread.
`to_musical_position(sample, tempo, rate)` / `to_sample(position, tempo, rate)` - converts between samples and bars.beats.ticks.

## platform/

Linux specifics. The only place that includes ALSA, sd-bus or PipeWire.

- `alsa_cards` - lists sound cards and matches device names to cards; finds a card's required sample rate.
- `alsa_controls` - reads and writes a card's hardware controls.
- `device_reservation` - borrows a card from PipeWire over D-Bus and gives it back.
- `pipewire_properties` - the properties our JACK client asks PipeWire for (buffer size, sample rate).
- `realtime_priority` - puts the current thread at real-time priority.
- `paths` - the XDG folders for state and data.

## ui/

Views. They read state every frame and call into `core/` on user input; they never change the project directly.

- `main_view` - the main window's zones: transport bar, tracks area, mixer.
- `mixer/mixer_view` - the master strip on the left, then one strip per track, scrolling sideways.
- `mixer/mixer_strip` - pan, values, fader with meter, mute, solo and a colour name plate; the master keeps the same sections so all faders line up.
- `mixer/meter_view` - a vertical two-channel meter.
- `transport_bar` - play, stop, record, position, tempo, time signature, metronome.
- `modal_overlay` - dims the window and shows a dialog in the middle.
- `theme` / `theme_look_and_feel` - all colours, applied to every JUCE widget; draws our own faders and knobs.
- `level_meter_view` - draws a `LevelMeter`.
- `tracks/track_area_view` - headers, ruler and timeline together; scrolling, zoom, following the playhead.
- `tracks/track_header` - one track's name, colour, arm, mute, solo and volume.
- `tracks/timeline_view` - lanes, grid, clips, live takes, playhead; clip selection and dragging.
- `tracks/timeline_viewport` - zoom and scroll state shared by ruler and timeline.
- `tracks/ruler_view` - bar numbers; click or drag to move the playhead.
- `tracks/clip_painter` - draws a clip with its waveform (one band per channel).
- `tracks/new_track_dialog` - the New Track dialog.
- `settings/settings_view` - the settings dialog with its sidebar.
- `settings/audio_page`, `device_picker`, `monitoring_section`, `status_section` - the Audio page.
- `settings/interface_page` - the interface's hardware controls.
- `settings/settings_layout`, `section_label`, `sidebar_button`, `settings_page` - shared pieces for settings pages.

`TrackAreaView::update()` - refreshes headers only when the project or selection changed; repaints the timeline while playing.
`paint_clip(graphics, bounds, appearance)` - draws any clip, recorded or live.

## tests/

Base tests for things that must never silently break: musical time, edit history, playback and mixing, the test monitor, device matching, PipeWire properties. Run with `./test.sh`.
