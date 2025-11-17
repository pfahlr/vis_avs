# Studio UI MVP Architecture

## Overview

The AVS Studio UI provides an interactive preset editor for creating and modifying visualizations in real-time. Currently a stub, the full implementation will use ImGui for immediate-mode GUI rendering.

## Current Status

**Status**: Stub (84 bytes placeholder)
**Location**: `apps/avs-studio/main.cpp`
**Job**: #21 - ui-studio-mvp

## Architecture Design

### Technology Stack

- **UI Framework**: [ImGui](https://github.com/ocornut/imgui) (MIT licensed)
- **Graphics Backend**: SDL2 + OpenGL (existing avs-player infrastructure)
- **Rendering**: Shared with avs-player (avs-compat engine)
- **Preset Format**: JSON + Binary .avs support

### Main Components

```
┌─────────────────────────────────────────────┐
│           Studio UI Application             │
├─────────────────────────────────────────────┤
│  ┌─────────────┐  ┌──────────────────────┐ │
│  │ Effect Tree │  │   Live Preview       │ │
│  │   Editor    │  │  (Rendering Window)  │ │
│  │             │  │                      │ │
│  │  - Add/Del  │  │  - Real-time render  │ │
│  │  - Reorder  │  │  - Audio reactive    │ │
│  │  - Drag/Drop│  │  - Beat trigger      │ │
│  └─────────────┘  └──────────────────────┘ │
│                                             │
│  ┌──────────────────────────────────────┐  │
│  │      Parameter Editor Panel          │  │
│  │                                       │  │
│  │  - Sliders (float/int)               │  │
│  │  - Checkboxes (bool)                 │  │
│  │  - Text fields (string)              │  │
│  │  - Color pickers (RGB/HSV)           │  │
│  │  - Code editor (EEL scripts)         │  │
│  └──────────────────────────────────────┘  │
└─────────────────────────────────────────────┘
```

### Window Layout

```
┌──────────────────────────────────────────────────────────┐
│ Menu Bar: File | Edit | View | Preset | Help             │
├────────────┬──────────────────────────┬──────────────────┤
│  Effect    │    Preview Window        │   Parameters     │
│  Tree      │                          │                  │
│  ========  │   ┌──────────────────┐   │  radius: [===] 5 │
│  □ Simple  │   │                  │   │  enabled: [x]    │
│  □ Blur    │   │  Live Rendering  │   │  mode: [ v ]     │
│  ▼ List    │   │                  │   │    - Normal      │
│    □ Water │   │                  │   │    - Blend       │
│    □ Shift │   └──────────────────┘   │                  │
│  + Add     │   FPS: 60 | Beat: [!]    │  [Apply] [Reset] │
│            │   Audio: ████▌░░░░░░░    │                  │
└────────────┴──────────────────────────┴──────────────────┘
│ Status: Preset modified | Auto-save: ON | 1920x1080      │
└──────────────────────────────────────────────────────────┘
```

## Implementation Plan

### Phase 1: Basic Window & ImGui Integration

```cpp
// apps/avs-studio/main.cpp
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <SDL2/SDL.h>

int main() {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  SDL_Window* window = SDL_CreateWindow(...);
  SDL_GLContext gl_context = SDL_GL_CreateContext(window);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init();

  // Main loop
  while (!quit) {
    // Event handling
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      // Handle keyboard shortcuts, window close, etc.
    }

    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Render UI components
    RenderMainWindow();

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(window);
  }

  // Cleanup
}
```

### Phase 2: Effect Tree View

```cpp
void RenderEffectTreeView(PresetState& preset) {
  ImGui::Begin("Effect List");

  for (size_t i = 0; i < preset.effects.size(); ++i) {
    auto& effect = preset.effects[i];

    ImGui::PushID(i);

    // Drag/drop for reordering
    if (ImGui::Selectable(effect.name.c_str(), preset.selectedEffect == i)) {
      preset.selectedEffect = i;
    }

    // Context menu (right-click)
    if (ImGui::BeginPopupContextItem()) {
      if (ImGui::MenuItem("Delete")) {
        preset.effects.erase(preset.effects.begin() + i);
      }
      if (ImGui::MenuItem("Duplicate")) {
        preset.effects.insert(preset.effects.begin() + i + 1, effect);
      }
      ImGui::EndPopup();
    }

    ImGui::PopID();
  }

  if (ImGui::Button("+ Add Effect")) {
    ImGui::OpenPopup("AddEffectMenu");
  }

  RenderAddEffectMenu(preset);

  ImGui::End();
}
```

### Phase 3: Parameter Editor

```cpp
void RenderParameterEditor(Effect& effect) {
  ImGui::Begin("Parameters");

  for (auto& param : effect.params) {
    switch (param.type) {
      case ParamType::Float:
        ImGui::SliderFloat(param.name.c_str(), &param.floatValue,
                          param.min, param.max);
        break;

      case ParamType::Int:
        ImGui::SliderInt(param.name.c_str(), &param.intValue,
                        param.min, param.max);
        break;

      case ParamType::Bool:
        ImGui::Checkbox(param.name.c_str(), &param.boolValue);
        break;

      case ParamType::String:
        ImGui::InputText(param.name.c_str(), param.stringValue);
        break;

      case ParamType::Color:
        ImGui::ColorEdit3(param.name.c_str(), param.colorValue);
        break;
    }

    if (ImGui::IsItemEdited()) {
      preset.markDirty();
      engine.updateEffectParams(effect);
    }
  }

  ImGui::End();
}
```

### Phase 4: Live Preview

```cpp
void RenderPreviewWindow(AVSEngine& engine) {
  ImGui::Begin("Preview");

  // Get framebuffer texture from engine
  GLuint texture = engine.getFramebufferTexture();
  ImVec2 size = ImGui::GetContentRegionAvail();

  // Display rendered frame
  ImGui::Image((void*)(intptr_t)texture, size);

  // Overlay controls
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::SameLine();
  if (engine.isBeat()) {
    ImGui::TextColored(ImVec4(1, 0, 0, 1), "[BEAT]");
  }

  ImGui::End();
}
```

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+N | New preset |
| Ctrl+O | Open preset |
| Ctrl+S | Save preset |
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |
| Space | Trigger beat |
| F11 | Toggle fullscreen |
| Delete | Delete selected effect |
| Ctrl+D | Duplicate selected effect |

## File Operations

### Save Workflow

```cpp
void SavePreset(const std::string& path, const PresetState& preset) {
  if (path.ends_with(".json")) {
    auto ir = convertToIRPreset(preset);
    auto json = avs::preset::serializeToJson(ir);
    writeFile(path, json);
  } else {
    // Binary .avs format
    auto binary = convertToBinaryPreset(preset);
    writeFile(path, binary);
  }
  preset.clearDirty();
}
```

### Auto-Save

- Auto-save to `.avs_autosave` every 2 minutes when dirty
- Restore from auto-save on crash recovery
- Configurable via preferences

## Dependencies

```cmake
# apps/avs-studio/CMakeLists.txt

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(avs-studio
  main.cpp
  ui/MainWindow.cpp
  ui/EffectTreeView.cpp
  ui/ParameterEditor.cpp
  ui/PreviewWindow.cpp
  ui/PresetBrowser.cpp
)

target_link_libraries(avs-studio PRIVATE
  avs::compat
  avs::preset
  SDL2::SDL2
  OpenGL::GL
  imgui
)
```

## Testing Strategy

### Manual Testing Checklist

- [ ] Load existing preset
- [ ] Add/remove effects
- [ ] Reorder effects via drag/drop
- [ ] Edit parameters with live preview
- [ ] Save preset (JSON and binary)
- [ ] Keyboard shortcuts
- [ ] Window resize handling
- [ ] Beat trigger response
- [ ] Audio reactivity

### Future Enhancements

- **Undo/Redo**: Command pattern for all edit operations
- **Preset Templates**: Library of starter presets
- **Effect Search**: Quick search/filter in add menu
- **Code Editor**: Syntax highlighting for EEL scripts
- **Performance Profiling**: Per-effect CPU/GPU timing
- **Export Video**: Render to MP4/WebM
- **Network Sync**: Multi-instance preset sharing
- **Plugin System**: Custom effect development

## References

- [ImGui Documentation](https://github.com/ocornut/imgui)
- [ImGui Examples](https://github.com/ocornut/imgui/tree/master/examples)
- Original Winamp AVS Editor UI patterns
- Modern DAW UI/UX patterns (Ableton, FL Studio)
