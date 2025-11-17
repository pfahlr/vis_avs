# Evallib - Original Expression Evaluation Library

## Overview

`evallib/` contains the **original** expression evaluation library for AVS, written by "lone". This was AVS's first attempt at providing user-programmable expressions for effects. It was later replaced by the more powerful **NS-EEL** (Nullsoft Embedded Expression Evaluator Language), which is what our modern port uses.

## Historical Context

Based on the readme.txt timestamp and simple architecture, evallib appears to be AVS 1.x technology (circa 2000-2001). It provided basic mathematical expression parsing for early programmable effects.

## Architecture

### Core Components

1. **Lexical Analyzer** (`SCAN.L` → `LEXTAB.C`)
   - Generated using LEX (lexical analyzer generator)
   - Tokenizes input strings like `"a=sin(pi/4)"`
   - Recognizes numbers, operators, function names, variables

2. **Parser** (`CAL.Y` → `CAL_TAB.C`)
   - Generated using BISON (yacc-compatible parser generator)
   - LALR(1) grammar for mathematical expressions
   - Produces an expression tree

3. **Evaluator** (`eval.c`, `eval.h`)
   - Interprets the parsed expression tree
   - Manages variable table (max 1024 variables)
   - Returns `double` results

4. **Built-in Functions** (`cfunc.c`)
   - Math functions: `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`
   - Algebraic: `sqr` (square), `sqrt`, `pow`, `exp`, `log`, `log10`

## Capabilities & Limitations

### Supported Features
- **Arithmetic**: `+ - * / %`
- **Bitwise**: `& |`
- **Functions**: Trig, logarithmic, power functions
- **Variables**: Assignment and reference (e.g., `pi=3.14159; a=sin(pi)`)
- **Number Bases**:
  - Decimal: `17d` or just `17`
  - Hexadecimal: `3bh` = 0x3B

### Limitations (per readme.txt)
- ❌ Max 1024 variables total
- ❌ No comparison operators (`<`, `>`, `==`)
- ❌ No logical operators (`&&`, `||`, `!`)
- ❌ No conditionals (`if`/`else`)
- ❌ No loops
- ❌ Functions limited to ≤2 parameters
- ❌ No user-defined functions
- ❌ No string operations
- ❌ Only double-precision floats

## API

### Simple Interface

```c
void resetVars(void);
double evaluate(char *expression, int *col);
```

**Usage**:
```c
int error_col;
resetVars();  // Initialize variable table to zero
double result = evaluate("x=5; y=x*2; sin(y)", &error_col);
if (error_col) {
    printf("Syntax error at column %d\n", error_col - 1);
}
```

### Function Registration

The `fnTable[]` in `eval.c` defines available functions:

```c
struct {
    char *name;       // Function name
    void *address;    // Function pointer
    int nParams;      // Parameter count (1 or 2)
} fnTable[] = {
    { "sin",   _sin,   1 },
    { "cos",   _cos,   1 },
    { "atan2", _atan2, 2 },
    // ...
};
```

To add a new function:
1. Implement the function in `cfunc.c`
2. Add entry to `fnTable[]`
3. If >2 params, extend `CAL.Y` grammar

## Why It Was Replaced

Evallib's limitations became apparent as AVS effects grew more sophisticated. Compare its capabilities to what users needed:

| Feature | Evallib | NS-EEL | Use Case |
|---------|---------|--------|----------|
| Conditionals | ❌ | ✅ | `if (above(v,0.5), fade, pulse)` |
| Loops | ❌ | ✅ | Per-point rendering in SuperScope |
| Comparisons | ❌ | ✅ | `x=if(above(bass,0.8), 1, 0)` |
| User functions | ❌ | ✅ | Reusable expression snippets |
| Memory access | ❌ | ✅ | `gmegabuf[]` for persistent state |
| Performance | Slow | JIT | Real-time per-pixel shaders |

NS-EEL (found in `/home/user/vis_avs/libs/third_party/ns-eel/`) addresses all these limitations and adds:
- **JIT compilation** to x86/x64 machine code
- **Rich standard library** (100+ functions)
- **Persistent state** via megabuf arrays
- **Integration with AVS internals** (beat detection, global registers, etc.)

## Current Port Status

**Evallib is NOT used in our modern port.** Instead, we use NS-EEL:

- ✅ NS-EEL ported as `libs/third_party/ns-eel/`
- ✅ All effects requiring expressions use NS-EEL
- ❌ Evallib remains in docs for historical reference only

## File Inventory

### Source Files
| File | Purpose |
|------|---------|
| `eval.c` / `eval.h` | Main evaluator, variable table |
| `cfunc.c` | Built-in math functions |
| `Compiler.c` / `Compiler.h` | Expression compiler/optimizer |
| `LEXTAB.C` | Lexer tables (generated from `SCAN.L`) |
| `CAL_TAB.C` | Parser tables (generated from `CAL.Y`) |
| `LEX.H` | Lexer header |
| `YYLEX.C` | Lexer driver |
| `GETTOK.C`, `LEXGET.C`, `LEXSWI.C`, `LLSAVE.C`, `LMOVB.C` | Lexer support |

### Build Files
| File | Purpose |
|------|---------|
| `SCAN.L` | LEX input (lexical rules) |
| `CAL.Y` | BISON input (grammar) |
| `makel.bat` | Rebuild LEXTAB.C from SCAN.L |
| `makey.bat` | Rebuild CAL_TAB.C from CAL.Y |
| `readme.txt` | Original documentation |

### Subdirectory
| Path | Contents |
|------|----------|
| `bison/` | BISON parser generator binaries (DOS/Windows tools) |

## Technical Insights

### Expression Grammar (from CAL.Y)

The grammar supports:
```
expression : assignment
           | comparison
           | arithmetic_expr
           ;

assignment : IDENTIFIER '=' expression
           ;

arithmetic_expr : term (('+' | '-') term)*
                ;

term : factor (('*' | '/' | '%') factor)*
     ;

factor : NUMBER
       | IDENTIFIER
       | FUNCTION1 '(' expression ')'
       | FUNCTION2 '(' expression ',' expression ')'
       | '(' expression ')'
       ;
```

### Variable Storage

Variables are stored in a simple hash table:
```c
typedef struct {
    char name[32];
    double value;
} Variable;

Variable varTable[1024];
```

Linear search for variable lookup—acceptable for small scripts but O(n) performance.

### Evaluation Strategy

**Tree-walking interpreter**: The parser builds an AST, then `eval()` recursively descends:
```c
double eval(Node *tree) {
    switch (tree->type) {
        case OP_ADD: return eval(tree->left) + eval(tree->right);
        case OP_MUL: return eval(tree->left) * eval(tree->right);
        case VAR:    return getVar(tree->varname);
        // ...
    }
}
```

No optimization passes, no bytecode, no JIT—pure interpretation. Fast enough for AVS 1.x's simpler effects but inadequate for per-pixel shaders.

## Lessons Learned

Evallib's design influenced NS-EEL's improvements:

1. **Keep grammar simple** - NS-EEL uses similar operator precedence
2. **Extensible function table** - Both use function tables, but NS-EEL's is more flexible
3. **JIT when possible** - NS-EEL compiles to native code for performance
4. **Rich standard library** - Users need more than just trig functions

## Archaeological Note

The evallib codebase is a snapshot of late-90s/early-2000s C programming practices:
- DOS batch files for build automation
- LEX/YACC for parsing (still industry standard but less common in apps)
- Minimal error handling
- Global variables for state
- No Unicode support

It's a testament to AVS's evolution that the project recognized evallib's limitations and invested in NS-EEL as a replacement. The jump in expression power enabled effects like SuperScope, Dynamic Movement, and Transform—arguably AVS's most creative and beloved features.
