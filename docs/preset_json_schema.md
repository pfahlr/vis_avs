# AVS JSON Preset Format Specification

## Overview

The JSON preset format provides a human-readable alternative to the binary .avs format. It enables version control, hand-editing, and easier debugging of AVS presets.

## Schema Version

Current version: **1.0**

## Top-Level Structure

```json
{
  "version": "1.0",
  "compat": "strict",
  "effects": [...]
}
```

### Fields

- **version** (string, required): Schema version (currently "1.0")
- **compat** (string, optional): Compatibility mode, default is "strict"
- **effects** (array, optional): Array of effect nodes representing the effect chain

## Effect Node Structure

Each effect in the effects array has the following structure:

```json
{
  "effect": "effect-type-token",
  "order": 0,
  "params": [...],
  "children": [...]
}
```

### Fields

- **effect** (string, required): Effect type token (e.g., "Render / Simple", "Trans / Blur")
- **order** (integer, optional): Rendering order index (default: 0)
- **params** (array, optional): Array of parameter objects
- **children** (array, optional): Array of child effect nodes (for container effects)

## Parameter Structure

Each parameter has a typed value:

```json
{
  "name": "parameter-name",
  "type": "float|int|bool|string",
  "value": <value>
}
```

### Parameter Types

| Type | JSON Type | Example |
|------|-----------|---------|
| `float` (or `f32`) | number | `{"name": "radius", "type": "float", "value": 5.5}` |
| `int` (or `i32`) | integer | `{"name": "count", "type": "int", "value": 10}` |
| `bool` (or `boolean`) | boolean | `{"name": "enabled", "type": "bool", "value": true}` |
| `string` (or `str`) | string | `{"name": "text", "type": "string", "value": "Hello"}` |

## Complete Example

```json
{
  "version": "1.0",
  "compat": "strict",
  "effects": [
    {
      "effect": "Render / Simple",
      "params": [
        {
          "name": "draw_mode",
          "type": "int",
          "value": 0
        },
        {
          "name": "num_colors",
          "type": "int",
          "value": 1
        },
        {
          "name": "color_1",
          "type": "int",
          "value": 16777215
        }
      ]
    },
    {
      "effect": "Trans / Blur",
      "params": [
        {
          "name": "radius",
          "type": "int",
          "value": 5
        }
      ]
    },
    {
      "effect": "Misc / Effect List",
      "params": [
        {
          "name": "enabled",
          "type": "bool",
          "value": true
        }
      ],
      "children": [
        {
          "effect": "Trans / Water",
          "params": [
            {
              "name": "enabled",
              "type": "bool",
              "value": true
            }
          ]
        }
      ]
    }
  ]
}
```

## Conversion

### Binary to JSON

Use the `avs-convert` tool to convert binary .avs presets to JSON:

```bash
avs-convert --input mypreset.avs --output mypreset.json
```

### JSON to Binary

JSON to binary conversion is not yet implemented. The JSON format is currently designed for human-readable representation and debugging.

## Validation

The JSON deserializer performs the following validations:

1. **Version check**: Ensures the schema version is supported
2. **Type validation**: Verifies all fields have correct JSON types
3. **Parameter validation**: Ensures parameters have name, type, and value fields
4. **Value type checking**: Validates that parameter values match their declared types

### Error Reporting

Validation errors include helpful messages indicating:
- The location of the error (effect name, parameter name)
- The expected type or format
- The actual value that caused the error

## Best Practices

1. **Indentation**: Use 2-space indentation for readability
2. **Parameter ordering**: Keep parameters in a consistent order
3. **Comments**: While JSON doesn't support comments natively, use effect names descriptively
4. **Version control**: JSON format works well with git - diffs are meaningful and readable
5. **Hand-editing**: When manually creating presets, validate with avs-player to catch errors early

## Future Extensions

Potential future additions to the schema:

- **Metadata**: Author, description, creation date
- **Presets library**: Referenced sub-presets
- **Macros**: Reusable effect templates
- **Binary round-trip**: Full bidirectional conversion support

## Implementation Details

The JSON preset support is implemented using:

- **Library**: [nlohmann/json](https://github.com/nlohmann/json) (single-header, MIT licensed)
- **Location**: `libs/avs-preset/src/json_serializer.cpp`
- **Header**: `libs/avs-preset/include/avs/preset/json.h`
- **Tool**: `tools/avs-convert.cpp`

## See Also

- [AVS Preset Binary Format](avs_original_source_port_notes/README.md)
- [Effect Registry](../libs/avs-effects-legacy/README.md)
- AVS2K25 reference implementation
