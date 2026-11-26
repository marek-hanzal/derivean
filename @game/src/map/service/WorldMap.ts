import type { Frame, GameBridge, Point, WorldInfo } from "../../../contract/GameBridge";
import { readClock } from "../../calendar/fn/readClock";
import type { Pilot } from "../../control/type/Pilot";
import { worldName } from "../../hud/fn/worldName";
import { ChartWater } from "../constant/chart";
import { ClickSlack, MotionMs, OpeningSpan, RedrawShift, RestMs, TravelClearance, ZoomPerWheelPixel } from "../constant/map";
import { readPoint } from "../fn/readPoint";
import { contourInterval, lengthLabel, scaleBar } from "../fn/roundLength";
import { chartSquare, scaleLimits, screenOf, worldAt, zoomAt } from "../fn/view";
import type { Extent, MapView } from "../type/MapView";
import { buildMapChrome } from "../ui/MapChrome";
import { createChartFeed, type ChartFeed } from "./ChartFeed";
import { createChartSurface, type ChartSurface } from "./ChartSurface";

export interface MapHost {
  // True while the player is in the world with the pointer captured: the only state a key opens the map from.
  playing(): boolean;
  // Captures the pointer again.
  lock(): void | Promise<void>;
}

const TextMs = 120;
const ScaleBarRoom = 150;

// The map over the game: opened and closed with M, drawn from charts of the simulation. While it is open the
// pointer is free and the world's controls rest, but the world itself runs on: `steer` keeps the session
// simulating with a pilot that only waits.
export function createWorldMap(root: HTMLElement, bridge: Pick<GameBridge, "chart">, host: MapHost) {
  const chrome = buildMapChrome(root);
  let drawing: { surface: ChartSurface; feed: ChartFeed } | undefined;
  let open = false;
  let following = true;
  let view: MapView = { x: 0, z: 0, scale: 0 };
  // The view the chart canvas shows, when it was drawn, and whether at reduced sharpness.
  let painted: { view: MapView; extent: Extent; time: number; coarse: boolean } | undefined;
  let extent: Extent = [1, 1];
  let latest: { world: WorldInfo; frame: Frame } | undefined;
  let cursor: [number, number] | undefined;
  let press: { x: number; y: number; moved: boolean } | undefined;
  let travel: Point | null = null;
  let texted = -Infinity;
  // The map was closed with Escape and the pointer waits for the key to come up.
  let escaping = false;

  // A map that closes without the pointer being captured again leaves the game unlocked with nothing on screen
  // saying so. The lock's own listeners decide what an unlocked game shows.
  const unlocked = () => document.dispatchEvent(new Event("pointerlockchange"));
  const capture = () => void Promise.resolve().then(() => host.lock()).catch(unlocked);

  // `resume` captures the pointer again: at once, or when Escape is released. Chromium ends a pointer lock
  // that is in force when Escape comes up, exactly as if the player had asked to leave it, so a map closed
  // with that key must not hold the lock before then.
  function hide(resume: "now" | "released" | "never") {
    if (!open) return;
    open = false;
    root.hidden = true;
    press = undefined;
    cursor = undefined;
    travel = null;
    delete root.dataset.dragging;
    escaping = resume === "released";
    if (resume === "now") capture();
    else if (resume === "never") unlocked();
  }

  // Opens the map on the player, or turns an open one back to them. `span` is the metres across the shorter
  // side of the viewport; without it the last zoom is kept.
  function show(span?: number) {
    if (!latest) return;
    if (!drawing) {
      const surface = createChartSurface(chrome.canvas);
      drawing = { surface, feed: createChartFeed(bridge, surface, () => hide("never")) };
    }
    open = true;
    escaping = false;
    following = true;
    painted = undefined;
    root.hidden = false;
    extent = [root.clientWidth, root.clientHeight];
    if (span !== undefined || view.scale === 0) view = { ...view, scale: (span ?? OpeningSpan) / Math.max(1, Math.min(...extent)) };
    if (document.pointerLockElement) document.exitPointerLock();
  }

  // The engine takes a traveller within one planet span of the origin and settles them on the ground itself:
  // lifted out of it if the chart was too low, dropped onto it if too high.
  function travelTo(x: number, z: number) {
    const here = drawing?.feed.at(x, z);
    const reach = latest?.world.planetSpan ?? 0;
    if (!here || Math.abs(x) > reach || Math.abs(z) > reach) return;
    travel = [x, (here.waters === ChartWater.Dry ? here.ground : Math.max(here.ground, here.water)) + TravelClearance, z];
    following = true;
  }

  function write(world: WorldInfo, frame: Frame) {
    const at = cursor ? worldAt(view, extent, ...cursor) : [frame.eye[0], frame.eye[2]] as [number, number];
    const bar = scaleBar(view.scale, ScaleBarRoom);
    chrome.world.textContent = worldName(world);
    chrome.date.textContent = readClock(frame.calendar);
    chrome.subject.textContent = cursor ? "AT THE CURSOR" : "AT THE PLAYER";
    chrome.readout.replaceChildren(...readPoint(at[0], at[1], drawing?.feed.at(at[0], at[1]), world, drawing?.feed.streamFloor() ?? Infinity).flatMap(([name, value]) => {
      const term = document.createElement("dt");
      const detail = document.createElement("dd");
      term.textContent = name;
      detail.textContent = value;
      return [term, detail];
    }));
    chrome.bar.style.width = `${bar.pixels}px`;
    chrome.length.textContent = bar.label;
    chrome.contours.textContent = `Contours every ${lengthLabel(contourInterval(view.scale))}`;
    chrome.planetKey.hidden = !drawing?.feed.marksPlanet();
    chrome.centre.hidden = following;
  }

  const local = (event: MouseEvent): [number, number] => {
    const box = chrome.canvas.getBoundingClientRect();
    return [event.clientX - box.left, event.clientY - box.top];
  };
  chrome.canvas.addEventListener("wheel", (event) => {
    if (!latest) return;
    event.preventDefault();
    view = zoomAt(view, extent, ...local(event), Math.exp(event.deltaY * ZoomPerWheelPixel), scaleLimits(extent, latest.world.planetSpan));
    following = false;
  }, { passive: false });
  chrome.canvas.addEventListener("pointerdown", (event) => {
    if (event.button !== 0) return;
    press = { x: event.clientX, y: event.clientY, moved: false };
    chrome.canvas.setPointerCapture(event.pointerId);
  });
  chrome.canvas.addEventListener("pointermove", (event) => {
    cursor = local(event);
    if (!press || (!press.moved && Math.hypot(event.clientX - press.x, event.clientY - press.y) < ClickSlack)) return;
    view = { ...view, x: view.x - (event.clientX - press.x) * view.scale, z: view.z - (event.clientY - press.y) * view.scale };
    press = { x: event.clientX, y: event.clientY, moved: true };
    following = false;
    root.dataset.dragging = "true";
  });
  for (const kind of ["pointerup", "pointercancel"] as const) {
    chrome.canvas.addEventListener(kind, (event) => {
      if (press && !press.moved && event.shiftKey && kind === "pointerup") travelTo(...worldAt(view, extent, ...local(event)));
      press = undefined;
      delete root.dataset.dragging;
    });
  }
  chrome.canvas.addEventListener("pointerleave", () => { cursor = undefined; });
  chrome.centre.addEventListener("click", () => { following = true; });
  window.addEventListener("keydown", (event) => {
    if (event.repeat) return;
    if (!open) {
      if (event.code === "KeyM" && host.playing()) show();
    } else if (event.code === "KeyM" || event.code === "Escape") {
      event.preventDefault();
      hide(event.code === "Escape" ? "released" : "now");
    } else if (event.code === "KeyC") {
      following = true;
    }
  });
  window.addEventListener("keyup", (event) => {
    if (!escaping || event.code !== "Escape") return;
    escaping = false;
    capture();
  });
  window.addEventListener("blur", () => {
    if (!escaping) return;
    escaping = false;
    unlocked();
  });

  return {
    get open() { return open; },
    show,
    hide: () => hide("never"),
    // Call every frame something is on screen.
    paint(world: WorldInfo, frame: Frame, yaw: number, time: number) {
      if (latest && latest.world !== world) drawing?.feed.reset();
      latest = { world, frame };
      if (!open || !drawing) return;
      const { surface, feed } = drawing;
      extent = [root.clientWidth, root.clientHeight];
      if (extent[0] < 1 || extent[1] < 1) return;
      const [closest, widest] = scaleLimits(extent, world.planetSpan);
      if (following) view = { ...view, x: frame.eye[0], z: frame.eye[2] };
      view = { ...view, scale: Math.max(closest, Math.min(widest, view.scale)) };
      feed.want(chartSquare(view, extent), time);
      const ratio = window.devicePixelRatio;
      const moved = !painted || painted.view.scale !== view.scale || painted.extent[0] !== extent[0] || painted.extent[1] !== extent[1]
        || Math.hypot(view.x - painted.view.x, view.z - painted.view.z) * ratio >= RedrawShift * view.scale;
      const coarse = moved && painted !== undefined && time - painted.time < MotionMs;
      const rested = !moved && painted !== undefined && painted.coarse && time - painted.time >= RestMs;
      if (feed.takeFresh() || moved || rested) {
        surface.draw(view, extent, { seaLevel: world.seaLevel, interval: contourInterval(view.scale) }, !coarse);
        painted = { view, extent, time, coarse };
      }
      const [px, py] = screenOf(view, extent, frame.eye[0], frame.eye[2]);
      chrome.player.style.visibility = px >= 0 && py >= 0 && px <= extent[0] && py <= extent[1] ? "visible" : "hidden";
      // Yaw 0 looks north and grows towards the west; the arrow points up at rest and CSS turns clockwise.
      chrome.player.style.transform = `translate(${px.toFixed(1)}px, ${py.toFixed(1)}px) rotate(${(-yaw).toFixed(3)}rad)`;
      if (time - texted >= TextMs) {
        texted = time;
        write(world, frame);
      }
    },
    // Wraps whoever steers the player. With the map open the world keeps running under a pilot that only waits,
    // or travels where the map was shift-clicked.
    steer(pilot: Pilot): Pilot {
      return {
        get yaw() { return pilot.yaw; },
        get pitch() { return pilot.pitch; },
        get radius() { return pilot.radius; },
        get shape() { return pilot.shape; },
        active: () => open || pilot.active(),
        command(elapsed, time, frame) {
          const command = pilot.command(elapsed, time, frame);
          if (!open) return command;
          const teleport = travel;
          travel = null;
          return { ...command, active: true, x: 0, z: 0, jump: false, sprint: false, dig: false, place: false, fly: false, boost: false, ascend: 0, teleport };
        },
      };
    },
    // True when the map is closed, or open with the sharp chart of its view on screen.
    settled: () => !open || Boolean(drawing?.feed.settled(chartSquare(view, extent)) && painted && !painted.coarse),
    // For a capture record.
    describe() {
      if (!open) return null;
      return {
        span: Math.round(view.scale * Math.min(...extent)), metresPerPixel: Number(view.scale.toPrecision(4)), following,
        contourInterval: contourInterval(view.scale), chart: drawing?.feed.describe() ?? null,
      };
    },
  };
}
