# Laser Effects - Vector Graphics for Physical Laser Hardware

## Overview

AVS laser effects represent a fundamentally different rendering paradigm designed for **physical laser projection hardware** rather than computer screens. While traditional AVS effects manipulate raster framebuffers, laser effects generate vector graphics primitives (lines and points) intended for output to professional laser systems via the **Pangolin QM2000** hardware interface.

## Architectural Differences

### Rendering Paradigm Shift

| Aspect | Regular Effects | Laser Effects |
|--------|----------------|---------------|
| **Output** | Pixel framebuffer | Vector line list |
| **Coordinates** | Integer pixels (0 to w-1, 0 to h-1) | Float normalized (-1.0 to 1.0) |
| **Primitives** | Pixels, lines, fills | Line segments, points |
| **Accumulation** | Overwrite framebuffer | Append to global list |
| **Return Value** | 1 = fbout used, 0 = framebuffer used | Always 0 (no framebuffer) |
| **Compilation** | Always built | `#ifdef LASER` conditional |

### Coordinate System

Laser effects operate in **device-independent normalized space**:
- X-axis: -1.0 (left) to +1.0 (right)
- Y-axis: -1.0 (bottom) to +1.0 (top)
- Origin: (0.0, 0.0) at screen center

This differs from framebuffer rendering, which uses top-left origin with pixel indices:
```
Framebuffer:        Laser Space:
(0,0)──────────→ x  (-1,1)────(0,1)────(1,1)
│                      │        │        │
│  Pixels             │     (0,0)       │
↓                      │        │        │
y                   (-1,-1)──(0,-1)──(1,-1)
```

## Core Data Structures

### `LineType` (defined in `linelist.h`)

```cpp
typedef struct {
    float x1, y1;      // Start point (normalized coords)
    float x2, y2;      // End point (normalized coords)
    int color;         // RGB color (0x00RRGGBB)
    int mode;          // 0 = line segment, 1 = point
} LineType;
```

### Global Line List: `g_laser_linelist`

All laser effects accumulate their output into a single shared line list:

```cpp
extern C_LineListBase *g_laser_linelist;
```

Defined in `laser.h` and implemented by `C_LineList` (in `linelist.cpp`), this manages:
- Array of up to **16,384 line/point primitives**
- Add, retrieve, and clear operations
- Per-frame reset and rebuild

## Laser Effects Workflow

### Typical Render Function

```cpp
int C_LaserEffectClass::render(char visdata[2][2][576], int isBeat,
                                int *framebuffer, int *fbout, int w, int h) {
    if (isBeat & 0x80000000) return 0;  // Skip if beat check disabled

    // Generate vector primitives
    for (int i = 0; i < numLines; ++i) {
        LineType line;
        line.x1 = start_x;
        line.y1 = start_y;
        line.x2 = end_x;
        line.y2 = end_y;
        line.color = RGB(255, 0, 0);  // Red
        line.mode = 0;                // Line segment

        g_laser_linelist->AddLine(&line);  // Accumulate to global list
    }

    return 0;  // Laser effects never write to framebuffer
}
```

### Helper Functions

```cpp
// Defined in laser.cpp or laser.h
void laser_drawpoint(float x, float y, int color);
```

Convenience function to add a single point without manually constructing a `LineType`.

## Hardware Integration

### Pangolin LD32 API

AVS interfaces with the Pangolin QM2000 laser DAC (Digital-to-Analog Converter) via the proprietary LD32.DLL library:

```cpp
// From LD32.H
int LD_Begin(int device);
int LD_WriteFrame(int device, LPPOINT points, int numPoints, int scanRate);
int LD_End(int device);
```

**`laser.cpp`** translates AVS's normalized `LineType` primitives into the LD32 API's `POINT` format, handling:
- Coordinate transformation (normalized → hardware-specific)
- Color space conversion
- Scan rate optimization (balancing flicker vs. detail)

### Dual-Mode Effects

Some effects (e.g., `r_oscring.cpp`) can render to **both** framebuffer and laser output:

```cpp
#ifdef LASER
int C_OscRingClass::render(...) {
    if (g_laser_linelist) {
        // Laser mode: add vector lines
        for (...) {
            LineType line = {...};
            g_laser_linelist->AddLine(&line);
        }
        return 0;
    }
}
#endif
// Non-laser mode: render to framebuffer
for (...) {
    line(framebuffer, x1, y1, x2, y2, w, h, color);
}
return 0;
```

This `#ifdef LASER` branching allows a single codebase to support both visualizer modes.

## Laser-Specific Effects

Five effects exist **exclusively** for laser output (in `laser/` directory):

| File | Effect | Description |
|------|--------|-------------|
| `rl_beathold.cpp` | Misc / Beat Hold | Freezes line list on beat for `decayMS` milliseconds |
| `rl_bren.cpp` | Render / Brennan's Effect | Procedural Bézier curves (artistic geometric patterns) |
| `rl_cones.cpp` | Render / Moving Cone | Rotating conical line meshes |
| `rl_line.cpp` | Render / Moving Line | Animated line segments following audio |
| `rl_trans.cpp` | Misc / Transform | Applies coordinate transformations (rotate, scale, skew) via EEL expressions |

### Example: Beat Hold

```cpp
int CLASER_BeatHoldClass::render(...) {
    if (isBeatDecay) {
        // Freeze: replay last beat's line list
        g_laser_linelist->SetLines(m_linelist, 0, linelist_used);
        g_laser_linelist->SetUsedLines(linelist_used);
    } else if (isBeat && ++beatCount > beatSkip) {
        // Capture current line list
        linelist_used = g_laser_linelist->GetUsedLines();
        memcpy(m_linelist, g_laser_linelist->GetLineList(),
               linelist_used * sizeof(LineType));
        isBeatDecay = GetTickCount() + decayMS;  // Start decay timer
    }
    return 0;
}
```

This creates a **visual echo effect** where complex laser graphics persist briefly after each beat.

## Line List Management: `C_LineListBase` Abstract Class

Defined in `linelist.h`:

```cpp
class C_LineListBase {
  public:
    virtual ~C_LineListBase() { }

    virtual void ClearLineList() = 0;
    virtual LineType *GetLineList() = 0;
    virtual int GetUsedLines() = 0;
    virtual int GetMaxLines() = 0;

    virtual void SetLines(LineType *list, int start, int length) = 0;
    virtual void SetUsedLines(int usedLines) = 0;
    virtual void AddLine(LineType *line) = 0;
};
```

### Implementation: `C_LineList`

The concrete implementation (in `linelist.cpp`) maintains:
- `LineType m_lines[16384];` - Fixed-size array
- `int m_lines_used;` - Current count
- Thread safety (critical sections for SMP builds)

### Per-Frame Lifecycle

```
Frame Start:
    └─> ClearLineList()
          └─> m_lines_used = 0

Effect Render (called for each laser effect):
    └─> AddLine(&lineType)
          └─> m_lines[m_lines_used++] = lineType

Frame End:
    └─> LD_WriteFrame(device, m_lines, m_lines_used, scanRate)
          └─> Hardware sends lines to laser projector
```

## Why Laser Mode?

### Historical Context

In the late 1990s/early 2000s, AVS was used by VJ artists and nightclub visuals operators. Physical laser shows were (and remain) a staple of electronic music events. By supporting laser output, AVS could:

1. **Drive professional laser systems** at concerts/raves
2. **Synchronize visuals with audio** using AVS's beat detection
3. **Create complex animations** using EEL scripting (e.g., `rl_trans.cpp`)
4. **Leverage existing AVS ecosystem** (presets, transitions, etc.)

### Technical Advantages

Vector rendering offers benefits for laser hardware:
- **Resolution independence** - Normalized coords scale to any projector
- **Sharp lines** - No pixelation artifacts
- **Efficient transmission** - 16K lines << megapixel framebuffers
- **Artistic style** - Wireframe aesthetics popular in electronic music visuals

## Current Port Status

Our modern port **does not** currently support laser mode:

- ❌ No Pangolin LD32 API integration
- ❌ No `g_laser_linelist` infrastructure
- ❌ Laser effects compile but do nothing (return 0, write no framebuffer)
- ✅ Binary parsers implemented for 5 laser effects (preserves preset compatibility)

### Future Laser Support

If laser mode were to be revived, modern alternatives to Pangolin could include:

1. **ILDA Standard** (.ild file format) - Open laser frame format
2. **Etherdream** - Open-source DAC hardware with USB/Ethernet API
3. **LaserDock** - Consumer laser projector with SDK
4. **OpenGL/Vulkan Vector Rendering** - Emulate laser look on screens
5. **OSC/DMX Protocols** - Integrate with lighting control systems

A modern implementation might:
- Replace `C_LineListBase` with `std::vector<LineType>`
- Abstract hardware via plugin interface (LD32, Etherdream, etc.)
- Add real-time vector→raster converter for preview
- Support SVG export for offline rendering

## Technical Deep Dive: Coordinate Transform

### Framebuffer to Laser Space

If converting a pixel-based effect to laser:

```cpp
// Framebuffer coords to normalized
float normalized_x = (pixel_x / (float)width) * 2.0f - 1.0f;
float normalized_y = (pixel_y / (float)height) * 2.0f - 1.0f;

// Note: pixel_y increases downward, laser_y increases upward
normalized_y = -normalized_y;  // Flip vertical axis
```

### Laser to Framebuffer (for preview)

```cpp
int pixel_x = (int)((laser_x + 1.0f) * 0.5f * width);
int pixel_y = (int)((1.0f - laser_y) * 0.5f * height);  // Flip Y
```

## Example: Simple Laser Circle

```cpp
void draw_laser_circle(float cx, float cy, float radius, int color) {
    const int segments = 64;
    for (int i = 0; i < segments; ++i) {
        float angle1 = (i / (float)segments) * 2.0f * M_PI;
        float angle2 = ((i + 1) / (float)segments) * 2.0f * M_PI;

        LineType line;
        line.x1 = cx + radius * cos(angle1);
        line.y1 = cy + radius * sin(angle1);
        line.x2 = cx + radius * cos(angle2);
        line.y2 = cy + radius * sin(angle2);
        line.color = color;
        line.mode = 0;  // Line segment

        g_laser_linelist->AddLine(&line);
    }
}
```

## Blend Modes in Laser Context

Unlike framebuffer rendering where blend modes determine pixel combination, laser blend modes affect:
- **Line width** (upper byte: `lineblendmode >> 16`)
- **Color mixing** for overlapping lines (lower byte: `lineblendmode & 0xFF`)

From `ape.h`:
```
lineblendmode: 0xbbccdd
bb = line width (minimum 1)
dd = blend mode:
    0 = replace, 1 = add, 2 = max, 3 = avg,
    4 = subtractive (1-2), 5 = subtractive (2-1),
    6 = multiplicative, 7 = adjustable (cc=blend ratio),
    8 = xor, 9 = minimum
```

## Summary

Laser effects demonstrate AVS's versatility as both a **software visualizer** and a **professional laser control system**. Though niche, this dual-mode capability showcases thoughtful architecture: a clean separation between rendering logic (effects) and output backend (framebuffer vs. line list). The vector-based approach, normalized coordinate system, and extensible line list design remain instructive for any real-time graphics system requiring multiple output targets.

For our modern port, laser mode remains dormant but documented—a fascinating piece of AVS history and a potential avenue for future extension if open-source laser hardware ecosystems mature.
