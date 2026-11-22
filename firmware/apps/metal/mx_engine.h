/* SPDX-License-Identifier: GPL-3.0-only
 * POCKET METAL's engine (firmware/apps/metal, docs/METAL.md): OpenPocket's engine (engine/px_dsp.c) built without NAM and
 * with the metal types (PX_METAL: a down-tuner, five pedals, ten fitted high-gain amps), then a stereo stage of its own.
 *
 * The stereo stage. A metal rhythm guitar is played twice and panned hard left and right; one take through a pedal can
 * only imitate that. WIDTH 0 is the mono sound on both sides. Above it the two sides' highs go through two different
 * chains of all-pass filters (second order, at frequencies between each other's): each side keeps every frequency at its level (the guitar stays in the middle, it never
 * moves from side to side, and neither side comes earlier), only the phases differ, which the ear hears as width.
 * Below about 150 Hz both sides stay the same, so the low end stays tight in the middle. Nothing moves with time.
 */
#pragma once
#include <stdint.h>
#include "px_dsp.h"

#define MX_AP 4                                        /* all-pass sections a side (second order) */
typedef struct {
    float w, tw;                                       /* the width now (glides) and where it goes, 0..1 */
    float x1, x2;                                      /* the guitar's lows / highs split (two poles) */
    float zl[MX_AP][4], zr[MX_AP][4];                  /* the two chains' states: x1 x2 y1 y2 */
} mx_stereo_t;

void mx_stereo_reset(mx_stereo_t *s);                  /* the filters cleared */
void mx_stereo_set(mx_stereo_t *s, int width);         /* WIDTH 0..100 (glides over ~20 ms) */
void mx_stereo_run(mx_stereo_t *s, const float *m, float *l, float *r, int n);   /* n mono samples -> left, right */
