// SPDX-License-Identifier: GPL-3.0-only
// POCKET METAL's screens in the hosted simulator (tools/sim.mjs --app metal): each page as a PNG, through the app's own
// code with tests/host as the chip.   node tests/metal/shots.mjs OUTDIR
import { mkdirSync, writeFileSync, mkdtempSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { deflateSync } from "node:zlib";
import { startSim } from "../../tools/sim.mjs";

const OUT = process.argv[2] || "build/metal-shots";
mkdirSync(OUT, { recursive: true });
const sim = await startSim({ port: 0, flash: join(mkdtempSync(join(tmpdir(), "mx-")), "flash.bin"), app: "metal" });
const act = async (path, body) => { const r = await fetch(sim.url + path, { method: "POST", body }); if (r.status !== 200) throw new Error(path + ": " + await r.text()); return r.json(); };
const wait = (ms) => act("/api/wait/" + ms);
function png(rgb565, w, h) {                     // RGB565 big-endian -> PNG (no library)
  const raw = Buffer.alloc((w * 3 + 1) * h);
  for (let y = 0; y < h; y++) {
    raw[y * (w * 3 + 1)] = 0;
    for (let x = 0; x < w; x++) {
      const v = rgb565.readUInt16BE((y * w + x) * 2), o = y * (w * 3 + 1) + 1 + x * 3;
      raw[o] = ((v >> 11) & 31) * 255 / 31; raw[o + 1] = ((v >> 5) & 63) * 255 / 63; raw[o + 2] = (v & 31) * 255 / 31;
    }
  }
  const crc = (b) => { let c = ~0; for (const x of b) { c ^= x; for (let k = 0; k < 8; k++) c = c & 1 ? (c >>> 1) ^ 0xEDB88320 : c >>> 1; } return ~c >>> 0; };
  const chunk = (t, d) => { const l = Buffer.alloc(4); l.writeUInt32BE(d.length); const td = Buffer.concat([Buffer.from(t), d]); const c = Buffer.alloc(4); c.writeUInt32BE(crc(td)); return Buffer.concat([l, td, c]); };
  const ihdr = Buffer.alloc(13); ihdr.writeUInt32BE(w, 0); ihdr.writeUInt32BE(h, 4); ihdr[8] = 8; ihdr[9] = 2;
  return Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk("IHDR", ihdr), chunk("IDAT", deflateSync(raw)), chunk("IEND", Buffer.alloc(0))]);
}
const shot = async (name) => {
  const b = Buffer.from(await (await fetch(sim.url + "/api/screen.rgb565")).arrayBuffer());
  writeFileSync(join(OUT, name + ".png"), png(b, 240, 240));
  console.log("shot", name);
};
const press = (b) => act("/api/press/" + b);
await wait(800); await shot("00-splash");
await wait(3000); await shot("01-play");
for (const n of ["02", "03"]) { await press("fs_up"); await wait(300); await shot(n + "-play-next"); }
await act("/api/knob/3"); await wait(200); await shot("04-browse");
await press("push"); await wait(400); await shot("05-loaded");
await press("amp"); await wait(300); await shot("10-amp");
await press("push"); await act("/api/knob/2"); await wait(300); await shot("11-amp-gain");
await press("fx"); await wait(300); await shot("12-gate");
await press("fx"); await wait(300); await shot("13-tune");
await press("fx"); await wait(300); await shot("14-boost");
await press("fx"); await wait(300); await shot("15-eq");
await press("cab"); await wait(300); await shot("16-cab");
await press("mod"); await wait(300); await shot("17-mod");
await press("dly"); await wait(300); await shot("18-delay");
await press("rev"); await wait(300); await shot("19-reverb");
await press("home"); await wait(300); await shot("20-play-edited");
await press("fx_long"); await act("/api/sound/110"); await wait(500); await shot("30-tuner");
await sim.close();
