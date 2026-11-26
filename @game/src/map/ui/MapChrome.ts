import { select } from "../../dom/fn/select";

const Markup = `
  <canvas class="map-chart" aria-label="Map of the world"></canvas>
  <svg class="map-player" viewBox="-17 -17 34 34" aria-hidden="true"><circle r="15"/><path d="M0 -10.5 L7.5 9 L0 4.5 L-7.5 9 Z"/></svg>
  <header class="map-title map-panel"><i class="diamond"></i><span>MAP</span><strong data-map="world"></strong><span data-map="date"></span></header>
  <section class="map-readout map-panel" aria-live="off">
    <h2 data-map="subject"></h2>
    <dl data-map="readout"></dl>
  </section>
  <footer class="map-foot">
    <div class="map-scale map-panel">
      <b class="map-north" title="North is up">N</b>
      <div><i data-map="bar"></i><span data-map="length"></span></div>
    </div>
    <ul class="map-legend map-panel">
      <li><i class="map-key-contour"></i><span data-map="contours"></span></li>
      <li><i class="map-key-stream"></i>Streams and rivers</li>
      <li data-map="planet-key"><i class="map-key-planet"></i>Planet-scale data, sharper once visited</li>
    </ul>
    <button type="button" class="map-centre" data-map="centre" hidden>Centre on player <kbd>C</kbd></button>
    <div class="map-keys map-panel">
      <span><kbd>M</kbd> Close</span><span><kbd>WHEEL</kbd> Zoom</span><span><kbd>DRAG</kbd> Pan</span><span><kbd>C</kbd> Centre</span>
      <span><kbd>SHIFT</kbd> + <kbd>CLICK</kbd> Travel</span>
    </div>
  </footer>
`;

// Builds the map's fixed parts inside its section: the chart canvas under the player's arrow and the panels.
export function buildMapChrome(root: HTMLElement) {
  root.innerHTML = Markup;
  const part = <T extends HTMLElement>(name: string) => select<T>(root, `[data-map="${name}"]`);
  return {
    canvas: select<HTMLCanvasElement>(root, ".map-chart"),
    player: select(root, ".map-player"),
    world: part("world"),
    date: part("date"),
    subject: part("subject"),
    readout: part("readout"),
    bar: part("bar"),
    length: part("length"),
    contours: part("contours"),
    planetKey: part("planet-key"),
    centre: part<HTMLButtonElement>("centre"),
  };
}

export type MapChrome = ReturnType<typeof buildMapChrome>;
