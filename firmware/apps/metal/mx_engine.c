/* SPDX-License-Identifier: GPL-3.0-only
 * POCKET METAL's engine: see mx_engine.h. OpenPocket's engine is this file's first part, unchanged; app.sh builds it with
 * PX_NO_NAM (engine/px_nam_none.h: no network, no weights) and PX_METAL (engine/px_metal_*.h: the metal types). */
#include "px_dsp.c"
#include "mx_engine.h"

void mx_stereo_reset(mx_stereo_t *s)
{
    for (int k = 0; k < MX_AP; k++) for (int q = 0; q < 4; q++) s->zl[k][q] = s->zr[k][q] = 0.0f;
    s->x1 = s->x2 = 0.0f; s->w = s->tw;
}

void mx_stereo_set(mx_stereo_t *s, int width)
{
    if (width < 0) width = 0;
    if (width > 100) width = 100;
    s->tw = (float)width * 0.01f;
}

/* the chains: second-order all-passes, H = (r^2 - 2 r c z^-1 + z^-2) / (1 - 2 r c z^-1 + r^2 z^-2), c = cos(2 pi f / fs),
 * each turning the phase a whole turn around its frequency f. The left side's at 300, 900, 2500 and 6000 Hz, the right
 * side's between them (550, 1500, 4000, 9000 Hz): over the whole range the two sides' phases differ, at equal level. The
 * poles' radius r is w x 0.86: at w 0 both chains are the same two-sample delays (mono). */
static const float AP_CL[MX_AP] = {0.99909f, 0.99179f, 0.93723f, 0.65633f};   /* cos(2 pi f / 44100) */
static const float AP_CR[MX_AP] = {0.99693f, 0.97725f, 0.84195f, 0.28453f};

/* v2: the coefficients once a block (the width glides a block at a time, ~20 ms as before), each section over the whole
 * block in turn (its state stays in registers), in the transposed direct form II: a third of the work a sample of the
 * first version, which made every section's coefficients again at every sample. An all-pass section with a1 = -2 r c,
 * a2 = r^2: y = a2 x + s1, s1 = a1 x - a1 y + s2, s2 = x - a2 y. */
#define MX_FR 128                                      /* (the largest block: base/audio.h A_FR) */
static void mx_chain(float (*z)[4], const float *c, float r, float *x, int n)
{
    const float a2 = r * r;
    for (int q = 0; q < MX_AP; q++) {
        const float a1 = -2.0f * r * c[q];
        float s1 = z[q][0], s2 = z[q][1];
        for (int j = 0; j < n; j++) {
            const float v = x[j], y = a2 * v + s1;
            s1 = a1 * (v - y) + s2;
            s2 = v - a2 * y;
            x[j] = y;
        }
        z[q][0] = s1; z[q][1] = s2;
    }
}
void mx_stereo_run(mx_stereo_t *s, const float *m, float *l, float *r, int n)
{
    float x1 = s->x1, x2 = s->x2;
    const float k = 0.0211f;                           /* 150 Hz */
    float hl[MX_FR], hr[MX_FR];
    if (n > MX_FR) n = MX_FR;
    {   /* the width: from where it was to the block's end along the same one-pole glide (0.0012 a sample) */
        float g = 1.0f;
        for (int j = 0; j < n; j++) g *= 0.9988f;
        s->w = s->tw + (s->w - s->tw) * g;
    }
    const float rr = s->w * 0.86f;
    for (int j = 0; j < n; j++) {                      /* the highs (two poles), and the rest */
        const float x = m[j];
        x1 += (x - x1) * k;   const float h1 = x - x1;   x2 += (h1 - x2) * k;
        const float hi = h1 - x2;
        hl[j] = hi; hr[j] = hi;
        l[j] = x - hi;
    }
    s->x1 = x1; s->x2 = x2;
    mx_chain(s->zl, AP_CL, rr, hl, n);
    mx_chain(s->zr, AP_CR, rr, hr, n);
    for (int j = 0; j < n; j++) { const float low = l[j]; l[j] = low + hl[j]; r[j] = low + hr[j]; }
}
