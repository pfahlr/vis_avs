# Holden05: Multi Delay

This effect exposes six shared delay buffers. Each instance can either store the
current frame into one of the buffers or fetch a delayed frame back into the
pipeline. Buffers may use fixed frame counts or reuse the measured beat length
to determine their history depth. Frame memory grows with the active delay and
is recycled deterministically, matching the behaviour of the original Winamp
plug-in.
