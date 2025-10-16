# Framebuffer utilities and register effects

The runtime now exposes a lightweight bridge between the high level effect
engine and the `avs::runtime::Framebuffers` manager that powers the classic
Winamp-style preset pipeline. The helper functions in
`src/runtime/framebuffers.{h,cpp}` convert the runtime double-buffered storage
into the simple `FrameBufferView` structure consumed by the core effects and
make it trivial to keep the views in sync when the runtime swaps buffers.

```cpp
avs::runtime::Framebuffers registers(width, height);
auto views = avs::runtime::makeFrameBuffers(registers);
ProcessContext ctx{timing, audio, views, rng, eel};
```

Because the views keep the original runtime storage alive, effects can now read
and write through `ProcessContext::fb.registers`. This unlocks the register
style buffer effects:

- **Save Buffer (`save_buf`)** copies the current framebuffer into a named slot
  (`A`–`H`). The effect exposes a `slot` select parameter and honours string,
  integer or floating-point assignments.
- **Restore Buffer (`restore_buf`)** restores the pixels from a previously
  stored slot back into the active framebuffer.

Both effects are safe when the runtime resizes or the slot has not yet been
populated; they simply short-circuit without modifying the framebuffer. The
`RuntimeCompositing.SaveRestoreBuffers` unit test exercises a full save/restore
round trip across register **B** using the runtime bridge helpers.
