# Beat Gating Helpers

The `effects/gating` helper encapsulates the simplified on-beat and sticky
latching logic required by several classic AVS renderers.  It is intentionally
stateless outside of the `BeatGate` class so that individual effects can embed
an instance and drive it with the per-frame audio state supplied by the engine.

## Gate states

`BeatGate::step()` returns a `GateResult` describing both whether the effect
should render for the current frame and which logical state was active.  The
returned `GateFlag` values map to the colour key rendered by
`transform_affine`:

- `Beat` — a fresh beat triggered this frame.
- `Hold` — the post-beat countdown is still active.
- `Sticky` — the sticky latch is engaged.
- `Off` — rendering should be skipped for the frame.

Effects can use the flag independently of the `render` boolean to visualise
state changes even when the pipeline skips drawing.

## Options

| Field | Purpose |
|-------|---------|
| `enableOnBeat` | When `true` the gate only activates on beats.  When `false`
  the gate is always active. |
| `stickyToggle` | Causes each beat to toggle a sticky latch instead of using a
  transient countdown. |
| `onlySticky` | Requires the sticky latch to be active before rendering.  Beat
  events are still reported via the `Beat` flag so that logs stay informative. |
| `holdFrames` | Number of frames the gate remains in the `Hold` state after a
  beat when `stickyToggle` is `false`. |

The class keeps no internal notion of time; callers are expected to advance the
`RenderContext::frameIndex` and supply a boolean beat flag each frame.  The
implementation is deterministic and side-effect free, making it straightforward
to unit test (see `tests/core/test_gating.cpp`).
