# Framebuffer Operations

The framebuffer utilities provide a deterministic dual-buffer model that mirrors
the behaviour of the original AVS runtime. Effects operate on the *current*
frame while retaining a copy of the *previous* frame for sampling. Eight named
slots (`A`–`H`) can be used to persist intermediate results across frames.

## Frame management

`Framebuffers::beginFrame()` copies the previous output into the working buffer
for the next frame. `finishFrame()` applies persistent overlays and advances the
frame index. Effects access raw pixels through `FrameView` structures which
expose width, height and stride information.

## Clear

`effect_clear` fills the active buffer with a constant colour. The operation
supports the legacy blend modes (replace, additive, average and "default"
blend) and honours the `CLEARFIRSTFRAME` flag by skipping subsequent frames
when requested.

## Save and restore

`effect_save` copies the active buffer into a named slot. `effect_restore`
re-populates the active buffer from that slot when the dimensions match. Slots
are invalidated automatically if the framebuffer is resized.

## Wrap

`effect_wrap` reads from the previous frame using toroidal addressing so that
out-of-bounds samples wrap around both axes. This reproduces the behaviour of
the classic Wrap effect and is used as the backbone for movement and transition
pipelines.

## Slide transitions

`effect_in_slide` and `effect_out_slide` perform deterministic screen wipes by
shifting the previous frame towards or away from the viewport boundary while
filling exposed regions with black. They provide the building blocks for
compound transition presets.

`effect_transition` blends the current and previous buffers using a linear
progress value in the range `[0,1]` which allows smooth cross-fades.

## Persistent overlays

`effect_persist_title`, `effect_persist_text1` and `effect_persist_text2`
activate frame-persistent stripes. Each overlay tracks its own decay counter,
blending towards the configured colour while gradually fading out over several
frames. Triggering an overlay again resets its timer which matches the way AVS
presents song titles and metadata.

## Testing

Deterministic ten-frame golden tests are stored under
`tests/presets/fb_ops`. Each scenario exercises the state machine:

* `save_restore.md5` validates round-trip buffer saves
* `wrap.md5` verifies toroidal sampling on both axes
* `slide_transition.md5` covers the slide and cross-fade logic
* `persist.md5` ensures overlays remain visible for the configured duration

The tests compute MD5 hashes for every frame making it easy to spot regressions
in CI without shipping large image artefacts.
