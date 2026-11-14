# Dreamyverb — Dreamy Reverb

A dreamy shimmer-reverb audio plugin built with JUCE (VST3 for Windows/Linux/macOS, AU for macOS).

## Features

- **Decay** — RT60 in seconds (0.1 – 10 s), physically mapped feedback
- **Size** — room size, controls the comb delay time (40 – 300 ms)
- **Damping** — high-frequency damping inside the reverb loop (100 Hz – 20 kHz)
- **Shimmer** — granular pitch-shift (0 – +12 st) inside the feedback loop for the classic octave shimmer
- **Mix** — dry/wet
- **HPF / LPF** — optional wet-signal filters (120 Hz / 8 kHz)
- **Depth / Rate** — slow LFO that modulates the feedback for tape-style drift
- **Bypass** — true bypass (resets the engine)
- **LOCK** — locks the five main knobs against accidental changes
- **Presets** — CELESTIAL VOID, CRYSTAL CAVE, ETHEREAL SHIMMER
- State (all parameters) is saved/restored with the DAW session

DSP lives in `Source/ReverbEngine.h` (no GUI dependencies) so it can be unit-tested standalone.

## Building

Requires CMake ≥ 3.22 and a C++17 compiler. JUCE 8.0.15 is fetched automatically
from GitHub if it is not already installed (`find_package(JUCE)` is tried first).

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

- Windows: `cmake -B build -G "Visual Studio 17 2022" -A x64` then `cmake --build build --config Release`
- Artifacts land in `build/DreamyReverb_artefacts/<Config>/VST3/` (and `AU/` on macOS)

Options:

| Option | Default | Meaning |
|---|---|---|
| `DREAMY_BUILD_TESTS` | `OFF` | Build the `ReverbTests` DSP test executable |
| `DREAMY_COPY_PLUGIN_AFTER_BUILD` | `ON` | Copy the plugin into the system plugin folders after building |

## Tests

```sh
cmake -B build -DDREAMY_BUILD_TESTS=ON
cmake --build build
./build/ReverbTests        # Windows: build\Release\ReverbTests.exe
```

The tests cover: bit-exact passthrough at mix 0, no self-oscillation, short/long
decay behaviour, worst-case stability (15 s of noise at maximum feedback),
block-size independence and mono safety.

## Continuous integration

The `Build Audio Plugin` GitHub Actions workflow builds the **Windows VST3** and
the **macOS AU + VST3**, runs the DSP tests and uploads them as artifacts.
It can be run manually from the *Actions* tab (`workflow_dispatch`).
