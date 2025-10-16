# AVS Cross-Platform Port

This repository hosts a cross-platform reimplementation of Winamp's Advanced Visualization Studio (AVS).
The legacy Nullsoft source is preserved under `docs/avs_original_source` for reference.

See [docs/README.md](docs/README.md) for build and contribution instructions.

## Live audio capture in `avs-player`

`avs-player` uses PortAudio for real-time audio capture. The player selects an
input device automatically when no explicit identifier is provided. The default
policy prefers the first full-duplex device whose reported default sample rate
matches 48 kHz; if none is available, the first capture-capable endpoint is
used.

Inspect available devices with:

```bash
./apps/avs-player/avs-player --list-input-devices
```

Select a specific device by index or by its exact name:

```bash
./apps/avs-player/avs-player --input-device 0
./apps/avs-player/avs-player --input-device "Built-in Microphone"
```

You can still override the capture sample rate via `--sample-rate <hz>`; using
`--sample-rate default` (the default behaviour) requests the device's preferred
rate.

## Deterministic Rendering & Golden Hashes

The repository includes an offscreen renderer for repeatable image captures that
do not require a windowing system. Rendering is seeded via the `AVS_SEED`
environment variable (default `1234`) so that the effect pipeline receives a
stable sequence of random numbers. A dedicated regression test renders the
`tests/regression/data/tiny_preset_fragment.avs` preset for ten frames at
320×240, computes MD5 hashes of the raw RGBA buffer, and compares them against
`tests/regression/data/expected_md5_320x240_seed1234.json`.

To regenerate the snapshot after changing the renderer, run the CLI tool from
the build directory:

```bash
./tools/gen_golden_md5 --frames 10 --width 320 --height 240 --seed 1234 \
  --preset tests/regression/data/tiny_preset_fragment.avs \
  > tests/regression/data/expected_md5_320x240_seed1234.json
```

The command prints a JSON payload containing the per-frame hashes. The same
snapshot check is wired into CTest via `offscreen_golden_md5_snapshot`, which
invokes the tool and verifies the output JSON matches the committed version.
