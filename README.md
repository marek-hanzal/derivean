# DeRivean

A desktop voxel physics sandbox on a simulated planet. The launcher takes a seed, the planet's age, and an optional spawn position, and generates that world before you enter it.

## Run on macOS (Apple Silicon)

Install mise and the Xcode Command Line Tools, then run these commands from the repository root.

```sh
mise trust --all
mise run setup
mise run play
```

The first build downloads Jolt, Electron, and JavaScript dependencies. Run `mise run check` to build the game, type-check the renderer, and run the native and bridge tests. See the [architecture](docs/architecture.md) for module ownership.

| Input | Action |
| --- | --- |
| WASD and mouse | Move and look |
| Space / Shift | Jump / sprint |
| Space twice | Start or stop flying. In flight Space rises, Shift sinks, and Ctrl boosts |
| Left mouse button | Excavate. Hold for repeat cuts |
| Right mouse button | Place a basalt voxel from your finite stock |
| 1 / 2 | Select a sphere / cube |
| Mouse wheel | Change brush size |
| Esc | Release the mouse and pause |
| F3 | Show position, timings, and material balances |
| I | Inspect the ground under the crosshair, or under you when nothing is targeted |
| M | Open or close the map. On it: wheel zooms, drag pans, C centres on you, Shift and a click travels there, Esc closes |
| L | Switch between daylight and plain relief shading |
| P | Hold or release the world clock |
| , / . | Slow down / speed up the clock: real time, a minute, an hour or a day per second |
| N / B | Skip a day / a season |
