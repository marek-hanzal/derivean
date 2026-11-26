import { createRequire } from "node:module";
import { Worker } from "node:worker_threads";

// Closing lets the world go without taking it down, and its threads run until the process ends. Loaded here as
// well, the addon stays in memory when the worker that used it is gone.
createRequire(import.meta.url)("../../.out/addon/derivean.node");

export function startEngine() {
  const worker = new Worker(new URL("engine-worker.cjs", import.meta.url));
  const requests = new Map();
  let serial = 0;
  let advancing = false;
  let failure;
  const fail = (error) => {
    failure = error;
    for (const { reject } of requests.values()) reject(error);
    requests.clear();
  };

  worker.on("error", fail);
  worker.on("exit", (code) => fail(new Error(`Engine worker exited (${code})`)));
  worker.on("message", ({ id, value, error }) => {
    const request = requests.get(id);
    if (!request) return;
    requests.delete(id);
    if (error) request.reject(new Error(error));
    else request.resolve(value);
  });

  function send(kind, payload) {
    if (failure) return Promise.reject(failure);
    return new Promise((resolve, reject) => {
      const id = ++serial;
      requests.set(id, { resolve, reject });
      try {
        worker.postMessage({ id, kind, payload });
      } catch (error) {
        requests.delete(id);
        reject(error);
      }
    });
  }

  // The engine answers in order, so one command at a time keeps frames from piling up behind a slow one.
  function advance(command) {
    if (advancing) return Promise.reject(new Error("Engine request already pending"));
    advancing = true;
    return send("advance", command).finally(() => { advancing = false; });
  }

  const start = (options) => send("start", options);

  return {
    start,
    advance,
    inspect: (x, z) => send("inspect", { x, z }),
    chart: (request) => send("chart", request),
    // For a process that is about to exit: taking a world down properly waits for every tile build in flight.
    close: () => send("close").catch(() => {}).then(() => worker.terminate()),
  };
}
