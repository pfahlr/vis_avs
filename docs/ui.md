# AVS Studio UI

`avs-studio` is a simple editor for AVS presets built with Dear ImGui using the docking branch.
It provides:

- **Preset browser** on the left listing `.avs` files.
- **Properties panel** on the right exposing effect parameters and an EEL script editor.
- **Waveform and spectrum scopes** driven by the live audio input.
- **Error panel** showing EEL compile errors.

Selecting a preset loads it instantly. Editing parameters such as the blur radius or EEL scripts
updates the render on the next frame. Scripts can be compiled via the *Compile* button and any
errors appear in the error panel. The renderer hot-reloads presets when their source files change.
