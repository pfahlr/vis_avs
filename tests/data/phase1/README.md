# Phase-one binary preset corpus

This folder mirrors the first group of legacy render-list scenarios that the parser
needs to ingest.  Each preset stores the original render-list metadata block
(clear flags, blend routing, beat gating, etc.) along with a single highlighted
effect so that we can exercise end-to-end decoding.

## Capture workflow

1. Build the legacy player in release mode so the frame hashes match CI:
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --target avs-player -j$(nproc)
   ```
2. For each preset below, run the headless renderer against the canonical sine
   wave and capture 180 frames (three seconds at 60 Hz):
   ```bash
   ./build/apps/avs-player/avs-player \
     --headless \
     --wav tests/data/test.wav \
     --preset tests/data/phase1/<preset>.avs \
     --frames 180 \
     --out build/captures/phase1/<preset>
   ```
   The command emits `hashes.txt` plus `frame_00000.png … frame_00179.png` in the
   chosen output directory. Developers who only need the shorter 10-frame capture
   expected by the automated tests can instead run the helper script:
   ```bash
   python3 tests/scripts/generate_phase1_goldens.py --install-to-golden --overwrite
   ```
   The script drives the same `avs-player` binary, defaults to ten frames per
   preset, and mirrors the results into the golden directories when
   `--install-to-golden` is supplied.
3. Copy the resulting files into the matching folder under
   `tests/golden/phase1/<preset>/` (replacing the placeholder README).  The
   integration tests expect:
   - `tests/golden/phase1/<preset>/hashes.txt`
   - `tests/golden/phase1/<preset>/frame_00000.png` … `frame_00179.png`

## Preset index

| Preset | Focus effect | Key parameters | Golden target |
|--------|--------------|----------------|---------------|
| `fadeout_color.avs` | Fadeout (`ID 3`) | `fadelen=32`, `color=0x003366CC` (non-zero target colour) | `tests/golden/phase1/fadeout_color/` |
| `brightness_exclude.avs` | Brightness (`ID 22`) | `exclude=1`, `distance=32`, sliders `(R=+512, G=-256, B=+768)` | `tests/golden/phase1/brightness_exclude/` |
| `blur_roundmode.avs` | Blur (`ID 6`) | `enabled=2` (normal blur), `roundmode=1` | `tests/golden/phase1/blur_roundmode/` |
| `nested_metadata.avs` | Nested render-list | Extended metadata retained; child list contains blur → fadeout chain | `tests/golden/phase1/nested_metadata/` |

Each preset also appears under the same name in `tests/golden/phase1/<preset>/preset.avs`
so that golden captures can be regenerated without chasing history in `tests/data/`.
