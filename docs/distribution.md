# Distribution: what the finished app looks like

Where an installed Voxalibera keeps its files, where it keeps the user's files, and how it's packaged. Follows the Linux conventions (FHS for the installed program, XDG for per-user data), so it behaves like any other well-made Linux app.

## Three kinds of files

| Kind | Who writes it | Where |
|---|---|---|
| **The program** | the package, read-only | `/usr` (or `/usr/local`, or inside an AppImage/Flatpak) |
| **User data** | the app, per user | `~/.config`, `~/.local/share`, `~/.local/state`, `~/.cache` |
| **Projects** | the user | wherever they choose; default `~/Music/Voxalibera/` |

## The installed program

```
/usr/
  bin/
    voxalibera                                   the app
  share/
    voxalibera/
      sounds/                                    metronome clicks, test tones
      templates/                                 factory project templates
      presets/                                   factory presets for our own devices
      themes/                                    colour themes
    applications/io.github.kapcpa.Voxalibera.desktop        app menu entry
    icons/hicolor/scalable/apps/io.github.kapcpa.Voxalibera.svg
    icons/hicolor/<size>/apps/io.github.kapcpa.Voxalibera.png
    metainfo/io.github.kapcpa.Voxalibera.metainfo.xml       description for software centres
    mime/packages/io.github.kapcpa.Voxalibera.xml           ".vxl is a Voxalibera project", double-click opens it
    licenses/voxalibera/LICENSE
```

- **App id** `io.github.kapcpa.Voxalibera`: the reverse-domain name Linux desktops, Flatpak and software centres use. It's tied to the GitHub account; if the project later gets its own domain (voxalibera.org was free when checked), it can move to `org.voxalibera.Voxalibera`.
- **Fonts, icons and the default click sound are embedded in the binary** (JUCE binary data), so the app always starts even if `share/` is missing. Bigger or user-visible content (templates, presets, themes, extra sounds) lives in `share/voxalibera/` as plain files that packagers and curious users can see.
- **Relocatable:** the app finds `share/voxalibera` relative to its own executable (`<exe>/../share/voxalibera`), so the same build works installed in `/usr`, in `/usr/local`, in an AppImage or straight from the build folder (which points at the repository's `resources/`).
- **Spread by file type** is the Linux norm (Firefox, GIMP, Ardour do the same): the package manager tracks every file, and the desktop finds menu entries, icons and file types in their standard places. A self-contained install in one folder (`/opt/voxalibera/{bin,lib,share}`, like Bitwig in `/opt/bitwig-studio`) works too, because the app only looks relative to itself; only the `.desktop` file, icon and mime type must sit where the desktop looks.

## Other systems

Not a goal now, but the same three kinds of files map cleanly onto Windows and macOS. Code never builds these paths itself: it asks `platform/system` for "the settings folder", "the cache folder", "the resources folder", and a port only adds a new implementation there.

| What | Linux | Windows | macOS |
|---|---|---|---|
| Program | `/usr/bin`, `/usr/share/voxalibera`... or `/opt/voxalibera` | `C:\Program Files\Voxalibera\` | `Voxalibera.app` |
| Settings | `~/.config/voxalibera` | `%APPDATA%\Voxalibera` | `~/Library/Application Support/Voxalibera` |
| Cache | `~/.cache/voxalibera` | `%LOCALAPPDATA%\Voxalibera\Cache` | `~/Library/Caches/Voxalibera` |
| Logs | `~/.local/state/voxalibera` | `%LOCALAPPDATA%\Voxalibera\Logs` | `~/Library/Logs/Voxalibera` |
| Projects | `~/Music/Voxalibera` | `Documents\Voxalibera` | `~/Music/Voxalibera` |
| Menu, file types | `.desktop` + mime type | Start menu shortcut + registry, set by the installer | `Info.plist` in the bundle |

## Per-user data

| Folder | Contents | Safe to delete? |
|---|---|---|
| `~/.config/voxalibera/` | `settings.json` (devices, interface controls, window layout), `keys.json` (remapped shortcuts), user themes | resets preferences |
| `~/.local/share/voxalibera/` | user templates, user presets, user sounds | loses user content |
| `~/.local/state/voxalibera/` | `voxalibera.log`, recent projects list, crash reports | yes |
| `~/.cache/voxalibera/` | plugin scan results and blacklist (`plugins.xml`) | yes, rescans |

Each honours its `XDG_*_HOME` variable. Projects never live here.

## Plugins the app looks for

Standard locations, the same ones every Linux host uses, plus whatever the user adds in settings:

| Format | Folders |
|---|---|
| VST3 | `~/.vst3`, `/usr/lib/vst3`, `/usr/local/lib/vst3` |
| LV2 | `~/.lv2`, `/usr/lib/lv2`, `/usr/local/lib/lv2` (and `LV2_PATH`) |
| CLAP (later) | `~/.clap`, `/usr/lib/clap` (and `CLAP_PATH`) |

## Packaging, in order

1. **From source:** `cmake --install build --prefix ~/.local` puts everything in the layout above, under the user's home. Works today once install rules exist.
2. **AUR** (Arch, CachyOS, Manjaro): a `PKGBUILD` that builds from a release tag. Smallest effort, and it's the user's own distribution.
3. **AppImage:** one file that runs on most distributions without installing; good for "try it out" downloads from GitHub releases.
4. **Flatpak / Flathub:** the widest reach. Needs care: plugin folders must be opened to the sandbox, the D-Bus device reservation needs permission to talk to `org.freedesktop.ReserveDevice1.*`, and audio goes through PipeWire (which works in Flatpak).

## In the repository

```
voxalibera/
  source/          code
  tests/           base tests
  resources/       everything that ends up in share/ or is embedded:
    sounds/  templates/  presets/  themes/  fonts/  icons/
    linux/         .desktop file, metainfo, mime type
  docs/
  packaging/       PKGBUILD, AppImage and Flatpak recipes (later)
  CMakeLists.txt   build plus install rules
```
