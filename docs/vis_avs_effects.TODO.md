
# VIS_AVS EFFECTS IMPLEMENTATIONS


## STATUS KEY
[ ] = todo
[+] = active development
[✅] = COMPLETED 


## VIS_AVS NEW EFFECTS - EFFECTS UNIQUE TO 2025 REBUILD 

---
---
```asciiart
🬞🬭🬭🬭🬭🬭🬏 🬭🬭🬭🬭 🬞🬭🬭🬭🬭  🬭🬭🬭🬭 
  ▐▒▌  ▐▒▌▐▒▌▐▒▌▐▒▌▐▒▌▐▒▌
  ▐─▌  ▐─▌▐─▌▐─▌▐─▌▐─▌▐─▌
  ▐░▌  ▐░▌▐░▌▐░▌▐░▌▐░▌▐░▌
  🬁🬂🬀   🬂🬂🬂🬂 🬁🬂🬂🬂🬂  🬂🬂🬂🬂 
```
---
---




---
---
```asciiart 
🬞🬭🬭🬭🬭  🬭🬭🬭🬭 🬞🬭🬼 🬞🬭🬏🬞🬭🬭🬭🬭🬏
▐▒▌▐▒▌▐▒▌▐▒▌▐▒🭌🬿▐▒▌▐▒▌   
▐─▌▐─▌▐─▌▐─▌▐─▌🭥🭒─▌▐─🬛🬋🬋 
▐░▌▐░▌▐░▌▐░▌▐░▌ ▐░▌▐░▌   
🬁🬂🬂🬂🬂  🬂🬂🬂🬂 🬁🬂🬀 🬁🬂🬀🬁🬂🬂🬂🬂🬀
```
---
---



## VIS_AVS LEGACY EFFECTS - ALL EFFECTS IMPLEMENTED IN ORIGINAL (WINAMP PLUGIN) VERSION

---
---
```
🬞🬭🬭🬭🬭🬭🬏 🬭🬭🬭🬭 🬞🬭🬭🬭🬭  🬭🬭🬭🬭 
  ▐▒▌  ▐▒▌▐▒▌▐▒▌▐▒▌▐▒▌▐▒▌
  ▐─▌  ▐─▌▐─▌▐─▌▐─▌▐─▌▐─▌
  ▐░▌  ▐░▌▐░▌▐░▌▐░▌▐░▌▐░▌
  🬁🬂🬀   🬂🬂🬂🬂 🬁🬂🬂🬂🬂  🬂🬂🬂🬂 
```
---
---




>>> REMAINING EFFECTS (as of 21 OCT 2025)

---
---

[+] - 1. Bass Spin

### Instruction ###
Your task is to implement or repair a previously attempted feature in the `vis_avs` repository. This feature implementation failed in a prior branch and must be re-executed correctly.

You are acting as a **senior software engineer** and **Codex development assistant**. You MUST analyze the given context, understand the expected implementation, and apply corrections and enhancements to meet the goal successfully. Ensure code is production-ready, idiomatic C++, and integrates seamlessly with the existing AVS plugin architecture.

### Effect Definition ###

```
Bass Spin

Effect Token: "Render / Bass Spin"

Source: docs/avs_original_source/vis_avs/r_bspin.cpp

Class: R_BSpin

Output: src/effects/render/effect_bass_spin.{h,cpp}

Prompt:
Rotate elements based on bass energy. Match bass window and spin curve from AVS.

failed development branch(es): 
- codex/add-bass-spin-effect

```

You are to implement it **again from scratch**, correcting or avoiding any past issues. Use the same original file and class references as provided.

### Git Setup ###
To ensure compatibility with your workspace, the following git commands should be run:

```bash
OWNER=pfahlr
REPO=vis_avs
: "${GITHUB_TOKEN:=$CODEX_READ_ALL_REPOSITORIES_TOKEN:-}"
git remote get-url origin >/dev/null 2>&1 || git remote add origin "https://${GITHUB_TOKEN}@github.com/pfahlr/vis_avs.git"
git config remote.origin.fetch "+refs/heads/*:refs/remotes/origin/*"
git config --add remote.origin.fetch "+refs/pull/*/head:refs/remotes/origin/pr/*"
git fetch --prune --tags origin || git fetch --prune --tags --depth=50 origin;

```--tags origin || git fetch --prune --tags --depth=50 origin;


---

[+] - 2. Moving Particle

### Instruction ###
Your task is to implement or repair a previously attempted feature in the `vis_avs` repository. This feature implementation failed in a prior branch and must be re-executed correctly.

You are acting as a **senior software engineer** and **Codex development assistant**. You MUST analyze the given context, understand the expected implementation, and apply corrections and enhancements to meet the goal successfully. Ensure code is production-ready, idiomatic C++, and integrates seamlessly with the existing AVS plugin architecture.

### Effect Definition ###

```
Moving Particle

Effect Token: "Render / Moving Particle"

Source: docs/avs_original_source/vis_avs/r_parts.cpp

Class: R_Parts

Output: src/effects/render/effect_moving_particle.{h,cpp}

Prompt:
Animate particles with motion trail. Support per-frame logic. AVS had bounce and fade.

failed development branch(es): 
 - codex/animate-moving-particles-with-trails
```

You are to implement it **again from scratch**, correcting or avoiding any past issues. Use the same original file and class references as provided.

### Git Setup ###
To ensure compatibility with your workspace, the following git commands should be run:

```bash
OWNER=pfahlr
REPO=vis_avs
: "${GITHUB_TOKEN:=$CODEX_READ_ALL_REPOSITORIES_TOKEN:-}"
git remote get-url origin >/dev/null 2>&1 || git remote add origin "https://${GITHUB_TOKEN}@github.com/pfahlr/vis_avs.git"
git config remote.origin.fetch "+refs/heads/*:refs/remotes/origin/*"
git config --add remote.origin.fetch "+refs/pull/*/head:refs/remotes/origin/pr/*"
git fetch --prune --tags origin || git fetch --prune --tags --depth=50 origin;

```--tags origin || git fetch --prune --tags --depth=50 origin;

---

---

[+] - 3. Oscillioscope Star

### Instruction ###
Your task is to implement or repair a previously attempted feature in the `vis_avs` repository. This feature implementation failed in a prior branch and must be re-executed correctly.

You are acting as a **senior software engineer** and **Codex development assistant**. You MUST analyze the given context, understand the expected implementation, and apply corrections and enhancements to meet the goal successfully. Ensure code is production-ready, idiomatic C++, and integrates seamlessly with the existing AVS plugin architecture.

### Effect Definition ###

```
Oscilloscope Star

Effect Token: "Render / Oscilliscope Star"

Source: docs/avs_original_source/vis_avs/r_oscstar.cpp

Class: R_OscStars

Output: src/effects/render/effect_oscilloscope_star.{h,cpp}

Prompt:
Draw radial oscillating lines from waveform. Integrate audio data.

failed development branch(es): 
- codex/integrate-audio-data-for-oscillating-lines
```

You are to implement it **again from scratch**, correcting or avoiding any past issues. Use the same original file and class references as provided.

### Git Setup ###
To ensure compatibility with your workspace, the following git commands should be run:

```bash
OWNER=pfahlr
REPO=vis_avs
: "${GITHUB_TOKEN:=$CODEX_READ_ALL_REPOSITORIES_TOKEN:-}"
git remote get-url origin >/dev/null 2>&1 || git remote add origin "https://${GITHUB_TOKEN}@github.com/pfahlr/vis_avs.git"
git config remote.origin.fetch "+refs/heads/*:refs/remotes/origin/*"
git config --add remote.origin.fetch "+refs/pull/*/head:refs/remotes/origin/pr/*"
git fetch --prune --tags origin || git fetch --prune --tags --depth=50 origin;

```--tags origin || git fetch --prune --tags --depth=50 origin;

---

[+] - 4. Simple Spectrum

### Instruction ###
Your task is to implement or repair a previously attempted feature in the `vis_avs` repository. This feature implementation failed in a prior branch and must be re-executed correctly.

You are acting as a **senior software engineer** and **Codex development assistant**. You MUST analyze the given context, understand the expected implementation, and apply corrections and enhancements to meet the goal successfully. Ensure code is production-ready, idiomatic C++, and integrates seamlessly with the existing AVS plugin architecture.

### Effect Definition ###
```
Simple Spectrum

Effect Token: "Render / Simple"

Source: docs/avs_original_source/vis_avs/r_simple.cpp

Class: R_SimpleSpectrum

Output: src/effects/render/effect_simple_spectrum.{h,cpp}

Prompt:
Bar-based spectrum visualizer. Use original AVS band logic and normalization.

failed development branch(es): 
- codex/implement-bar-based-spectrum-visualizer
```

You are to implement it **again from scratch**, correcting or avoiding any past issues. Use the same original file and class references as provided.

### Git Setup ###
To ensure compatibility with your workspace, the following git commands should be run:

```bash
OWNER=pfahlr
REPO=vis_avs
: "${GITHUB_TOKEN:=$CODEX_READ_ALL_REPOSITORIES_TOKEN:-}"
git remote get-url origin >/dev/null 2>&1 || git remote add origin "https://${GITHUB_TOKEN}@github.com/pfahlr/vis_avs.git"
git config remote.origin.fetch "+refs/heads/*:refs/remotes/origin/*"
git config --add remote.origin.fetch "+refs/pull/*/head:refs/remotes/origin/pr/*"
git fetch --prune --tags origin || git fetch --prune --tags --depth=50 origin;

```--tags origin || git fetch --prune --tags --depth=50 origin;

---

[+] - 5. SVP Loader

### Instruction ###
Your task is to implement or repair a previously attempted feature in the `vis_avs` repository. This feature implementation failed in a prior branch and must be re-executed correctly.

You are acting as a **senior software engineer** and **Codex development assistant**. You MUST analyze the given context, understand the expected implementation, and apply corrections and enhancements to meet the goal successfully. Ensure code is production-ready, idiomatic C++, and integrates seamlessly with the existing AVS plugin architecture.

### Effect Definition ###
```
SVP Loader

Effect Token: "Render / SVP Loader"

Source: docs/avs_original_source/vis_avs/r_svp.cpp

Class: R_SVP

Output: src/effects/render/effect_svp_loader.{h,cpp}

Prompt:
Load and display SVP files (if supported). Stub fallback is acceptable.

failed development branch(es): 
- codex/add-svp-file-loading-feature
```
You are to implement it **again from scratch**, correcting or avoiding any past issues. Use the same original file and class references as provided.

### Git Setup ###
To ensure compatibility with your workspace, the following git commands should be run:

```bash
OWNER=pfahlr
REPO=vis_avs
: "${GITHUB_TOKEN:=$CODEX_READ_ALL_REPOSITORIES_TOKEN:-}"
git remote get-url origin >/dev/null 2>&1 || git remote add origin "https://${GITHUB_TOKEN}@github.com/pfahlr/vis_avs.git"
git config remote.origin.fetch "+refs/heads/*:refs/remotes/origin/*"
git config --add remote.origin.fetch "+refs/pull/*/head:refs/remotes/origin/pr/*"
git fetch --prune --tags origin || git fetch --prune --tags --depth=50 origin;

```--tags origin || git fetch --prune --tags --depth=50 origin;

---

[+] - 6. Color Modifier

### Instruction ###
Your task is to implement or repair a previously attempted feature in the `vis_avs` repository. This feature implementation failed in a prior branch and must be re-executed correctly.

You are acting as a **senior software engineer** and **Codex development assistant**. You MUST analyze the given context, understand the expected implementation, and apply corrections and enhancements to meet the goal successfully. Ensure code is production-ready, idiomatic C++, and integrates seamlessly with the existing AVS plugin architecture.

### Effect Definition ###
```
Color Modifier

Effect Token: "Trans / Color Modifier"

Source: docs/avs_original_source/vis_avs/r_dcolormod.cpp

Class: R_DColorMod

Output: src/effects/trans/effect_color_modifier.{h,cpp}

Prompt:
Apply sine/cosine transformations to R/G/B. Expose mode selector.

failed development branch(es): 
- codex/add-sine/cosine-transformations-to-color-modifier

```
You are to implement it **again from scratch**, correcting or avoiding any past issues. Use the same original file and class references as provided.

### Git Setup ###
To ensure compatibility with your workspace, the following git commands should be run:

```bash
OWNER=pfahlr
REPO=vis_avs
: "${GITHUB_TOKEN:=$CODEX_READ_ALL_REPOSITORIES_TOKEN:-}"
git remote get-url origin >/dev/null 2>&1 || git remote add origin "https://${GITHUB_TOKEN}@github.com/pfahlr/vis_avs.git"
git config remote.origin.fetch "+refs/heads/*:refs/remotes/origin/*"
git config --add remote.origin.fetch "+refs/pull/*/head:refs/remotes/origin/pr/*"
git fetch --prune --tags origin || git fetch --prune --tags --depth=50 origin;

```--tags origin || git fetch --prune --tags --depth=50 origin;

---

[+] - 7. Timescope

### Instruction ###
Your task is to implement or repair a previously attempted feature in the `vis_avs` repository. This feature implementation failed in a prior branch and must be re-executed correctly.

You are acting as a **senior software engineer** and **Codex development assistant**. You MUST analyze the given context, understand the expected implementation, and apply corrections and enhancements to meet the goal successfully. Ensure code is production-ready, idiomatic C++, and integrates seamlessly with the existing AVS plugin architecture.

### Effect Definition ###
```
Timescope

Effect Token: "Render / Timescope"

Source: docs/avs_original_source/vis_avs/r_timescope.cpp

Class: R_Timescope

Output: src/effects/render/effect_timescope.{h,cpp}

Prompt:
Scrolling waveform display (time vs. amplitude). Integrate audio analyzer.

failed development branch: 
- codex/integrate-audio-analyzer-for-scrolling-waveform
```
You are to implement it **again from scratch**, correcting or avoiding any past issues. Use the same original file and class references as provided.

### Git Setup ###
To ensure compatibility with your workspace, the following git commands should be run:

```bash
OWNER=pfahlr
REPO=vis_avs
: "${GITHUB_TOKEN:=$CODEX_READ_ALL_REPOSITORIES_TOKEN:-}"
git remote get-url origin >/dev/null 2>&1 || git remote add origin "https://${GITHUB_TOKEN}@github.com/pfahlr/vis_avs.git"
git config remote.origin.fetch "+refs/heads/*:refs/remotes/origin/*"
git config --add remote.origin.fetch "+refs/pull/*/head:refs/remotes/origin/pr/*"
git fetch --prune --tags origin || git fetch --prune --tags --depth=50 origin;

```--tags origin || git fetch --prune --tags --depth=50 origin;


---

[+] - 8. Scatter 


### Instruction ###
Your task is to implement or repair a previously attempted feature in the `vis_avs` repository. This feature implementation failed in a prior branch and must be re-executed correctly.

You are acting as a **senior software engineer** and **Codex development assistant**. You MUST analyze the given context, understand the expected implementation, and apply corrections and enhancements to meet the goal successfully. Ensure code is production-ready, idiomatic C++, and integrates seamlessly with the existing AVS plugin architecture.

### Effect Definition ###
```
Scatter

Effect Token: "Trans / Scatter"

Source: docs/avs_original_source/vis_avs/r_scat.cpp

Class: R_Scat

Output: src/effects/trans/effect_scatter.{h,cpp}

Prompt:
Randomized pixel displace with falloff. Respect seed for determinism.

failed development branch: codex/add-randomized-pixel-displace-effect
```

You are to implement it **again from scratch**, correcting or avoiding any past issues. Use the same original file and class references as provided.

### Git Setup ###
To ensure compatibility with your workspace, the following git commands should be run:

```bash
OWNER=pfahlr
REPO=vis_avs
: "${GITHUB_TOKEN:=$CODEX_READ_ALL_REPOSITORIES_TOKEN:-}"
git remote get-url origin >/dev/null 2>&1 || git remote add origin "https://${GITHUB_TOKEN}@github.com/pfahlr/vis_avs.git"
git config remote.origin.fetch "+refs/heads/*:refs/remotes/origin/*"
git config --add remote.origin.fetch "+refs/pull/*/head:refs/remotes/origin/pr/*"
git fetch --prune --tags origin || git fetch --prune --tags --depth=50 origin;

```--tags origin || git fetch --prune --tags --depth=50 origin;


---

[+] - 9. Unique Tone

### Instruction ###
Your task is to implement or repair a previously attempted feature in the `vis_avs` repository. This feature implementation failed in a prior branch and must be re-executed correctly.

You are acting as a **senior software engineer** and **Codex development assistant**. You MUST analyze the given context, understand the expected implementation, and apply corrections and enhancements to meet the goal successfully. Ensure code is production-ready, idiomatic C++, and integrates seamlessly with the existing AVS plugin architecture.

### Effect Definition ###
```
Unique Tone

Effect Token: "Trans / Unique tone"

Source: docs/avs_original_source/vis_avs/r_onetone.cpp

Class: R_Onetone

Output: src/effects/trans/effect_unique_tone.{h,cpp}

Prompt:
Flatten frame to single color tone. Preserve luminance.

failed development branch: codex/add-unique-tone-effect
```
You are to implement it **again from scratch**, correcting or avoiding any past issues. Use the same original file and class references as provided.

### Git Setup ###
To ensure compatibility with your workspace, the following git commands should be run:

```bash
OWNER=pfahlr
REPO=vis_avs
: "${GITHUB_TOKEN:=$CODEX_READ_ALL_REPOSITORIES_TOKEN:-}"
git remote get-url origin >/dev/null 2>&1 || git remote add origin "https://${GITHUB_TOKEN}@github.com/pfahlr/vis_avs.git"
git config remote.origin.fetch "+refs/heads/*:refs/remotes/origin/*"
git config --add remote.origin.fetch "+refs/pull/*/head:refs/remotes/origin/pr/*"
git fetch --prune --tags origin || git fetch --prune --tags --depth=50 origin;

```--tags origin || git fetch --prune --tags --depth=50 origin;


---

[+] - 10. Water Bump

### Instruction ###
Your task is to implement or repair a previously attempted feature in the `vis_avs` repository. This feature implementation failed in a prior branch and must be re-executed correctly.

You are acting as a **senior software engineer** and **Codex development assistant**. You MUST analyze the given context, understand the expected implementation, and apply corrections and enhancements to meet the goal successfully. Ensure code is production-ready, idiomatic C++, and integrates seamlessly with the existing AVS plugin architecture.

### Effect Definition ###
```
# Water Bump

**Effect Token:** 
"Trans / Water Bump"

**Source:**
- docs/avs_original_source/vis_avs/r_waterbump.cpp

**Class:**
- R_WaterBump

**Output:**
- src/effects/trans/effect_water_bump.{h,cpp}

**prompt:**:
Combine bump mapping with water displacement. Requires heightmap calc.

**failed development branch**: 
-codex/combine-bump-mapping-with-water-displacement
```
You are to implement it **again from scratch**, correcting or avoiding any past issues. Use the same original file and class references as provided.

### Git Setup ###
To ensure compatibility with your workspace, the following git commands should be run:

```bash
OWNER=pfahlr
REPO=vis_avs
: "${GITHUB_TOKEN:=$CODEX_READ_ALL_REPOSITORIES_TOKEN:-}"
git remote get-url origin >/dev/null 2>&1 || git remote add origin "https://${GITHUB_TOKEN}@github.com/pfahlr/vis_avs.git"
git config remote.origin.fetch "+refs/heads/*:refs/remotes/origin/*"
git config --add remote.origin.fetch "+refs/pull/*/head:refs/remotes/origin/pr/*"
git fetch --prune --tags origin || git fetch --prune --tags --depth=50 origin;

```--tags origin || git fetch --prune --tags --depth=50 origin;
---

[+] - 11. Holden05: Multi Delay

### Instruction ###
Your task is to implement or repair a previously attempted feature in the `vis_avs` repository. This feature implementation failed in a prior branch and must be re-executed correctly.

You are acting as a **senior software engineer** and **Codex development assistant**. You MUST analyze the given context, understand the expected implementation, and apply corrections and enhancements to meet the goal successfully. Ensure code is production-ready, idiomatic C++, and integrates seamlessly with the existing AVS plugin architecture.

### Effect Definition ###
```
**Holden05: Multi Delay**

**Effect Token:** "Holden05: Multi Delay"

**Source:** 
docs/avs_original_source/vis_avs/rlib.cpp

**Class:** 
R_MultiDelay

**Output:** 
src/effects/trans/effect_multi_delay.{h,cpp}

**Prompt:**
Hold and composite multiple delayed frames. Ensure predictable memory and frame cycling behavior.

**failed development branch:**
- codex/add-multi-delay-frame-effect
```
You are to implement it **again from scratch**, correcting or avoiding any past issues. Use the same original file and class references as provided.

### Git Setup ###
To ensure compatibility with your workspace, the following git commands should be run:

```bash
OWNER=pfahlr
REPO=vis_avs
: "${GITHUB_TOKEN:=$CODEX_READ_ALL_REPOSITORIES_TOKEN:-}"
git remote get-url origin >/dev/null 2>&1 || git remote add origin "https://${GITHUB_TOKEN}@github.com/pfahlr/vis_avs.git"
git config remote.origin.fetch "+refs/heads/*:refs/remotes/origin/*"
git config --add remote.origin.fetch "+refs/pull/*/head:refs/remotes/origin/pr/*"
git fetch --prune --tags origin || git fetch --prune --tags --depth=50 origin;
--tags origin || git fetch --prune --tags --depth=50 origin;
```





---
---
```asciiart 
🬞🬭🬭🬭🬭  🬭🬭🬭🬭 🬞🬭🬼 🬞🬭🬏🬞🬭🬭🬭🬭🬏
▐▒▌▐▒▌▐▒▌▐▒▌▐▒🭌🬿▐▒▌▐▒▌   
▐─▌▐─▌▐─▌▐─▌▐─▌🭥🭒─▌▐─🬛🬋🬋 
▐░▌▐░▌▐░▌▐░▌▐░▌ ▐░▌▐░▌   
🬁🬂🬂🬂🬂  🬂🬂🬂🬂 🬁🬂🬀 🬁🬂🬀🬁🬂🬂🬂🬂🬀
```
---
---





---
COMPLETED EFFECTS PROMPTS (week of 13 OCT 2025)
---
🧩 vis_avs Effect Implementation Prompts (Legacy AVS Sources)

This document provides implementation prompts for all currently unported visual effects from the original Winamp AVS source code. Each section ensures proper token mapping, parser behavior, and implementation fidelity.

🔧 Format Per Effect

Effect Token: (exactly how the AVS preset references it)

Source: (path to AVS source file)

Class: (original AVS class name)

Output Path: (C++ file target in vis_avs repo)

---

🟦 TRANSFORMATION EFFECTS

---

Channel Shift

Effect Token: "Channel Shift"

Source: docs/avs_original_source/vis_avs/rlib.cpp

Class: R_ChannelShift

Output: src/effects/trans/effect_channel_shift.{h,cpp}

Prompt:
Implement a per-channel RGB shift with wraparound. Respect AVS-style clamping. Match UI parameters from the original.

---

Color Reduction

Effect Token: "Color Reduction"

Source: docs/avs_original_source/vis_avs/rlib.cpp

Class: R_ColorReduction

Output: src/effects/trans/effect_color_reduction.{h,cpp}

Prompt:
Reduce color depth for each channel. Support parameterized bit reduction. Match rounding and output fidelity of the original.

---

Holden04: Video Delay

Effect Token: "Holden04: Video Delay"

Source: docs/avs_original_source/vis_avs/rlib.cpp

Class: R_VideoDelay

Output: src/effects/trans/effect_video_delay.{h,cpp}

Prompt:
Implement multi-frame delay buffer with alpha blending. Match blending and timing from original AVS.

---

Holden05: Multi Delay

Effect Token: "Holden05: Multi Delay"

Source: docs/avs_original_source/vis_avs/rlib.cpp

Class: R_MultiDelay

Output: src/effects/trans/effect_multi_delay.{h,cpp}

Prompt:
Hold and composite multiple delayed frames. Ensure predictable memory and frame cycling behavior.

---

Multiplier

Effect Token: "Multiplier"

Source: docs/avs_original_source/vis_avs/rlib.cpp

Class: R_Multiplier

Output: src/effects/trans/effect_multiplier.{h,cpp}

Prompt:
Multiply RGB values per channel with configurable factors. Clamp at 8-bit and avoid overflow artifacts.

---

🟩 MISCELLANEOUS / UTILITY

---

Comment

Effect Token: "Misc / Comment"

Source: docs/avs_original_source/vis_avs/r_comment.cpp

Class: R_Comment

Output: src/effects/misc/effect_comment.{h,cpp}

Prompt:
Expose non-rendering comment field. Ensure parser loads it without affecting visuals.

---

Custom BPM

Effect Token: "Misc / Custom BPM"

Source: docs/avs_original_source/vis_avs/r_bpm.cpp

Class: R_Bpm

Output: src/effects/misc/effect_custom_bpm.{h,cpp}

Prompt:
Override BPM used in beat detection. Integrate with analyzer and gating system.

---

Set Render Mode

Effect Token: "Misc / Set render mode"

Source: docs/avs_original_source/vis_avs/r_linemode.cpp

Class: R_LineMode

Output: src/effects/misc/effect_render_mode.{h,cpp}

Prompt:
Expose a render mode override. Useful for lines/dots/spectrums. Respect compatibility across presets.

---

🟥 RENDER EFFECTS

---

AVI Loader

Effect Token: "Render / AVI"

Source: docs/avs_original_source/vis_avs/r_avi.cpp

Class: R_AVI

Output: src/effects/render/effect_avi.{h,cpp}

Prompt:
Load and render AVI video. Provide fallback if unsupported (e.g., PNG thumbnail). Stub acceptable initially.

---

Bass Spin

Effect Token: "Render / Bass Spin"

Source: docs/avs_original_source/vis_avs/r_bspin.cpp

Class: R_BSpin

Output: src/effects/render/effect_bass_spin.{h,cpp}

Prompt:
Rotate elements based on bass energy. Match bass window and spin curve from AVS.

---

Dot Fountain

Effect Token: "Render / Dot Fountain"

Source: docs/avs_original_source/vis_avs/r_dotfnt.cpp

Class: R_DotFountain

Output: src/effects/render/effect_dot_fountain.{h,cpp}

Prompt:
Emit particles from a center point. Trajectory and spread should match AVS visuals.

---

Dot Plane

Effect Token: "Render / Dot Plane"

Source: docs/avs_original_source/vis_avs/r_dotpln.cpp

Class: R_DotPlane

Output: src/effects/render/effect_dot_plane.{h,cpp}

Prompt:
Grid of animated dots with rotation/zoom. Match preset offsets and color modulation.

---

Moving Particle

Effect Token: "Render / Moving Particle"

Source: docs/avs_original_source/vis_avs/r_parts.cpp

Class: R_Parts

Output: src/effects/render/effect_moving_particle.{h,cpp}

Prompt:
Animate particles with motion trail. Support per-frame logic. AVS had bounce and fade.

---

Oscilloscope Star

Effect Token: "Render / Oscilliscope Star"

Source: docs/avs_original_source/vis_avs/r_oscstar.cpp

Class: R_OscStars

Output: src/effects/render/effect_oscilloscope_star.{h,cpp}

Prompt:
Draw radial oscillating lines from waveform. Integrate audio data.

---

Ring

Effect Token: "Render / Ring"

Source: docs/avs_original_source/vis_avs/r_oscring.cpp

Class: R_OscRings

Output: src/effects/render/effect_ring.{h,cpp}

Prompt:
Pulse ring shapes from waveform. Match scaling and modulation logic.

---

Rotating Stars

Effect Token: "Render / Rotating Stars"

Source: docs/avs_original_source/vis_avs/r_rotstar.cpp

Class: R_RotStar

Output: src/effects/render/effect_rotating_stars.{h,cpp}

Prompt:
Draw and rotate star shapes based on beat or frame count.

---

Simple Spectrum

Effect Token: "Render / Simple"

Source: docs/avs_original_source/vis_avs/r_simple.cpp

Class: R_SimpleSpectrum

Output: src/effects/render/effect_simple_spectrum.{h,cpp}

Prompt:
Bar-based spectrum visualizer. Use original AVS band logic and normalization.

---

SVP Loader

Effect Token: "Render / SVP Loader"

Source: docs/avs_original_source/vis_avs/r_svp.cpp

Class: R_SVP

Output: src/effects/render/effect_svp_loader.{h,cpp}

Prompt:
Load and display SVP files (if supported). Stub fallback is acceptable.

---

Timescope

Effect Token: "Render / Timescope"

Source: docs/avs_original_source/vis_avs/r_timescope.cpp

Class: R_Timescope

Output: src/effects/render/effect_timescope.{h,cpp}

Prompt:
Scrolling waveform display (time vs. amplitude). Integrate audio analyzer.

---

🟨 TRANSITION / POST EFFECTS

---

Blitter Feedback

Effect Token: "Trans / Blitter Feedback"

Source: docs/avs_original_source/vis_avs/r_blit.cpp

Class: R_BlitterFB

Output: src/effects/trans/effect_blitter_feedback.{h,cpp}

Prompt:
Render previous frame with transform (mirror/rotate). Ensure feedback is bounded.

---

Blur

Effect Token: "Trans / Blur"

Source: docs/avs_original_source/vis_avs/r_blur.cpp

Class: R_Blur

Output: src/effects/trans/effect_blur.{h,cpp}

Prompt:
Box blur with optional horizontal/vertical isolation. Port legacy radius/strength controls.

---

Brightness

Effect Token: "Trans / Brightness"

Source: docs/avs_original_source/vis_avs/r_bright.cpp

Class: R_Brightness

Output: src/effects/trans/effect_brightness.{h,cpp}

Prompt:
Apply brightness adjustment. Clamp at 255. Match original curve.

---

Color Clip

Effect Token: "Trans / Color Clip"

Source: docs/avs_original_source/vis_avs/r_colorreplace.cpp

Class: R_ContrastEnhance

Output: src/effects/trans/effect_color_clip.{h,cpp}

Prompt:
Clamp color channels within bounds. Match AVS integer logic.

---

Color Modifier

Effect Token: "Trans / Color Modifier"

Source: docs/avs_original_source/vis_avs/r_dcolormod.cpp

Class: R_DColorMod

Output: src/effects/trans/effect_color_modifier.{h,cpp}

Prompt:
Apply sine/cosine transformations to R/G/B. Expose mode selector.

---

Colorfade

Effect Token: "Trans / Colorfade"

Source: docs/avs_original_source/vis_avs/r_colorfade.cpp

Class: R_ColorFade

Output: src/effects/trans/effect_colorfade.{h,cpp}

Prompt:
Blend toward specified color. AVS uses pre-multiplied blending.

---

Mosaic

Effect Token: "Trans / Mosaic"

Source: docs/avs_original_source/vis_avs/r_mosaic.cpp

Class: R_Mosaic

Output: src/effects/trans/effect_mosaic.{h,cpp}

---

Prompt:
Pixelate screen by block sampling. Match AVS integer math.

Roto Blitter

Effect Token: "Trans / Roto Blitter"

Source: docs/avs_original_source/vis_avs/r_rotblit.cpp

Class: R_RotBlit

Output: src/effects/trans/effect_roto_blitter.{h,cpp}

Prompt:
Rotate and blend previous frame. Support anchor modes.

---

Scatter

Effect Token: "Trans / Scatter"

Source: docs/avs_original_source/vis_avs/r_scat.cpp

Class: R_Scat

Output: src/effects/trans/effect_scatter.{h,cpp}

Prompt:
Randomized pixel displace with falloff. Respect seed for determinism.

---

Unique Tone

Effect Token: "Trans / Unique tone"

Source: docs/avs_original_source/vis_avs/r_onetone.cpp

Class: R_Onetone

Output: src/effects/trans/effect_unique_tone.{h,cpp}

Prompt:
Flatten frame to single color tone. Preserve luminance.

---

Water

Effect Token: "Trans / Water"

Source: docs/avs_original_source/vis_avs/r_water.cpp

Class: R_Water

Output: src/effects/trans/effect_water.{h,cpp}

Prompt:
Simulate water ripple effect. Port integer wave propagation.

---

Water Bump

Effect Token: "Trans / Water Bump"

Source: docs/avs_original_source/vis_avs/r_waterbump.cpp

Class: R_WaterBump

Output: src/effects/trans/effect_water_bump.{h,cpp}

Prompt:
Combine bump mapping with water displacement. Requires heightmap calc.


