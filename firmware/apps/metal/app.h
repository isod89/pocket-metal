/* SPDX-License-Identifier: GPL-3.0-only
 * POCKET METAL, the app itself (docs/METAL.md): its sounds, its screens, and what the controls and MIDI do. The pedal's
 * file (metal.c) and the simulator (tests/host, `node tools/op.mjs sim --app metal`) both include it, after the base
 * firmware's parts it uses: nothing here touches the chip.
 *
 * The controls (app_turn, app_button, app_master: base/input.h)
 *   footswitches        the sound before / after, at once. Both together: the tuner (the sound stays the one you had)
 *   PLAY page, knob     browse the sounds (nothing changes yet); PUSH loads the one shown
 *   FX                  the pages GATE, TUNE (down-tuning, octaves, harmony), BOOST, EQ in turn; held: the tuner
 *   MOD AMP CAB DLY REV that block's page; pressed again on it: the block on or off
 *   on a block page     the knob changes the line marked; PUSH marks the next line (the type first, then the settings)
 *   HOME                back to the PLAY page; held 0.8 s: SAVE the sound in its slot
 *   PUSH held 1.2 s     on the PLAY page: the slot back to its factory sound (PUSH again to confirm)
 *   MASTER              the volume
 * MIDI (app_midi, base/midi.h): Program Change 0..59 a sound; Control Change 80..88 a block off / on (GATE, TUNE, BOOST,
 *   AMP, CAB, EQ, MOD, DELAY, REVERB: below 64 off), 89 the tuner; the clock's tempo is shown on the PLAY page.
 * Its own editor requests (app_editor, base/product.h), protocol 1, beside INFO:
 *   5 STATE   -> slot, flags (bit 0 edited, bit 1 the slot holds a saved sound), the sound (mx_preset_t, 112 bytes)
 *   6 SET b k v -> block b's setting k = v (k 5: on, k 6: the type, b 12 k 0: WIDTH): the new value back
 */
#pragma once
#include "mx_engine.h"
#include "mx_presets.h"
#include "mx_fonts.h"

/* ---------------------------------------------------------------- data --- */
#define MX_NSLOT MX_NFACT
#define MX_MAGIC 0x4C544D50u                    /* "PMTL": this app's data in flash (base/store.h) */
#define MX_VER   2u                             /* 2: sixty slots. 1 (thirty) is read and moved into place */
#define MX_NSLOT1 30u
typedef struct { uint8_t last, saved[MX_NSLOT], pad; mx_preset_t slot[MX_NSLOT]; } mx_store_t;
typedef char mx_size_check[sizeof(mx_preset_t) == 112 && sizeof(mx_store_t) <= BS_MAX ? 1 : -1];

/* the blocks the app shows, in the order of the chain, and the engine's block of each */
enum { MB_GATE, MB_TUNE, MB_BOOST, MB_AMP, MB_CAB, MB_EQ, MB_MOD, MB_DLY, MB_REV, MB_N };
static const uint8_t MB_PX[MB_N] = {PX_B_GATE, PX_B_PITCH, PX_B_DRIVE, PX_B_AMP, PX_B_CAB, PX_B_EQ, PX_B_MOD, PX_B_DELAY,
                                    PX_B_REVERB};
static const char *const MB_TAG[MB_N] = {"GT", "TN", "BST", "AMP", "CAB", "EQ", "MOD", "DLY", "REV"};
static const char *const MB_TITLE[MB_N] = {"Gate", "Tuning", "Boost", "Amplifier", "Cabinet", "Equalizer", "Modulation",
                                           "Delay", "Reverb"};
/* the types each page offers, in the order the knob goes through them */
static const uint8_t TY_GATE[] = {0};
static const uint8_t TY_TUNE[] = {PX_PIT_DOWN, PX_PIT_OCTAVER, PX_PIT_HARMONY, PX_PIT_WHAMMY, PX_PIT_DETUNE};
static const uint8_t TY_BOOST[] = {PX_DRV_TS9, PX_DRV_PRECISION, PX_DRV_M77, PX_DRV_KLON, PX_DRV_DS1, PX_DRV_RAT, PX_DRV_OS2,
                                   PX_DRV_HM2, PX_DRV_UBER};
static const uint8_t TY_AMP[] = {PX_AMP_RECTO, PX_AMP_6505, PX_AMP_IIIRED, PX_AMP_5150, PX_AMP_XXX, PX_AMP_YERASOV,
                                 PX_AMP_SAVAGE, PX_AMP_MARK4, PX_AMP_JVM, PX_AMP_JCM2000, PX_AMP_JCM900, PX_AMP_PROMOD,
                                 PX_AMP_GH100, PX_AMP_ROCKER, PX_AMP_MIG50, PX_AMP_SHIRLEY, PX_AMP_AC15, PX_AMP_TWIN};
static const uint8_t TY_CAB[] = {PX_CAB_412V30, PX_CAB_412GB, PX_CAB_412T75, PX_CAB_212BLUE, PX_CAB_112JN, PX_CAB_112CREAM,
                                 PX_CAB_DI};
static const uint8_t TY_EQ[] = {PX_EQ_GEQ, PX_EQ_5BAND, PX_EQ_PARAM};
static const uint8_t TY_MOD[] = {PX_MOD_CHORUS, PX_MOD_FLANGER, PX_MOD_PHASE90, PX_MOD_DIMENSION, PX_MOD_STONE,
                                 PX_MOD_UNIVIBE, PX_MOD_OPTOTREM, PX_MOD_VIBRATO};
static const uint8_t TY_DLY[] = {PX_DLY_DIGITAL, PX_DLY_ANALOG, PX_DLY_TAPE, PX_DLY_DUAL, PX_DLY_MEMORY, PX_DLY_SPACE,
                                 PX_DLY_REVERSE, PX_DLY_LOFI};
static const uint8_t TY_REV[] = {PX_REV_ROOM, PX_REV_HALL, PX_REV_CATHEDRAL, PX_REV_PLATE, PX_REV_SPRING, PX_REV_SHIMMER,
                                 PX_REV_AMBIENT, PX_REV_GATED};
static const struct { const uint8_t *t; uint8_t n; } MB_TY[MB_N] = {
    {TY_GATE, sizeof TY_GATE}, {TY_TUNE, sizeof TY_TUNE}, {TY_BOOST, sizeof TY_BOOST}, {TY_AMP, sizeof TY_AMP},
    {TY_CAB, sizeof TY_CAB}, {TY_EQ, sizeof TY_EQ}, {TY_MOD, sizeof TY_MOD}, {TY_DLY, sizeof TY_DLY}, {TY_REV, sizeof TY_REV}};
/* DOWN TUNE's TUNING, as a guitarist says it: the low string's note */
static const char *mx_tuning_name(int st)
{
    static const char *const N[13] = {"E STANDARD", "Eb STANDARD", "D STANDARD", "C# STANDARD", "C STANDARD",
                                      "B STANDARD", "Bb STANDARD", "A STANDARD", "Ab STANDARD", "G STANDARD",
                                      "F# STANDARD", "F STANDARD", "E LOW OCTAVE"};
    if (st > 0) st = 0;
    if (st < -12) st = -12;
    return N[-st];
}

/* ---------------------------------------------------------------- state --- */
static px_engine mx_eng OP_POOL __attribute__((aligned(8)));   /* the engine (162 KiB), in the POOL region (memory.ld) */
static mx_stereo_t mx_st OP_POOL;
static float mx_mono[A_FR];
static mx_store_t mx_sto;
static mx_preset_t mx_now;                      /* the sound as it is now (edited or not) */
static px_preset_t mx_kp;                       /* settings handed to the interrupt (px_engine_knobs) */
static volatile uint8_t mx_kpend, mx_kw;
static uint32_t mx_slot, mx_vol = 80u, mx_bpm;
static uint8_t mx_dirty_apply = 1;              /* mx_now changed since the engine last took it */
static uint32_t mx_t_edit;
static midi_tempo_t mx_clk;
enum { PG_PLAY, PG_BLOCK, PG_TUNER };
static struct {
    uint8_t page, blk, row, fx;                 /* the page; on a block page: which, its marked line; FX's last page */
    int8_t browse;                              /* PLAY: the sound shown while browsing (-1: none) */
    uint32_t t_browse, note_ms, t_note, restore_until, fs_t[2];
    uint8_t fs_from[2];                         /* each footswitch's press: the slot before it */
    const char *note;                           /* a word for a moment in the status line */
    uint8_t full;                               /* the whole screen must be drawn again */
} ui = {PG_PLAY, MB_AMP, 0, MB_GATE, -1, 0, 0, 0, 0, {0, 0}, {0, 0}, 0, 1};

/* ---------------------------------------------------------------- sound --- */
/* Two cores (base/audio_dual.h), as OpenPocket runs the same engine: the chain on CPU1 for a whole block, the delay and
 * the reverb beside it on CPU0 (the engine's wet lanes, px_wet_*: they make the sends of two blocks before, and their
 * taps make up for it). On one core (CPU1 retired, or the simulator with H_CPU1_OFF) they run in the chain again. */
static uint8_t mx_cores = 1;
static uint32_t mx_ticks(void) { return fm1_ticks(); }   /* the engine's clock (px_clock): a function of our own to point at */
static void mx_lanes(void) { px_wet_mode(&mx_eng, mx_cores == 2, 2 * (int)A_FR); }
static void app_sound(const float *in, float *out)   /* on CPU1 (or in the I2S interrupt on one core) */
{
    if (mx_kpend) { px_engine_knobs(&mx_eng, &mx_kp); mx_stereo_set(&mx_st, mx_kw); mx_kpend = 0; }
    for (uint32_t o = 0; o < A_FR; o += PX_MAX_BLOCK) px_engine_process(&mx_eng, in + o, mx_mono + o, PX_MAX_BLOCK);
    mx_stereo_run(&mx_st, mx_mono, out, out + A_FR, (int)A_FR);
}
static void app_sound_reset(void)                /* interrupt held, CPU1 idle: the whole sound again, from silence */
{
    px_engine_set(&mx_eng, &mx_now.p);
    mx_lanes();                                  /* (px_engine_set cleared them) */
    mx_stereo_set(&mx_st, mx_now.width);
    mx_stereo_reset(&mx_st);
    mx_kpend = 0;
}
static void app_cores(int n) { mx_cores = (uint8_t)n; mx_lanes(); }
static void app_beside(uint32_t deadline) { if (mx_eng.wl_on) px_wet_run(&mx_eng, deadline, 0); }   /* (CPU0) */
static void app_boundary(void) { if (mx_eng.wl_on) px_wet_swap(&mx_eng); }
/* the engine follows mx_now: a setting glides (px_engine_knobs in the interrupt, the heavy part made here first); another
 * type or another sound's blocks are a change in silence (a_change) */
static void mx_apply(uint32_t now)
{
    if (!mx_dirty_apply || mx_kpend) return;
    if (now - mx_t_edit < 15u) return;          /* (a knob turning fast: one change at a time) */
    mx_dirty_apply = 0;
    const int ch = px_engine_change(&mx_eng, &mx_now.p);
    if (ch == PX_CH_FULL) { a_change(app_sound_reset); return; }
    if (ch == PX_CH_KNOBS) px_engine_prepare(&mx_eng, &mx_now.p);
    mx_kp = mx_now.p; mx_kw = mx_now.width;
    __asm__ volatile("" ::: "memory");          /* (the settings written before the flag: the other core reads them) */
    mx_kpend = 1;
}
static void mx_edited(void) { mx_dirty_apply = 1; mx_t_edit = ms_now(); }

static const mx_preset_t *mx_ref(uint32_t k)    /* a slot's sound: the saved one, else the factory one */
{
    static mx_preset_t f;
    if (mx_sto.saved[k]) return &mx_sto.slot[k];
    mx_factory(&f, (int)k);
    return &f;
}
static int mx_dirty(void) { return memcmp(mx_ref(mx_slot), &mx_now, sizeof mx_now) != 0; }
static void mx_load(uint32_t k)
{
    mx_slot = k % MX_NSLOT;
    mx_now = *mx_ref(mx_slot);
    mx_edited();
    ui.full = 1;
}
static void mx_say(const char *w, uint32_t ms) { ui.note = w; ui.note_ms = ms; ui.t_note = ms_now(); }
static void mx_save(void)
{
    mx_sto.slot[mx_slot] = mx_now; mx_sto.saved[mx_slot] = 1; mx_sto.last = (uint8_t)mx_slot;
    mx_say(bs_save(MX_MAGIC, MX_VER, &mx_sto, sizeof mx_sto) ? "SAVE FAILED" : "SAVED", 1500u);
    ui.full = 1;
}
static void mx_restore(void)
{
    mx_sto.saved[mx_slot] = 0;
    mx_say(bs_save(MX_MAGIC, MX_VER, &mx_sto, sizeof mx_sto) ? "RESTORE FAILED" : "FACTORY SOUND BACK", 1500u);
    mx_load(mx_slot);
}

/* a block's lines on its page: 0 the type, then its settings; the cabinet's last is WIDTH (the stereo stage) */
static int mb_nrows(int b)
{
    const int px = MB_PX[b];
    int n = 1 + px_nparams(px, mx_now.p.type[px]);
    if (b == MB_CAB) n++;
    return n;
}
static int mb_type_idx(int b)
{
    const int t = mx_now.p.type[MB_PX[b]];
    for (int i = 0; i < MB_TY[b].n; i++) if (MB_TY[b].t[i] == t) return i;
    return 0;
}
static void mb_set_type(int b, int dd)
{
    const int n = MB_TY[b].n, i = ((mb_type_idx(b) + dd) % n + n) % n, px = MB_PX[b];
    mx_now.p.type[px] = MB_TY[b].t[i];
    if (px == PX_B_PITCH && MB_TY[b].t[i] == PX_PIT_DOWN && mx_now.p.p[px][1] < 50) mx_now.p.p[px][1] = 100;
    ui.full = 1;
}
static void mb_toggle(int b) { uint8_t *on = &mx_now.p.on[MB_PX[b]]; *on = !*on; }

/* --------------------------------------------------------- the controls --- */
static const uint16_t MX_HOLD[IN_N] = { [IN_PUSH] = 1200, [IN_HOME] = 800, [IN_FX] = 800 };
static void mx_page(int page, int blk) { ui.page = (uint8_t)page; ui.blk = (uint8_t)blk; ui.row = 0; ui.full = 1; ui.browse = -1; }
static void mx_tuner(int on)
{
    a_tmute = (uint8_t)on;
    mx_page(on ? PG_TUNER : PG_PLAY, ui.blk);
}
static void app_turn(int dd, int fast)
{
    if (ui.page == PG_TUNER) return;
    if (ui.page == PG_PLAY) {                   /* browse: the sounds shown, not loaded */
        const int from = ui.browse < 0 ? (int)mx_slot : ui.browse;
        ui.browse = (int8_t)(((from + dd) % (int)MX_NSLOT + (int)MX_NSLOT) % (int)MX_NSLOT);
        ui.t_browse = ms_now();
        return;
    }
    const int b = ui.blk, px = MB_PX[b];
    if (ui.row == 0) { if (MB_TY[b].n > 1) { mb_set_type(b, dd); mx_edited(); } return; }
    const int np = px_nparams(px, mx_now.p.type[px]);
    uint8_t *v = ui.row <= np ? &mx_now.p.p[px][ui.row - 1] : &mx_now.width;
    int x = (int)*v + dd * (fast ? 4 : 1);
    *v = (uint8_t)(x < 0 ? 0 : x > 100 ? 100 : x);
    mx_edited();
}
static void app_button(uint32_t b, uint32_t ev, uint32_t held)
{
    const uint32_t now = ms_now();
    if (b == IN_FSUP || b == IN_FSDN) {
        if (ev != IN_DOWN) return;
        const uint32_t i = b == IN_FSUP ? 0u : 1u, other = b == IN_FSUP ? IN_FSDN : IN_FSUP;
        if ((held & (1u << other)) && now - ui.fs_t[1u - i] < 400u) {   /* both: the tuner, and the sound of before */
            if (ui.fs_from[1u - i] != mx_slot) mx_load(ui.fs_from[1u - i]);
            mx_tuner(ui.page != PG_TUNER);
            return;
        }
        ui.fs_t[i] = now; ui.fs_from[i] = (uint8_t)mx_slot;
        if (ui.page == PG_TUNER) { mx_tuner(0); return; }
        mx_load(b == IN_FSUP ? mx_slot + 1u : mx_slot + MX_NSLOT - 1u);
        if (ui.page != PG_PLAY) mx_page(PG_PLAY, ui.blk);
        return;
    }
    if (ui.page == PG_TUNER) { if (ev == IN_DOWN || ev == IN_CLICK) mx_tuner(0); return; }
    switch (b) {
    case IN_HOME:
        if (ev == IN_HOLD) mx_save();
        else if (ev == IN_CLICK) mx_page(PG_PLAY, ui.blk);
        break;
    case IN_PUSH:
        if (ev == IN_HOLD && ui.page == PG_PLAY && ui.browse < 0) { ui.restore_until = now + 4000u; mx_say("PUSH AGAIN: FACTORY SOUND", 4000u); }
        if (ev != IN_CLICK) break;
        if (ui.page == PG_PLAY) {
            if (ui.restore_until && (int32_t)(ui.restore_until - now) > 0) { ui.restore_until = 0; mx_restore(); break; }
            if (ui.browse >= 0) { const int k = ui.browse; ui.browse = -1; mx_load((uint32_t)k); }
            else mx_page(PG_BLOCK, MB_AMP);
        } else {
            ui.row = (uint8_t)((ui.row + 1) % mb_nrows(ui.blk));
            ui.full = 1;
        }
        break;
    case IN_FX:
        if (ev == IN_HOLD) { mx_tuner(1); break; }
        if (ev != IN_CLICK) break;
        {
            static const uint8_t CYC[] = {MB_GATE, MB_TUNE, MB_BOOST, MB_EQ};
            int k = 0;
            if (ui.page == PG_BLOCK) for (int i = 0; i < 4; i++) if (CYC[i] == ui.blk) k = (i + 1) % 4;
            if (ui.page != PG_BLOCK) for (int i = 0; i < 4; i++) if (CYC[i] == ui.fx) k = i;
            ui.fx = CYC[k];
            mx_page(PG_BLOCK, CYC[k]);
        }
        break;
    default: {
        static const uint8_t BTN[IN_N] = { [IN_MOD] = MB_MOD, [IN_AMP] = MB_AMP, [IN_CAB] = MB_CAB, [IN_DLY] = MB_DLY,
                                           [IN_REV] = MB_REV };
        if (ev != IN_DOWN || b >= IN_N || !BTN[b]) break;
        if (ui.page == PG_BLOCK && ui.blk == BTN[b]) { mb_toggle(BTN[b]); mx_edited(); }
        else mx_page(PG_BLOCK, BTN[b]);
    } break;
    }
}
static void app_master(uint32_t vol) { mx_vol = vol; audio_volume(vol); }

/* ---------------------------------------------------------------- MIDI --- */
static void app_midi(const midi_msg_t *m, uint32_t ms)
{
    switch (m->type) {
    case MIDI_PC: if (m->d1 < MX_NSLOT) { mx_load(m->d1); if (ui.page == PG_TUNER) mx_tuner(0); } break;
    case MIDI_CC:
        if (m->d1 >= 80u && m->d1 < 80u + MB_N) { mx_now.p.on[MB_PX[m->d1 - 80u]] = m->d2 >= 64u; mx_edited(); ui.full = 1; }
        else if (m->d1 == 89u) mx_tuner(m->d2 >= 64u);
        break;
    case MIDI_CLOCK: { const uint32_t q = midi_tempo(&mx_clk, ms); if (q) mx_bpm = (60000u + q / 2u) / q; break; }
    case MIDI_START: case MIDI_STOP: midi_tempo_reset(&mx_clk); mx_bpm = 0; break;
    }
}

/* ------------------------------------------------- its own editor requests --- */
static void app_editor(uint32_t cmd, const uint8_t *a, uint32_t n)
{
    if (cmd == 5u) {
        ed_begin(cmd);
        ed_b(mx_slot); ed_b((mx_dirty() ? 1u : 0u) | (mx_sto.saved[mx_slot] ? 2u : 0u));
        for (uint32_t i = 0; i < sizeof mx_now; i++) ed_b(((const uint8_t *)&mx_now)[i] & 0x7Fu);
        ed_send();
    } else if (cmd == 6u && n >= 3u) {
        uint8_t *v = 0;
        if (a[0] < PX_NBLK && a[1] < PX_NPAR && a[2] <= 100u) v = &mx_now.p.p[a[0]][a[1]];
        else if (a[0] < PX_NBLK && a[1] == 5u) { mx_now.p.on[a[0]] = a[2] ? 1u : 0u; v = &mx_now.p.on[a[0]]; }
        else if (a[0] == 12u && a[1] == 0u && a[2] <= 100u) v = &mx_now.width;
        if (v && a[1] < PX_NPAR) *v = a[2];
        if (v) { mx_edited(); ui.full = 1; }
        ed_begin(cmd); ed_b(a[0]); ed_b(a[1]); ed_b(v ? *v : 0x7Fu); ed_send();
    }
}

#include "mx_ui.h"                              /* the screens */

/* ------------------------------------------------- the start and the loop --- */
static void app_init(void)                      /* after base_begin(): the saved sounds, the slot to start on. Interrupts off */
{
    mx_store_t *t = &mx_sto;                    /* (read straight into place, checked, cleared when it does not hold) */
    uint32_t len = 0, ver = 0, k;
    int ok = bs_load(MX_MAGIC, t, sizeof *t, &len, &ver);
    if (ok && ver == 1u && len == 2u + MX_NSLOT1 * (1u + sizeof(mx_preset_t))) {   /* thirty slots: each keeps its number */
        const uint8_t *b = (const uint8_t *)t;
        volatile uint8_t *to = (volatile uint8_t *)t->slot;   /* (volatile: no memmove on the pedal for the compiler to call) */
        for (k = MX_NSLOT1 * sizeof(mx_preset_t); k--; ) to[k] = b[2u + MX_NSLOT1 + k];   /* (forward: from the end) */
        memset(&t->slot[MX_NSLOT1], 0, (MX_NSLOT - MX_NSLOT1) * sizeof(mx_preset_t));
        memset(&t->saved[MX_NSLOT1], 0, MX_NSLOT - MX_NSLOT1);
        t->pad = 0;
        ver = MX_VER; len = sizeof *t;
    }
    ok = ok && ver == MX_VER && len == sizeof *t && t->last < MX_NSLOT;
    for (k = 0; ok && k < MX_NSLOT; k++) {      /* what the flash held is checked like anything from outside */
        const px_preset_t *p = &t->slot[k].p;
        if (t->saved[k] > 1u) ok = 0;
        if (!ok || !t->saved[k]) continue;      /* (a slot never saved holds nothing to check) */
        if (t->slot[k].width > 100u || t->slot[k].style >= SY_N || p->name[13]) ok = 0;
        for (int b = 0; ok && b < PX_NBLK; b++) {
            if (p->on[b] > 1u || p->type[b] >= px_ntypes(b) || (b == PX_B_AMP && p->type[b] == PX_AMP_NAM)) ok = 0;
            for (int i = 0; i < PX_NPAR; i++) if (p->p[b][i] > 100u) ok = 0;
        }
        if (ok && !px_order_valid(p->order)) ok = 0;
    }
    if (!ok) memset(t, 0, sizeof *t);
    memset(&mx_eng, 0, sizeof mx_eng);          /* (the POOL is not zeroed at the start) */
    memset(&mx_st, 0, sizeof mx_st);
    px_engine_init(&mx_eng, 44100.0f);
    px_clock = mx_ticks;                        /* (the lanes stop by their deadline: base/audio_dual.h app_beside) */
    mx_slot = mx_sto.last;
    mx_now = *mx_ref(mx_slot);
    app_sound_reset();
    mx_dirty_apply = 0;
    in_start(MX_HOLD);
    mx_splash();
}
static void app_step(uint32_t now)              /* every main-loop pass, after base_loop(now) */
{
    in_poll(now);
    if (ui.browse >= 0 && (int32_t)(now - ui.t_browse) > 5000) ui.browse = -1;   /* (browsing, left alone: back to the sound) */
    if (ui.restore_until && (int32_t)(now - ui.restore_until) >= 0) ui.restore_until = 0;
    mx_apply(now);
    mx_draw(now);
}
