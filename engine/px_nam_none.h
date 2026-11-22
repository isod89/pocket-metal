/* SPDX-License-Identifier: GPL-3.0-only
 * OpenPocket's engine without NAM (PX_NO_NAM 1): what engine/px_dsp.c needs of px_nam.c and px_nam_factory.h, as stand-ins
 * that hold no network. An app on the base firmware that uses the engine but no NAM amp (firmware/apps/metal) builds with
 * it: about 570 KB of source less, the factory networks' weights out of the flash, no ring in the POOL.
 * px_dsp.c includes this file in place of px_nam.c and px_nam_factory.h; OpenPocket never defines PX_NO_NAM, so its image
 * is the same bytes. The NAM amp type stays in the catalog; an app keeps it out of its presets (a model that never loads
 * leaves the block silent, as an empty user slot does).
 */
#pragma once
#include "px_nam.h"

static const char *const PXN_FNAME[PX_NAM_NFACT] = {"", "", "", "", "", "", "", "", "", "", "", ""};
static const float PXN_FLVL[PX_NAM_NFACT] = {0};
static const float PXN_FW[PX_NAM_NFACT][1];
#undef PXN_RING_BYTES
#define PXN_RING_BYTES 8u                      /* (px_dsp.c's ring: two words, never used) */

int pxn_load(pxn_model *m, const float *w, int32_t *r0, unsigned n0, int32_t *r1, unsigned n1)
{ (void)m; (void)w; (void)r0; (void)n0; (void)r1; (void)n1; return -1; }   /* (nothing loads: rc_nam keeps the block silent) */
int pxn_selftest(int32_t *scratch) { (void)scratch; return 0; }
void pxn_set_helper(int (*run)(void (*fn)(uint32_t), uint32_t arg), int (*wait)(void)) { (void)run; (void)wait; }
int pxn_dual(void) { return 0; }
int pxn_tune(pxn_model *m, uint32_t (*ticks)(void), uint32_t t[PXN_MODES]) { (void)m; (void)ticks; (void)t; return 0; }
void pxn_set_fast(int32_t *base, unsigned bytes) { (void)base; (void)bytes; }
void pxn_set_lp(void *base, unsigned bytes) { (void)base; (void)bytes; }
int pxn_lp_ok(void) { return 0; }
void pxn_lp_reset(void) { }
void pxn_lp_split(int s) { (void)s; }
int pxn_lp_split_now(void) { return 0; }
uint32_t pxn_lp_queued(void) { return 0; }
int pxn_lp_split_used(void) { return 0; }
void pxn_lp_force(void) { }
uint32_t pxn_lp_handed(void) { return 0; }
int pxn_lp_before(uint32_t upto) { (void)upto; return 0; }
uint32_t pxn_lp_share(int s) { (void)s; return 0; }
void pxn_lp_clock(uint32_t (*ticks)(void), uint32_t limit) { (void)ticks; (void)limit; }
int pxn_lp_gave_up(void) { return 0; }
void pxn_lp_a(pxn_model *m, const float *in, float *out) { (void)m; (void)in; (void)out; }
int pxn_lp_b(pxn_model *m) { (void)m; return 0; }
int pxn_lp_equiv(pxn_model *m) { (void)m; return 0; }
int pxn_equiv(pxn_model *m, int mode) { (void)m; (void)mode; return 0; }
void pxn_process(pxn_model *m, const float *in, float *out, int n)
{ (void)m; (void)in; for (int i = 0; i < n; i++) out[i] = 0.0f; }
