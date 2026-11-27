// Runs the simulation checks as one process per group of test/Main.cpp, side by side, and fails if
// any group fails. Each group still stops at its first failing check; the others run on. Node, not a
// shell script, so that `mise run test` runs alike on macOS and Windows (docs/windows.md).
//   node test/groups.mjs [build directory, relative to @simulation] [-word...]
//   node test/groups.mjs ../.out/simulation/build -glaciated     # leave out testReliefGlaciatedSite
// Every argument goes to every group; a filter word a group holds no test for leaves that group
// empty and passing. To run some tests by name, run simulation-tests itself.
import {spawn, spawnSync} from "node:child_process";
import {existsSync, readdirSync, statSync} from "node:fs";
import {basename, dirname, join, resolve} from "node:path";
import {createInterface} from "node:readline";
import {fileURLToPath} from "node:url";

process.chdir(join(dirname(fileURLToPath(import.meta.url)), ".."));
const args = process.argv.slice(2);
let build = "../.out/simulation/build";
if (args.length > 0 && existsSync(args[0]) && statSync(args[0]).isDirectory()) build = args.shift();

// xmake puts the binary under <plat>/<arch>/release/.
const name = process.platform === "win32" ? "simulation-tests.exe" : "simulation-tests";
function find(directory) {
    let entries;
    try {
        entries = readdirSync(directory, {withFileTypes: true});
    } catch {
        return null; // an unreadable directory holds no build of ours
    }
    for (const entry of entries) {
        const path = join(directory, entry.name);
        if (entry.isFile() && entry.name === name && basename(directory) === "release") return path;
        if (entry.isDirectory()) {
            const found = find(path);
            if (found) return found;
        }
    }
    return null;
}
const binary = existsSync(build) ? find(resolve(build)) : null;
if (!binary) {
    console.error(`no simulation-tests under ${build}; build it first`);
    process.exit(2);
}

const listed = spawnSync(binary, ["--groups"], {encoding: "utf8"});
const groups = listed.status === 0 ? listed.stdout.split(/\r?\n/).filter(Boolean) : [];
if (groups.length === 0) {
    console.error(`${binary} lists no test groups${listed.error ? `: ${listed.error.message}` : listed.stderr ? `:\n${listed.stderr}` : ""}`);
    process.exit(2);
}

const seconds = (since) => Math.round((Date.now() - since) / 1000);
const start = Date.now();
const children = [];
const passed = groups.map((group) => {
    const begin = Date.now();
    const child = spawn(binary, ["--group", group, ...args], {stdio: ["ignore", "pipe", "pipe"]});
    children.push(child);
    for (const stream of [child.stdout, child.stderr])
        createInterface({input: stream, crlfDelay: Infinity}).on("line", (line) => console.log(`[${group}] ${line}`));
    child.on("error", (error) => console.log(`[${group}] ${error.message}`));
    return new Promise((done) =>
        child.on("close", (code, signal) => {
            console.log(`[${group}] exit ${code ?? signal} after ${seconds(begin)} s`);
            done(code === 0);
        }),
    );
});

// The groups are this process's children; stop them with it.
for (const signal of ["SIGINT", "SIGTERM"])
    process.on(signal, () => {
        for (const child of children) child.kill();
        process.exit(130);
    });

const failed = (await Promise.all(passed)).includes(false);
console.log(`simulation-tests: ${groups.length} groups in ${seconds(start)} s, ${failed ? "FAILED" : "passed"}`);
// exitCode, not exit(): a piped stdout keeps its last lines only if the process ends on its own.
process.exitCode = failed ? 1 : 0;
