# Projects: saving, loading, media, plugins

How a project lives on disk and how it's rebuilt when opened. Part of the [plan](plan.md).

## Goals

1. **Never lose a take.** A crash or power cut during recording or saving loses at most a few seconds.
2. **Always open.** Missing audio files, missing plugins or a newer/older format never stop a project from opening; missing parts become placeholders that keep their data.
3. **Portable.** A project folder can be zipped, moved to another machine or put in git, and still works.
4. **Readable.** The main file is plain JSON that a person can read and a diff tool can compare.
5. **Non-destructive.** Recorded and imported audio files are never modified. Edits only change the project file.

## The project folder

```
My Song/
  my-song.vxl              the project: readable JSON
  audio/                   recorded takes and imported audio, referenced by the project
  impulses/                impulse responses loaded into the Cabinet effect, copied in like imports
  backups/                 previous saves, newest last (rolling, e.g. the last 10)
  autosave/                autosaved project, written every few minutes while there are unsaved changes
  cache/                   waveform overviews; safe to delete, rebuilt automatically
  .lock                    present while the project is open; left behind means the app crashed
```

`cache/` and `autosave/` can be ignored in version control; everything else is the project.

## The project file

JSON with `kebab-case` keys, stable key order and pretty printing, so diffs show real changes only.

```json
{
  "format": 1,
  "app-version": "0.3.0",
  "sample-rate": 44100,
  "tempo": 120.0,
  "time-signature": [4, 4],
  "tracks": [
    {
      "id": 7,
      "name": "Guitar",
      "color": "#d9813b",
      "channels": "mono",
      "input": {"device-channels": [0]},
      "volume-db": -3.0,
      "pan": 0.0,
      "mute": false,
      "solo": false,
      "plugins": [
        {
          "id": 12,
          "format": "VST3",
          "identifier": "VST3-Archetype-...",
          "name": "Neural DSP Archetype",
          "manufacturer": "Neural DSP",
          "version": "3.1.0",
          "bypass": false,
          "state": "1024.base64..."
        }
      ],
      "clips": [
        {
          "id": 31,
          "file": "audio/guitar-take-3.wav",
          "start": 352800,
          "source-offset": 0,
          "length": 1764000,
          "gain-db": 0.0,
          "lane": 0
        }
      ]
    }
  ],
  "markers": [{"id": 40, "name": "Chorus", "position": 3528000}]
}
```

- **Ids** are integers unique within the project and never reused. Everything that refers to something (clips to files, sends to tracks, automation to parameters) uses ids, never names or list positions.
- **Positions and lengths are in samples** at the project's sample rate. Bars and beats are derived from the tempo, so changing the tempo of a recorded song doesn't silently move audio.
- **`format`** is an integer bumped whenever the structure changes. Opening an older project runs small migration steps (format 1 to 2, 2 to 3...). A project from a newer app version opens read-only with a warning instead of being damaged.
- The undo history isn't saved.

## Audio files

- **Recording:** each take is written to `audio/` while it's being recorded, by the disk write thread. File names are readable and unique: `<track>-take-<n>.wav` (`guitar-take-3.wav`).
- **Crash safety while recording:** the WAV header is updated every few seconds, so a crash leaves a valid file with all but the last moments. On the next open, files in `audio/` that no clip references are offered for recovery ("2 recorded takes were found that aren't in the project").
- **Importing:** audio dragged in from elsewhere is **copied** into `audio/` by default, so the project stays self-contained. WAV files are copied as they are; other formats (FLAC, Ogg, MP3, AIFF) are converted to WAV, since playback reads only WAV. The sample rate is kept: files at a different rate are resampled while playing, like any clip.
- **Missing files:** the clip stays, drawn as missing, and plays silence. A "find missing files" action searches by file name and lets the user point at a folder.
- **Before the first save** a project lives in a temporary folder, `~/.local/share/voxalibera/unsaved/<date-time>/`, so takes have a home from the first recording. The first "save as" moves the whole folder to the chosen place; after a crash the folder is still there to recover.
- **Unused files** are never deleted silently:
  - undoing a recording or deleting clips and tracks keeps the files, because undo and redo need them;
  - "Clean up project" (File menu) lists files no clip uses, with their sizes, and deletes them after confirmation; undo can't go back past that point;
  - closing a saved project keeps everything, and mentions it once when unused takes pass a size threshold;
  - closing an unsaved project with "don't save" deletes its temporary folder, after a clear warning.

## Plugins

- Each plugin instance stores **who it is** (format, identifier, name, manufacturer, version) and **its state** (the binary blob the plugin gives us, base64 under `state`) in the JSON. `format` + `identifier` find the plugin: for VST3/LV2 the identifier is JUCE's `PluginDescription::createIdentifierString()`, for our own effects (`builtin`) a kebab-case name like `overdrive`.
- The state lives inside the `.vxl` rather than in separate files so that a save, an autosave and every backup are each one complete, consistent file: separate `plugins/<id>.bin` files would be overwritten by the next save while older backups still pointed at them. The cost is one long line per plugin, which still diffs line by line.
- **Missing plugin:** its track plays as if the plugin weren't there; the model keeps its identity and state untouched, so it saves back unchanged. When the plugin is installed later, it loads with its state.
- **Different version installed:** load it with the saved state; most plugins handle their own old states. Note the version change in the log.
- **Plugin that crashes on load:** marked as failed for this session, treated like a missing plugin, reported to the user; the project still opens.

## Saving

1. Write the new project file to a temporary file, then rename it over the old one (already the rule for every file we write).
2. Before replacing it, the previous version moves into `backups/` with a timestamp; only the newest N are kept.
3. Just before writing, the live state of every plugin is copied into the model (`PluginPool::store_states`), so the file always has the current sound.
4. **Autosave:** every few minutes, only when there are unsaved changes, into `autosave/`. Never overwrites the real project file.
5. Saving runs on the message thread from a snapshot of the model; the audio keeps playing.

## Opening (rebuilding a project)

1. **Lock:** if `.lock` exists, the app crashed last time; offer the autosave if it's newer than the project file. Then create our own `.lock`.
2. **Read and migrate:** parse the JSON, run migrations up to the current format.
3. **Validate:** check that ids are unique and references point at things that exist; anything broken is dropped with a note in the log, never a failed open.
4. **Build the model** in `core/`; the UI can show the project now.
5. **Media:** open readers for the audio files (lazily, on the disk read thread); load waveform overviews from `cache/` or compute them in the background.
6. **Plugins:** create plugin instances and restore their states in the background; tracks play dry until their chain is ready, then switch in without a click.
7. **Engine:** compile the first engine snapshot and hand it to the audio thread.
8. **Recovery check:** offer unreferenced takes found in `audio/`.

## Sample rate

- A project keeps the sample rate it was created at.
- If the audio device runs at a different rate when the project opens, playback is resampled in the engine and the user is told once, with an option to switch the device.
- Recording always happens at the project rate; with PipeWire the app asks for that rate.
- Exporting lets the user pick the output sample rate and bit depth (44.1/48/96 kHz, 16/24/32-bit); the mix is converted while exporting.

## Kept separate from projects

- **App settings** (device, interface controls, window layout): `~/.config/voxalibera/`.
- **Plugin scan results and blacklist:** `~/.cache/voxalibera/` (rebuildable).
- Projects never store machine-specific things like device names; tracks store input channels by number.

## Code layout

```
core/project/      model structs (Project, Track, Clip, Insert...) and the commands that change them
core/project_file  reading and writing the JSON, migrations, validation
core/media/        the audio file pool: naming, importing, missing-file handling, recovery of takes
core/recovery      lock file, autosave, backups
```

## Later

Templates, saving the undo history, DAWproject import and export, collecting all used plugins into a report, project archives (zip with everything).
