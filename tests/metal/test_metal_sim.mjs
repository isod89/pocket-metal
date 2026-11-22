// SPDX-License-Identifier: GPL-3.0-only
// POCKET METAL in the hosted simulator (tools/sim.mjs --app metal, docs/METAL.md): the app's own C files and its engine,
// compiled for this computer with tests/host as the chip. Driven over the simulator's HTTP interface: who it says it is,
// the footswitches, browsing, the block pages, the tuner (both footswitches), a save that survives a restart and stays
// in its own flash, the factory sound back, MIDI, its stereo sound, its own editor requests.
//   node tests/metal/test_metal_sim.mjs
import assert from "node:assert/strict";
import { mkdtempSync, readFileSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { startSim } from "../../tools/sim.mjs";
import { parseInfo, IS_ALT } from "../../docs/pxota.js";

const TMP = mkdtempSync(join(tmpdir(), "mxsim-"));
let failed = 0;
const ok = (c, what) => { console.log(`${what.padEnd(100)} ${c ? "ok" : "FAIL"}`); if (!c) failed++; };
const flash = join(TMP, "flash.bin");
let sim = await startSim({ port: 0, flash, app: "metal" });
const get = async (path) => (await fetch(sim.url + path)).json();
const act = async (path, body) => { const r = await fetch(sim.url + path, { method: "POST", body }); assert.equal(r.status, 200, `${path}: ${await r.clone().text()}`); return r.json(); };
const sysex = async (hex) => { const { reply } = await act("/api/sysex", hex); return reply ? Uint8Array.from(Buffer.from(reply, "hex")) : null; };
const midi = (hex) => act("/api/midi", hex).catch(() => null);
const press = (b) => act("/api/press/" + b);
const wait = (ms) => act("/api/wait/" + ms);
const screen = async () => Buffer.from(await (await fetch(sim.url + "/api/screen.rgb565")).arrayBuffer());
const B = { GATE: 0, PITCH: 2, DRIVE: 4, AMP: 5, CAB: 6, EQ: 7, MOD: 8, DELAY: 9, REVERB: 10 };
// the app's STATE request (app.h): slot, flags, the sound (px_preset_t: name 14, on 12, type 12, p 12 x 5, order 12; width, style)
async function sound() {
  const m = await sysex("f07d4f5005f7");
  const a = [...m.slice(5, -1)];
  const p = a.slice(2);
  return { slot: a[0], dirty: a[1] & 1, saved: a[1] >> 1 & 1, name: String.fromCharCode(...p.slice(0, 14)).replace(/\0.*$/, ""),
    on: p.slice(14, 26), type: p.slice(26, 38), par: (b) => p.slice(38 + b * 5, 43 + b * 5), width: p[110], style: p[111] };
}
const red = (buf) => { let n = 0; for (let i = 0; i < buf.length; i += 2) { const v = buf.readUInt16BE(i); if ((v >> 11) >= 16 && ((v >> 5) & 63) < 12 && (v & 31) < 8) n++; } return n; };

// ---- who it is ----
await wait(2500);
let st = await get("/api/state");
ok(st.app === "metal" && st.name === "POCKET METAL" && st.identity === "PocketX_902", `it is ${st.name} ${st.version}, identity ${st.identity}`);
const p = parseInfo(await sysex("f07d4f5001f7"));
ok(p && IS_ALT(p) && p.app === "metal" && p.caps.includes("stereo") && p.caps.includes("midi") && p.caps.includes("store"),
  "INFO: an alternative firmware (protocol 0), stereo, MIDI, its own store");
let s = await sound();
ok(s.slot === 0 && s.name === "HEAVY ANTHEM" && !s.dirty && s.on[B.AMP] === 1, `it starts on sound 1, ${s.name}`);
ok(red(await screen()) > 2000, "the PLAY page is drawn in blood red");

// ---- the footswitches, browsing ----
await press("fs_up"); s = await sound();
ok(s.slot === 1 && s.name === "TWIN GUITARS", "the footswitch up: the next sound");
await press("fs_dn"); await press("fs_dn"); s = await sound();
ok(s.slot === 59 && s.name === "WHAMMY SCREAM", "down twice: around to the last sound (60)");
await act("/api/knob/-41"); await wait(100); s = await sound();
ok(s.slot === 59, "the knob on the PLAY page browses: nothing is loaded yet");
await press("push"); s = await sound();
ok(s.slot === 18 && s.name === "POLYRHYTHM" && s.type[B.PITCH] !== undefined, `PUSH loads the sound shown: ${s.slot + 1} ${s.name}`);

// ---- a block page ----
await press("amp"); await act("/api/knob/5"); await wait(50); s = await sound();
const pol = s;
ok(s.type[B.AMP] !== 0 && s.dirty, "AMP opens its page, the knob goes through the amps: the sound is EDITED");
await press("push"); await act("/api/knob/3"); await wait(50); s = await sound();
ok(s.par(B.AMP)[0] === pol.par(B.AMP)[0] + 3, "PUSH marks GAIN, the knob moves it");
await press("amp"); s = await sound();
ok(s.on[B.AMP] === 0, "AMP pressed again on its page: the amp off");
await press("amp"); s = await sound();
ok(s.on[B.AMP] === 1, "and on again");
await press("cab"); for (let i = 0; i < 4; i++) await press("push"); await act("/api/knob/-20"); await wait(50); s = await sound();
ok(s.width === pol.width - 20, `the cabinet's last line is WIDTH: ${pol.width} -> ${s.width}`);
await press("fx"); await press("fx"); s = await sound();
await act("/api/knob/0"); ok(s.type[B.PITCH] !== undefined, "FX steps through GATE, TUNE...");

// ---- save, restart, factory sound back ----
await press("home"); await press("home_long"); s = await sound();
ok(!s.dirty && s.saved && s.slot === 18, "HOME held: SAVED in its slot");
await act("/api/restart"); await wait(2500); s = await sound();
ok(s.slot === 18 && s.saved && s.width === pol.width - 20, "after a restart: the same slot, the saved sound");
st = await get("/api/state");
ok(st.writes_ok === true, "every flash write was inside the app's own range");

// ---- a store of the first version (thirty slots) is read, each saved sound in its slot ----
await sim.close();
{
  const BS_LO = 0xE5000, USE = 0xF00, NSEC = 5, PR = 112;
  const img = readFileSync(flash);
  const off = (base, i) => base + Math.floor(i / USE) * 0x1000 + (i % USE);
  const at = (base, i, n) => { const o = Buffer.alloc(n); for (let k = 0; k < n; k++) o[k] = img[off(base, i + k)]; return o; };
  const crc16 = (b, c) => { for (const x of b) { c ^= x << 8; for (let i = 0; i < 8; i++) c = c & 0x8000 ? (c << 1) ^ 0x1021 : c << 1; c &= 0xffff; } return c; };
  const copies = [BS_LO, BS_LO + NSEC * 0x1000].map((base) => ({ base, h: img.subarray(base, base + 16) }))
    .filter((c) => c.h.readUInt32LE(0) === 0x4c544d50);
  const cur = copies.sort((a, b) => b.h.readUInt32LE(4) - a.h.readUInt32LE(4))[0];
  const n = cur.h.readUInt32LE(8), d2 = at(cur.base, 16, n), N2 = (n - 2) / (PR + 1);
  const d1 = Buffer.concat([d2.subarray(0, 31), Buffer.from([0]), d2.subarray(2 + N2, 2 + N2 + 30 * PR)]);
  const base = cur.base === BS_LO ? BS_LO + NSEC * 0x1000 : BS_LO;
  for (let i = 0; i < NSEC; i++) img.fill(0xff, base + i * 0x1000, base + (i + 1) * 0x1000);
  const h = Buffer.alloc(16);
  h.writeUInt32LE(0x4c544d50, 0); h.writeUInt32LE(cur.h.readUInt32LE(4) + 1, 4); h.writeUInt32LE(d1.length, 8); h[14] = 1;
  h.writeUInt16LE(crc16(d1, crc16(h.subarray(0, 12), 1)), 12);
  const all = Buffer.concat([h, d1]);
  for (let i = 0; i < all.length; i++) img[off(base, i)] = all[i];
  writeFileSync(flash, img);
  ok(N2 === 60 && d1.length === 2 + 30 * (PR + 1), `the sixty-slot store (${n} bytes) rewritten as the first version's thirty (${d1.length})`);
}
sim = await startSim({ port: 0, flash, app: "metal" });
await wait(2500); s = await sound();
ok(s.slot === 18 && s.saved && s.width === pol.width - 20, "the first version's store: the same slot, the saved sound");
await midi("c0 3b"); await wait(50); s = await sound();
ok(s.slot === 59 && !s.saved && s.name === "WHAMMY SCREAM", "Program Change 59: sound 60, the factory sound (new slots hold nothing saved)");
await midi("c0 3c"); await wait(50); s = await sound();
ok(s.slot === 59, "Program Change 60: no such sound, nothing changes");
await midi("c0 12"); await wait(50);
await press("push_long"); await press("push"); s = await sound();
ok(!s.saved && s.width === pol.width && s.name === "POLYRHYTHM", "PUSH held, PUSH again: the factory sound is back");

// ---- the tuner ----
await act("/api/down/fs_up"); await act("/api/press/fs_dn"); await act("/api/up/fs_up"); s = await sound();
ok(s.slot === 18, "both footswitches: the sound stays the one before");
let scr = await screen();
await act("/api/sound/110"); await wait(400);
const scr2 = await screen();
ok(!scr.equals(scr2), "and the tuner page follows a note (110 Hz)");
await press("home"); await wait(100);

// ---- MIDI ----
await midi("c0 07"); await wait(50); s = await sound();
ok(s.slot === 7 && s.name === "BRUTAL DEATH", "Program Change 7: sound 8");
await midi("b0 58 00"); await wait(50); s = await sound();
ok(s.on[B.REVERB] === 0, "Control Change 88 below 64: the reverb off");
await midi("b0 52 7f"); await wait(50); s = await sound();
ok(s.on[B.DRIVE] === 1, "Control Change 82: the boost on");

// ---- its sound: stereo ----
const lv = await act("/api/sound/440");
ok(lv.left > 0.005 && lv.right > 0.005, `a 440 Hz tone comes out on both sides (${lv.left} / ${lv.right})`);
await midi("c0 1c"); await wait(300);
const cl = await act("/api/sound/1000");
ok(cl.left > 0.005 && cl.right > 0.005 && cl.left !== cl.right, `POST CLEAN at WIDTH 70: the two sides differ (${cl.left} / ${cl.right})`);

// ---- its own editor requests ----
const r = await sysex("f07d4f5006" + "05" + "00" + "21" + "f7");
s = await sound();
ok(r && r[7] === 0x21 && s.par(B.AMP)[0] === 0x21, "SET: the amp's GAIN to 33 over USB");

await sim.close();
if (failed) { console.log(`metal simulator: ${failed} FAILED`); process.exit(1); }
console.log("metal simulator: all ok");
