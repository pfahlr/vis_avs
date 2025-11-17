# AVS APE (Advanced Plugin Effect) Architecture

## Overview

The APE (Advanced Plugin Effect) system was AVS's attempt to provide a plugin SDK that would allow third-party developers to create custom visual effects. The architecture is defined in `ape.h` and demonstrated in the `apesdk/` tutorial directory.

## Core Interface: `C_RBASE`

All APE plugins must derive from the `C_RBASE` abstract base class defined in `apesdk/avs_ape.h`:

```cpp
class C_RBASE {
  public:
    virtual int render(char visdata[2][2][576], int isBeat,
                      int *framebuffer, int *fbout, int w, int h) = 0;
    virtual HWND conf(HINSTANCE hInstance, HWND hwndParent) { return 0; }
    virtual char *get_desc() = 0;
    virtual void load_config(unsigned char *data, int len) { }
    virtual int save_config(unsigned char *data) { return 0; }
};
```

### Key Methods

- **`render()`** - Core rendering function called every frame
  - `visdata[2][2][576]`: Audio waveform/spectrum data
  - `isBeat`: Beat detection flag (0x80000000 = skip beat check)
  - `framebuffer`: Input pixel buffer
  - `fbout`: Output pixel buffer
  - Returns 1 if output in `fbout`, 0 if output in `framebuffer`

- **`get_desc()`** - Returns effect display name (e.g., "Trans / Channel Shift")

- **`load_config()` / `save_config()`** - Binary preset serialization
  - Effects serialize their parameters using a raw byte format
  - Loaded from .avs preset files, saved via PUT_INT/GET_INT macros

- **`conf()`** - Optional configuration dialog (Win32 UI)

## Extended Interface: `C_RBASE2` (SMP Support)

Version 2 added multi-threading support via `C_RBASE2`:

```cpp
class C_RBASE2 : public C_RBASE {
  virtual int smp_getflags() { return 0; }  // return 1 to enable SMP
  virtual int smp_begin(int max_threads, ...);  // setup phase
  virtual void smp_render(int this_thread, int max_threads, ...);  // parallel
  virtual int smp_finish(...);  // combine results
};
```

This allowed effects to split rendering work across multiple CPU cores—a significant performance optimization for computationally intensive effects.

## Extended API: `APEinfo` Struct

APE plugins could optionally receive an `APEinfo*` pointer via the exported function:

```cpp
void __declspec(dllexport) AVS_APE_SetExtInfo(HINSTANCE hDllInstance, APEinfo *ptr);
```

This structure (defined in both `ape.h` and `apesdk/avs_ape.h`) provided access to advanced AVS features:

### 1. Global State
- `double *global_registers` - 100 shared registers for inter-effect communication
- `int *lineblendmode` - Current blend mode settings (0xbbccdd format)

### 2. Expression Evaluation (EEL Integration)
The most powerful feature—access to AVS's embedded scripting engine:

```cpp
VM_CONTEXT (*allocVM)();                              // Create VM instance
void (*freeVM)(VM_CONTEXT);                          // Destroy VM
void (*resetVM)(VM_CONTEXT);                         // Clear state
double* (*regVMvariable)(VM_CONTEXT, char *name);    // Register variable
VM_CODEHANDLE (*compileVMcode)(VM_CONTEXT, char*);   // Compile expression
void (*executeCode)(VM_CODEHANDLE, char visdata[2][2][576]);  // Execute
void (*freeCode)(VM_CODEHANDLE);                     // Free compiled code
```

This allowed plugins to embed user-programmable expressions (like SuperScope's per-point code).

### 3. Utility Functions (ver >= 2)
- `doscripthelp()` - Display scripting help dialog
- `getNbuffer()` - Access to 8 shared render buffers (ver >= 3)

## Helper Functions

The SDK provided inline pixel blending utilities:

```cpp
BLEND(a, b)      // Additive blend with clamping
BLEND_AVG(a, b)  // 50/50 average blend
```

## Binary Packing Format

APE effects in preset files use a special format:
- **Effect ID**: `0x4000` (16384, `DLLRENDERBASE` constant)
- **APE Identifier**: 32-byte null-terminated string (e.g., "Channel Shift", "Multiplier")
- **Payload Length**: uint32
- **Payload Data**: Effect-specific binary configuration

This differs from built-in effects (IDs 0-45) which use numeric indices only.

## Why APEs Didn't Catch On

Several factors likely contributed to limited adoption:

### 1. **Win32-Specific**
The entire API was Windows-centric:
- `HWND`, `HINSTANCE` for configuration dialogs
- `__declspec(dllexport)` linkage
- No cross-platform story

### 2. **C++ ABI Fragility**
Distributing C++ virtual interfaces as DLLs is notoriously brittle:
- Compiler version mismatches
- Runtime library conflicts
- No stable ABI like COM or C-style exports

### 3. **Limited Documentation**
The `apesdk/` folder contains only a minimal tutorial (`avstut00.cpp`):
- Single example effect
- No comprehensive guide to advanced features
- No explanation of blend modes, buffer management, etc.

### 4. **Complex Binary Format**
Effects had to manually serialize state using raw byte arrays—error-prone and unforgiving.

### 5. **No Package Manager**
Discovery and distribution were manual:
- Users had to find DLL files online
- No centralized repository
- Installation required manually copying files

### 6. **EEL Accessibility**
For many users, built-in EEL-based effects (SuperScope, Dynamic Movement) already provided sufficient programmability without writing C++.

## Lessons for a Modern SDK

A future AVS SDK could improve on APEs by:

1. **C API with Stable ABI**
   - Pure C interfaces (no vtables)
   - Version negotiation
   - Plugin validation/sandboxing

2. **Cross-Platform from Day One**
   - Abstract platform types (no HWND)
   - Standardized UI description (JSON/declarative)
   - WASM support for ultimate portability

3. **Modern Serialization**
   - JSON or TOML configuration
   - Schema validation
   - Forward/backward compatibility built-in

4. **Package Ecosystem**
   - Central registry (like npm, crates.io)
   - Dependency management
   - Automated testing/CI

5. **Rich DSL**
   - Extend the EEL language
   - Visual node-based effect builder
   - Hot-reload for development

6. **Effect Composition**
   - Effects as first-class parameters to other effects
   - Shader-like composability
   - Automatic GPU acceleration when possible

7. **Community Governance**
   - Open contribution process
   - Curated official library
   - Security review for distributed effects

## Current Port Status

Our modern port does **not** support loading binary APE DLLs from the original AVS. However, we have:

- ✅ Implemented binary parsers for 5 common APE effects (Channel Shift, Color Reduction, Multiplier, Multi Delay, Video Delay)
- ✅ Preserved the `APEinfo` concept in our documentation
- ❌ No dynamic plugin loading yet

The APE architecture is documented here for future reference when designing a modern extensibility system. The insights gained from analyzing its strengths and weaknesses will inform a cleaner, more maintainable plugin API for the next generation of AVS.
