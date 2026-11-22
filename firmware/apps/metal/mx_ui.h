/* SPDX-License-Identifier: GPL-3.0-only
 * POCKET METAL's screens (app.h includes this file): black iron and blood, a blackletter for the names, a Fraktur for the
 * titles, a condensed sans for what must be read at a glance (mx_fonts.h). Only what changed is drawn again: each part
 * of a page keeps a hash of what it shows.
 *   PLAY    the style, the load; the sound's number, its tuning and amp; its name; the chain (a tile a block, lit when
 *           on); a status line (EDITED, a word for a moment, the tempo); what the controls do
 *   BLOCK   its title and ON / OFF; its type; a line a setting, with a bar; the marked line has a thorn
 *   TUNER   the note in Fraktur, a scale of +-50 cents with a spike, IN TUNE
 */
#pragma once

#define MX_RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
enum {
    C_BG = MX_RGB(10, 6, 6),          /* near black, warm */
    C_IRON = MX_RGB(34, 24, 24),      /* a tile that is off, a bar's track */
    C_IRON2 = MX_RGB(58, 40, 40),     /* its edge */
    C_BLOOD = MX_RGB(150, 12, 20),    /* a block that is on, the bars */
    C_RED = MX_RGB(228, 30, 38),      /* what is marked, the number */
    C_EMBER = MX_RGB(96, 22, 24),     /* the dark end of the bands */
    C_BONE = MX_RGB(232, 220, 200),   /* the text */
    C_ASH = MX_RGB(140, 128, 120),    /* text that matters less */
    C_STEEL = MX_RGB(190, 192, 198),  /* the tuner's scale */
};

/* a band whose colour goes from c0 (top) to c1 (bottom), a line at a time */
static uint16_t mx_mix(uint16_t a, uint16_t b, uint32_t t, uint32_t n)   /* a -> b, t of n */
{
    const uint32_t ra = a >> 11, ga = (a >> 5) & 63u, ba = a & 31u, rb = b >> 11, gb = (b >> 5) & 63u, bb = b & 31u;
    return (uint16_t)(((ra * (n - t) + rb * t) / n) << 11 | ((ga * (n - t) + gb * t) / n) << 5 | ((ba * (n - t) + bb * t) / n));
}
static void mx_band(uint32_t y, uint32_t h, uint16_t c0, uint16_t c1)
{
    for (uint32_t i = 0; i < h; i++) lcd_rect(0, y + i, 240, 1, mx_mix(c0, c1, i, h));
}
/* a rule with thorns: a 2-pixel line that tapers to points at both ends, small spikes standing on it every 24 pixels */
static void mx_rule(uint32_t y, uint16_t col)
{
    lcd_rect(14, y, 212, 2, col);
    for (uint32_t k = 1; k <= 10; k++) { lcd_rect(14u - k, y + (k & 1u), 1, 1, col); lcd_rect(225u + k, y + (k & 1u), 1, 1, col); }
    for (uint32_t x = 24; x < 220; x += 24) {
        lcd_rect(x, y - 1u, 3, 1, col); lcd_rect(x + 1u, y - 3u, 1, 2, col);
    }
}
/* the skull (an 11 x 12 drawing), drawn s times larger */
static void mx_skull(uint32_t x, uint32_t y, uint32_t s, uint16_t col)
{
    static const uint16_t S[12] = {0x0F8, 0x3FE, 0x7FF, 0x7FF, 0x673, 0x673, 0x7FF, 0x3DE, 0x1FC, 0x154, 0x1FC, 0x0A8};
    for (uint32_t r = 0; r < 12u; r++)
        for (uint32_t c = 0; c < 11u; c++)
            if (S[r] >> (10u - c) & 1u) lcd_rect(x + c * s, y + r * s, s, s, col);
}
/* a box of text: the font's height, cleared to bg, text left (0), centred (1) or right (2) */
static void mx_text(uint32_t x, uint32_t y, uint32_t w, const char *s, const op_font *f, uint16_t fg, uint16_t bg, int al)
{
    ft_box(x, y, w, s, f, fg, bg, al);
}
/* text over a band (mx_band(by, bh, c0, c1)): each line of the text on the band's colour at that line, as ft_box */
static void mx_btext(uint32_t x, uint32_t y, uint32_t w, const char *s, const op_font *f, uint16_t fg,
                     uint32_t by, uint32_t bh, uint16_t c0, uint16_t c1, int al)
{
    uint8_t gi[40]; uint32_t n = 0, tw = 0;
    if (x >= 240u || w == 0u) return;
    if (x + w > 240u) w = 240u - x;
    while (s[n] && n < 40u) {
        const int g = ft_glyph(f, s[n]);
        if (tw + f->adv[g] > w) break;
        gi[n] = (uint8_t)g; tw += f->adv[g]; n++;
    }
    const uint32_t lpad = al == 1 ? (w - tw) / 2u : (al == 2 ? w - tw : 0u), rpad = w - tw - lpad;
    lcd_win(x, y, w, f->h);
    for (uint32_t r = 0; r < f->h; r++) {
        const uint32_t yy = y + r;
        const uint16_t bg = yy >= by && yy < by + bh ? mx_mix(c0, c1, yy - by, bh) : C_BG;
        uint16_t pal[16];
        for (uint32_t a = 0; a < 16u; a++) pal[a] = ft_mix(fg, bg, a);
        for (uint32_t i = 0; i < lpad; i++) lcd_px(bg);
        for (uint32_t k = 0; k < n; k++) {
            const uint32_t a = f->adv[gi[k]];
            const uint8_t *b = f->bits + f->off[gi[k]];
            uint32_t q = r * a;
            for (uint32_t c = 0; c < a; c++, q++) lcd_px(pal[(b[q >> 1] >> ((q & 1u) ? 0 : 4)) & 15u]);
        }
        for (uint32_t i = 0; i < rpad; i++) lcd_px(bg);
        if ((r & 7u) == 7u) fm1_wdt_feed();
    }
    lcd_end();
}
static uint32_t mx_hash(const void *p, uint32_t n, uint32_t h)
{
    const uint8_t *b = (const uint8_t *)p;
    for (uint32_t i = 0; i < n; i++) h = (h ^ b[i]) * 16777619u;
    return h;
}
static uint32_t mx_hs(const char *s, uint32_t h) { while (*s) h = (h ^ (uint8_t)*s++) * 16777619u; return h; }

/* ---------------------------------------------------------------- splash --- */
static void mx_splash(void)
{
    lcd_rect(0, 0, 240, 240, C_BG);
    mx_band(0, 70, C_EMBER, C_BG);
    mx_skull(98, 34, 4, C_BLOOD);
    mx_text(0, 96, 240, "Pocket", &MF_FRAK, C_RED, C_BG, 1);
    mx_text(0, 128, 240, "Metal", &MF_FRAK, C_RED, C_BG, 1);
    mx_rule(170, C_BLOOD);
    mx_text(0, 182, 240, "NO NAM. NO MERCY. " OP_VERSION, &MF_LBL, C_ASH, C_BG, 1);
    for (int i = 0; i < 12; i++) { fm1_wdt_feed(); fm1_delay_ms(100); }
    ui.full = 1;
}

/* --------------------------------------------------------------- PLAY ---- */
static uint32_t mx_seen[8];
static void mx_play(uint32_t now)
{
    char l[48];
    uint32_t h;
    const int br = ui.browse >= 0;
    const mx_preset_t *s = br ? mx_ref((uint32_t)ui.browse) : &mx_now;
    const uint32_t slot = br ? (uint32_t)ui.browse : mx_slot;
    const px_preset_t *p = &s->p;
    if (ui.full) {
        lcd_rect(0, 0, 240, 240, C_BG);
        mx_band(0, 24, C_EMBER, C_BG);
        mx_rule(25, C_BLOOD);
        mx_rule(124, C_EMBER);
        mx_text(0, 222, 240, "KNOB: BROWSE   FX: EFFECTS   HOME: SAVE", &MF_LBL, C_ASH, C_BG, 1);
        for (int i = 0; i < 8; i++) mx_seen[i] = ~0u;
    }
    /* the style, the load (twice a second) */
    h = mx_hs(MX_STYLE[s->style < SY_N ? s->style : 0], (now / 500u) * 2654435761u);
    if (h != mx_seen[0]) {
        mx_seen[0] = h;
        mx_btext(6, 4, 150, MX_STYLE[s->style < SY_N ? s->style : 0], &MF_LBL, C_RED, 0, 24, C_EMBER, C_BG, 0);
        l[0] = 0;
        if (base_safe) b_str(l, "SAFE MODE");
        else if (a_bypass) b_str(l, "OVERLOAD");
        else { b_str(l, "DSP "); b_num(l, a_load); b_str(l, "%"); }
        mx_btext(156, 4, 78, l, &MF_LBL, a_bypass || base_safe ? C_RED : C_ASH, 0, 24, C_EMBER, C_BG, 2);
    }
    /* the number, the tuning, the amp */
    h = mx_hash(p, sizeof *p, slot * 977u + (uint32_t)br + (uint32_t)s->width * 7919u);
    if (h != mx_seen[1]) {
        mx_seen[1] = h;
        l[0] = 0;
        if (slot + 1u < 10u) b_str(l, "0");
        b_num(l, slot + 1u);
        ft_box(4, 32, 84, l, &MF_NUM, br ? C_BONE : C_RED, C_BG, 1);
        const int pit = p->on[PX_B_PITCH] ? p->type[PX_B_PITCH] : -1;   /* the tuning, or TUNE's other effect (E under it) */
        const int down = pit == PX_PIT_DOWN ? px_down_steps(p->p[PX_B_PITCH][0]) : 0;
        ft_box(92, 34, 146, pit >= 0 && pit != PX_PIT_DOWN ? px_type_name(PX_B_PITCH, pit) : mx_tuning_name(down), &MF_VAL,
               down || (pit >= 0 && pit != PX_PIT_DOWN) ? C_RED : C_BONE, C_BG, 0);
        l[0] = 0;
        b_str(l, p->on[PX_B_AMP] ? px_type_name(PX_B_AMP, p->type[PX_B_AMP]) : "NO AMP");
        if (p->on[PX_B_DRIVE]) { b_str(l, " + "); b_str(l, px_type_name(PX_B_DRIVE, p->type[PX_B_DRIVE])); }
        ft_box(92, 58, 146, l, &MF_LBL, C_ASH, C_BG, 0);
        l[0] = 0;
        b_str(l, p->on[PX_B_CAB] ? px_type_name(PX_B_CAB, p->type[PX_B_CAB]) : "NO CABINET");
        b_str(l, "  WIDTH "); b_num(l, s->width);
        ft_box(92, 74, 146, l, &MF_LBL, C_ASH, C_BG, 0);
    }
    /* the name */
    h = mx_hs(p->name, (uint32_t)br * 31u);
    if (h != mx_seen[2]) { mx_seen[2] = h; ft_box(0, 86, 240, p->name, &MF_NAME, br ? C_RED : C_BONE, C_BG, 1); }
    /* the chain: a tile a block */
    h = 0;
    for (int b = 0; b < MB_N; b++) h = h * 3u + p->on[MB_PX[b]];
    if (h != mx_seen[3]) {
        mx_seen[3] = h;
        for (int b = 0; b < MB_N; b++) {
            const uint32_t x = 4u + (uint32_t)b * 26u, on = p->on[MB_PX[b]];
            lcd_rect(x, 132, 24, 30, on ? C_BLOOD : C_IRON);
            lcd_rect(x, 132, 24, 1, on ? C_RED : C_IRON2);
            ft_box(x + 1u, 140, 22, MB_TAG[b], &MF_LBL, on ? C_BONE : C_ASH, on ? C_BLOOD : C_IRON, 1);
        }
    }
    /* the status line */
    l[0] = 0;
    if (ui.note_ms && now - ui.t_note < ui.note_ms) b_str(l, ui.note);
    else if (br) b_str(l, "PUSH: LOAD THIS SOUND");
    else {
        if (mx_dirty()) b_str(l, "EDITED   ");
        b_str(l, "VOL "); b_num(l, mx_vol);
        if (mx_bpm) { b_str(l, "   "); b_num(l, mx_bpm); b_str(l, " BPM"); }
    }
    h = mx_hs(l, 7u);
    if (h != mx_seen[4]) { mx_seen[4] = h; ft_box(0, 176, 240, l, &MF_VAL, ui.note_ms && now - ui.t_note < ui.note_ms ? C_RED : C_BONE, C_BG, 1); }
}

/* -------------------------------------------------------------- BLOCK ---- */
static void mx_block(void)
{
    char l[24];
    const int b = ui.blk, px = MB_PX[b], t = mx_now.p.type[px], np = px_nparams(px, t), rows = mb_nrows(b);
    uint32_t h;
    if (ui.full) {
        lcd_rect(0, 0, 240, 240, C_BG);
        mx_band(0, 36, C_EMBER, C_BG);
        mx_rule(37, C_BLOOD);
        for (int i = 0; i < 8; i++) mx_seen[i] = ~0u;
        mx_btext(8, 3, 166, MB_TITLE[b], &MF_FRAK, C_BONE, 0, 36, C_EMBER, C_BG, 0);
    }
    const int on = mx_now.p.on[px];
    if ((uint32_t)on != mx_seen[0]) {
        mx_seen[0] = (uint32_t)on;
        lcd_rect(178, 9, 54, 22, on ? C_BLOOD : C_IRON);
        ft_box(180, 12, 50, on ? "ON" : "OFF", &MF_VAL, on ? C_BONE : C_ASH, on ? C_BLOOD : C_IRON, 1);
    }
    /* the type */
    h = (uint32_t)t * 7u + (ui.row == 0);
    if (h != mx_seen[1]) {
        mx_seen[1] = h;
        const uint16_t bg = ui.row == 0 ? C_IRON : C_BG;
        lcd_rect(4, 44, 232, 30, bg);
        if (ui.row == 0) lcd_rect(4, 44, 3, 30, C_RED);
        ft_box(12, 46, 216, px_type_name(px, t), &MF_HEAD, ui.row == 0 ? C_RED : C_BONE, bg, 1);
    }
    /* the settings: a line each, 26 pixels */
    for (int r = 1; r < 7; r++) {
        const int k = r - 1;
        uint32_t y = 52u + (uint32_t)r * 26u;
        if (r >= rows) {
            if (mx_seen[1 + r] != 0u) { mx_seen[1 + r] = 0u; lcd_rect(0, y, 240, 26, C_BG); }
            continue;
        }
        const int width = k >= np;                     /* the cabinet's last line: WIDTH */
        const int v = width ? mx_now.width : mx_now.p.p[px][k];
        const int sel = ui.row == r;
        h = 1u + (uint32_t)v * 4u + (uint32_t)sel * 1000u + (uint32_t)t * 3000u;
        if (h == mx_seen[1 + r]) continue;
        mx_seen[1 + r] = h;
        const char *name = width ? "WIDTH" : (px == PX_B_GATE && k == 2) ? "LEVEL" : px_param_name(px, t, k);
        lcd_rect(0, y, 240, 26, C_BG);
        if (sel) { lcd_rect(4, y + 8u, 5, 9, C_RED); lcd_rect(9, y + 10u, 3, 5, C_RED); lcd_rect(12, y + 12u, 2, 1, C_RED); }
        ft_box(16, y + 5u, 66, name, &MF_LBL, sel ? C_BONE : C_ASH, C_BG, 0);
        lcd_rect(84, y + 10u, 84, 6, C_IRON);
        if (v) lcd_rect(84, y + 10u, (uint32_t)v * 84u / 100u, 6, sel ? C_RED : C_BLOOD);
        l[0] = 0;
        if (!width && px == PX_B_PITCH && t == PX_PIT_DOWN && k == 0) {
            const char *tn = mx_tuning_name(px_down_steps(v));
            int i = 0; while (tn[i] && tn[i] != ' ' && i < 3) { l[i] = tn[i]; i++; } l[i] = 0;
        } else if (width || !px_param_text(px, t, k, v, l)) { l[0] = 0; b_num(l, (uint32_t)v); }
        ft_box(170, y + 2u, 66, l, &MF_VAL, sel ? C_RED : C_BONE, C_BG, 2);
    }
}

/* -------------------------------------------------------------- TUNER ---- */
static void mx_tuner_draw(uint32_t now)
{
    static uint32_t t_est;
    static float hz;
    static const char *const NOTE[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    if (ui.full) {
        lcd_rect(0, 0, 240, 240, C_BG);
        mx_band(0, 36, C_EMBER, C_BG);
        mx_rule(37, C_BLOOD);
        mx_btext(8, 3, 160, "Tuner", &MF_FRAK, C_BONE, 0, 36, C_EMBER, C_BG, 0);
        ft_box(0, 222, 240, "GUITAR IN E: THE TUNE BLOCK LOWERS IT", &MF_LBL, C_ASH, C_BG, 1);
        for (int i = 0; i < 8; i++) mx_seen[i] = ~0u;
    }
    if (now - t_est < 90u && !ui.full) return;
    t_est = now;
    hz = tuner_estimate();
    int note = -1, cents = 0;
    if (hz > 20.0f) {
        const float m = 17.3123405f * logf(hz / 440.0f) + 69.0f;       /* MIDI note, fractional */
        const int n = (int)(m + 0.5f);
        cents = (int)((m - (float)n) * 100.0f + (m >= (float)n ? 0.5f : -0.5f));
        note = ((n % 12) + 12) % 12;
    }
    const int in = note >= 0 && cents >= -3 && cents <= 3;
    uint32_t h = (uint32_t)(note + 1) * 1000u + (uint32_t)(cents + 60) * 2u + (uint32_t)in;
    if (h == mx_seen[0]) return;
    mx_seen[0] = h;
    ft_box(30, 56, 180, note >= 0 ? NOTE[note] : "-", &MF_NOTE, in ? C_RED : C_BONE, C_BG, 1);
    lcd_rect(20, 150, 200, 30, C_BG);
    lcd_rect(20, 164, 200, 2, C_STEEL);
    for (int c = -50; c <= 50; c += 10) lcd_rect((uint32_t)(120 + c * 2), c ? 160u : 154u, 1, c ? 10u : 22u, C_STEEL);
    if (note >= 0) {
        const uint32_t x = (uint32_t)(120 + (cents < -50 ? -50 : cents > 50 ? 50 : cents) * 2);
        for (uint32_t i = 0; i < 8u; i++) lcd_rect(x - (7u - i) / 2u, 152u + i * 2u, 7u - i + 1u, 2, in ? C_RED : C_BLOOD);
    }
    char l[24];
    l[0] = 0;
    if (note >= 0) {
        if (in) b_str(l, "IN TUNE");
        else { if (cents > 0) b_str(l, "+"); else b_str(l, "-"); b_num(l, (uint32_t)(cents < 0 ? -cents : cents)); b_str(l, " CENTS"); }
    }
    ft_box(0, 188, 240, l, &MF_VAL, in ? C_RED : C_ASH, C_BG, 1);
}

static void mx_draw(uint32_t now)
{
    static uint8_t page = 0xFF;
    if (page != ui.page) { page = ui.page; ui.full = 1; }
    if (ui.page == PG_TUNER) mx_tuner_draw(now);
    else if (ui.page == PG_BLOCK) mx_block();
    else mx_play(now);
    ui.full = 0;
}
