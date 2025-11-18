# APE Emulator Usage Guide

## Overview

The Wine-based APE emulator allows vis_avs to load Windows APE (Advanced Plugin Effect) DLLs on Linux. This is a **proof-of-concept fallback mechanism** for effects that haven't been reimplemented natively.

## Architecture

```
┌─────────────────────────────────────┐
│   vis_avs (Linux native)            │
├─────────────────────────────────────┤
│   Preset Loader                     │
│   ├─ Native Effect Registry         │
│   └─ Wine APE Loader (fallback)     │
├─────────────────────────────────────┤
│   APE DLL (Windows PE binary)       │
└─────────────────────────────────────┘
```

When loading a preset:
1. Check native effect registry first
2. If not found and effect is APE (ID >= 16384), try Wine APE loader
3. If Wine loader fails, create unknown placeholder

## Installation

### Prerequisites

The APE emulator is **built-in** and requires no special compilation flags. However, to actually load Windows DLLs, you need:

- A compatible PE loader (Wine or winelib)
- APE DLL files in searchable locations

### APE DLL Search Paths

The emulator searches for APE DLLs in these locations:

1. `~/.config/vis_avs/ape_plugins/` (user-specific)
2. `/usr/local/share/vis_avs/ape_plugins/` (system-wide)
3. `/usr/share/vis_avs/ape_plugins/` (distribution)

### Setting Up APE DLLs

```bash
# Create user plugin directory
mkdir -p ~/.config/vis_avs/ape_plugins

# Copy APE DLLs from Winamp installation
# Example DLL names (actual names may vary):
cp /path/to/winamp/Plugins/avs/*.dll ~/.config/vis_avs/ape_plugins/

# Common APE DLLs:
# - Holden03_ConvolutionFilter.dll
# - Texer2.dll
# - AVSTrans.dll
# - etc.
```

## Usage

### Loading Presets with APE Effects

The APE emulator works automatically when loading presets:

```cpp
#include <avs/preset.hpp>

avs::ParsedPreset preset = avs::parsePreset("mypreset.avs");

// If the preset contains APE effects:
// 1. Native APE implementations are used if available
// 2. Wine APE loader attempts to load unknown APE DLLs
// 3. Warnings are logged if loading fails
```

### Checking APE Support

```cpp
#include <avs/compat/ape_loader.hpp>

if (avs::ape::isWineAPESupported()) {
  std::cout << "Wine APE emulator is available\n";
}

// Find an APE DLL
auto dllPath = avs::ape::findAPEDLL("Holden03: Convolution Filter");
if (!dllPath.empty()) {
  std::cout << "Found APE DLL: " << dllPath << "\n";
}
```

### Manual APE Loading

```cpp
#include <avs/compat/ape_loader.hpp>

// Load an APE DLL
avs::ape::WineAPELoader loader;
if (loader.load("/path/to/effect.dll")) {
  // Create effect instance
  auto apeInstance = loader.createEffectInstance();

  // Wrap in Effect interface
  std::vector<std::uint8_t> config; // Binary config data
  auto effect = std::make_unique<avs::ape::WineAPEEffect>(
    std::move(apeInstance), config
  );

  // Use like any other effect
  effect->init(640, 480);
  effect->process(inputBuffer, outputBuffer);
} else {
  std::cerr << "Failed to load: " << loader.getLastError() << "\n";
}
```

### Custom Search Paths

```cpp
#include <avs/compat/ape_loader.hpp>

// Add custom search paths
std::vector<std::filesystem::path> customPaths = {
  "/mnt/winamp_plugins/avs",
  "/opt/ape_plugins"
};
avs::ape::setAPESearchPaths(customPaths);
```

## Limitations

### Current Implementation Status

This is a **proof-of-concept** implementation with several limitations:

1. **PE Loader Required**: Actually loading Windows DLLs requires Wine or a compatible PE loader
2. **No Configuration Dialogs**: Win32 `conf()` dialogs are not supported on Linux
3. **Headless Only**: Effects can render but not show UI
4. **C++ ABI Challenges**: MSVC vs GCC vtable compatibility issues may occur
5. **Limited Testing**: Not all APE DLLs have been tested

### Known Issues

- **Symbol Resolution**: Finding exported functions in DLLs may fail without proper PE loader
- **EEL VM Context**: Some APE effects using embedded EEL scripts may have issues
- **Thread Safety**: SMP support (C_RBASE2) is implemented but untested
- **Memory Management**: Shared buffers (`getNbuffer`) return nullptr (stub)

## Troubleshooting

### "Failed to load APE DLL: unknown error"

The DLL couldn't be loaded. Common causes:
- Not a valid PE/DLL file
- Missing Wine or PE loader
- 32-bit vs 64-bit mismatch

**Solution**: Use native reimplementation or run on Windows

### "APE factory function not found"

The DLL doesn't export `_AVS_APE_RetrFunc` or `AVS_APE_RetrFunc`.

**Solution**: Verify it's an actual APE plugin DLL

### "APE DLL not found for: <effect>"

The DLL file couldn't be found in search paths.

**Solution**:
```bash
# List expected filenames
ls ~/.config/vis_avs/ape_plugins/

# The loader tries these patterns:
# - <identifier>.dll
# - <identifier>.so
# - ape_<identifier>.dll
# - ape_<identifier>.so
```

### Effect Renders Incorrectly

The APE effect may depend on features not fully implemented:
- EEL VM integration incomplete
- Blend modes not matching original
- Audio data not correctly marshaled

**Solution**: File an issue or consider native reimplementation

## Debugging

### Enable Verbose Logging

The preset loader outputs warnings to `stderr`:

```bash
# See APE loading attempts
./avs-player mypreset.avs 2>&1 | grep -i ape

# Look for:
# INFO: Attempting to load APE plugin: ...
# INFO: Successfully loaded APE plugin via Wine emulator: ...
# WARNING: Failed to load APE plugin via Wine emulator: ...
```

### Test APE DLL Loading

```cpp
#include <avs/compat/ape_loader.hpp>
#include <iostream>

int main() {
  avs::ape::WineAPELoader loader;

  if (loader.load("/path/to/test.dll")) {
    std::cout << "Success! Identifier: " << loader.getIdentifier() << "\n";

    auto instance = loader.createEffectInstance();
    if (instance) {
      std::cout << "Instance created: " << instance->get_desc() << "\n";
    }
  } else {
    std::cerr << "Failed: " << loader.getLastError() << "\n";
  }

  return 0;
}
```

## Production Recommendations

For production use, we recommend:

1. **Native Reimplementation** - Port popular APE effects to native C++
2. **Community Contributions** - Submit native implementations to the project
3. **Wine Prefix** - For true Windows DLL compatibility, run vis_avs.exe under Wine
4. **Windows Build** - Use the native Windows build if maximum compatibility is needed

## Contributing

### Implementing Native APE Effects

See [ape_linux_support_strategy.md](./ape_linux_support_strategy.md) for the native reimplementation process.

### Improving the Emulator

Areas for improvement:
- Better PE loader integration (winelib)
- Full EEL VM context marshaling
- Shared buffer management
- SMP support testing
- Configuration file generation (JSON alternative to dialogs)

## References

- [APE Architecture Documentation](./avs_original_source_port_notes/ape-plugin-architecture.md)
- [APE Support Strategy](./ape_linux_support_strategy.md)
- [Original APE SDK](../docs/avs_original_source/vis_avs/apesdk/)
- [grandchild's vis_avs](https://github.com/grandchild/vis_avs)

## License

The APE emulator module follows the same license as vis_avs. APE DLLs retain their original licenses.
