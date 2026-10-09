# custom-daw

An open-source DAW for Linux in early development. Right now it opens an audio interface with low latency, passes the input through to the output, and shows levels, latency and device diagnostics.

## Build

```
./compile.sh    builds and runs
./run.sh        runs the last build
```

Requires `cmake`, `ninja`, `alsa-lib`, `libsystemd` and `pipewire-jack`.

Code rules and architecture: [AGENTS.md](AGENTS.md).
