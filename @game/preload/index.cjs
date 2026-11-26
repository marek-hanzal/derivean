const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("derivean", {
  startWorld: (options) => ipcRenderer.invoke("derivean:start-world", options),
  advance: (command) => ipcRenderer.invoke("derivean:advance", command),
  inspect: (x, z) => ipcRenderer.invoke("derivean:inspect", { x, z }),
  chart: (request) => ipcRenderer.invoke("derivean:chart", request),
  quit: () => ipcRenderer.invoke("derivean:quit"),
});

// Keep in step with CaptureFlag in main/capture.mjs.
if (process.argv.includes("--derivean-capture")) {
  contextBridge.exposeInMainWorld("deriveanCapture", {
    plan: () => ipcRenderer.invoke("derivean:capture-plan"),
    shot: (name, record) => ipcRenderer.invoke("derivean:capture-shot", { name, record }),
    finish: (failure) => ipcRenderer.invoke("derivean:capture-finish", failure),
  });
}
