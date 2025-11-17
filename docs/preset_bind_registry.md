# Preset Parse → Bind Pipeline

The preset loader now produces an intermediate representation (IR) before any
legacy effects are instantiated. `libs/avs-preset/include/avs/preset/ir.h`
defines the JSON-like structures that parser and binder share.

1. `avs::preset::parse_legacy_preset()` reads a legacy `.avs` payload and
   returns an `IRPreset`. Parsing stops at token/parameter extraction—no effect
   headers are included here.
2. `avs::preset::bind_preset()` consumes the IR and an
   `avs::effects::Registry` to build a runtime `avs::effects::Graph`.
3. Apps wire concrete legacy effects by calling
   `avs::effects::legacy::register_all(registry);` before binding.

## Adding a new legacy effect

1. Write a small factory that returns `std::unique_ptr<avs::effects::IEffect>`
   (see `libs/avs-effects-legacy/src/trans/effect_color_modifier_registry.cpp`
   for the pattern).
2. Register the descriptor in
   `libs/avs-effects-legacy/src/register_all.cpp` so it participates in the
   shared registry.
3. Optional: add a regression test in
   `libs/avs-effects-legacy/tests/test_registry.cpp` that exercises token
   normalization/factory creation for the new effect.
