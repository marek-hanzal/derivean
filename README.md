# DeRivean

A desktop voxel physics sandbox. The Stacked Channels test bed has two destructible troughs, a walking deck, and a sand bank suspended above open space.

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
| Left mouse button | Excavate. Hold for repeat cuts |
| Right mouse button | Place a basalt voxel from your finite stock |
| 1 / 2 | Select a sphere / cube |
| Mouse wheel | Change brush size |
| Esc | Release the mouse and pause |
| F3 | Show position, timings, and material balances |

## Try the stacked channels

1. Leave the opening view facing north. Hold **A** for roughly a second, then **W** for roughly a second to reach the front-left edge of the deck. The **F3** eye position should be near `(-3.9, 1.6, -2.8)`.
2. Select **2** and shrink the brush to **0.88 m**. Look up at the rusty clay underside near `(-4.0, 2.27, -4.45)` and cut once. Falling water collects in the lower trough and throws off foam where it lands.
3. Aim down at the light chalk bottom near `(-4.0, -0.78, -4.30)` and cut once. The caught water now falls into open space. **F3** tracks pooled, falling, and escaped water against the original 6,234.4 L.
4. The sand bank is on the right side of the deck near `(4.75, 0, -0.63)`. Cut the shale underneath it to release the grains. Check the settled count or cut through the deck as well to send them into open space.
