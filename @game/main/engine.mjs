import { Worker } from "node:worker_threads";

export function startEngine() {
  const worker = new Worker(new URL("engine-worker.cjs", import.meta.url));
  const requests = new Map();
  let serial = 0;
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

  return {
    request(command) {
      if (failure) return Promise.reject(failure);
      if (command !== undefined && requests.size !== 0) {
        return Promise.reject(new Error("Engine request already pending"));
      }
      return new Promise((resolve, reject) => {
        const id = ++serial;
        requests.set(id, { resolve, reject });
        try {
          worker.postMessage({ id, command });
        } catch (error) {
          requests.delete(id);
          reject(error);
        }
      });
    },
    close: () => worker.terminate(),
  };
}
