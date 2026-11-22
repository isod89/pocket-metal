/* SPDX-License-Identifier: GPL-3.0-only
 * POCKET METAL on the PC: a factory sound (firmware/apps/metal/mx_presets.h) through the app's engine (OpenPocket's engine
 * built with PX_NO_NAM and PX_METAL, then the stereo stage), as the pedal runs it: 64 frames at a time, the guitar at the
 * pedal's level (DI x 0.15: hard playing peaks about -17 dBFS at the converter).
 *   mx_render IN.wav PRESET OUT.f32             the left output, float (tools/metal/level_metal.py measures it)
 *   mx_render IN.wav PRESET OUT.wav GAIN_DB     both outputs, 16-bit WAV, GAIN_DB added (to listen to)
 *   mx_render IN.wav PRESET -                   only checks: peak, NaN; prints "name peak_db nan"
 * IN.wav: 16-bit mono, 44.1 kHz.
 *   cc -O2 -DPX_NO_NAM=1 -DPX_METAL=1 -Iengine -Ifirmware/apps/metal tests/metal/mx_render.c \
 *      firmware/apps/metal/mx_engine.c firmware/px_math.c -lm -o build/mx_render
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mx_engine.h"
#include "mx_presets.h"

static px_engine e;
static mx_stereo_t st;

static short *read_wav(const char *path, int *n)
{
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    unsigned char h[12];
    if (fread(h, 1, 12, f) != 12 || memcmp(h, "RIFF", 4) || memcmp(h + 8, "WAVE", 4)) { fprintf(stderr, "%s: not a WAV\n", path); exit(1); }
    for (;;) {                                    /* the chunks, up to "data" */
        unsigned char c[8];
        if (fread(c, 1, 8, f) != 8) { fprintf(stderr, "%s: no data\n", path); exit(1); }
        const unsigned len = c[4] | c[5] << 8 | c[6] << 16 | (unsigned)c[7] << 24;
        if (!memcmp(c, "data", 4)) {
            short *b = malloc(len);
            *n = (int)(fread(b, 2, len / 2, f));
            fclose(f);
            return b;
        }
        fseek(f, (long)(len + (len & 1u)), SEEK_CUR);
    }
}
static void put32(FILE *f, unsigned v) { fputc(v & 255, f); fputc(v >> 8 & 255, f); fputc(v >> 16 & 255, f); fputc(v >> 24, f); }
static void put16(FILE *f, unsigned v) { fputc(v & 255, f); fputc(v >> 8 & 255, f); }

int main(int argc, char **argv)
{
    if (argc < 4) { fprintf(stderr, "usage: mx_render IN.wav PRESET OUT.f32|OUT.wav|- [GAIN_DB]\n"); return 2; }
    int n;
    short *in = read_wav(argv[1], &n);
    const int k = atoi(argv[2]);
    const char *out = argv[3];
    const float gdb = argc > 4 ? (float)atof(argv[4]) : 0.0f, g = powf(10.0f, gdb / 20.0f);
    mx_preset_t m;
    mx_factory(&m, k);
    n = n / 64 * 64;
    float *x = malloc(sizeof(float) * (size_t)n), *y = malloc(sizeof(float) * (size_t)n), *l = malloc(sizeof(float) * (size_t)n),
          *r = malloc(sizeof(float) * (size_t)n);
    for (int i = 0; i < n; i++) x[i] = (float)in[i] / 32768.0f * 0.15f;
    px_engine_init(&e, 44100.0f);
    px_engine_set(&e, &m.p);
    memset(&st, 0, sizeof st);
    mx_stereo_set(&st, m.width);
    mx_stereo_reset(&st);
    for (int i = 0; i < n; i += 64) {
        px_engine_process(&e, x + i, y + i, 64);
        mx_stereo_run(&st, y + i, l + i, r + i, 64);
    }
    float pk = 0.0f;
    int nan = 0;
    for (int i = 0; i < n; i++) {
        if (!isfinite(l[i]) || !isfinite(r[i])) nan++;
        else { if (fabsf(l[i]) > pk) pk = fabsf(l[i]); if (fabsf(r[i]) > pk) pk = fabsf(r[i]); }
    }
    printf("%s %.2f %d\n", m.p.name, 20.0f * log10f(pk + 1e-12f), nan);
    if (!strcmp(out, "-")) return nan ? 1 : 0;
    const size_t ol = strlen(out);
    FILE *f = fopen(out, "wb");
    if (!f) { perror(out); return 1; }
    if (ol > 4 && !strcmp(out + ol - 4, ".wav")) {
        fwrite("RIFF", 1, 4, f); put32(f, 36u + (unsigned)n * 4u); fwrite("WAVEfmt ", 1, 8, f); put32(f, 16); put16(f, 1);
        put16(f, 2); put32(f, 44100); put32(f, 44100 * 4); put16(f, 4); put16(f, 16); fwrite("data", 1, 4, f); put32(f, (unsigned)n * 4u);
        for (int i = 0; i < n; i++) {
            float a = l[i] * g, b = r[i] * g;
            a = a > 0.999f ? 0.999f : a < -0.999f ? -0.999f : a; b = b > 0.999f ? 0.999f : b < -0.999f ? -0.999f : b;
            put16(f, (unsigned)(unsigned short)(short)lrintf(a * 32767.0f)); put16(f, (unsigned)(unsigned short)(short)lrintf(b * 32767.0f));
        }
    } else fwrite(l, sizeof(float), (size_t)n, f);
    fclose(f);
    return nan ? 1 : 0;
}
