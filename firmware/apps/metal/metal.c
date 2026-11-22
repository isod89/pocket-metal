/* SPDX-License-Identifier: GPL-3.0-only
 * POCKET METAL: an alternative firmware for the Donner Pocket X / M-VAVE BlackBox, made for metal (docs/METAL.md). It is
 * built on OpenPocket's base firmware (docs/BASE-FIRMWARE.md) and runs OpenPocket's engine without NAM: every style of
 * metal from fitted amps and pedals, a down-tuner, a stereo stage, black and blood screens.
 *   node tools/op.mjs app metal        ->  build/apps/metal/ (the image, its two packages, the manifest)
 *   node tools/op.mjs sim --app metal  ->  the same app.h and engine on this computer (docs/SIMULATOR.md)
 * This file is the pedal's side only: what the app declares to the base, and the order of the base's parts. The app
 * itself is app.h; its screens mx_ui.h; its sounds mx_presets.h; its engine mx_engine.c.
 */
#include <stdint.h>
#define AUDIO_PROBE 0
#define DSP_CPU1 1                      /* two cores: the chain on CPU1, the delay and the reverb on CPU0 (base/audio_dual.h) */
#define OP_CPU1_BUILD 0x4D540002u       /* (a new build: the second core gets another chance on a unit that retired it) */
#define OP_APP_ID "metal"               /* this folder's name: the product's id in its packages and for the tools */
#define OP_NAME "POCKET METAL"          /* its name: the boot screen, the installer */
#define OP_VERSION_NUM "02"             /* the package's number: PocketX_902, BlackBox_902 (firmware/build.sh, px_usb.h) */
#define OP_VERSION "v" OP_VERSION_NUM
#define A_CH 2                          /* its sound has a left and a right side (base/audio.h): the stereo stage */
#include "base/board.h"                 /* the chip, the pins, the screen and its text */
#include "base/controls.h"              /* the controls' pins, the ADC, the MASTER knob */
#include "base/diag.h"                  /* the crash screen, the boot stages kept across a reset */
#include "base/audio.h"                 /* the I2S, a_input, dsp_out, dsp_load, a_output, the tuner */
static void app_sound(const float *in, float *out);   /* app.h */
static void app_sound_reset(void);
static void app_cores(int n);
static void app_beside(uint32_t deadline);
static void app_boundary(void);
#include "base/audio_dual.h"            /* a_block, a_send, a_collect: app_sound on CPU1, app_beside on CPU0; a_change */
#define OP_FLASH_OWN(off, n) BS_FLASH_OWN(off, n)     /* the flash it may write: its store's (base/flashmap.h) */
#define APP_MIDI app_midi               /* its MIDI map (app.h) */
#define APP_EDITOR app_editor           /* its own editor requests (app.h), protocol 1 */
#include "px_usb.h"                     /* USB-MIDI and the update entry: the installer, the way back to OpenPocket */
#include "base/store.h"                 /* its data in flash */
#include "base/midi.h"                  /* the MIDI ring read, each message to app_midi */
#include "base/input.h"                 /* the controls as events: app_turn, app_button, app_master */
#include "base/product.h"               /* the product's descriptor, INFO, app_editor's frames */
#include "base/loop.h"                  /* base_begin, base_go, base_loop */
#include "app.h"                        /* the app */
#include "base/start.h"                 /* fm1_cstart: it calls app_main, below */

static void app_main(void)
{
    base_begin();                        /* the screen, the boot screens, the controls, the flash */
    app_init();                          /* its saved sounds, the engine (the POOL region is not zeroed: it clears it) */
    base_go();                           /* USB, then guitar -> app_sound -> output */
    for (;;) {
        const uint32_t now = ms_now();
        base_loop(now);                  /* the watchdog, the installer, the boot guard, the MIDI ring */
        app_step(now);                   /* the controls, the engine following the sound, the screen */
    }
}
