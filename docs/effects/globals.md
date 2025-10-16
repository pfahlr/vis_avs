# Globals effect and shared state

The `globals` effect exposes persistent registers (`g1` … `g64`) that mirror the
behaviour of Winamp AVS's global variables. Each pipeline owns an
`avs::runtime::GlobalState` instance that is injected into the
`avs::core::RenderContext` before every effect. Any effect can read or write the
registers for cross-effect coordination.

`globals` is registered by `avs::effects::registerCoreEffects()` and executes
short EEL scripts. Parameters are supplied through `avs::core::ParamBlock`
strings:

| Key    | Type   | Default | Description |
|--------|--------|---------|-------------|
| `init` | string | `""` | Optional script executed once after compilation. |
| `frame` | string | `""` | Script executed at the start of every frame. |

Before execution the effect copies the current `g*` register values from the
shared `GlobalState`. After the scripts finish the updated values are written
back so downstream effects observe the same state. Scripts receive the standard
variables exposed by the scripted renderer (`frame`, `time`) plus the global
registers themselves.

Example: accumulate a smoothed beat indicator and expose it to later effects.

```cpp
avs::core::ParamBlock globals;
globals.setString("frame", "g1 = g1*0.9 + (beat?1:0);");
pipeline.add("globals", globals);
```

Any effect that sees the same `RenderContext` (including scripted pixel
effects) can read `g1` to react to the aggregated signal.

