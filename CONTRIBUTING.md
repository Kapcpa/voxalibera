# Contributing

## Bugs

Open an issue: what you did, what happened, your distro and audio interface, and whether you're on JACK or ALSA (Settings > Audio). Attach `~/.local/state/voxalibera/voxalibera.log`. For crashes, a debug build (`./compile.sh --debug`) prints more to the terminal.

## Code

Fork, branch off `main`, open a pull request. Name the branch after what it does (`fix/meter-overflow`, `feature/clip-import`), keep one thing per PR, and say in it how you tested.

Code rules are in [AGENTS.md](AGENTS.md), a map of the source in [docs/code.md](docs/code.md). Run `clang-format -i` on the files you changed and `./test.sh` before pushing; CI checks both.

Pull requests are reviewed and squash-merged into `main`.

Contributions are licensed under AGPL-3.0-or-later, like the rest of the project.
