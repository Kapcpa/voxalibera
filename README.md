# custom-daw

An open-source DAW for Linux in early development. Right now it opens an audio interface with low latency, passes the input through to the output, and shows levels, latency and device diagnostics.

## Build

```
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/custom_daw_artefacts/Release/custom-daw
```

Requires `cmake`, `ninja`, `alsa-lib`, `libsystemd` and `pipewire-jack`.

Code rules and architecture: [AGENTS.md](AGENTS.md).
