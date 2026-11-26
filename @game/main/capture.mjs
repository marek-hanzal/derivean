import { app } from "electron";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

// Passed to the preload script so that the capture bridge exists only in capture runs.
export const CaptureFlag = "--derivean-capture";

// A run that reports nothing for this long is hung: a world that never starts, an engine that stops answering.
// It is longer than any wait the renderer allows itself by default (src/capture/constant/timing.ts).
const IdleSeconds = 600;

const repository = fileURLToPath(new URL("../../", import.meta.url));
const finite = (value) => typeof value === "number" && Number.isFinite(value);
const flag = (value) => value === undefined || typeof value === "boolean";

function reject(file, reason) {
  throw new Error(`Capture plan ${file}: ${reason}`);
}

function checkShot(file, shot) {
  if (typeof shot?.name !== "string" || !/^[\w.-]+$/.test(shot.name)) reject(file, `shot name ${JSON.stringify(shot?.name)} is not a file stem`);
  if (!finite(shot.yaw) || !finite(shot.pitch)) reject(file, `shot ${shot.name} needs a numeric yaw and pitch`);
  const { position, above, cruise } = shot;
  if (typeof shot.fly !== "boolean" || !flag(shot.plain)) reject(file, `shot ${shot.name} needs fly, and plain if given, as true or false`);
  if (position !== undefined && !(Array.isArray(position) && position.length === 3 && finite(position[0])
    && (position[1] === null || finite(position[1])) && finite(position[2]))) reject(file, `shot ${shot.name} has a malformed position`);
  if (above !== undefined && !(finite(above) && above >= 0 && (position === undefined || position[1] === null))) reject(file, `shot ${shot.name} gives a height above ground together with a height`);
  if (cruise !== undefined && !(finite(cruise?.seconds) && cruise.seconds >= 0 && flag(cruise.boost) && shot.fly)) reject(file, `shot ${shot.name} has a malformed cruise or cruises without flying`);
  const { time, seek } = shot;
  if (time !== undefined && !["dayOfYear", "hour", "rate"].every((key) => time?.[key] === undefined || (finite(time[key]) && time[key] >= 0))) reject(file, `shot ${shot.name} has a malformed time`);
  if (seek !== undefined && (typeof seek !== "object" || seek === null)) reject(file, `shot ${shot.name} has a malformed seek`);
  if (!flag(shot.clouds)) reject(file, `shot ${shot.name} needs clouds, if given, as true or false`);
  const { brush } = shot;
  if (brush !== undefined && !(Number.isInteger(brush?.frames) && brush.frames > 0 && flag(brush.place)
    && (brush.radius === undefined || (Number.isInteger(brush.radius) && brush.radius > 0))
    && (brush.seconds === undefined || (finite(brush.seconds) && brush.seconds >= 0)) && cruise === undefined)) reject(file, `shot ${shot.name} has a malformed brush or one with a cruise`);
  const { turn } = shot;
  if (turn !== undefined && !(finite(turn?.seconds) && turn.seconds >= 0 && finite(turn.radiansPerSecond) && cruise === undefined
    && brush === undefined)) reject(file, `shot ${shot.name} has a malformed turn or one with a cruise or brush`);
  if (shot.map !== undefined && !(finite(shot.map?.span) && shot.map.span > 0)) reject(file, `shot ${shot.name} has a malformed map`);
}

// The plan named by DERIVEAN_CAPTURE, or null for a normal run. src/capture/type/CapturePlan.ts owns its shape.
export async function readCapturePlan() {
  const file = process.env.DERIVEAN_CAPTURE;
  if (!file) return null;
  const plan = JSON.parse(await readFile(path.resolve(repository, file), "utf8"));
  if (!Array.isArray(plan.shots) || !plan.shots.length) reject(file, "lists no shots");
  for (const shot of plan.shots) checkShot(file, shot);
  if (new Set(plan.shots.map(({ name }) => name)).size !== plan.shots.length) reject(file, "names a shot twice");
  for (const key of ["settleSeconds", "timeoutSeconds"]) {
    if (plan[key] !== undefined && !(finite(plan[key]) && plan[key] >= 0)) reject(file, `${key} is not a duration`);
  }
  const { size } = plan;
  if (size !== undefined && !(Array.isArray(size) && size.length === 2 && size.every((edge) => Number.isInteger(edge) && edge > 0))) reject(file, "size is not a width and a height");
  return { ...plan, out: path.resolve(repository, plan.out ?? ".out/playwright/capture") };
}

// Serves the renderer's capture driver: hands out the plan, writes each shot, and ends the run.
export function serveCapture(plan, window, handle) {
  // A shot may wait its timeout once for the weather of its time and once more for its surroundings.
  const longest = Math.max(0, ...plan.shots.map((shot) => shot.cruise?.seconds ?? 0)) + 2 * (plan.timeoutSeconds ?? 0);
  let watchdog;
  let finished = false;
  const fail = (reason) => {
    if (finished) return;
    finished = true;
    console.error(`Capture failed: ${reason}`);
    app.exit(1);
  };
  const alive = () => {
    clearTimeout(watchdog);
    watchdog = setTimeout(() => fail("no progress; the run is hung"), (IdleSeconds + longest) * 1000);
  };

  alive();
  window.on("closed", () => fail("the window closed before the last shot"));
  // An unattended run has no console to look at: warnings and errors of the page go to this process's output.
  window.webContents.on("console-message", (event) => {
    if (event.level === "warning" || event.level === "error") console.error(`[page ${event.level}] ${event.message}`);
  });
  handle("derivean:capture-plan", () => plan);
  handle("derivean:capture-shot", async ({ name, record }) => {
    alive();
    if (!plan.shots.some((shot) => shot.name === name)) throw new Error(`Unplanned capture shot ${name}`);
    await mkdir(plan.out, { recursive: true });
    const image = await window.webContents.capturePage();
    if (image.isEmpty()) throw new Error(`The window gave no image for ${name}`);
    const { width } = window.getContentBounds();
    await writeFile(path.join(plan.out, `${name}.png`), image.resize({ width, quality: "best" }).toPNG());
    await writeFile(path.join(plan.out, `${name}.json`), `${JSON.stringify(record, null, 2)}\n`);
    console.log(`Captured ${path.join(plan.out, `${name}.png`)}`);
  });
  handle("derivean:capture-finish", (failure) => {
    clearTimeout(watchdog);
    if (typeof failure === "string") return fail(failure || "the renderer gave no reason");
    finished = true;
    app.quit();
  });
}
