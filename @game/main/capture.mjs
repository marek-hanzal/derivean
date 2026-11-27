import { app } from "electron";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

// Passed to the preload script so that the capture bridge exists only in capture runs.
export const CaptureFlag = "--derivean-capture";

// A run that reports nothing for this long, beyond the shots' own timeouts, is hung: a world that never starts, an
// engine that stops answering. It covers an engine start under load (up to 8 minutes) and the 5 minutes a shot may
// take to become playable where it stands (PlayableSeconds in src/capture/constant/timing.ts).
const IdleSeconds = 900;

const repository = fileURLToPath(new URL("../../", import.meta.url));
const finite = (value) => typeof value === "number" && Number.isFinite(value);
const flag = (value) => value === undefined || typeof value === "boolean";
const Readiness = ["site", "local", "region", "far"];
const readiness = (value) => value === undefined || (Array.isArray(value) && value.every((level) => Readiness.includes(level)));

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
  if (!readiness(shot.ready)) reject(file, `shot ${shot.name} asks for readiness other than ${Readiness.join(", ")}`);
  const { bench } = shot;
  if (bench !== undefined && !(Number.isInteger(bench?.frames) && bench.frames > 0
    && (bench.batches === undefined || (Number.isInteger(bench.batches) && bench.batches > 0)))) reject(file, `shot ${shot.name} has a malformed bench`);
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
  if (!readiness(plan.ready)) reject(file, `ready asks for readiness other than ${Readiness.join(", ")}`);
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
  // How each shot ended: settled, stalled (pending work stopped delivering) or timed out, and which lacked the
  // readiness they asked for. A run in which a shot lacked it is not an acceptance run and exits 2.
  const outcomes = [];
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
    await mkdir(plan.out, { recursive: true });
    // A shot's bench comes after its picture, as a record of its own.
    const benched = plan.shots.find((shot) => shot.bench && `${shot.name}-bench` === name);
    if (benched) {
      await writeFile(path.join(plan.out, `${name}.json`), `${JSON.stringify(record, null, 2)}\n`);
      console.log(`Benched ${benched.name}: ${JSON.stringify({ cloudyMs: record?.cloudyMs, clearMs: record?.clearMs, cloudMs: record?.cloudMs })}`);
      return;
    }
    if (!plan.shots.some((shot) => shot.name === name)) throw new Error(`Unplanned capture shot ${name}`);
    const image = await window.webContents.capturePage();
    if (image.isEmpty()) throw new Error(`The window gave no image for ${name}`);
    const { width } = window.getContentBounds();
    await writeFile(path.join(plan.out, `${name}.png`), image.resize({ width, quality: "best" }).toPNG());
    await writeFile(path.join(plan.out, `${name}.json`), `${JSON.stringify(record, null, 2)}\n`);
    const outcome = record?.outcome ?? "taken";
    const unready = Array.isArray(record?.unready) ? record.unready : [];
    outcomes.push({ name, outcome, unready });
    console.log(`Captured ${path.join(plan.out, `${name}.png`)} ${outcome}${unready.length ? `, unready ${unready.join(" ")}` : ""}`);
  });
  handle("derivean:capture-finish", (failure) => {
    clearTimeout(watchdog);
    if (typeof failure === "string") return fail(failure || "the renderer gave no reason");
    finished = true;
    const count = (outcome) => outcomes.filter((shot) => shot.outcome === outcome).length;
    const unready = outcomes.filter((shot) => shot.unready.length).map((shot) => shot.name);
    console.log(`Capture ended: ${outcomes.length} shots, ${count("settled")} settled, ${count("stalled")} stalled, ${count("timedOut")} timed out; `
      + (unready.length ? `${unready.length} lacked the readiness they asked for (${unready.join(", ")})` : "none lacked the readiness asked for"));
    // Electron leaves process.exitCode unread; the code is set as the quit ends.
    if (unready.length) app.once("will-quit", () => app.exit(2));
    app.quit();
  });
}
