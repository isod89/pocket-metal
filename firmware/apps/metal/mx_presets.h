/* SPDX-License-Identifier: GPL-3.0-only
 * POCKET METAL's factory sounds (docs/METAL.md): sixty, one or more for each style of metal, built from the engine's blocks
 * (engine/px_dsp.c with PX_METAL). Knobs at noon on an amp or a pedal are the captured gear's setting; the levels come
 * from tools/metal/level_metal.py (mx_levels.h: every sound at the same loudness on real DI guitar). This file is the
 * pedal's (app.h) and the PC tools' (tests/metal/mx_render.c): nothing here touches the chip.
 */
#pragma once
#include <stdint.h>
#include <string.h>
#include "px_dsp.h"

/* a sound: OpenPocket's preset, then what the metal app adds. 112 bytes */
typedef struct { px_preset_t p; uint8_t width, style; } mx_preset_t;

enum { SY_HEAVY, SY_THRASH, SY_SPEED, SY_POWER, SY_DEATH, SY_SWEDE, SY_BLACK, SY_DOOM, SY_SLUDGE, SY_STONER, SY_DJENT,
       SY_PROG, SY_NU, SY_CORE, SY_DCORE, SY_GRIND, SY_INDUS, SY_GROOVE, SY_GOTHIC, SY_SYMPH, SY_POST, SY_CLEAN, SY_LEAD,
       SY_NWOBHM, SY_GLAM, SY_XOVER, SY_TECH, SY_MELO, SY_DDOOM, SY_DRONE, SY_GAZE, SY_DSBM, SY_WAR, SY_FOLK, SY_VIKING,
       SY_NEO, SY_MATH, SY_ALT, SY_CRUST, SY_EPIC, SY_MCORE, SY_THALL,
       SY_N };
static const char *const MX_STYLE[SY_N] = {
    "HEAVY METAL", "THRASH", "SPEED METAL", "POWER METAL", "DEATH METAL", "SWEDISH DEATH", "BLACK METAL", "DOOM",
    "SLUDGE", "STONER", "DJENT", "PROG METAL", "NU METAL", "METALCORE", "DEATHCORE", "GRINDCORE", "INDUSTRIAL", "GROOVE",
    "GOTHIC", "SYMPHONIC", "POST-METAL", "CLEAN", "LEAD",
    "NWOBHM", "GLAM METAL", "CROSSOVER", "TECH DEATH", "MELODEATH", "DEATH-DOOM", "DRONE", "BLACKGAZE", "DEPRESSIVE",
    "WAR METAL", "FOLK METAL", "VIKING METAL", "NEOCLASSICAL", "MATHCORE", "ALT METAL", "CRUST", "EPIC DOOM",
    "MELODIC CORE", "THALL"};

#define MX_NFACT 60
#include "mx_levels.h"                                  /* MX_LVL[MX_NFACT]: each sound's LEVEL (tools/metal/level_metal.py) */

static void mx_b(px_preset_t *p, int b, int on, int type, int a, int c, int d, int e, int f)
{
    p->on[b] = (uint8_t)on; p->type[b] = (uint8_t)type;
    p->p[b][0] = (uint8_t)a; p->p[b][1] = (uint8_t)c; p->p[b][2] = (uint8_t)d; p->p[b][3] = (uint8_t)e; p->p[b][4] = (uint8_t)f;
}
/* what every sound starts from: a gate, an amp and a 4x12, the rest off with settings worth starting from */
static void mx_base(mx_preset_t *m)
{
    px_preset_t *p = &m->p;
    memset(m, 0, sizeof *m);
    mx_b(p, PX_B_GATE,   1, 0, 45, 25, 50, 0, 0);
    mx_b(p, PX_B_COMP,   0, PX_CMP_STUDIO, 40, 40, 50, 100, 0);
    mx_b(p, PX_B_PITCH,  0, PX_PIT_DOWN, 0, 100, 0, 0, 0);
    mx_b(p, PX_B_SFX,    0, PX_SFX_AUTOWAH, 60, 60, 60, 40, 0);
    mx_b(p, PX_B_DRIVE,  0, PX_DRV_TS9, 0, 55, 75, 0, 0);
    mx_b(p, PX_B_AMP,    1, PX_AMP_5150, 50, 50, 50, 50, 50);
    mx_b(p, PX_B_CAB,    1, PX_CAB_412V30, 50, 50, 50, 0, 0);
    mx_b(p, PX_B_EQ,     0, PX_EQ_GEQ, 50, 50, 50, 50, 50);
    mx_b(p, PX_B_MOD,    0, PX_MOD_CHORUS, 30, 40, 40, 0, 0);
    mx_b(p, PX_B_DELAY,  0, PX_DLY_DIGITAL, 29, 25, 20, 50, 0);
    mx_b(p, PX_B_REVERB, 0, PX_REV_ROOM, 35, 50, 15, 0, 0);
    mx_b(p, PX_B_SYN,    0, PX_SYN_ORGAN, 50, 70, 40, 30, 0);
    px_order_default(p->order);
    m->width = 50;
}
static void mx_name(mx_preset_t *m, const char *s, int style)
{
    int i = 0;
    for (; s[i] && i < (int)sizeof m->p.name - 1; i++) m->p.name[i] = s[i];
    m->p.name[i] = 0;
    m->style = (uint8_t)style;
}

/* the settings of one sound, by block: B(block, on, type, five settings) */
#define B(b, on, t, a, c, d, e, f) mx_b(p, PX_B_##b, on, t, a, c, d, e, f)
#define GATE(th, rel) B(GATE, 1, 0, th, rel, 50, 0, 0)
#define NOGATE (p->on[PX_B_GATE] = 0)
#define TUNE(st) B(PITCH, 1, PX_PIT_DOWN, px_down_knob(st), 100, 0, 0, 0)
#define AMP(t, g, b, md, tr, pr) B(AMP, 1, PX_AMP_##t, g, b, md, tr, pr)
#define CAB(t, body, bright, mic) B(CAB, 1, PX_CAB_##t, body, bright, mic, 0, 0)
#define DRV(t, a, c, d, e) B(DRIVE, 1, PX_DRV_##t, a, c, d, e, 0)
#define GEQ(a, c, d, e, f) B(EQ, 1, PX_EQ_GEQ, a, c, d, e, f)
#define DLY(t, ms, fb, mix, tone) B(DELAY, 1, PX_DLY_##t, px_dly_value_for_ms(PX_DLY_##t, ms), fb, mix, tone, 0)
#define REV(t, dec, tone, mix) B(REVERB, 1, PX_REV_##t, dec, tone, mix, 0, 0)
#define REV4(t, dec, tone, mix, x) B(REVERB, 1, PX_REV_##t, dec, tone, mix, x, 0)
#define MOD(t, a, c, d) B(MOD, 1, PX_MOD_##t, a, c, d, 0, 0)
#define WIDTH(w) (m->width = (uint8_t)(w))

static void mx_factory(mx_preset_t *m, int idx)
{
    px_preset_t *p = &m->p;
    mx_base(m);
    if (idx < 0 || idx >= MX_NFACT) idx = 0;
    switch (idx) {
    /* ---- the classics ---- */
    case 0: mx_name(m, "HEAVY ANTHEM", SY_HEAVY); GATE(35, 30);       /* British crunch pushed by an overdrive */
        DRV(TS9, 15, 55, 70, 0); AMP(JCM900, 60, 50, 60, 58, 58); CAB(412V30, 50, 55, 50);
        REV(PLATE, 35, 50, 12); WIDTH(45); break;
    case 1: mx_name(m, "TWIN GUITARS", SY_HEAVY); GATE(30, 35);       /* the harmony lead: a third above, delay, hall */
        AMP(JCM2000, 55, 50, 60, 52, 55); CAB(412GB, 50, 50, 50);
        B(PITCH, 1, PX_PIT_HARMONY, 70, 70, 0, 0, 0);
        DLY(ANALOG, 420, 30, 18, 45); REV(HALL, 45, 45, 18); WIDTH(60); break;
    case 2: mx_name(m, "THRASH ATTACK", SY_THRASH); GATE(50, 15);     /* tight, scooped, fast: a Mark and a boost */
        DRV(TS9, 0, 60, 80, 0); AMP(MARK4, 55, 55, 40, 55, 55); CAB(412V30, 50, 55, 50);
        GEQ(55, 50, 38, 50, 55); WIDTH(70); break;
    case 3: mx_name(m, "BAY AREA", SY_THRASH); GATE(50, 15);
        DRV(TS9, 0, 55, 75, 0); AMP(6505, 50, 55, 45, 55, 50); CAB(412V30, 50, 50, 50); WIDTH(70); break;
    case 4: mx_name(m, "SPEED DEMON", SY_SPEED); GATE(40, 20);
        DRV(M77, 20, 60, 65, 0); AMP(JVM, 55, 50, 55, 58, 58); CAB(412GB, 50, 55, 50);
        REV(ROOM, 30, 55, 10); WIDTH(60); break;
    case 5: mx_name(m, "STEEL VALOR", SY_POWER); GATE(40, 25);         /* bright and singing, delay and a hall */
        AMP(5150, 52, 50, 55, 55, 55); CAB(412V30, 50, 55, 50);
        DLY(DIGITAL, 375, 25, 15, 55); REV(HALL, 45, 55, 15); WIDTH(55); break;
    /* ---- death ---- */
    case 6: mx_name(m, "OLD SCHOOL DM", SY_DEATH); GATE(55, 15); TUNE(-2);   /* D standard, a dark Recto */
        DRV(DS1, 25, 45, 60, 0); AMP(RECTO, 50, 55, 50, 50, 50); CAB(412V30, 55, 45, 50); WIDTH(60); break;
    case 7: mx_name(m, "BRUTAL DEATH", SY_DEATH); GATE(60, 10); TUNE(-5);   /* B standard, very tight */
        DRV(PRECISION, 0, 55, 80, 0); AMP(IIIRED, 55, 50, 55, 50, 50); CAB(412V30, 50, 50, 50);
        GEQ(55, 45, 50, 52, 50); WIDTH(75); break;
    case 8: mx_name(m, "CHAINSAW", SY_SWEDE); GATE(55, 20); TUNE(-2);     /* every knob of the Swedish pedal up */
        DRV(HM2, 80, 100, 100, 50); AMP(JCM900, 35, 50, 50, 55, 55); CAB(412V30, 50, 55, 50); WIDTH(65); break;
    case 9: mx_name(m, "SWEDE BUZZ", SY_SWEDE); GATE(55, 20); TUNE(-4);   /* C standard, the buzz into a Laney */
        DRV(HM2, 70, 85, 90, 50); AMP(GH100, 40, 50, 55, 55, 55); CAB(412V30, 50, 55, 50); WIDTH(65); break;
    /* ---- black ---- */
    case 10: mx_name(m, "NORTHERN COLD", SY_BLACK); GATE(35, 30);       /* thin, bright, a cathedral behind tremolo picking */
        AMP(JCM2000, 70, 30, 55, 70, 65); CAB(412GB, 40, 60, 55);
        GEQ(35, 45, 55, 60, 55); REV(CATHEDRAL, 60, 45, 25); WIDTH(80); break;
    case 11: mx_name(m, "FROSTBITE", SY_BLACK); GATE(30, 35);           /* raw: a cheap distortion, lo-fi and fizzy */
        DRV(DS1, 70, 70, 55, 0); AMP(MIG50, 45, 35, 55, 65, 60); CAB(112CREAM, 40, 60, 60);
        REV(HALL, 50, 40, 20); WIDTH(40); break;
    case 12: mx_name(m, "FUNERAL HALL", SY_BLACK); GATE(30, 40);        /* atmospheric: long delay into an ambient tail */
        AMP(SAVAGE, 55, 40, 50, 60, 55); CAB(412V30, 45, 55, 50);
        DLY(TAPE, 480, 40, 22, 40); REV(AMBIENT, 70, 40, 35); WIDTH(85); break;
    /* ---- slow and heavy ---- */
    case 13: mx_name(m, "DOOM CANDLE", SY_DOOM); GATE(30, 40); TUNE(-3);    /* the first doom: a Laney head, C# */
        DRV(M77, 25, 45, 65, 0); AMP(GH100, 60, 60, 60, 45, 45); CAB(412GB, 60, 40, 45);
        REV(HALL, 40, 40, 15); WIDTH(50); break;
    case 14: mx_name(m, "FUNERAL DOOM", SY_DOOM); GATE(30, 50); TUNE(-5);   /* B, an orange wall and a huge hall */
        AMP(ROCKER, 60, 65, 60, 45, 45); CAB(412GB, 65, 40, 45);
        DLY(ANALOG, 600, 30, 15, 35); REV(CATHEDRAL, 65, 35, 25); WIDTH(70); break;
    case 15: mx_name(m, "SWAMP SLUDGE", SY_SLUDGE); GATE(40, 30); TUNE(-4);  /* C, a dirty pedal into a mean little head */
        DRV(RAT, 55, 45, 60, 0); AMP(MIG50, 60, 60, 60, 50, 50); CAB(412GB, 60, 45, 50); WIDTH(55); break;
    case 16: mx_name(m, "DESERT FUZZ", SY_STONER); GATE(25, 40); TUNE(-2);   /* D, fuzzy and warm, a spring of air */
        DRV(OS2, 60, 50, 60, 0); AMP(ROCKER, 50, 60, 65, 50, 45); CAB(412GB, 60, 45, 50);
        REV(SPRING, 35, 45, 12); WIDTH(50); break;
    /* ---- modern ---- */
    case 17: mx_name(m, "DJENT MACHINE", SY_DJENT); GATE(65, 8); TUNE(-7);   /* A: the eight-string's low string */
        DRV(PRECISION, 0, 60, 85, 0); AMP(YERASOV, 50, 50, 50, 50, 50); CAB(412V30, 45, 55, 50);
        GEQ(45, 50, 45, 55, 50); WIDTH(80); break;
    case 18: mx_name(m, "POLYRHYTHM", SY_DJENT); GATE(65, 8); TUNE(-5);      /* B, the modern 5150 */
        DRV(PRECISION, 0, 55, 80, 0); AMP(IIIRED, 50, 50, 50, 55, 55); CAB(412V30, 50, 50, 50); WIDTH(75); break;
    case 19: mx_name(m, "PROG ODYSSEY", SY_PROG); GATE(45, 25);             /* clear high gain, a dotted delay */
        AMP(SAVAGE, 50, 50, 55, 55, 55); CAB(412V30, 50, 50, 50);
        DLY(DUAL, 360, 30, 18, 50); REV(PLATE, 40, 50, 15); WIDTH(65); break;
    case 20: mx_name(m, "NU GROOVE", SY_NU); GATE(55, 15); TUNE(-5);        /* B, scooped and loose */
        AMP(RECTO, 55, 60, 35, 55, 50); CAB(412V30, 55, 50, 50);
        GEQ(60, 50, 38, 50, 55); WIDTH(80); break;
    case 21: mx_name(m, "BREAKDOWN", SY_CORE); GATE(60, 10); TUNE(-4);      /* C, the 6505 pushed by a clean boost */
        DRV(PRECISION, 0, 55, 80, 0); AMP(6505, 50, 55, 50, 55, 50); CAB(412V30, 50, 50, 50); WIDTH(80); break;
    case 22: mx_name(m, "SLAM", SY_DCORE); GATE(70, 6); TUNE(-7);           /* A, the lowest and the tightest */
        DRV(PRECISION, 0, 55, 85, 0); AMP(XXX, 50, 50, 55, 50, 50); CAB(412V30, 45, 50, 50);
        GEQ(45, 50, 50, 55, 50); WIDTH(80); break;
    case 23: mx_name(m, "BLAST BEAT", SY_GRIND); GATE(45, 15); TUNE(-2);    /* D, raw and buzzing */
        DRV(UBER, 50, 60, 60, 50); AMP(RECTO, 40, 50, 55, 50, 50); CAB(412V30, 50, 50, 50); WIDTH(55); break;
    case 24: mx_name(m, "STEEL FACTORY", SY_INDUS); GATE(70, 5); TUNE(-2);  /* D, hard-gated, machine-tight */
        DRV(TS9, 0, 60, 80, 0); AMP(5150, 55, 55, 50, 55, 55); CAB(412V30, 50, 50, 50);
        GEQ(55, 40, 50, 55, 50); WIDTH(85); break;
    case 25: mx_name(m, "COWBOY GROOVE", SY_GROOVE); GATE(50, 15); TUNE(-1);  /* Eb, a sharp scooped crunch */
        AMP(MARK4, 60, 55, 30, 60, 60); CAB(412V30, 50, 55, 50);
        GEQ(55, 50, 35, 55, 55); WIDTH(65); break;
    /* ---- atmosphere, clean, lead ---- */
    case 26: mx_name(m, "GOTHIC VEIL", SY_GOTHIC); GATE(35, 30);             /* dark, a chorus, a cathedral */
        AMP(JCM900, 55, 55, 50, 45, 45); CAB(412GB, 55, 45, 50);
        MOD(CHORUS, 20, 35, 35); REV(CATHEDRAL, 55, 40, 25); WIDTH(70); break;
    case 27: mx_name(m, "SYMPHONIC", SY_SYMPH); GATE(45, 20);                /* modern and clear under an orchestra */
        AMP(SAVAGE, 50, 50, 55, 55, 50); CAB(412V30, 50, 50, 50);
        REV(PLATE, 45, 50, 15); WIDTH(70); break;
    case 28: mx_name(m, "POST CLEAN", SY_POST); GATE(15, 50);                /* the clean part before the storm */
        AMP(TWIN, 45, 50, 50, 55, 50); CAB(212BLUE, 50, 50, 50);
        MOD(CHORUS, 25, 40, 40); DLY(DIGITAL, 450, 40, 25, 45); REV4(SHIMMER, 60, 45, 30, 40); WIDTH(70); break;
    case 29: mx_name(m, "SHRED LEAD", SY_LEAD); GATE(35, 30);                /* sustain, delay and a hall */
        DRV(TS9, 20, 55, 70, 0); AMP(JVM, 60, 50, 60, 55, 55); CAB(412V30, 50, 50, 50);
        DLY(DIGITAL, 400, 30, 20, 50); REV(HALL, 50, 50, 20); WIDTH(45); break;
    /* ---- v2: thirty more, so that every style of metal has its sound ---- */
    case 30: mx_name(m, "IRON GALLOP", SY_NWOBHM); GATE(25, 35);            /* the British new wave: a Marshall, no pedal */
        AMP(JCM900, 42, 50, 62, 58, 55); CAB(412GB, 50, 55, 50);
        REV(PLATE, 35, 50, 12); WIDTH(55); break;
    case 31: mx_name(m, "SUNSET STRIP", SY_GLAM); GATE(30, 30);             /* bright crunch, chorus, a big hall */
        DRV(TS9, 10, 60, 65, 0); AMP(JCM2000, 50, 45, 55, 62, 60); CAB(412GB, 45, 60, 50);
        MOD(CHORUS, 25, 30, 25); DLY(DIGITAL, 380, 25, 15, 55); REV(HALL, 45, 55, 15); WIDTH(70); break;
    case 32: mx_name(m, "SKATE THRASH", SY_XOVER); GATE(45, 15);            /* punk speed, thrash bite, a small room */
        DRV(DS1, 30, 55, 60, 0); AMP(6505, 42, 50, 50, 58, 55); CAB(412V30, 50, 55, 50);
        REV(ROOM, 25, 55, 8); WIDTH(55); break;
    case 33: mx_name(m, "TEUTONIC", SY_THRASH); GATE(50, 12);               /* German thrash: harsh, buzzing, mids down */
        DRV(M77, 10, 60, 75, 0); AMP(5150, 58, 50, 40, 60, 58); CAB(412V30, 50, 55, 50);
        GEQ(55, 50, 42, 55, 58); WIDTH(70); break;
    case 34: mx_name(m, "TECH SURGEON", SY_TECH); GATE(65, 8); TUNE(-5);   /* B, surgical: every note of a sweep apart */
        DRV(PRECISION, 0, 60, 85, 0); AMP(IIIRED, 45, 45, 58, 55, 55); CAB(412V30, 45, 55, 50);
        GEQ(45, 50, 55, 55, 50); WIDTH(70); break;
    case 35: mx_name(m, "GOTHENBURG", SY_MELO); GATE(50, 15); TUNE(-4);    /* C, melodic death: twin melodies over the riff */
        DRV(TS9, 0, 55, 75, 0); AMP(6505, 50, 50, 55, 58, 55); CAB(412V30, 50, 55, 50);
        DLY(DIGITAL, 330, 20, 10, 50); REV(HALL, 40, 50, 12); WIDTH(75); break;
    case 36: mx_name(m, "GRAVE BLOOM", SY_DDOOM); GATE(40, 40); TUNE(-5);  /* B, death growl at doom pace, a long hall */
        DRV(M77, 15, 45, 70, 0); AMP(RECTO, 58, 60, 50, 48, 45); CAB(412GB, 60, 45, 50);
        DLY(ANALOG, 650, 25, 12, 35); REV(HALL, 60, 40, 22); WIDTH(70); break;
    case 37: mx_name(m, "MONOLITH", SY_DRONE); GATE(15, 70); TUNE(-7);     /* A, one chord held until the room shakes */
        DRV(OS2, 50, 40, 55, 0); AMP(ROCKER, 75, 70, 60, 40, 40); CAB(412GB, 70, 35, 45);
        REV(AMBIENT, 75, 35, 30); WIDTH(60); break;
    case 38: mx_name(m, "DAWN SHROUD", SY_GAZE); GATE(25, 45);             /* black metal into a wall of shimmer */
        AMP(JCM2000, 62, 35, 55, 65, 60); CAB(412GB, 45, 55, 50);
        DLY(TAPE, 380, 35, 22, 40); REV4(SHIMMER, 70, 45, 35, 30); WIDTH(95); break;
    case 39: mx_name(m, "GREY WINTER", SY_DSBM); GATE(30, 40);             /* depressive black: buzzing, far away, alone */
        DRV(DS1, 55, 55, 50, 0); AMP(MIG50, 45, 40, 55, 60, 55); CAB(112CREAM, 45, 55, 55);
        REV(CATHEDRAL, 70, 35, 32); WIDTH(40); break;
    case 40: mx_name(m, "WAR ASSAULT", SY_WAR); GATE(45, 12); TUNE(-1);   /* Eb, bestial: raw distortion, no air */
        DRV(DS1, 75, 50, 65, 0); AMP(RECTO, 55, 50, 50, 55, 50); CAB(412V30, 50, 50, 50); WIDTH(50); break;
    case 41: mx_name(m, "MEAD HALL", SY_FOLK); GATE(40, 25);               /* folk metal: present mids for the melodies */
        AMP(JVM, 52, 50, 62, 55, 55); CAB(412GB, 50, 50, 50);
        GEQ(48, 50, 58, 55, 50); REV(HALL, 45, 50, 15); WIDTH(70); break;
    case 42: mx_name(m, "LONGSHIP", SY_VIKING); GATE(50, 18); TUNE(-4);    /* C, broad and marching, a hall like a fjord */
        DRV(TS9, 0, 55, 75, 0); AMP(5150, 55, 58, 52, 55, 52); CAB(412V30, 55, 50, 50);
        REV(HALL, 55, 45, 18); WIDTH(80); break;
    case 43: mx_name(m, "BAROQUE FIRE", SY_NEO); GATE(30, 30); TUNE(-1);   /* Eb, neoclassical: a singing Marshall, arpeggios */
        DRV(TS9, 35, 60, 65, 0); AMP(JCM2000, 50, 45, 62, 58, 55); CAB(412GB, 50, 55, 50);
        DLY(DIGITAL, 340, 25, 15, 55); REV(HALL, 50, 55, 18); WIDTH(45); break;
    case 44: mx_name(m, "CHAOS THEORY", SY_MATH); GATE(65, 6); TUNE(-2);   /* D, mathcore: stops that stop dead */
        DRV(PRECISION, 0, 55, 80, 0); AMP(5150, 48, 50, 55, 58, 55); CAB(412V30, 50, 55, 50); WIDTH(60); break;
    case 45: mx_name(m, "DREAM DROP", SY_ALT); GATE(40, 25); TUNE(-5);     /* B, alternative metal: heavy and dreamy at once */
        AMP(RECTO, 48, 55, 50, 52, 50); CAB(412V30, 55, 50, 50);
        MOD(CHORUS, 20, 35, 25); REV(PLATE, 40, 45, 15); WIDTH(75); break;
    case 46: mx_name(m, "D-BEAT", SY_CRUST); GATE(40, 20); TUNE(-2);       /* D, crust punk's metal: a dirty Swedish buzz */
        DRV(HM2, 55, 70, 70, 50); AMP(JCM900, 30, 50, 55, 55, 55); CAB(412V30, 50, 55, 50); WIDTH(50); break;
    case 47: mx_name(m, "EPIC CRYPT", SY_EPIC); GATE(30, 40); TUNE(-1);    /* Eb, epic doom: slow, grand, a choir of reverb */
        DRV(TS9, 20, 50, 65, 0); AMP(JCM900, 62, 58, 58, 50, 50); CAB(412GB, 58, 45, 50);
        REV(CATHEDRAL, 60, 40, 22); WIDTH(65); break;
    case 48: mx_name(m, "FADING SKY", SY_MCORE); GATE(55, 12); TUNE(-4);   /* C, melodic metalcore: tight, then wide open */
        DRV(PRECISION, 0, 55, 80, 0); AMP(6505, 50, 52, 52, 58, 55); CAB(412V30, 50, 55, 50);
        DLY(DUAL, 360, 25, 12, 50); REV(PLATE, 40, 50, 12); WIDTH(85); break;
    case 49: mx_name(m, "THALL ABYSS", SY_THALL); GATE(70, 5); TUNE(-10);  /* F#, the eight-string's low string and space */
        DRV(PRECISION, 0, 60, 90, 0); AMP(YERASOV, 48, 45, 50, 55, 50); CAB(412V30, 45, 55, 50);
        GEQ(40, 50, 48, 55, 50); REV(AMBIENT, 60, 40, 18); WIDTH(85); break;
    case 50: mx_name(m, "SMOKE RITUAL", SY_STONER); GATE(25, 50); TUNE(-4); /* C, stoner doom: a fuzz into an orange wall */
        DRV(OS2, 70, 45, 65, 0); AMP(ROCKER, 62, 65, 65, 45, 45); CAB(412GB, 65, 40, 45);
        REV(SPRING, 40, 40, 15); WIDTH(55); break;
    case 51: mx_name(m, "BAYOU BURN", SY_SLUDGE); GATE(40, 25); TUNE(-2);  /* drop-D Southern sludge: thick and swinging */
        DRV(RAT, 45, 50, 60, 0); AMP(JCM900, 55, 62, 55, 50, 50); CAB(412GB, 60, 45, 50); WIDTH(60); break;
    case 52: mx_name(m, "BOUNCE", SY_NU); GATE(60, 10); TUNE(-7);          /* A, the seven-string's bounce, mids gone */
        AMP(RECTO, 52, 62, 30, 58, 55); CAB(412V30, 55, 50, 50);
        GEQ(60, 50, 35, 55, 55); WIDTH(85); break;
    case 53: mx_name(m, "EUROPOWER", SY_POWER); GATE(40, 20);              /* European power metal: fast, bright, epic */
        DRV(TS9, 0, 60, 75, 0); AMP(SAVAGE, 52, 50, 55, 58, 55); CAB(412V30, 50, 55, 50);
        DLY(DIGITAL, 300, 20, 12, 55); REV(HALL, 50, 55, 15); WIDTH(70); break;
    case 54: mx_name(m, "LABYRINTH", SY_PROG); GATE(45, 20); TUNE(-1);     /* Eb, prog: a Mark's clarity on every odd meter */
        AMP(MARK4, 52, 50, 50, 58, 55); CAB(412V30, 50, 55, 50);
        DLY(DUAL, 420, 25, 15, 50); REV(PLATE, 40, 50, 12); WIDTH(65); break;
    case 55: mx_name(m, "ORTHODOX", SY_BLACK); GATE(40, 25); TUNE(-1);     /* Eb, dissonant black: dense and dark */
        AMP(RECTO, 55, 45, 50, 55, 55); CAB(412V30, 50, 50, 50);
        GEQ(45, 50, 52, 55, 50); REV(CATHEDRAL, 55, 35, 20); WIDTH(70); break;
    case 56: mx_name(m, "CYBER PULSE", SY_INDUS); GATE(70, 5); TUNE(-5);   /* B, industrial: a machine gate, a slow flanger */
        DRV(PRECISION, 0, 55, 80, 0); AMP(6505, 52, 52, 50, 55, 55); CAB(412V30, 50, 50, 50);
        MOD(FLANGER, 15, 35, 25); WIDTH(85); break;
    case 57: mx_name(m, "OCTAVE CRUSH", SY_DOOM); GATE(30, 45);            /* an octave below the riff, fuzz, a long hall */
        B(PITCH, 1, PX_PIT_OCTAVER, 70, 55, 0, 0, 40);
        DRV(OS2, 55, 45, 60, 0); AMP(GH100, 55, 60, 55, 48, 45); CAB(412GB, 60, 45, 50);
        REV(HALL, 50, 40, 18); WIDTH(55); break;
    case 58: mx_name(m, "CRYSTAL CLEAN", SY_CLEAN); GATE(10, 50);          /* the ballad's clean: chorus, delay, a hall */
        AMP(TWIN, 40, 50, 55, 58, 50); CAB(212BLUE, 50, 55, 50);
        MOD(CHORUS, 25, 45, 40); DLY(DIGITAL, 400, 30, 20, 50); REV(HALL, 55, 50, 25); WIDTH(75); break;
    case 59: mx_name(m, "WHAMMY SCREAM", SY_LEAD); GATE(35, 30);           /* the solo an octave up, as a pedal screams it */
        B(PITCH, 1, PX_PIT_WHAMMY, 75, 70, 10, 0, 0);
        DRV(TS9, 25, 55, 70, 0); AMP(5150, 60, 50, 60, 55, 55); CAB(412V30, 50, 55, 50);
        DLY(DIGITAL, 380, 25, 15, 50); REV(HALL, 45, 50, 15); WIDTH(50); break;
    }
    if (MX_LVL[idx]) p->p[PX_B_GATE][PX_PATCH_LEVEL] = MX_LVL[idx];
}
#undef B
#undef GATE
#undef NOGATE
#undef TUNE
#undef AMP
#undef CAB
#undef DRV
#undef GEQ
#undef DLY
#undef REV
#undef REV4
#undef MOD
#undef WIDTH
