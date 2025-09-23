# Phase-one legacy render-list reference

This note captures the binary structures emitted by the classic Advanced Visualization Studio (AVS)
renderer for render-lists and for the first tranche of legacy effects targeted for modern support.
All offsets and sizes are expressed in bytes, and integers follow the little-endian Win32 ABI used by
the original plug-in.

## Render-list envelope (`C_RenderListClass`)

### Packed `mode` field

`load_config()` reads a single byte and, when bit 7 is set, pulls in the remaining 24 bits so the
full 32-bit `mode` value can be reconstructed. `save_config_ex()` always writes the extended form for
non-root lists, so modern readers should expect the sequence:

1. `uint8_t mode_lo` with bit 7 acting as an "extended header present" flag.
2. Optional `uint32_t mode_full` providing the remaining bits (bit 7 cleared on read).

The meaning of each bitfield is defined in `r_list.h`:

| Bits | Meaning | Notes |
|------|---------|-------|
| 0 | `clearfb()` flag | Clears the target framebuffer before evaluating child effects when set. |
| 1 | `enabled()` latch (inverted) | A set bit disables the list until `fake_enabled` is bumped by beat gating. |
| 8–12 | Input blend selector (`blendin()`) | Index into the blend-mode combo box. Value `10` enables the adjustable-alpha slider; value `12` routes from an auxiliary buffer. |
| 16–20 | Output blend selector (`blendout()`) | Stored as `(selection ^ 1)` for backwards compatibility. XOR with `1` before matching UI entries. |
| 24–31 | Extended metadata size | `set_extended_datasize()` stores the byte count of the follow-on block. |

### Extended metadata block

Legacy builds fix the block length to 36 bytes (the value recorded in the upper `mode` bits). The
fields appear in the following order and are only written for non-root lists:

| Offset | Field | Type | Purpose |
|--------|-------|------|---------|
| +0 | `inblendval` | `uint32_t` | Adjustable alpha when input blend mode 10 ("Adjustable") is selected. |
| +4 | `outblendval` | `uint32_t` | Adjustable alpha for output blend mode 10. |
| +8 | `bufferin` | `uint32_t` | Source buffer index when input blend mode 12 ("Buffer") is active. |
| +12 | `bufferout` | `uint32_t` | Destination buffer index for output blend mode 12. |
| +16 | `ininvert` | `uint32_t` | Inverts the sampled buffer before blending when non-zero. |
| +20 | `outinvert` | `uint32_t` | Inverts the outgoing composited buffer when non-zero. |
| +24 | `beat_render` | `uint32_t` | Enables beat-gated rendering; the list renders only on or shortly after beats. |
| +28 | `beat_render_frames` | `uint32_t` | Number of frames to keep the list enabled after a beat (`fake_enabled` countdown). |

### Embedded script chunk

Non-root render-lists push an artificial child whose `effect_index` is `DLLRENDERBASE` and whose
32-byte identifier string equals `"AVS 2.8+ Effect List Config"`. Its payload encodes:

1. `uint32_t use_code` — enables the per-frame script hook.
2. Two null-terminated strings serialized via `C_RBASE::save_string()` — the initialization and
   frame scripts (`effect_exp[0]` and `effect_exp[1]`).

### Nested effect records

Each real child uses the common envelope written by `save_config_ex()`:

1. `uint32_t effect_index`. `LIST_ID` (`0xFFFFFFFE`) denotes a nested render-list; values ≥
   `DLLRENDERBASE` (16384) refer to registered APE plug-ins and are followed by a 32-byte,
   NUL-padded identifier string.
2. `uint32_t payload_length` — the size of the renderer-specific configuration blob.
3. `payload_length` bytes — raw effect data as emitted by the module’s `save_config()`.

Unknown payloads fall back to `C_UnknClass`, which preserves the bytes verbatim so presets can round
trip without data loss.

## Legacy effect catalogue (IDs 0–45)

Factory registration order in `rlib.cpp` defines the numeric identifiers. `DECLARE_EFFECT2`
entries return `C_RBASE2` instances that advertise SMP support.

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

The following sections describe the configuration layout for each phase-one effect. Tables list
fields in read order and their defaults as supplied by the constructors.

### ID 0 — Simple Spectrum (`r_simple.cpp`)

| Field | Bytes | Description | Valid range | Legacy default |
|-------|-------|-------------|--------------|----------------|
| `effect` | 4 | Packed bitfield controlling draw mode, audio source, and vertical anchor. | Bit-combined (see below) | `0 \| (2 << 2) \| (2 << 4)` |
| `num_colors` | 4 | Palette entry count. Values above 16 zero the list. | 0–16 | 1 |
| `colors[i]` | 4 × `num_colors` | Palette entries stored as `0x00RRGGBB`, cycled with interpolation every 64 frames. | 24-bit RGB | `0x00FFFFFF` for the lone default swatch |

Bit layout for `effect`:

- Bits 0–1: analyzer/scope style when bit 6 is clear (0 = solid analyzer, 1 = line analyzer,
  2 = line scope, 3 = solid scope).
- Bit 6: toggles the dot renderer; with the bit set, bit 1 selects analyzer (0) versus oscilloscope (1).
- Bits 2–3: audio source (0 = left, 1 = right, 2 = merged stereo; value 3 is unused by the UI).
- Bits 4–5: vertical placement (0 = top, 1 = bottom, 2 = centered, 3 unused).

Runtime notes:

- `color_pos` advances once per frame, interpolating between adjacent palette entries and wrapping
  to index 0, which recreates AVS’s smooth gradient cycling.
- Stereo-merge modes average left/right samples when `(effect & 3) > 1` before drawing.
- Rendering is skipped during the `0x80000000` preinit pass that AVS performs before activating a preset.

### ID 3 — Fadeout (`r_fadeout.cpp`)

| Field | Bytes | Description | Valid range | Legacy default |
|-------|-------|-------------|--------------|----------------|
| `fadelen` | 4 | Tolerance radius. Channels within ±`fadelen` of the target are pulled toward `color`; others are pushed away. | 0–255 (clamped internally) | 16 |
| `color` | 4 | Target RGB color (`0x00RRGGBB`). | 24-bit RGB | `0x00000000` |

Runtime notes:

- `fadelen == 0` short-circuits the effect early in `render()`.
- `maketab()` precomputes three 256-entry lookup tables for R/G/B. Channels nearer than
  `fadelen` land exactly on the target color; channels outside the window are offset by
  `fadelen`, recreating the legacy "fade toward/away" look.
- The MMX branch saturates subtractions so darkening toward black never underflows.

### ID 6 — Blur (`r_blur.cpp`)

| Field | Bytes | Description | Valid range | Legacy default |
|-------|-------|-------------|--------------|----------------|
| `enabled` | 4 | Kernel selector: 0 = disabled, 1 = normal blur, 2 = heavy blur, 3 = "more blur" kernel. | 0–3 | 1 |
| `roundmode` | 4 | Bias toggle that emulates float-style rounding by injecting small constants before shifts. | 0–1 | 0 |

Runtime notes:

- Rendering is skipped when `enabled == 0` or during the preinit pass.
- All kernels treat top and bottom scanlines specially to avoid out-of-bounds reads; middle lines
  vectorize across four pixels at a time in the MMX path.
- Mode 2 adds both axial and diagonal neighbours with 1/8 weighting, producing a heavier blur than
  the default mode-1 kernel. Mode 3 skews the weighting to emphasise contributions from preceding
  pixels, matching the original "more blur" UI option.
- `roundmode` applies constants such as `0x03`/`0x04` before shifting so the integer math rounds up
  instead of truncating, which is essential for parity with legacy visuals.

### ID 22 — Brightness (`r_bright.cpp`)

| Field | Bytes | Description | Valid range | Legacy default |
|-------|-------|-------------|--------------|----------------|
| `enabled` | 4 | Master enable. | 0 or 1 | 1 |
| `blend` | 4 | When non-zero, use additive blending against the adjusted color. | 0 or 1 | 0 |
| `blendavg` | 4 | When non-zero, blend 50/50 with the adjusted color instead of replacing. | 0 or 1 | 1 |
| `redp` | 4 | Red multiplier slider (scaled by 1/4096, sign controls dim/brighten). | ≈[-4096, 4096] | 0 |
| `greenp` | 4 | Green multiplier slider. | ≈[-4096, 4096] | 0 |
| `bluep` | 4 | Blue multiplier slider. | ≈[-4096, 4096] | 0 |
| `dissoc` | 4 | Unlocks independent channel sliders when non-zero. | 0 or 1 | 0 |
| `color` | 4 | Reference color for optional exclusion (`0x00RRGGBB`). | 24-bit RGB | `0x00000000` |
| `exclude` | 4 | When non-zero, skip pixels within `distance` of `color`. | 0 or 1 | 0 |
| `distance` | 4 | Absolute per-channel threshold for the exclusion mask. | 0–255 | 16 |

Runtime notes:

- `tabs_needinit` forces lookup-table recomputation whenever multipliers change. The tables clamp to
  `[0, 255]` to avoid overflow and are cached across frames for performance.
- The slider response is asymmetric: negative values scale by `(1 + value/4096)`, positive values by
  `(1 + 16*value/4096)`, reproducing the original UI feel.
- When `exclude` is active, `inRange()` performs a per-channel absolute comparison using the same
  byte scaling as the framebuffer, so the exclusion window matches the legacy implementation.
- `dissoc == 0` keeps sliders locked together in the UI, but the stored payload always contains the
  per-channel values to preserve round-trip fidelity.

### ID 25 — Clear (`r_clear.cpp`)

| Field | Bytes | Description | Valid range | Legacy default |
|-------|-------|-------------|--------------|----------------|
| `enabled` | 4 | Master enable. | 0 or 1 | 1 |
| `color` | 4 | Fill color (`0x00RRGGBB`). | 24-bit RGB | `0x00000000` |
| `blend` | 4 | Blend mode: 0 = replace, 1 = additive, 2 = line-based blend (`BLEND_LINE`). | 0–2 | 0 |
| `blendavg` | 4 | When non-zero, performs `BLEND_AVG` (50/50) against `color`. | 0 or 1 | 0 |
| `onlyfirst` | 4 | When non-zero, clears only the first rendered frame (`fcounter` guards subsequent frames). | 0 or 1 | 0 |

Runtime notes:

- `fcounter` increments each frame; `onlyfirst` forces a single-shot clear by leaving subsequent
  frames untouched.
- The effect exits early when disabled, when handling the preinit pass, or when `onlyfirst` suppresses
  rendering.
- `blend` and `blendavg` mirror the UI radio buttons. If both are zero the framebuffer is overwritten
  directly; otherwise the corresponding blend macro is used to match legacy compositing.

### ID 37 — Invert (`r_invert.cpp`)

| Field | Bytes | Description | Valid range | Legacy default |
|-------|-------|-------------|--------------|----------------|
| `enabled` | 4 | Master enable. | 0 or 1 | 1 |

Runtime notes:

- When enabled, the renderer XORs every pixel with `0x00FFFFFF`, preserving the alpha channel.
- Both scalar and MMX implementations bail out during the preinit pass or when the effect is disabled.

### ID 45 — Color Modifier (`r_dcolormod.cpp`)

| Field | Bytes | Description | Valid range | Legacy default |
|-------|-------|-------------|--------------|----------------|
| `script_format` | 1 | Serialization tag. `1` selects the modern variable-length string layout; any other value triggers the legacy 1024-byte block. | 0–255 | 1 (writer always uses 1) |
| `effect_exp[0]` | Variable (`uint32_t` length + bytes) | Per-pixel color script (`"code"` page). | N/A | empty string |
| `effect_exp[1]` | Variable | Per-frame script (`"per_frame"`). | N/A | empty string |
| `effect_exp[2]` | Variable | Per-beat script (`"per_beat"`). | N/A | empty string |
| `effect_exp[3]` | Variable | Initialization script (`"init"`). | N/A | empty string |
| `m_recompute` | 4 | When non-zero, rebuild the lookup table every frame. | 0 or 1 | 1 |

Legacy payloads (those without the `script_format` tag) pack the four scripts into a fixed 1024-byte
slab of four 256-byte, NUL-terminated strings in the order init/per-frame/per-beat/per-pixel. The
loader preserves both layouts for round-trip compatibility.

Runtime notes:

- `need_recompile` guards AVS EEL recompilation. On the next render after a reload, the effect calls
  `clearVars()`, registers `red`, `green`, `blue`, and `beat`, then compiles each script into
  `codehandle[0..3]` inside a critical section.
- `render()` skips work during the preinit pass. Otherwise it runs the init script once, then executes
  the per-frame script every frame and the per-beat script when `isBeat` is set before generating the
  256-entry lookup table.
- The lookup recomputation loop clamps `*var_r`, `*var_g`, and `*var_b` to `[0, 255]` after scaling.
  Table entries are stored sequentially as B, G, R bytes to match AVS’s framebuffer layout.
- `m_recompute` allows presets to defer table rebuilds until scripts signal a change; the flag is
  automatically cleared by the configuration dialog when the "recompute" checkbox is disabled.

