# NS-EEL (Nullsoft Embedded Expression Evaluator Language)

## Overview

**NS-EEL** (pronounced "eel") is AVS's embedded scripting language that powers programmable effects like SuperScope, Dynamic Movement, and the laser Transform effect. It replaced the earlier `evallib` system with a far more capable expression compiler that features JIT (Just-In-Time) compilation to native machine code for real-time performance.

## Location in Original Source

- **Original AVS**: `vis_avs/ns-eel/` (mixed case, various files)
- **Our Modern Port**: `/home/user/vis_avs/libs/third_party/ns-eel/`

## Architecture

### Three-Stage Pipeline

```
1. LEXER (nseel-lextab.c)
   └─> Tokenize source text
       └─> "x=sin(t)" → [ID:x, OP:=, FUNC:sin, LPAREN, ID:t, RPAREN]

2. PARSER (nseel-compiler.c)
   └─> Build AST (Abstract Syntax Tree)
       └─> Assignment(x, FuncCall(sin, Variable(t)))

3. CODE GENERATOR (nseel-compiler.c)
   └─> Emit x86/x64 machine code
       └─> JIT-compile to executable memory
```

### Key Components

| File | Purpose |
|------|---------|
| `ns-eel.h` | Public API header |
| `ns-eel-int.h` | Internal structures, opcodes |
| `nseel-compiler.c` | Parser and code generator (3000+ lines) |
| `nseel-eval.c` | Runtime evaluator, context management |
| `nseel-cfunc.c` | Standard library (sin, cos, abs, etc.) |
| `nseel-caltab.c` | Operator precedence tables |
| `nseel-lextab.c` | Lexer state machine |
| `ns-eel-addfuncs.h` | Macro for registering C functions |

## Public API

### Context Management

```c
NSEEL_CODEHANDLE NSEEL_code_compile(NSEEL_VMCTX ctx, const char *code);
void NSEEL_code_execute(NSEEL_CODEHANDLE code);
void NSEEL_code_free(NSEEL_CODEHANDLE code);
```

### Variable Registration

```c
EEL_F *NSEEL_VM_regvar(NSEEL_VMCTX ctx, const char *name);
```

**Returns**: Pointer to a double that the script can read/write.

**Example**:
```c
NSEEL_VMCTX vm = NSEEL_VM_alloc();

// Register variables that EEL code can access
EEL_F *x = NSEEL_VM_regvar(vm, "x");
EEL_F *y = NSEEL_VM_regvar(vm, "y");
EEL_F *w = NSEEL_VM_regvar(vm, "w");
EEL_F *h = NSEEL_VM_regvar(vm, "h");

// Set initial values (from C)
*x = 0.5;
*y = 0.25;
*w = 1920.0;
*h = 1080.0;

// Compile user code that references these variables
NSEEL_CODEHANDLE code = NSEEL_code_compile(vm, "x=x+0.01; y=sin(x)");

// Execute (modifies *x and *y)
NSEEL_code_execute(code);

// Read results
printf("x=%f, y=%f\n", *x, *y);  // x=0.51, y=0.488...
```

This pointer-based variable binding is how AVS effects share state with user scripts.

## Language Features

### Data Types
- **Single type**: `double` (64-bit floating point)
- No integers, strings, or booleans (use 0.0 = false, non-zero = true)

### Operators

| Category | Operators |
|----------|-----------|
| Arithmetic | `+ - * / %` |
| Comparison | `< > <= >= == !=` (return 1.0 or 0.0) |
| Logical | `&& \|\|` (short-circuit evaluation) |
| Bitwise | `& \| ^ ~` |
| Assignment | `=` |
| Ternary | `cond ? true_val : false_val` |

### Built-in Functions (from nseel-cfunc.c)

**Math**: `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `sqr`, `sqrt`, `pow`, `exp`, `log`, `log10`, `abs`, `min`, `max`, `sign`, `rand`, `floor`, `ceil`

**AVS-Specific**:
- `above(val, threshold)` - Returns 1.0 if val > threshold, else 0.0
- `below(val, threshold)` - Returns 1.0 if val < threshold, else 0.0
- `equal(a, b)` - Returns 1.0 if a == b (with tolerance)
- `if(cond, true_val, false_val)` - Functional if expression

**Memory Access**:
- `gmegabuf(index)` - Access global 1MB buffer (shared across effects)
- `megabuf(index)` - Access effect-local buffer

### Control Flow

**Conditionals**:
```eel
x = if(above(bass, 0.5), 1, 0);
y = (bass > 0.5) ? 1 : 0;  // Same as above
```

**Sequencing** (semicolons):
```eel
x = x + 1;
y = sin(x);
z = y * 2;
```

**Implicit return**: Last evaluated expression is the return value.

## JIT Compilation

NS-EEL doesn't interpret an AST—it **compiles to machine code**.

### Code Generation Flow

1. **Parse** source → AST
2. **Walk AST**, emitting x86/x64 instructions into executable memory
3. **Return function pointer** to compiled code

**Example**: Compiling `x = sin(t) * 2`

```nasm
; Pseudocode of generated assembly
mov rax, [t_ptr]       ; Load 't' address
fld qword [rax]        ; Push t value to FPU stack
call sin_func          ; Call sin() (C function)
fmul 2.0               ; Multiply by 2
mov rax, [x_ptr]       ; Load 'x' address
fstp qword [rax]       ; Store result to x
ret
```

This is why EEL is fast enough for **per-pixel effects** like SuperScope—the script runs at native CPU speed, not through an interpreter.

### Platform Support

- **x86 (32-bit)**: Full JIT support
- **x86-64 (64-bit)**: Full JIT support
- **ARM/RISC-V**: Falls back to **interpreted mode** (slower but portable)

The compiler in `nseel-compiler.c` has conditional `#ifdef` blocks for architecture-specific code emission.

## AVS Integration

### Effects Using EEL

| Effect | EEL Usage |
|--------|-----------|
| **SuperScope** | Per-point rendering (`init`, `frame`, `beat`, `point`) |
| **Dynamic Movement** (`r_dmove.cpp`) | Coordinate transformation expressions |
| **Dynamic Distance Modifier** (`r_ddm.cpp`) | Distance-based displacement |
| **Dynamic Shift** (`r_shift.cpp`) | Pixel shifting formulas |
| **Laser Transform** (`laser/rl_trans.cpp`) | Geometric transforms for laser vectors |

### Execution Phases

Many effects separate EEL code into phases:

1. **Init**: Run once when effect loads
2. **Frame**: Run once per frame
3. **Beat**: Run on beat detection
4. **Point** (SuperScope only): Run for every pixel/point

**Example** (SuperScope):
```eel
// Init (once)
n=100;

// Frame (60 FPS)
t=t+0.01;

// Beat (on bass drum)
n=n*2;

// Point (n times per frame)
x=cos(t+i/n*6.28);
y=sin(t+i/n*6.28);
```

### Variable Namespace

EEL scripts in AVS have access to:

**Predefined Variables** (registered by effect):
- `w`, `h` - Framebuffer width/height
- `t` - Time (increments each frame)
- `i`, `n` - Loop counter, total iterations (SuperScope)
- `bass`, `mid`, `treb` - Audio frequency bands
- `beat` - Beat flag (1.0 on beat)

**Global Registers** (shared across all effects):
- `reg00` through `reg99` - 100 global doubles
- Persist across frame boundaries
- Enable inter-effect communication

**Megabuf** (persistent memory):
- `gmegabuf(index)` - 1MB global array
- `megabuf(index)` - Effect-local array
- Used for history, state machines, etc.

### Example: Cross-Effect Communication

**Effect 1 (Dynamic Movement)**:
```eel
// Store bass level for other effects
reg00 = bass;
reg01 = beat;
```

**Effect 2 (SuperScope)**:
```eel
// Read bass from reg00, set by earlier effect
x = cos(t) * reg00;
y = sin(t) * reg00;
```

This `reg` array is the AVS equivalent of global variables.

## Performance Characteristics

### JIT Compilation Overhead

- **First compile**: ~1-10ms (one-time cost when preset loads)
- **Execution**: Native speed (10-100x faster than interpreted)

For a 1920x1080 SuperScope effect running per-pixel code:
- Interpreted: ~500ms per frame → 2 FPS 🐌
- JIT compiled: ~16ms per frame → 60 FPS ⚡

### Memory Usage

- **Code cache**: Compiled functions stored in executable memory (typically 1-50KB per effect)
- **Variable table**: Hash map of registered variables (~10KB)
- **Megabuf**: Up to 1MB for user data

## Extending NS-EEL

### Adding Custom Functions

Use the `NS_EEL_DEFINE_FUNCTION` macro:

```c
static EEL_F NSEEL_CGEN_CALL my_lerp(EEL_F *a, EEL_F *b, EEL_F *t) {
    return *a + (*b - *a) * (*t);
}

// In initialization
NSEEL_addfunc_ret3("lerp", NSEEL_PProc_THIS, &my_lerp);
```

Now user scripts can call `lerp(0, 100, 0.5)` → 50.0.

### Function Signature Types

From `ns-eel-addfuncs.h`:
- `NSEEL_PProc_THIS` - Returns double
- `NSEEL_PProc_THIS_END` - Void return
- `NSEEL_PProc_THIS_START` - Initialization hook

## Comparison to Other Embedded Languages

| Feature | NS-EEL | Lua | JavaScript | Python |
|---------|--------|-----|------------|--------|
| Type system | Double only | Dynamic | Dynamic | Dynamic |
| JIT | x86/x64 | Yes (LuaJIT) | Yes (V8) | No (CPython) |
| Syntax | C-like | Lua | JS | Pythonic |
| Sandboxing | Limited | Good | Good | Limited |
| Binary size | ~50KB | ~200KB | ~10MB | ~5MB |
| Startup time | <1ms | ~10ms | ~100ms | ~50ms |

NS-EEL trades expressiveness for **simplicity and speed**. Perfect for real-time graphics where every millisecond counts.

## Security Considerations

### No Sandboxing

EEL scripts execute **in-process** with **full memory access** (via pointers). Malicious code could:
- Crash AVS (null pointer dereference)
- Corrupt memory (out-of-bounds megabuf access)
- Execute infinite loops (no timeout mechanism)

**Current approach**: Trust the user (presets are considered code).

**Future hardening**:
- Bounds checking on `megabuf()`
- Execution timeout (watchdog timer)
- WASM compilation target (memory isolation)

## Porting Notes

Our modern port **already uses NS-EEL**:

- ✅ Located at `/home/user/vis_avs/libs/third_party/ns-eel/`
- ✅ Integrated into effects like SuperScope, Dynamic Movement
- ✅ 64-bit and ARM support
- ❌ Not yet exposed to users as a standalone scripting engine

The original AVS embedded EEL deep in effect code. A modern architecture could:
1. Elevate EEL to a **first-class scripting layer**
2. Provide a **debugger** with breakpoints, variable inspection
3. Add **syntax highlighting** in the UI
4. Support **hot reload** (recompile on save without reloading preset)

## Example Programs

### Bouncing Ball
```eel
// Init
x=0; y=0; vx=0.02; vy=0.03;

// Frame
x=x+vx; y=y+vy;
vx=if(abs(x)>0.9, -vx, vx);  // Bounce horizontal
vy=if(abs(y)>0.9, -vy, vy);  // Bounce vertical
```

### Beat-Reactive Color
```eel
// Frame
hue = hue + 0.01;

// Beat
hue = rand(1.0);  // Random hue on beat

// Use hue for color calculation
red = abs(sin(hue * 6.28));
green = abs(sin((hue + 0.33) * 6.28));
blue = abs(sin((hue + 0.66) * 6.28));
```

### Persistent State via Megabuf
```eel
// Init
megabuf(0) = 0;  // Frame counter

// Frame
megabuf(0) = megabuf(0) + 1;
frame_num = megabuf(0);

// Display every 60th frame
show = if(frame_num % 60 < 30, 1, 0);
```

## Future Evolution

Modern successors to NS-EEL could include:

1. **TypeScript/JavaScript** - Familiar syntax, rich ecosystem
2. **Lua** - Industry-standard game scripting, great tooling
3. **WASM** - Compile any language (Rust, C, etc.) to safe bytecode
4. **Shader languages** (GLSL/HLSL/WGSL) - Leverage GPU for per-pixel work
5. **Visual node graphs** - No-code alternative (like TouchDesigner)

But NS-EEL's **minimalism remains its strength**: zero dependencies, tiny footprint, blazing fast. For real-time audio visualization, it's hard to beat.

## Further Reading

- **NS-EEL original documentation**: Limited, mostly code comments
- **REAPER's JSFX**: Uses NS-EEL for audio plugins (good real-world examples)
- **WinAmp AVS forums**: User-created EEL tutorials (archived)

## Summary

NS-EEL is AVS's **secret weapon**—the engine that transformed it from a collection of fixed effects into a **programmable visualization platform**. Its JIT compiler, tight integration with AVS's rendering pipeline, and elegant simplicity enabled the creative explosion of user-generated presets that defined the AVS community. Understanding EEL is essential to understanding what makes AVS more than just another music visualizer—it's a real-time graphics programming environment disguised as a Winamp plugin.
