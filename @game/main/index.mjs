import { app, BrowserWindow, dialog, ipcMain, Menu } from "electron";
import { fileURLToPath } from "node:url";

import { startEngine } from "./engine.mjs";

app.setName("DeRivean");

async function launch() {
  const engine = startEngine();
  const renderer = new URL("../dist/index.html", import.meta.url);
  const window = new BrowserWindow({
    title: "DeRivean",
    width: 1440,
    height: 900,
    minWidth: 900,
    minHeight: 580,
    show: false,
    backgroundColor: "#111a19",
    autoHideMenuBar: true,
    webPreferences: {
      preload: fileURLToPath(new URL("../preload/index.cjs", import.meta.url)),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true,
      navigateOnDragDrop: false,
    },
  });

  Menu.setApplicationMenu(null);
  window.webContents.setWindowOpenHandler(() => ({ action: "deny" }));
  window.webContents.on("will-navigate", (event) => event.preventDefault());
  window.once("ready-to-show", () => window.show());
  app.on("window-all-closed", () => app.quit());
  app.once("before-quit", () => { void engine.close(); });

  function handle(channel, action) {
    ipcMain.handle(channel, (event, payload) => {
      if (event.sender !== window.webContents || event.senderFrame?.url !== renderer.href) {
        throw new Error("Untrusted engine request");
      }
      return action(payload);
    });
  }

  handle("derivean:start-world", () => engine.request());
  handle("derivean:advance", (command) => engine.request(command));
  handle("derivean:quit", () => app.quit());

  await window.loadFile(fileURLToPath(renderer));
}

void app.whenReady().then(launch).catch((error) => {
  dialog.showErrorBox("DeRivean could not start", error instanceof Error ? error.message : String(error));
  app.exit(1);
});
