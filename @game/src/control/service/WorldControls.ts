import { Euler, Vector3 } from "three";
import type { BrushShape, Frame, Point, WorldInfo } from "../../../contract/GameBridge";
import type { Pilot, Steering } from "../type/Pilot";

const CutInterval = 180;
const DoubleTap = 300;

export function createWorldControls(canvas: HTMLCanvasElement, onLock: (locked: boolean) => void) {
  const keys = new Set<string>();
  const direction = new Vector3();
  const rotation = new Euler(0, 0, 0, "YXZ");
  let yaw = 0;
  let pitch = -0.035;
  let shape: BrushShape = "sphere";
  let radius = 1;
  let maxRadius = 1;
  let holding = false;
  let digQueued = false;
  let placeQueued = false;
  let jumpQueued = false;
  let flyQueued = false;
  let lastCut = -Infinity;
  let lastSpace = -Infinity;

  const locked = () => document.pointerLockElement === canvas;
  const held = (...codes: string[]) => codes.some((code) => keys.has(code));
  function clear() {
    keys.clear();
    holding = false;
    digQueued = false;
    placeQueued = false;
    jumpQueued = false;
    flyQueued = false;
    lastSpace = -Infinity;
  }

  window.addEventListener("blur", () => {
    clear();
    if (locked()) document.exitPointerLock();
  });
  document.addEventListener("pointerlockchange", () => {
    if (!locked()) clear();
    onLock(locked());
  });
  document.addEventListener("mousemove", (event) => {
    if (!locked()) return;
    yaw -= event.movementX * 0.0024;
    pitch = Math.max(-1.54, Math.min(1.54, pitch - event.movementY * 0.0024));
  });
  document.addEventListener("mousedown", (event) => {
    if (!locked()) return;
    if (event.button === 0) {
      digQueued = true;
      holding = true;
    }
    if (event.button === 2) placeQueued = true;
  });
  canvas.addEventListener("contextmenu", (event) => {
    if (locked()) event.preventDefault();
  });
  document.addEventListener("mouseup", (event) => {
    if (event.button === 0) holding = false;
  });
  canvas.addEventListener("wheel", (event) => {
    if (!locked() || !event.deltaY) return;
    event.preventDefault();
    radius = Math.max(1, Math.min(maxRadius, radius - Math.sign(event.deltaY)));
  }, { passive: false });
  document.addEventListener("keydown", (event) => {
    if (!locked()) return;
    if (event.code === "Escape") {
      document.exitPointerLock();
      return;
    }
    keys.add(event.code);
    if (event.code === "Digit1") shape = "sphere";
    if (event.code === "Digit2") shape = "cube";
    if (event.code === "Space") {
      event.preventDefault();
      if (event.repeat) return;
      jumpQueued = true;
      // The second press of a double tap toggles flight; a third press starts a new pair.
      const second = event.timeStamp - lastSpace <= DoubleTap;
      flyQueued ||= second;
      lastSpace = second ? -Infinity : event.timeStamp;
    }
  });
  document.addEventListener("keyup", (event) => keys.delete(event.code));

  function command(elapsed: number, time: number, frame: Frame | undefined): Steering {
    const active = locked();
    const flying = frame?.flying ?? false;
    const sideways = Number(keys.has("KeyD")) - Number(keys.has("KeyA"));
    const forward = Number(keys.has("KeyS")) - Number(keys.has("KeyW"));
    const diagonal = Math.max(1, Math.hypot(sideways, forward));
    const cos = Math.cos(yaw);
    const sin = Math.sin(yaw);
    const shift = held("ShiftLeft", "ShiftRight");
    const aim: Point = direction.set(0, 0, -1).applyEuler(rotation.set(pitch, yaw, 0)).toArray();
    const dig = active && (holding || digQueued) && time - lastCut >= CutInterval;
    if (dig) {
      digQueued = false;
      lastCut = time;
    }
    const place = active && placeQueued;
    placeQueued = false;
    const fly = active && flyQueued;
    const jump = jumpQueued && !flying && !fly;
    jumpQueued = false;
    flyQueued = false;
    return {
      elapsed: Math.min(elapsed, 0.1), active, direction: aim, radius, shape, dig, place, jump, fly,
      x: Math.max(-1, Math.min(1, (sideways * cos + forward * sin) / diagonal)),
      z: Math.max(-1, Math.min(1, (-sideways * sin + forward * cos) / diagonal)),
      sprint: shift && !flying,
      boost: held("ControlLeft", "ControlRight"),
      ascend: flying ? Number(keys.has("Space")) - Number(shift) : 0,
      teleport: null,
    };
  }

  const pilot: Pilot = {
    command,
    active: locked,
    get yaw() { return yaw; },
    get pitch() { return pitch; },
    get radius() { return radius; },
    get shape() { return shape; },
  };

  return {
    pilot,
    lock: () => canvas.requestPointerLock(),
    locked,
    configure(world: WorldInfo) {
      maxRadius = world.maxBrushRadius;
      radius = world.defaultBrushRadius;
    },
  };
}
