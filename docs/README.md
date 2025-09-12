# AVS Cross-Platform Port

This project aims to reimplement Winamp's Advanced Visualization Studio (AVS) as a standalone,
portable engine and tooling.

## Build Instructions

```bash
git clone <repo-url>
cd avs-cross-platform  # repository root
mkdir build && cd build
cmake ..
cmake --build .
ctest
```

Run the stub player after building:

```bash
./apps/avs-player/avs-player
```

More documentation will be added as the project evolves.
