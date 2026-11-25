const { parentPort } = require("node:worker_threads");
const { Session, world } = require("../../.out/addon/derivean.node");

let session;

parentPort.on("message", ({ id, command }) => {
  try {
    if (command === undefined) {
      if (session) session.restart();
      else session = new Session();
      parentPort.postMessage({ id, value: world });
      return;
    }
    const frame = session.advance(command);
    const transfer = [
      ...frame.chunks.map(({ faces }) => faces.buffer),
      ...frame.waterMeshes.map(({ faces }) => faces.buffer),
      frame.particles.buffer,
      ...frame.spawned.map(({ faces }) => faces.buffer),
    ];
    parentPort.postMessage({ id, value: frame }, transfer);
  } catch (cause) {
    parentPort.postMessage({ id, error: cause instanceof Error ? cause.message : String(cause) });
  }
});
