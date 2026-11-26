const { parentPort } = require("node:worker_threads");
const { Session, world } = require("../../.out/addon/derivean.node");

let session;

// Starting a world blocks this thread for as long as the planet takes to evolve.
function start(options) {
  if (session) session.restart(options);
  else session = new Session(options);
  return { ...world, ...session.info() };
}

function advance(command) {
  const frame = session.advance(command);
  const transfer = [
    ...frame.chunks.map(({ faces }) => faces.buffer),
    ...frame.waterMeshes.map(({ faces }) => faces.buffer),
    frame.particles.buffer,
    ...frame.spawned.map(({ faces }) => faces.buffer),
    ...frame.chunks.flatMap(({ quads }) => quads ? [quads.buffer] : []),
    ...frame.cubes.added.flatMap(({ quads, water }) => [quads.buffer, ...water.map((sheet) => sheet.quads.buffer)]),
  ];
  if (frame.clouds) {
    const { bands, heaping, rain, snow, snowpack, snowLadder, snowLine, aerosolDepth, ashAloft, ashLevel } = frame.clouds;
    transfer.push(heaping.buffer, rain.buffer, snow.buffer, snowpack.buffer, snowLadder.buffer, snowLine.buffer, aerosolDepth.buffer, ashAloft.buffer, ashLevel.buffer, ...bands.flatMap((band) => Object.values(band).map(({ buffer }) => buffer)));
  }
  return { value: frame, transfer };
}

parentPort.on("message", ({ id, kind, payload }) => {
  try {
    if (kind === "start") parentPort.postMessage({ id, value: start(payload) });
    else if (kind === "close") {
      if (session) session.abandon();
      session = undefined;
      parentPort.postMessage({ id, value: null });
    }
    else if (kind === "inspect") parentPort.postMessage({ id, value: session.inspect(payload.x, payload.z) });
    // A chart is read off this thread and answered between frames; its rasters are views of one buffer.
    else if (kind === "chart") {
      session.chart(payload).then((value) => parentPort.postMessage({ id, value }, value ? [value.ground.buffer] : []),
        (cause) => parentPort.postMessage({ id, error: cause instanceof Error ? cause.message : String(cause) }));
    }
    else {
      const { value, transfer } = advance(payload);
      parentPort.postMessage({ id, value }, transfer);
    }
  } catch (cause) {
    parentPort.postMessage({ id, error: cause instanceof Error ? cause.message : String(cause) });
  }
});
