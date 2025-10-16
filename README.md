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
