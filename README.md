# Gravity Sim

A C++17 gravity simulator with SFML 3 rendering, collision merging, orbit trails,
asteroid launching, and trajectory previews.

On Windows, run `./run.ps1` to build and launch the Release version. The CMake
presets use MinGW at `C:/msys64/ucrt64/bin/g++.exe` and the bundled Ninja.
CMake downloads SFML and nlohmann/json on the first configure.

## Saving experiments

- **F5** saves the current bodies to `simulation.json` in the working directory,
  replacing the previous snapshot after the new file has been written successfully.
- **F9** loads that file, fits the camera to the scene, and pauses. Press **Space**
  to continue. The loaded bodies become the new **R** reset point.
- The HUD shows success or failure. A failed load leaves the current scene intact.

Snapshots preserve full double precision for positions, velocities, mass, and
radius, plus body names, colors, black hole flags, elapsed time, simulation speed,
and launch properties. An asteroid still being aimed is excluded. Trails,
selection, and conservation baselines restart when loading. Reset starts time at
zero, using the loaded bodies. The versioned JSON format supports up to 10000
bodies and 16 MiB per file. Copy or rename snapshots to keep multiple experiments;
restore a snapshot to `simulation.json` before pressing F9.

## Checks

```powershell
cmake --preset release
cmake --build --preset release
ctest --test-dir build/release --output-on-failure
```
