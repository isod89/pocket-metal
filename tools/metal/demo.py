#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# POCKET METAL: every factory sound, one after the other, on the bundled DI loops (assets/loops, CC BY 4.0), through the
# app's engine on the PC (build/mx_render, tests/metal/mx_render.c) -> OUT.wav (and OUT.mp3 when ffmpeg is there), and
# OUT.txt, where each sound starts. Each sound is raised by up to 9 dB, less when its peak would pass -1 dBFS.
#   python3 tools/metal/demo.py build/metal-demo [FIRST [LAST]]     (sounds by number, 1..60: all of them by default)
import os, re, shutil, subprocess, sys, wave
import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
R = os.path.join(ROOT, "build", "mx_render")
LOOPS = os.path.join(ROOT, "assets", "loops")
LOOP = {1: "lead", 5: "lead", 29: "lead", 28: "arpeggio", 26: "chords", 12: "chords",
        31: "chords", 37: "chords", 38: "chords", 43: "lead", 47: "chords", 58: "arpeggio", 59: "lead"}   # the rest: chug (a low riff)
NFACT = int(re.search(r"#define MX_NFACT (\d+)", open(os.path.join(ROOT, "firmware", "apps", "metal", "mx_presets.h")).read()).group(1))


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "build", "metal-demo")
    first = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    last = int(sys.argv[3]) if len(sys.argv) > 3 else NFACT
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    subprocess.run(["cc", "-O2", "-DPX_NO_NAM=1", "-DPX_METAL=1", "-I" + os.path.join(ROOT, "engine"),
                    "-I" + os.path.join(ROOT, "firmware", "apps", "metal"), os.path.join(ROOT, "tests", "metal", "mx_render.c"),
                    os.path.join(ROOT, "firmware", "apps", "metal", "mx_engine.c"), os.path.join(ROOT, "firmware", "px_math.c"),
                    "-lm", "-o", R], check=True)
    parts, cue, t, tmp = [], [], 0.0, out + ".part.wav"
    gap = np.zeros((int(44100 * 0.7), 2))
    for k in range(first - 1, last):
        src = os.path.join(LOOPS, LOOP.get(k, "chug") + ".wav")
        r = subprocess.run([R, src, str(k), "-"], capture_output=True, text=True).stdout.split()
        name, pk = " ".join(r[:-2]), float(r[-2])
        subprocess.run([R, src, str(k), tmp, str(min(9.0, -1.0 - pk))], check=True, capture_output=True)
        w = wave.open(tmp)
        x = np.frombuffer(w.readframes(w.getnframes()), dtype="<i2").reshape(-1, 2)[:int(44100 * 6.5)].astype(np.float64)
        x[-4410:] *= np.linspace(1, 0, 4410)[:, None]
        cue.append("%d:%02d  %02d %s" % (t // 60, t % 60, k + 1, name))
        parts += [x, gap]
        t += (len(x) + len(gap)) / 44100
    os.remove(tmp)
    y = np.concatenate(parts).astype("<i2")
    o = wave.open(out + ".wav", "wb"); o.setnchannels(2); o.setsampwidth(2); o.setframerate(44100); o.writeframes(y.tobytes()); o.close()
    open(out + ".txt", "w").write("\n".join(cue) + "\n")
    if shutil.which("ffmpeg"):
        subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", out + ".wav", "-codec:a", "libmp3lame", "-b:a", "192k", out + ".mp3"], check=True)
    print("\n".join(cue))


if __name__ == "__main__":
    main()
