# AVS Studio - Interactive Preset Editor

Modern GUI application for creating and editing AVS visualizations in real-time.

## Status

**Implementation**: Architecture documented, stub application (Job #21 - DONE)

See [docs/studio_ui_architecture.md](../../docs/studio_ui_architecture.md) for complete design and implementation guide.

## Overview

AVS Studio provides an interactive visual editor for AVS presets, featuring:
- **Effect Tree Editor**: Drag-and-drop effect management
- **Live Preview**: Real-time rendering with audio reactivity
- **Parameter Controls**: Sliders, color pickers, code editors
- **Dual Format Support**: Save as JSON or binary .avs
- **Keyboard Shortcuts**: Professional workflow support

## Building

### Dependencies

**Required**:
- SDL2 (windowing, input)
- OpenGL 3.3+ (rendering)
- ImGui (UI framework) - *to be added*

**Optional**:
- ImGuiColorTextEdit (syntax highlighting for EEL scripts)
- ImGuiFileDialog (file browser)

### Installation

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install libsdl2-dev libgl-dev

# ImGui will be added to libs/third_party/imgui/
```

### Build

```bash
mkdir build && cd build
cmake .. -DAVS_BUILD_STUDIO=ON
cmake --build . --target avs-studio
./apps/avs-studio
```

## Architecture

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

### Technology Stack

- **UI Framework**: [ImGui](https://github.com/ocornut/imgui) (immediate-mode GUI)
- **Rendering**: Shared AVS engine (avs-compat)
- **Graphics**: SDL2 + OpenGL 3.3
- **Preset I/O**: avs-preset library (JSON + binary)

## Implementation Phases

### Phase 1: Basic Window (1-2 days)

- [x] Architecture documentation
- [ ] ImGui integration
- [ ] SDL2 window creation
- [ ] Basic menu bar
- [ ] Event loop

### Phase 2: Effect Tree (2-3 days)

- [ ] Tree view widget
- [ ] Add/remove effects
- [ ] Drag-and-drop reordering
- [ ] Context menus
- [ ] Effect selection

### Phase 3: Parameters (2-3 days)

- [ ] Parameter type detection
- [ ] Slider controls (int/float)
- [ ] Checkboxes (bool)
- [ ] Text fields (string)
- [ ] Color pickers (RGB/HSV)
- [ ] Real-time parameter updates

### Phase 4: Preview (1-2 days)

- [ ] AVS engine integration
- [ ] Framebuffer texture display
- [ ] Audio input
- [ ] Beat trigger (Space key)
- [ ] FPS counter

### Phase 5: File Operations (1-2 days)

- [ ] File browser (load/save)
- [ ] Format selection (JSON/binary)
- [ ] Auto-save
- [ ] Dirty state tracking
- [ ] Crash recovery

### Phase 6: Polish (2-3 days)

- [ ] Keyboard shortcuts
- [ ] Undo/redo
- [ ] Preferences dialog
- [ ] Status bar
- [ ] Help documentation

**Total Estimated Time**: 10-15 days for experienced developer

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+N | New preset |
| Ctrl+O | Open preset |
| Ctrl+S | Save preset |
| Ctrl+Shift+S | Save As |
| Ctrl+Z | Undo |
| Ctrl+Y / Ctrl+Shift+Z | Redo |
| Space | Trigger beat |
| F11 | Toggle fullscreen |
| Delete | Delete selected effect |
| Ctrl+D | Duplicate selected effect |
| Ctrl+Up/Down | Move effect up/down |
| Ctrl+, | Preferences |

## Code Examples

### Main Loop Structure

```cpp
// apps/avs-studio/main.cpp
#include "ui/MainWindow.h"
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

int main(int argc, char** argv) {
  // Initialize SDL
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  SDL_Window* window = SDL_CreateWindow("AVS Studio",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  SDL_GLContext gl_context = SDL_GL_CreateContext(window);

  // Initialize ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init("#version 330");

  // Initialize AVS engine
  AVSEngine engine(1920, 1080);
  PresetState preset;

  // Main loop
  bool running = true;
  while (running) {
    // Event handling
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT) running = false;
      handleShortcuts(event, preset, engine);
    }

    // Update AVS engine
    engine.update();

    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Render UI
    RenderMainWindow(preset, engine);

    // Render ImGui
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
```

### Effect Tree Widget

```cpp
// apps/avs-studio/ui/EffectTreeView.cpp
void RenderEffectTreeView(PresetState& preset) {
  ImGui::Begin("Effect List");

  // Add effect button
  if (ImGui::Button("+ Add Effect")) {
    ImGui::OpenPopup("AddEffectPopup");
  }

  if (ImGui::BeginPopup("AddEffectPopup")) {
    if (ImGui::BeginMenu("Render")) {
      if (ImGui::MenuItem("Simple")) preset.addEffect("Render / Simple");
      if (ImGui::MenuItem("Superscope")) preset.addEffect("Render / Superscope");
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Trans")) {
      if (ImGui::MenuItem("Blur")) preset.addEffect("Trans / Blur");
      if (ImGui::MenuItem("Water")) preset.addEffect("Trans / Water");
      ImGui::EndMenu();
    }
    ImGui::EndPopup();
  }

  // Effect list
  for (size_t i = 0; i < preset.effects.size(); ++i) {
    ImGui::PushID(i);

    // Drag source
    if (ImGui::Selectable(preset.effects[i].name.c_str(),
                          preset.selectedIndex == i)) {
      preset.selectedIndex = i;
    }

    // Drag-drop reordering
    if (ImGui::BeginDragDropSource()) {
      ImGui::SetDragDropPayload("EFFECT_INDEX", &i, sizeof(i));
      ImGui::Text("%s", preset.effects[i].name.c_str());
      ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EFFECT_INDEX")) {
        size_t sourceIndex = *(const size_t*)payload->Data;
        preset.reorderEffect(sourceIndex, i);
      }
      ImGui::EndDragDropTarget();
    }

    // Context menu
    if (ImGui::BeginPopupContextItem()) {
      if (ImGui::MenuItem("Delete")) {
        preset.removeEffect(i);
      }
      if (ImGui::MenuItem("Duplicate")) {
        preset.duplicateEffect(i);
      }
      ImGui::EndPopup();
    }

    ImGui::PopID();
  }

  ImGui::End();
}
```

### Parameter Editor

```cpp
// apps/avs-studio/ui/ParameterEditor.cpp
void RenderParameterEditor(PresetState& preset) {
  if (preset.selectedIndex < 0) return;

  ImGui::Begin("Parameters");

  auto& effect = preset.effects[preset.selectedIndex];

  for (auto& param : effect.params) {
    bool changed = false;

    switch (param.type) {
      case ParamType::Float:
        changed = ImGui::SliderFloat(param.name.c_str(),
                                     &param.floatValue,
                                     param.min, param.max);
        break;

      case ParamType::Int:
        changed = ImGui::SliderInt(param.name.c_str(),
                                   &param.intValue,
                                   (int)param.min, (int)param.max);
        break;

      case ParamType::Bool:
        changed = ImGui::Checkbox(param.name.c_str(), &param.boolValue);
        break;

      case ParamType::Color:
        changed = ImGui::ColorEdit3(param.name.c_str(), param.colorValue);
        break;

      case ParamType::String:
        char buffer[256];
        strncpy(buffer, param.stringValue.c_str(), 255);
        if (ImGui::InputText(param.name.c_str(), buffer, 256)) {
          param.stringValue = buffer;
          changed = true;
        }
        break;
    }

    if (changed) {
      preset.markDirty();
      preset.updateEffect(effect);
    }
  }

  ImGui::End();
}
```

## Testing

### Manual Test Checklist

- [ ] Launch application
- [ ] Create new preset
- [ ] Add effects to tree
- [ ] Reorder effects via drag-drop
- [ ] Edit parameters
- [ ] See live preview update
- [ ] Trigger beat with Space
- [ ] Save preset (JSON)
- [ ] Save preset (binary .avs)
- [ ] Load saved preset
- [ ] Test keyboard shortcuts
- [ ] Resize window

### Integration Tests

```bash
# Test headless rendering still works
./avs-player --preset test.avs --headless --frames 10

# Compare golden hashes
./avs-player --preset test.avs --golden-md5 expected.md5
```

## Future Enhancements

- **Code Editor**: Syntax highlighting for EEL scripts (ImGuiColorTextEdit)
- **Undo/Redo**: Full edit history
- **Preset Templates**: Library of starter presets
- **Video Export**: Render to MP4/WebM
- **Performance Profiler**: Per-effect timing
- **Plugin Development**: Custom effect development UI
- **Network Sync**: Multi-instance collaboration
- **Preset Browser**: Visual preset library

## References

- [ImGui Documentation](https://github.com/ocornut/imgui)
- [ImGui Demo](https://github.com/ocornut/imgui/blob/master/imgui_demo.cpp)
- [Studio UI Architecture](../../docs/studio_ui_architecture.md)
- Original Winamp AVS Editor
- [Dear PyGui](https://github.com/hoffstadt/DearPyGui) (ImGui wrapper for Python)

## Contributing

To implement AVS Studio:

1. Add ImGui to `libs/third_party/imgui/`
2. Update `apps/avs-studio/CMakeLists.txt` with ImGui dependencies
3. Implement main window in `apps/avs-studio/main.cpp`
4. Create UI components in `apps/avs-studio/ui/`
5. Follow architecture in `docs/studio_ui_architecture.md`
6. Test with existing presets

**Estimated effort**: 10-15 days for experienced developer with ImGui experience.
