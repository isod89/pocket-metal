/* SPDX-License-Identifier: GPL-3.0-only
 * POCKET METAL's engine on the PC (docs/METAL.md): OpenPocket's engine with PX_NO_NAM and PX_METAL, and the stereo stage.
 *  - every factory sound on real DI guitar: finite, under full scale, its tuning and amp where mx_presets.h says
 *  - every metal amp over its GAIN knob and every metal pedal over its first knob: finite, not silent, not clipping
 *  - DOWN TUNE: a 220 Hz tone comes out 0..12 semitones lower, to the cent (zero crossings over 1 s)
 *  - the stereo stage: WIDTH 0 is the same sound on both sides; at 100 the sides differ above ~150 Hz, not below
 *   cc -O2 -DPX_NO_NAM=1 -DPX_METAL=1 -Iengine -Ifirmware/apps/metal tests/metal/test_metal.c \
 *      firmware/apps/metal/mx_engine.c firmware/px_math.c -lm -o build/test_metal && build/test_metal assets/loops/riff.wav
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mx_engine.h"
#include "mx_presets.h"

static px_engine e;
static mx_stereo_t st;
static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); } } while (0)

static float *di; static int ndi;
static void load_di(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    fseek(f, 0, SEEK_END); long nb = ftell(f) - 44; fseek(f, 44, SEEK_SET);
    ndi = (int)(nb / 2) / 64 * 64;
    short *b = malloc((size_t)nb);
    if (fread(b, 2, (size_t)ndi, f) != (size_t)ndi) { fprintf(stderr, "%s: short\n", path); exit(1); }
    fclose(f);
    di = malloc(sizeof(float) * (size_t)ndi);
    for (int i = 0; i < ndi; i++) di[i] = (float)b[i] / 32768.0f * 0.15f;
    free(b);
}
/* x through p (and the stereo stage at width w, w < 0: none): the peak, the RMS (left), NaN or not */
static void run(const px_preset_t *p, int w, const float *x, int n, float *out_l, float *pk, float *rms, int *bad)
{
    static float y[64], l[64], r[64];
    double s = 0.0;
    *pk = 0.0f; *bad = 0;
    px_engine_init(&e, 44100.0f);
    px_engine_set(&e, p);
    if (w >= 0) { memset(&st, 0, sizeof st); mx_stereo_set(&st, w); mx_stereo_reset(&st); }
    for (int i = 0; i + 64 <= n; i += 64) {
        px_engine_process(&e, x + i, y, 64);
        if (w >= 0) mx_stereo_run(&st, y, l, r, 64); else { memcpy(l, y, sizeof y); memcpy(r, y, sizeof y); }
        for (int k = 0; k < 64; k++) {
            if (!isfinite(l[k]) || !isfinite(r[k])) { (*bad)++; continue; }
            const float a = fabsf(l[k]) > fabsf(r[k]) ? fabsf(l[k]) : fabsf(r[k]);
            if (a > *pk) *pk = a;
            s += (double)l[k] * l[k];
            if (out_l) out_l[i + k] = l[k];
        }
    }
    *rms = (float)sqrt(s / (double)n);
}
static void only(px_preset_t *p, int blk, int type, int a, int b, int c, int d, int f)
{
    mx_preset_t m;
    mx_factory(&m, 0);
    *p = m.p;
    for (int k = 0; k < PX_NBLK; k++) p->on[k] = 0;
    p->on[blk] = 1; p->type[blk] = (uint8_t)type;
    p->p[blk][0] = (uint8_t)a; p->p[blk][1] = (uint8_t)b; p->p[blk][2] = (uint8_t)c; p->p[blk][3] = (uint8_t)d; p->p[blk][4] = (uint8_t)f;
}
static double freq(const float *x, int n)            /* rising zero crossings over the last part, interpolated */
{
    int first = -1, last = -1, cnt = 0;
    double t0 = 0, t1 = 0;
    for (int i = n / 2; i + 1 < n; i++)
        if (x[i] <= 0.0f && x[i + 1] > 0.0f) {
            const double t = i + (double)(-x[i]) / (double)(x[i + 1] - x[i]);
            if (first < 0) { first = i; t0 = t; } else { last = i; t1 = t; cnt++; }
        }
    return last < 0 ? 0.0 : cnt * 44100.0 / (t1 - t0);
}

int main(int argc, char **argv)
{
    load_di(argc > 1 ? argv[1] : "assets/loops/riff.wav");
    float pk, rms;
    int bad;
    /* 1. the factory sounds */
    for (int k = 0; k < MX_NFACT; k++) {
        mx_preset_t m;
        mx_factory(&m, k);
        run(&m.p, m.width, di, ndi, 0, &pk, &rms, &bad);
        CHECK(!bad, "sound %d %s: %d samples not finite", k + 1, m.p.name, bad);
        CHECK(pk < 1.5f && rms > 1e-3f, "sound %d %s: peak %.3f rms %.4f", k + 1, m.p.name, pk, rms);
        CHECK(m.p.type[PX_B_AMP] != PX_AMP_NAM && m.style < SY_N && m.width <= 100 && px_order_valid(m.p.order),
              "sound %d %s: its settings", k + 1, m.p.name);
    }
    printf("factory sounds: %d, finite, peaks under +3.5 dBFS (the pedal's soft knee takes the rest)\n", MX_NFACT);
    /* 2. the metal amps and pedals */
    for (int a = PX_AMP_RECTO; a < PX_AMP_COUNT; a++)
        for (int g = 0; g <= 100; g += 25) {
            px_preset_t p;
            only(&p, PX_B_AMP, a, g, 50, 50, 50, 50);
            p.on[PX_B_CAB] = 1; p.type[PX_B_CAB] = PX_CAB_412V30;
            run(&p, -1, di, ndi < 441000 ? ndi : 441000, 0, &pk, &rms, &bad);
            CHECK(!bad && pk < 1.0f && rms > 1e-3f, "amp %s gain %d: peak %.3f rms %.4f bad %d", px_type_name(PX_B_AMP, a), g, pk, rms, bad);
        }
    for (int d = PX_DRV_PRECISION; d < PX_DRV_COUNT; d++)
        for (int g = 0; g <= 100; g += 50) {
            px_preset_t p;
            only(&p, PX_B_DRIVE, d, g, 50, 50, 50, 50);
            run(&p, -1, di, ndi < 441000 ? ndi : 441000, 0, &pk, &rms, &bad);
            CHECK(!bad && pk < 1.5f && rms > 1e-4f, "pedal %s at %d: peak %.3f rms %.4f bad %d", px_type_name(PX_B_DRIVE, d), g, pk, rms, bad);
        }
    printf("metal amps: %d over their gain, pedals: %d, finite, not silent, peaks bounded\n", PX_AMP_COUNT - PX_AMP_RECTO, PX_DRV_COUNT - PX_DRV_PRECISION);
    /* 3. DOWN TUNE, to the cent */
    {
        const int n = 44100 * 2;
        float *x = malloc(sizeof(float) * n), *y = malloc(sizeof(float) * n);
        for (int i = 0; i < n; i++) x[i] = 0.2f * sinf(6.2831853f * 220.0f * (float)i / 44100.0f);
        for (int st_ = 0; st_ >= -12; st_ = st_ == -10 ? -12 : st_ - 1) {
            px_preset_t p;
            only(&p, PX_B_PITCH, PX_PIT_DOWN, px_down_knob(st_), 100, 0, 0, 0);
            CHECK(px_down_steps(px_down_knob(st_)) == st_, "the knob of %d semitones reads back %d", st_, px_down_steps(px_down_knob(st_)));
            memset(y, 0, sizeof(float) * n);
            run(&p, -1, x, n, y, &pk, &rms, &bad);
            const double want = 220.0 * pow(2.0, st_ / 12.0), got = freq(y, n), cents = 1200.0 * log2(got / want);
            CHECK(!bad && fabs(cents) < 3.0, "DOWN TUNE %d: %.2f Hz, wanted %.2f (%.1f cents)", st_, got, want, cents);
        }
        printf("DOWN TUNE: 0 to -10 and -12 semitones, each within 3 cents\n");
        free(x); free(y);
    }
    /* 4. the stereo stage */
    {
        const int n = 44100 / 64 * 64;
        float *m = malloc(sizeof(float) * n), *l = malloc(sizeof(float) * n), *r = malloc(sizeof(float) * n);
        for (int band = 0; band < 2; band++) {
            const float hz = band ? 2000.0f : 50.0f;
            for (int i = 0; i < n; i++) m[i] = 0.3f * sinf(6.2831853f * hz * (float)i / 44100.0f);
            for (int w = 0; w <= 100; w += 100) {
                memset(&st, 0, sizeof st); mx_stereo_set(&st, w); mx_stereo_reset(&st);
                for (int i = 0; i < n; i += 64) mx_stereo_run(&st, m + i, l + i, r + i, 64);
                double d = 0, s = 0;
                for (int i = n / 2; i < n; i++) { d += (double)(l[i] - r[i]) * (l[i] - r[i]); s += (double)l[i] * l[i]; }
                const double diff = sqrt(d / s);
                if (w == 0) CHECK(diff < 1e-6, "WIDTH 0 at %.0f Hz: the sides differ by %.3g", hz, diff);
                else if (band) CHECK(diff > 0.3, "WIDTH 100 at 2 kHz: the sides differ only by %.3f", diff);
                else CHECK(diff < 0.25, "WIDTH 100 at 50 Hz: the lows differ by %.3f (they should stay in the middle)", diff);
            }
        }
        printf("stereo: WIDTH 0 mono; WIDTH 100 wide above ~150 Hz, the lows in the middle\n");
        free(m); free(l); free(r);
    }
    if (fails) { printf("metal engine: %d FAILED\n", fails); return 1; }
    printf("metal engine: ok\n");
    return 0;
}
