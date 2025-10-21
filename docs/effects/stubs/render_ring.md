# Render / Ring

The Render / Ring effect draws an animated ring whose radius responds to the audio
waveform or spectrum. The ring cycles through the configured palette while it
pulses in sync with the selected audio channel.

## Parameters

- **source** (`"osc"` or `"spec"`): choose between waveform or spectrum data.
- **channel** (`"left"`, `"right"`, `"mix"`/`"mid"`): select which audio channel
  modulates the radius.
- **placement** (`"left"`, `"right"`, `"center"` or legacy `"top"`/`"bottom"`):
  horizontal offset of the ring.
- **size** (`int` in the range 1–64): base ring radius as a fraction of the frame size.
- **colors** (`"#RRGGBB"` list or `color0`…`color15` ints): palette for the
  gradient cycle (up to 16 entries).

Legacy presets may also provide the packed `effect` bitfield and `source` flag from
Nullsoft AVS. These values are decoded for compatibility.
