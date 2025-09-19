# Phase-one legacy render-list reference

This note captures the binary structures that the classic Advanced Visualization Studio (AVS) renderer
writes for render-lists and for the first tranche of legacy effects that we plan to revive in the modern
loader. All offsets and sizes are expressed in bytes, and all integers are stored little-endian as in the
original Win32 implementation.

## Render-list envelope (`C_RenderListClass`)

### Packed `mode` field (first byte, optionally extended)

| Bits | Meaning | Notes |
|------|---------|-------|
| 0    | `clearfb()` flag | When set, the list clears its target framebuffer before evaluating child effects. |
| 1    | `enabled()` flag (inverted) | `mode & 0x2` disables the list unless `fake_enabled` is non-zero (beat gating). |
| 8–12 | Input blend mode (`blendin`) | Direct index into the blend mode combo box; value 10 exposes the adjustable alpha slider, value 12 enables buffer routing. |
| 16–20 | Output blend mode (`blendout`) | Stored as `(selection ^ 1)` for backwards compatibility; decode by XOR with 1 before looking up `blendmodes`. |
| 24–31 | Extended data size sentinel | `set_extended_datasize()` stores the byte count of the extended metadata block (exclusive of the leading `mode` byte). |

Legacy files set the high bit of the very first byte to advertise the presence of the extended header. If
`mode & 0x80` is present, the loader removes that bit, reads the full 32-bit `mode`, and continues with the
extended metadata block.

### Extended metadata block (optional)

The legacy writer fixes the extended data size to 36 bytes, and `load_config()` interprets the payload in the
following order:

| Offset | Field | Type | Purpose |
|--------|-------|------|---------|
| +0 | `inblendval` | `uint32_t` | Adjustable alpha for the input blend when mode 10 ("Adjustable") is active. |
| +4 | `outblendval` | `uint32_t` | Adjustable alpha for the output blend when mode 10 is selected. |
| +8 | `bufferin` | `uint32_t` | Source buffer index when input blend mode 12 ("Buffer") is chosen. |
| +12 | `bufferout` | `uint32_t` | Destination buffer index when output blend mode 12 is chosen. |
| +16 | `ininvert` | `uint32_t` | Non-zero requests inversion when sampling the input buffer for mode 12. |
| +20 | `outinvert` | `uint32_t` | Non-zero requests inversion when compositing into the output buffer for mode 12. |
| +24 | `beat_render` | `uint32_t` | Toggles beat-gated rendering; when true, the list only renders immediately after a beat. |
| +28 | `beat_render_frames` | `uint32_t` | Number of frames to keep the list enabled after a beat (`fake_enabled` countdown). |

### Embedded script chunk

After the extended metadata, non-root render-lists inject an additional pseudo-renderer whose effect index is
`DLLRENDERBASE` and whose identifier string matches the sentinel `"AVS 2.8+ Effect List Config"`. Its payload
encodes:

1. `use_code` (`uint32_t`) — whether to evaluate the compiled script per frame.
2. Two null-terminated strings serialized with `load_string()` — the initialization and per-frame EEL scripts
   (`effect_exp[0]` and `effect_exp[1]`).

### Nested effect records

Each child entry serialized by `save_config_ex()` follows the same structure:

1. `uint32_t effect_index`. `LIST_ID` (`0xFFFFFFFE`) denotes a nested render-list; any value ≥ `DLLRENDERBASE`
   (`16384`) refers to a registered APE and is immediately followed by a 32-byte, NUL-padded identifier string.
2. `uint32_t payload_length` indicating the size of the renderer-specific configuration blob.
3. `payload_length` bytes of renderer data as emitted by the module’s `save_config()` implementation.

Unrecognized payloads fall back to `C_UnknClass`, which preserves the original bytes verbatim so they can round-trip
back to disk.

## Legacy effect catalogue (IDs 0–45)

| ID | Factory symbol | Legacy UI name | R2 capable? |
|----|----------------|----------------|-------------|
| 0 | `R_SimpleSpectrum` | Render / Simple | No |
| 1 | `R_DotPlane` | Render / Dot Plane | No |
| 2 | `R_OscStars` | Render / Oscilliscope Star | No |
| 3 | `R_FadeOut` | Trans / Fadeout | No |
| 4 | `R_BlitterFB` | Trans / Blitter Feedback | No |
| 5 | `R_NFClear` | Render / OnBeat Clear | No |
| 6 | `R_Blur` | Trans / Blur | Yes |
| 7 | `R_BSpin` | Render / Bass Spin | No |
| 8 | `R_Parts` | Render / Moving Particle | No |
| 9 | `R_RotBlit` | Trans / Roto Blitter | No |
| 10 | `R_SVP` | Render / SVP Loader | No |
| 11 | `R_ColorFade` | Trans / Colorfade | Yes |
| 12 | `R_ContrastEnhance` | Trans / Color Clip | No |
| 13 | `R_RotStar` | Render / Rotating Stars | No |
| 14 | `R_OscRings` | Render / Ring | No |
| 15 | `R_Trans` | Trans / Movement | Yes |
| 16 | `R_Scat` | Trans / Scatter | No |
| 17 | `R_DotGrid` | Render / Dot Grid | No |
| 18 | `R_Stack` | Misc / Buffer Save | No |
| 19 | `R_DotFountain` | Render / Dot Fountain | No |
| 20 | `R_Water` | Trans / Water | Yes |
| 21 | `R_Comment` | Misc / Comment | No |
| 22 | `R_Brightness` | Trans / Brightness | Yes |
| 23 | `R_Interleave` | Trans / Interleave | No |
| 24 | `R_Grain` | Trans / Grain | No |
| 25 | `R_Clear` | Render / Clear screen | No |
| 26 | `R_Mirror` | Trans / Mirror | No |
| 27 | `R_StarField` | Render / Starfield | No |
| 28 | `R_Text` | Render / Text | No |
| 29 | `R_Bump` | Trans / Bump | No |
| 30 | `R_Mosaic` | Trans / Mosaic | No |
| 31 | `R_WaterBump` | Trans / Water Bump | No |
| 32 | `R_AVI` | Render / AVI | No |
| 33 | `R_Bpm` | Misc / Custom BPM | No |
| 34 | `R_Picture` | Render / Picture | No |
| 35 | `R_DDM` | Trans / Dynamic Distance Modifier | No |
| 36 | `R_SScope` | Render / SuperScope | No |
| 37 | `R_Invert` | Trans / Invert | No |
| 38 | `R_Onetone` | Trans / Unique tone | No |
| 39 | `R_Timescope` | Render / Timescope | No |
| 40 | `R_LineMode` | Misc / Set render mode | No |
| 41 | `R_Interferences` | Trans / Interferences | No |
| 42 | `R_Shift` | Trans / Dynamic Shift | No |
| 43 | `R_DMove` | Trans / Dynamic Movement | Yes |
| 44 | `R_FastBright` | Trans / Fast Brightness | No |
| 45 | `R_DColorMod` | Trans / Color Modifier | No |

## Phase-one renderer payloads

### ID 0 — Simple Spectrum (`r_simple.cpp`)

| Field (in read order) | Bytes | Description | Valid range | Legacy default |
|-----------------------|-------|-------------|--------------|----------------|
| `effect` | 4 | Packed bitfield controlling drawing mode, channel selection, and vertical placement. | Mode bits documented below | `0 | (2 << 2) | (2 << 4)` (solid analyzer, mid channel, centered) |
| `num_colors` | 4 | Number of palette entries that follow. Values above 16 are clamped to zero. | 0–16 | 1 |
| `colors[i]` | 4 × `num_colors` | RGB colors cycled every 64 frames. Stored as `0x00RRGGBB`. | 24-bit RGB | `0x00FFFFFF` for the lone default swatch |

Bit layout for `effect`:

- Bits 0–1: analyzer/scope style when bit 6 is clear (0 = solid analyzer, 1 = line analyzer, 2 = line scope, 3 = solid scope).
- Bit 6: dot modes enable an alternate branch. With bit 6 set, bit 1 selects analyzer (0) versus oscilloscope (1) dots.
- Bits 2–3: audio source (0 = left, 1 = right, 2 = merged stereo; the UI keeps bit 3 cleared so value 3 is unused).
- Bits 4–5: vertical anchor (0 = top, 1 = bottom, 2 = center). Center mode shifts oscilloscope variants upward.

Runtime notes:

- `color_pos` advances every frame and smoothly interpolates between adjacent palette entries, wrapping back to index 0.
- Combined-channel modes average left/right samples when `(effect & 3) > 1`.
- The renderer ignores draw calls during the `0x80000000` "preinit" pass that AVS issues before a preset activates.

### ID 3 — Fadeout (`r_fadeout.cpp`)

| Field | Bytes | Description | Valid range | Legacy default |
|-------|-------|-------------|--------------|----------------|
| `fadelen` | 4 | Radius around the target color that should be pulled toward `color` each frame. | 0–255 (values above 255 behave as 255) | 16 |
| `color` | 4 | Target RGB color (`0x00RRGGBB`). | 24-bit RGB | `0x00000000` |

Runtime notes:

- `fadelen == 0` disables the effect early in `render()`.
- `maketab()` precomputes three lookup tables that clamp each color channel toward `color` whenever the source channel falls
  within ±`fadelen` of the target value; channels outside that window are pushed away by `fadelen`, creating the legacy
  "fade toward/away" look.
- When `color` is zero the MMX code path reduces each component by `fadelen` without underflow thanks to saturated
  subtraction.

### ID 6 — Blur (`r_blur.cpp`)

| Field | Bytes | Description | Valid range | Legacy default |
|-------|-------|-------------|--------------|----------------|
| `enabled` | 4 | Blur mode selector: 0 = disabled, 1 = light blur, 2 = normal blur, 3 = extra blur. | 0–3 | 1 |
| `roundmode` | 4 | Bias toggle for rounding fractional contributions. 0 rounds down, 1 biases the sums upward. | 0–1 | 0 |

Runtime notes:

- The renderer skips work when `enabled == 0` or during the preinit pass.
- Each blur mode uses a different kernel: mode 1 averages cardinal neighbors, mode 2 adds diagonals for a heavier blur,
  and mode 3 increases weighting on the opposite side to simulate a "more blur" preset. The loops explicitly handle top
  and bottom scanlines to avoid reading past the framebuffer.
- `roundmode` injects small constants (`0x03`/`0x04` etc.) before shifting so the integer divisions mimic floating-point
  rounding up.

### ID 22 — Brightness (`r_bright.cpp`)

| Field | Bytes | Description | Valid range | Legacy default |
|-------|-------|-------------|--------------|----------------|
| `enabled` | 4 | Master enable. | 0 or 1 | 1 |
| `blend` | 4 | When non-zero, enables additive blending against the adjusted color. | 0 or 1 | 0 |
| `blendavg` | 4 | When non-zero, enables 50/50 blending against the adjusted color. | 0 or 1 | 1 |
| `redp` | 4 | Red channel multiplier slider scaled by 1/4096. Negative values dim, positive brighten. | ≈[-4096, 4096] | 0 |
| `greenp` | 4 | Green channel multiplier slider. | ≈[-4096, 4096] | 0 |
| `bluep` | 4 | Blue channel multiplier slider. | ≈[-4096, 4096] | 0 |
| `dissoc` | 4 | UI toggle that allows RGB sliders to move independently when non-zero. | 0 or 1 | 0 |
| `color` | 4 | Reference color for exclusion comparisons (`0x00RRGGBB`). | 24-bit RGB | `0x00000000` |
| `exclude` | 4 | When true, pixels within `distance` of `color` are left untouched. | 0 or 1 | 0 |
| `distance` | 4 | Threshold for the exclusion mask. Compared per channel with absolute differences. | 0–255 | 16 |

Runtime notes:

- `tabs_needinit` ensures the per-channel lookup tables rebuild whenever multipliers change. Each table clamps to
  `[0, 255]` after scaling so overflow cannot wrap.
- Multipliers derive from the UI formula `(1 + scale * slider/4096)` where `scale` is `1` for negative sliders and `16`
  for positive ones, matching the legacy feel.
- When `exclude` is set, `inRange()` checks the Manhattan distance per channel (scaled to each byte lane) before applying
  brightness; pixels inside the color window remain unchanged.
- `dissoc == 0` keeps the three sliders in lockstep within the configuration dialog, but the stored payload still preserves
  the last explicit values for round-trip fidelity.

### ID 25 — Clear (`r_clear.cpp`)

| Field | Bytes | Description | Valid range | Legacy default |
|-------|-------|-------------|--------------|----------------|
| `enabled` | 4 | Master enable. | 0 or 1 | 1 |
| `color` | 4 | Fill color (`0x00RRGGBB`). | 24-bit RGB | `0x00000000` |
| `blend` | 4 | Additive (`1`) or "def. rend" (`2`) clear mode. | 0–2 | 0 |
| `blendavg` | 4 | When non-zero, performs 50/50 blending with `color` instead of replacement. | 0 or 1 | 0 |
| `onlyfirst` | 4 | When non-zero, clears only the first frame after activation. | 0 or 1 | 0 |

Runtime notes:

- `fcounter` increments every frame and suppresses subsequent clears when `onlyfirst` is set.
- `blend == 2` delegates to the legacy `BLEND_LINE` macro (alternating-line blend); `blend == 1` does full additive
  blending, and `blendavg` uses `BLEND_AVG`.
- Preinit calls and disabled states exit early without modifying the framebuffer.

### ID 37 — Invert (`r_invert.cpp`)

| Field | Bytes | Description | Valid range | Legacy default |
|-------|-------|-------------|--------------|----------------|
| `enabled` | 4 | Master enable. | 0 or 1 | 1 |

Runtime notes:

- When enabled, every pixel is XORed with `0x00FFFFFF`, effectively inverting RGB while preserving alpha.
- The renderer provides both scalar and MMX implementations; both guard against the preinit pass and skip work when the
  effect is disabled.

### ID 45 — Color Modifier (`r_color.cpp` / handled separately)

Already supported by the modern loader; no additional documentation required for this phase.

