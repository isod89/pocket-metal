#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-only
// POCKET METAL's command line (macOS, Linux, Windows; Node 18+). BUILD.bat, INSTALL.bat and PUBLISH.bat only call this.
//   node tools/op.mjs doctor     what the build needs, one line each
//   node tools/op.mjs build      the firmware and its two packages -> build/apps/metal/ (in the pinned Docker container)
//   node tools/op.mjs test       the host tests: the engine on real DI guitar, the app in the simulator, the base
//   node tools/op.mjs sim        the app on this computer: its screen and controls in a page (a C compiler, no Docker)
//   node tools/op.mjs serve      the installer on this computer (http://localhost:8765/), with the packages just built
//   node tools/op.mjs clean | shell | image
// NATIVE=1 runs build and test on this machine (the JieLi toolchain from firmware/setup.sh) instead of in the container.
import { spawnSync } from "node:child_process";
import { existsSync, rmSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const APP = "metal";
const IMAGE = "openpocket-build";               // (the same container as OpenPocket's: one image for both trees)
const native = Boolean(process.env.NATIVE || process.env.OP_IN_CONTAINER);
const python = process.platform === "win32" ? ["py", "-3"] : ["python3"];
const OUT = `build/apps/${APP}`;

function fail(message) { console.error(`\nERROR: ${message}`); process.exit(1); }
function exec(cmd, args, opts = {}) {
  const r = spawnSync(cmd, args, { stdio: "inherit", cwd: root, ...opts });
  if (r.error) fail(`could not run ${cmd}: ${r.error.message}`);
  return r.status ?? 1;
}
function needDocker() {
  const r = spawnSync("docker", ["info"], { stdio: "ignore" });
  if (r.error || r.status !== 0) fail("Docker is not installed or not running. Start Docker Desktop (or Docker on Linux, or `colima start`), or build in WSL: BUILD.bat wsl.");
}
function image() { needDocker(); return exec("docker", ["build", "--platform", "linux/amd64", "-t", IMAGE, "."]); }
function inContainer(args, { tty = false } = {}) {
  if (native) return exec(args[0], args.slice(1));
  const built = image();
  if (built !== 0) return built;
  const user = process.getuid ? ["--user", `${process.getuid()}:${process.getgid()}`] : [];
  return exec("docker", ["run", "--rm", ...(tty ? ["-it"] : []), "--platform", "linux/amd64", ...user,
    "-e", "HOME=/tmp", "-v", `${root}:/src`, "-w", "/src", IMAGE, ...args]);
}

const commands = {
  doctor() {
    let bad = 0;
    const line = (ok, text) => { console.log(`${ok ? "ok  " : "FAIL"} ${text}`); if (!ok) bad = 1; };
    line(Number(process.versions.node.split(".")[0]) >= 18, `Node ${process.versions.node} (18 or newer)`);
    const info = spawnSync("docker", ["info"], { stdio: "ignore" });
    const up = !info.error && info.status === 0;
    line(up, up ? "Docker is running" : "Docker is not installed or not running (Docker Desktop, Docker, or `colima start`)");
    const cc = spawnSync(process.env.CC || "cc", ["--version"], { stdio: "ignore" });
    console.log(cc.error || cc.status !== 0 ? "note no C compiler (cc): `sim` needs one (Xcode command line tools, build-essential)" : "ok   C compiler for `sim`");
    const py = spawnSync(python[0], [...python.slice(1), "--version"], { stdio: "ignore" });
    console.log(py.error || py.status !== 0 ? "note Python 3 not found: `serve` (the installer offline) needs it" : "ok   Python 3 for `serve`");
    return bad;
  },
  image,
  build: () => inContainer(["sh", "-c", `OP_APP=${APP} sh firmware/build.sh && sha256sum ${OUT}/${APP}.bin ${OUT}/*.fwsc ${OUT}/manifest.json > ${OUT}/SHA256SUMS && cat ${OUT}/SHA256SUMS`]),
  test: () => inContainer(["sh", "tests/run_tests.sh"]),
  sim: (args = []) => exec(process.execPath, ["tools/sim.mjs", "--app", APP, ...args]),
  serve() {
    if (!existsSync(resolve(root, OUT, "manifest.json"))) {
      const built = commands.build();
      if (built !== 0) return built;
    }
    const r = spawnSync(python[0], [...python.slice(1), "tools/serve.py"], { stdio: "inherit", cwd: root });
    if (r.error) fail("Python 3 is needed to serve the installer offline (python.org).");
    return r.status ?? 0;
  },
  clean() { for (const p of ["build", "firmware/build"]) rmSync(resolve(root, p), { recursive: true, force: true }); return 0; },
  shell: () => inContainer(["bash"], { tty: true }),
};

const [name, ...rest] = process.argv.slice(2);
if (!Object.hasOwn(commands, name)) fail(`usage: node tools/op.mjs ${Object.keys(commands).join(" | ")}`);
process.exit(await commands[name](rest));
