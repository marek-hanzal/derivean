import type { GameBridge } from "../../../contract/GameBridge";

// The bridge as the renderer that draws cubes uses it: every world it starts is one of cubes. The engine makes
// either cubes or height-field patches until cubes are all there is.
export function cubeWorlds(bridge: GameBridge): GameBridge {
  return {
    startWorld: (options) => bridge.startWorld({ seed: 42, age: 4.6e9, ...options, cubes: true }),
    advance: (command) => bridge.advance(command),
    inspect: (x, z) => bridge.inspect(x, z),
    chart: (request) => bridge.chart(request),
    quit: () => bridge.quit(),
  };
}
