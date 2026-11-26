import { app, BrowserWindow, dialog, ipcMain, Menu } from "electron";
import { fileURLToPath } from "node:url";

import { CaptureFlag, readCapturePlan, serveCapture } from "./capture.mjs";
import { startEngine } from "./engine.mjs";

app.setName("DeRivean");

let quitting = false;

async function launch() {
  const capture = await readCapturePlan();
  const capturing = capture !== null;
  const engine = startEngine();
  const renderer = new URL("../dist/index.html", import.meta.url);
  // DERIVEAN_RENDERER=webgpu draws the world of cubes with WebGPU; the page reads the choice from its address.
  if (process.env.DERIVEAN_RENDERER === "webgpu") renderer.search = "renderer=webgpu";
  const [width, height] = capture?.size ?? [1440, 900];
  const window = new BrowserWindow({
    title: "DeRivean · Fable",
    width,
    height,
    useContentSize: capturing,
    // A capture plan's size is taken as given; a player's window keeps the launcher legible.
    minWidth: capturing ? undefined : 900,
    minHeight: capturing ? undefined : 580,
    show: false,
    backgroundColor: "#111a19",
    autoHideMenuBar: true,
    webPreferences: {
      preload: fileURLToPath(new URL("../preload/index.cjs", import.meta.url)),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true,
      navigateOnDragDrop: false,
      // A capture run draws without focus and may sit behind other windows.
      backgroundThrottling: !capturing,
      additionalArguments: capturing ? [CaptureFlag] : [],
    },
  });

  Menu.setApplicationMenu(null);
  window.webContents.setWindowOpenHandler(() => ({ action: "deny" }));
  window.webContents.on("will-navigate", (event) => event.preventDefault());
  window.once("ready-to-show", () => capturing ? window.showInactive() : window.show());
  app.on("window-all-closed", () => app.quit());
  app.once("before-quit", () => {
    quitting = true;
    void engine.close();
  });

  function handle(channel, action) {
    ipcMain.handle(channel, async (event, payload) => {
      if (event.sender !== window.webContents || event.senderFrame?.url !== renderer.href) {
        throw new Error("Untrusted engine request");
      }
      try {
        return await action(payload);
      } catch (error) {
        // Closing the engine fails whatever it was still answering; nobody is left to hear about it.
        if (quitting) return new Promise(() => {});
        throw error;
      }
    });
  }

  handle("derivean:start-world", (options) => engine.start(options));
  handle("derivean:advance", (command) => engine.advance(command));
  handle("derivean:inspect", ({ x, z }) => engine.inspect(x, z));
  handle("derivean:chart", (request) => engine.chart(request));
  handle("derivean:quit", () => app.quit());
  if (capture) serveCapture(capture, window, handle);

  await window.loadURL(renderer.href);
}

if (process.env.DERIVEAN_CAPTURE) {
  app.commandLine.appendSwitch("disable-renderer-backgrounding");
  app.commandLine.appendSwitch("disable-backgrounding-occluded-windows");
}

void app.whenReady().then(launch).catch((error) => {
  const reason = error instanceof Error ? error.message : String(error);
  if (process.env.DERIVEAN_CAPTURE) console.error(`DeRivean could not start: ${reason}`);
  else dialog.showErrorBox("DeRivean could not start", reason);
  app.exit(1);
});
