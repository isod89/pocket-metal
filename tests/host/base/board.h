/* SPDX-License-Identifier: GPL-3.0-only
 * The pedal's chip and screen on this computer: firmware/base/board.h's stand-in for the simulator of an app on the
 * base firmware (tools/sim.mjs --app NAME; docs/SIMULATOR.md, "An app on the base firmware").
 * The app's own file (firmware/apps/NAME/NAME.c) is compiled as it is, with -Itests/host before -Ifirmware: its
 * "base/board.h", "px_usb.h" and "base/start.h" are the three files here, and every other part of the base firmware is
 * the pedal's own (the controls, the audio's stages, the store, MIDI, the product's INFO, the start and the loop).
 * Time is the simulator's: each main-loop pass is 1 ms, and an audio block is made every 2.9 ms of it.
 * An app on two cores (DSP_CPU1 1, base/audio_dual.h) gets a stand-in for the second core: fm1_cpu1_run keeps the job
 * and fm1_cpu1_wait runs it, so a block's work happens in the order the pedal may finish it in (CPU0's side work first,
 * then CPU1's); H_CPU1_OFF in the environment starts it held, as on a unit whose second core never answers.
 */
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define OP_NOINIT                               /* (no sections here: firmware/base/board.h) */
#define OP_POOL
#include "pins.h"                               /* the ports and the ADC, as variables */

/* ---- time: the simulator's (tests/host/base/start.h moves it) ---- */
#define FM1_TICKS_PER_US 24u
static uint32_t h_ms, h_ticks;
static uint32_t ms_now(void) { return h_ms; }
static uint32_t fm1_ticks(void) { return h_ticks; }
static void h_tick(void);                       /* 1 ms passes: the audio's interrupt when it is due */
static void fm1_wdt_feed(void) { h_tick(); }    /* (every wait of the firmware feeds the watchdog: time passes in it) */
static void fm1_delay_ms(uint32_t ms) { (void)ms; }

#include "../simlcd.h"                          /* the screen: a frame buffer, the colours */
#include "ui_text.h"                            /* the pedal's own text drawing */

/* ---- the chip's parts the base firmware names ---- */
#define FM1_IRQ_ALNK0 0
static int h_irq_held;
static void fm1_irq_mask(int irq) { (void)irq; h_irq_held++; }
static void fm1_irq_unmask(int irq) { (void)irq; h_irq_held--; }
static void fm1_irq_attach(int irq, void (*isr)(void), int prio) { (void)irq; (void)isr; (void)prio; }
static void fm1_irq_enable_all(void) { }
void isr_alnk0(void) { }
static void fm1_reboot(void) { exit(3); }
#if DSP_CPU1
typedef void (*fm1_cpu1_fn)(uint32_t);
static uint8_t fm1_cpu1_on, fm1_cpu1_gave_up;
static fm1_cpu1_fn h_c1_fn;
static uint32_t h_c1_arg, h_c1_jobs;
static void fm1_cpu1_bar_build(uint32_t build) { (void)build; }
static int fm1_cpu1_start(void) { fm1_cpu1_on = !getenv("H_CPU1_OFF"); return fm1_cpu1_on ? 0 : -1; }
static int fm1_cpu1_run(fm1_cpu1_fn fn, uint32_t arg) { if (!fm1_cpu1_on || h_c1_fn) return 0; h_c1_fn = fn; h_c1_arg = arg; return 1; }
static int fm1_cpu1_busy(void) { return h_c1_fn != 0; }
static int fm1_cpu1_wait(void) { if (h_c1_fn) { fm1_cpu1_fn f = h_c1_fn; h_c1_fn = 0; f(h_c1_arg); h_c1_jobs++; } return 0; }
static void fm1_cpu1_stop(void) { fm1_cpu1_on = 0; }
static void fm1_cpu1_sync(void) { }
#endif
#define FM1_CRASH_MAGIC 0x43525348u
typedef struct { uint32_t magic, count, vec, pc, rets, emu, dbg, uptime_ms; } fm1_crash_t;
static fm1_crash_t fm1_crash;
static volatile struct { uint32_t magic, pending, failed; } guard;
static void lcd_init(void) { }
static void lcd_text(uint32_t x, uint32_t y, const char *s, uint16_t fg, uint16_t bg, uint32_t k) { (void)x; (void)y; (void)s; (void)fg; (void)bg; (void)k; }
static void b_num(char *d, uint32_t v) { char t[12]; int n = 0; while (*d) d++; do { t[n++] = (char)('0' + v % 10u); v /= 10u; } while (v); while (n) *d++ = t[--n]; *d = 0; }
static void b_str(char *d, const char *s) { while (*d) d++; while (*s) *d++ = *s++; *d = 0; }

/* firmware/base/audio.h defines the C library functions the I2S driver calls: under other names here, this computer
 * has its own (the simulator writes with fputs and fprintf) */
#define printf jl_printf
#define malloc jl_malloc
#define free jl_free
static uint32_t h_gpio[0x200];                  /* the ports' registers audio.h writes (the amp's enable) */
#define JL_GPIO ((uintptr_t)h_gpio)
