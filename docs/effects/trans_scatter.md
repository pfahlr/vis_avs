# Trans / Scatter

The scatter transition randomly samples pixels from a small 7×7 neighbourhood
around the current location.  The probability of selecting a neighbour falls
off with distance because the legacy lookup table contains more entries for
small offsets.  The top and bottom four rows are copied verbatim to match the
behaviour of Winamp's original R_Scat module.

## Parameters

| Token / key | Description |
|-------------|-------------|
| `enabled`   | Optional boolean flag that disables the effect when set to `false`.  Defaults to `true` for legacy parity. |

## Determinism

Random samples are drawn from the per-frame deterministic RNG provided by the
rendering pipeline.  When the frame index and global seed are identical the
scatter pattern is bit-for-bit reproducible, which makes the effect suitable
for automated regression testing.
