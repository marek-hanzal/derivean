// Helper of chart.test.cjs, run as its own process: terminates a session's thread while its charts are in flight.
const { Worker, isMainThread, parentPort } = require("node:worker_threads");

if (isMainThread) {
  const worker = new Worker(__filename);
  worker.once("error", (error) => {
    console.error(error);
    process.exit(1);
  });
  worker.once("message", () => worker.terminate().then(() => process.exit(0)));
} else {
  const { Session } = require("../../.out/addon/derivean.node");
  const session = new Session({ seed: 42, age: 3e8, spawn: [0, 0] });
  for (let index = 0; index < 12; index++) void session.chart({ x: index * 1e5, z: 0, span: 16777216, size: 1024 });
  parentPort.postMessage("asked");
}
