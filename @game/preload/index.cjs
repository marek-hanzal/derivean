const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("derivean", {
  startWorld: () => ipcRenderer.invoke("derivean:start-world"),
  advance: (command) => ipcRenderer.invoke("derivean:advance", command),
  quit: () => ipcRenderer.invoke("derivean:quit"),
});
