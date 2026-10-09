# Research: DAWs (October 2026)

What popular DAWs share, how they differ, what users love and hate, and what that means for Voxalibera.

**Sources and gaps:** web research across official manuals, Sound On Sound, MusicRadar, MusicTech, KVR, Gearspace, the Ableton, Ardour and Bitwig forums, Production Expert and Hacker News. **Reddit could not be read** (fetching was blocked), so beginner, FL Studio and Logic voices are under-represented; it's worth reading r/linuxaudio, r/Logic_Studio and r/Reaper by hand. A few shortcut and pricing details came from single sources.

## Current versions

| DAW | Version | Platforms | Price model |
|---|---|---|---|
| Logic Pro | 12.3 | macOS (Apple Silicon) | $199.99 one-time, free updates since 2013; also a $12.99/mo Apple bundle |
| Ableton Live | 12.3 (12.4 beta) | Win, Mac | 3 tiers, $99 to $749, paid major upgrades |
| FL Studio | 2025 | Win, Mac | 4 tiers, ~$99 to ~$499, lifetime free updates |
| Reaper | 7 | Win, Mac, **Linux** | $60 / $225, honor-system trial, free updates to 8.99 |
| Cubase | 15 | Win, Mac | 3 tiers, $99 to $579, paid majors |
| Bitwig Studio | 6 | Win, Mac, **Linux** | perpetual + 12-month upgrade plan |
| Fender Studio Pro (was Studio One) | 8 | Win, Mac, Linux beta | $199.99 perpetual or subscription |
| Ardour | 9 | Win, Mac, **Linux** | GPL, pay-what-you-want binaries |

Only Reaper, Bitwig and Ardour are mature on Linux.

## The baseline every serious DAW has

- **Views:** arrange/timeline, mixer, piano roll with velocity lane, audio editor, browser. A clip launcher next to the timeline is spreading (Ableton, Bitwig, Logic Live Loops, Studio Pro, Ardour 9).
- **Objects:** audio/instrument/bus/folder tracks, non-destructive clips, insert chains, pre/post sends, automation lanes (read/write/touch/latch), markers, tempo and time-signature track, loop range, metronome with count-in, quantize with swing, bounce/freeze/export stems.
- **Recording:** takes and comping (strong in Logic, Cubase, Studio One, Reaper; weak in FL, audio-only in Bitwig).
- **Project:** project file plus media folder, autosave, templates, a start window with recent projects, templates and demos.
- **Remappable key commands** in every major DAW.

### Conventions a new DAW must not break

1. Space plays/stops; Ctrl+Z, Ctrl+S, Ctrl+C/V/X, Delete behave as everywhere. The record key is **not** standard (R, F9, Numpad *, Ctrl+R).
2. Drag from the browser: a plugin or sample on empty space creates a track, on a track it adds an insert or a clip. Files from the OS file manager import at the drop position.
3. Track header: color, name, then Mute / Solo / Arm / Monitor, meter and volume on wider headers.
4. Clips: edges trim, top corners fade, a handle or line for clip gain, Alt-drag copies, double-click opens the editor.
5. Grid adapts to zoom, a visible snap toggle, a modifier temporarily bypasses snap.
6. Mouse wheel scrolls, Ctrl+wheel zooms, pinch zooms, dragging vertically on the ruler zooms. Wheel-zooms-by-default frustrates people.
7. Faders in dB with 0 dB = unity, double-click resets to default, Shift-drag for fine adjustment, inserts above sends.
8. Right-click context menus everywhere.
9. Track color flows to its clips and mixer strip.
10. Undo covers everything, including mixer moves.
11. Editing never alters source audio files.
12. One window with toggleable zones (inspector, editor, mixer, browser). FL Studio's floating windows are the outlier and a beginner complaint.

## What makes each one special

**Logic Pro** (the style we like, details below): approachable yet deep, one clean window, library-first sound loading, great comping, huge content for the price.

**Ableton Live:** Session View clip launching next to the Arrangement, reliable warping, Racks and Macros, Max for Live. Weak mixer and audio editing; best features locked to the Suite tier; no Linux, no CLAP.

**FL Studio:** pattern workflow (Channel Rack step sequencer, patterns painted into a free-form Playlist), the most praised piano roll (ghost notes, scale and chord tools), lifetime free updates. Weak at recording audio or MIDI and at band work.

**Reaper:** small, fast, stable; one track type that can be anything (audio, MIDI, folder, bus); everything is an **action** that can be chained, keyed and scripted (Lua, EEL2, Python); JSFX text-based plugins compiled on the fly; any-to-any routing; native Linux, CLAP, VST3, LV2. The default look and menus are the main reason people quit it.

**Cubase:** deepest MIDI and orchestral toolset (Expression Maps, Logical Editor, Chord Track, VariAudio, score editor), Modulators since v14. Overwhelming for newcomers; no Linux, no CLAP.

**Bitwig Studio:** universal modulation (any modulator onto any parameter, per voice and per note), The Grid modular environment, nested devices, **per-plugin crash sandboxing**, early MPE, co-creator of CLAP, native Linux. Comping is audio-only; weak groove tools and pitch correction.

**Fender Studio Pro (Studio One):** drag-and-drop everything, Arranger Track to move song sections, Scratch Pads as alternate timelines, an integrated mastering page, built-in Atmos. Ageing stock plugins; the Fender rebrand upset users.

### Same problem, different answers

- **Arrangement:** two views (Live, Bitwig), patterns plus free playlist (FL), linear plus arranger sections (Cubase, Studio One), linear with regions (Reaper).
- **Routing:** any track to any track with 64 channels (Reaper), separate mixer tracks (FL), nested chains (Bitwig), drag a send onto a channel (Studio One).
- **Modulation:** universal per-voice (Bitwig, now copied by Cubase), parameter-mapping devices (Live, FL), per-parameter LFO and linking (Reaper).
- **Plugin crashes:** sandboxed (Bitwig, Reaper optional) vs one process (most others).
- **Extensibility:** Reaper's actions + scripts + JSFX and Ableton's Max for Live are the ecosystems people stay for.

## Logic Pro in detail

**Layout:** one window. Control bar with transport and the LCD (position, tempo, key, signature) on top; Library on the left (Y); Inspector with the selected track's channel strip **and its destination strip** (I); tracks area with global tracks (arrangement, markers, tempo, signature, chords) in the center; bottom pane for Mixer (X), Editors (E, piano roll P), Smart Controls (B); right pane for browsers (F) and lists. Option-K opens the key-command editor, T the tool menu.

**Why it feels clean:** dark low-contrast grey with color reserved for regions; flat icons; few controls visible by default; advanced tools hidden behind a setting; Smart Controls show the important knobs of a whole patch so beginners never open plugin windows; choosing a sound loads a whole ready-made channel strip.

**Workflow worth knowing:** tracks vs non-destructive regions; Track Stacks (folder vs summing bus); take folders with swipe comping; capture of performances played while not recording; Flex Time and Flex Pitch; Smart Tempo; Live Loops; Step Sequencer; Drummer and Session Players driven by the Chord Track; Stem Splitter; left-click tool plus Cmd-click tool plus a marquee tool for time ranges; screensets.

**What Logic users love:** value and free updates, comping and editing speed, approachable but deep, stock plugins and patches, low-latency mode while tracking.

**What Logic users hate:** latency compensation gaps (sidechains, automation on latent plugins, parallel buses), CPU overloads from one core pinned to the selected track, menu diving to add effects, confusing I/O and monitoring setup, the legacy Environment, region vs track automation confusion, Mac-only, bugs after updates.

**Copy (ideas are free):** one window with single-key toggles; inspector with two channel strips; patch = whole channel strip with instant preview; macro panel per patch; two-tool mouse model plus marquee; take folders with swipe comping; folder vs summing stacks; global tracks; simple mode by default; track color into regions.

**Do better than Logic:** correct latency compensation everywhere; searchable insert for plugins and patches (command palette); multithreaded processing; a visible I/O and monitoring model; one automation model; scriptable MIDI instead of the Environment; open project format; real Linux audio.

**Don't copy literally:** names (Logic, Smart Tempo, Drummer, Session Players, Flex Time/Pitch, Track Stack, Live Loops, Smart Controls, Apple Loops), Apple's icons and SF Symbols, the exact LCD styling, any Apple sound content. Use neutral names (time stretch, pitch editor, comping, folder/bus groups, macros) and our own palette and icons so it doesn't read as a Logic clone. Not legal advice.

## What users complain about

Ranked by how many independent sources raised it:

1. **Plugin crashes and scanning.** The single most common complaint across all DAWs: scans crash, plugins disappear, one bad plugin kills the session. Bitwig's sandboxing is the reference users name.
2. **Comping and take lanes**, for audio **and** MIDI (Bitwig users' top request).
3. **Browsers without tagging**, favourites or good preview for samples, presets and plugins.
4. **Project portability and version control:** switching DAWs means exporting audio; nobody can diff projects; projects should open even when plugins are missing; requests for DAWproject support.
5. **Editing fundamentals:** inserting and reordering tracks, nested folders, overlapping clips, bezier automation, reversible freeze and bounce, multi-track editing.
6. **Keyboard-driven work** with remappable keys (still an open request in Ableton).
7. **Several projects open at once** (tabs).
8. **Built-in utilities:** loudness meter, level-matched A/B, mono sum, per-channel polarity.
9. **Using all CPU cores** instead of overloading one per track.
10. **Learning curve**, and defaults that look complicated (Reaper's menus).
11. **Licensing:** subscription-only (Pro Tools backlash), iLok, licences scattered over accounts.
12. **Data safety:** losing work after crashes or power cuts.

### What people won't give up

Reaper's stability and scripting, Bitwig's modulators and sandboxing, Ableton's session view and macros, Logic's intuitive interface, Studio One's comping and drag-and-drop, trackers' keyboard speed. In a KVR poll of ~1,200 votes Reaper, Live and Bitwig were about equal (15 to 17% each).

### Linux specifically

- **Plugins:** big vendors (NI, iZotope, Waves) are absent; iLok doesn't exist; u-he, Toneboosters, Kazrog and Pianoteq are the bright spots. yabridge works but is fragile (Wine versions break GUIs, higher latency needed).
- **Audio setup:** PipeWire is seen as having fixed most of it; remaining friction is obscure config files, DAWs that don't show PipeWire properly, audio lost after suspend.
- **HiDPI and Wayland:** tiny or misplaced plugin windows, fractional scaling cutting off plugin UIs, JUCE VST3 child windows at the wrong scale. Must be tested early.
- **Hardware:** class-compliant devices work; vendor control panels don't exist.
- **Open-source DAWs:** Ardour is solid for audio but its **MIDI is the main complaint** (lost notes, CC problems, a weak piano roll); LMMS can't record audio; Zrythm spent years unstable; Qtractor looks dated. Nobody owns "good MIDI + good modern UI + open source on Linux".

## Trends 2024 to 2026

Stem separation became standard (FL, Studio One, Live, Cubase, Logic); AI assistants and session players; sample-store integration (Splice); CLAP adoption (Bitwig, Reaper, FL, Studio One; not Ableton or Cubase); modulation everywhere; step sequencers inside linear DAWs; built-in Atmos; audio over the network (Ableton Link Audio). Cloud collaboration is still marginal.

## What this means for Voxalibera

### Positioning

A gap nobody fills: **an open-source, Linux-first DAW with Logic's clarity, strong MIDI and modern plugin hosting.** Ardour has the audio but not the MIDI or the approachable UI; Reaper has the power but not the look; Bitwig is closed and expensive. Our edge besides being free: clean Logic-like design, being the best Linux citizen (PipeWire, device handling, HiDPI), crash-proof plugins, and an open, diffable project format.

### Decide early (expensive to retrofit)

1. **Action system:** every command is a named action, triggered by menus, keys, a command palette and later scripts; remappable keys from day one.
2. **Unified parameter model:** one way to address any parameter (track, plugin, send), used by automation, macros, modulation and controllers.
3. **Project format:** human-readable text (JSON), diffable, versionable, opens with missing plugins (keeps their state and shows placeholders); DAWproject import/export later.
4. **Undo through commands:** every model change is a command (fits the "views don't own data" rule).
5. **Latency compensation** designed into the audio graph from the start, including sidechains and automation.
6. **Multithreaded graph:** parallel processing across tracks without pinning one track to one core.
7. **Plugin hosting:** scanning in a separate process with a blacklist and a readable report; out-of-process hosting (sandbox) as the next step. Formats: VST3 and LV2 (JUCE hosts both), CLAP (not in JUCE, needs our own host code against the CLAP C API).
8. **Non-destructive media handling** and crash-safe saving (autosave, atomic writes, backups).
9. **HiDPI and Wayland/XWayland testing** of our UI and plugin windows from the first plugin window.

### Build first (core loop people need)

Timeline with audio and MIDI clips, mixer with inserts and sends, piano roll, recording with take lanes, automation, browser with tags and preview, project save/load, export. Keep the Logic-like single window with toggleable zones.

### Defer or use existing libraries

Time-stretching (use Rubber Band), stem separation (an open model like Demucs as an offline job), a modular environment, score editor, Atmos, ARA, video, bundled instruments and content, cloud collaboration.

## Open questions for us

1. Who is the first user: guitar/band recording (UX1 workflow), beat making, or both?
2. Linear timeline only, or a clip launcher too (and if so, when)?
3. How far to follow Logic's layout and keys vs Reaper/Ableton conventions?
4. License: JUCE's open-source license is AGPLv3, so Voxalibera is AGPLv3 (or buy a JUCE license).
