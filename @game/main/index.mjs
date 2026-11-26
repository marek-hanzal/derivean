import { app, BrowserWindow, dialog, ipcMain, Menu } from "electron";
import { readFileSync, statSync } from "node:fs";
import { fileURLToPath } from "node:url";

import { CaptureFlag, readCapturePlan, serveCapture } from "./capture.mjs";
import { startEngine } from "./engine.mjs";

app.setName("DeRivean");

let quitting = false;

// What drew a window, shown in its HUD line and title so that a screenshot names its build: the mode, the engine
// addon's build from the stamp the native build writes beside it (time, commit, and "dirty" with the first digits
// of the uncommitted diff's hash), and the stream that launched it (DERIVEAN_STREAM). A replay (scratch tooling)
// sets the same line from its recording.
function windowOrigin(capturing) {
  const addon = fileURLToPath(new URL("../../.out/addon/derivean.node", import.meta.url));
  const two = (value) => String(value).padStart(2, "0");
  const at = (time) => `${two(time.getMonth() + 1)}-${two(time.getDate())} ${two(time.getHours())}:${two(time.getMinutes())}`;
  let build;
  try {
    const stamp = JSON.parse(readFileSync(`${addon}.json`, "utf8"));
    // A clean tree is its commit; a dirty one, or one whose state git could not tell (dirty null), is told apart
    // by the hash of its diff.
    const hash = String(stamp.diffHash).slice(0, 6);
    const tree = stamp.dirty === false ? "" : stamp.dirty ? ` dirty ${hash}` : ` tree ${hash}`;
    build = `${at(new Date(stamp.builtAt))} ${stamp.commit ? String(stamp.commit).slice(0, 7) : "NO COMMIT"}${tree}`;
  } catch {
    try {
      build = `${at(statSync(addon).mtime)} UNSTAMPED`;
    } catch {
      build = "MISSING";
    }
  }
  return `${capturing ? "CAPTURE" : "LIVE"} · ADDON ${build} · ${process.env.DERIVEAN_STREAM || "NO STREAM"}`;
}

async function launch() {
  const capture = await readCapturePlan();
  const capturing = capture !== null;
  const engine = startEngine();
  const origin = windowOrigin(capturing);
  const renderer = new URL("../dist/index.html", import.meta.url);
  const [width, height] = capture?.size ?? [1440, 900];
  const window = new BrowserWindow({
    title: `DeRivean · Fable · ${origin}`,
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
  // The title carries the origin; the page's own would drop it.
  window.on("page-title-updated", (event) => event.preventDefault());
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

  handle("derivean:start-world", async (options) => ({ ...(await engine.start(options)), origin }));
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
