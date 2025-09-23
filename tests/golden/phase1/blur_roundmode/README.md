# blur_roundmode capture placeholder

Drop `hashes.txt` and the 180 PNG frames produced by:

```bash
./build/apps/avs-player/avs-player \
  --headless \
  --wav tests/data/test.wav \
  --preset tests/data/phase1/blur_roundmode.avs \
  --frames 180 \
  --out tests/golden/phase1/blur_roundmode
```

The copy of the preset used to render (`preset.avs`) should remain in this
folder alongside the captured outputs.
