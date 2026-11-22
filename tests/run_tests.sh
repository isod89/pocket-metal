#!/bin/sh
# POCKET METAL's host tests (Linux / macOS, or in the container: node tools/op.mjs test; no pedal, no JieLi toolchain):
#  - the base firmware the app stands on (the audio stages, the controls, the store), each with the pedal's own code
#  - the app's engine on real DI guitar (assets/loops)
#  - the app in the simulator, from its own C files (tools/sim.mjs --app metal): its screens, controls, store, MIDI
set -e
cd "$(dirname "$0")/.."
mkdir -p build
CC="${CC:-cc}"
t() { name="$1"; shift; "$@" > "build/$name.log" || { cat "build/$name.log"; exit 1; }; tail -1 "build/$name.log"; }
echo "== base audio (firmware/base/audio.h, audio_plain.h, audio_dual.h)"
BAF="-O1 -Wall -Wno-unused-function -Wno-unused-variable -Wno-int-to-pointer-cast -Ifirmware"
$CC $BAF tests/test_base_audio.c firmware/px_math.c -lm -o build/test_base_audio && t base_audio build/test_base_audio
$CC $BAF -DA_CH=2 tests/test_base_audio.c firmware/px_math.c -lm -o build/test_base_audio2 && t base_audio2 build/test_base_audio2
$CC $BAF -DA_CH=2 -DT_DUAL=1 tests/test_base_audio.c firmware/px_math.c -lm -o build/test_base_audio_dual && t base_audio_dual build/test_base_audio_dual
echo "== base controls (firmware/base/input.h over base/controls.h)"
$CC -O1 -Wall -Wno-unused-function -Wno-unused-variable -Itests/host -Ifirmware tests/test_base_input.c -o build/test_base_input
t base_input build/test_base_input
echo "== base store (firmware/base/store.h: the app's own data in flash)"
$CC -O1 -Wall -Wno-unused-function -Ifirmware tests/test_base_store.c -o build/test_base_store && t base_store build/test_base_store
echo "== POCKET METAL's engine on real DI guitar: every sound, amp, pedal, the down-tuner, the stereo stage"
$CC -O2 -Wall -Wno-unused-function -DPX_NO_NAM=1 -DPX_METAL=1 -Iengine -Ifirmware/apps/metal tests/metal/test_metal.c \
    firmware/apps/metal/mx_engine.c firmware/px_math.c -lm -o build/test_metal
t metal build/test_metal assets/loops/riff.wav
echo "== POCKET METAL in the simulator (tools/sim.mjs --app metal)"
t metal_sim node tests/metal/test_metal_sim.mjs
echo "all POCKET METAL tests passed"
