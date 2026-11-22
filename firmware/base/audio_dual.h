/* SPDX-License-Identifier: GPL-3.0-only
 * The base firmware (docs/BASE-FIRMWARE.md): the two-core way to make the sound, for an app whose sound is heavy. The
 * app's sound runs on the second core (CPU1) for a whole block's time, while CPU0 is free for the screen, the controls
 * and USB, and does the app's side work beside it in the interrupt. It is OpenPocket's own schedule for a chain without
 * NAM (px_audio.h: the chain on CPU1, the delay and the reverb on CPU0), for an app.
 *
 *   block N's interrupt:  the block before collected (CPU1 done with it), app_boundary(), the guitar into a_in, then
 *                         CPU1 handed app_sound(a_in, a_out) + dsp_out; CPU0 runs app_beside(deadline) meanwhile
 *   its output request:   the block collected (CPU1 must be done), sent
 * The latency is the plain schedule's: a block is made between its input and its output.
 *
 * The app's file defines DSP_CPU1 1 (firmware/build.sh reads that line to link the second core's entry) and provides,
 * before this one, what base/audio_plain.h asks for, and three more:
 *   app_sound(in, out)       on CPU1 (or on CPU0, in the interrupt, when CPU1 is off)
 *   app_sound_reset()        its sound's state cleared; called with the interrupt held and CPU1 idle
 *   app_cores(n)             the sound now runs on n cores (2 at the start when CPU1 answered, 1 from the moment it is
 *                            retired); called with CPU1 idle. The app moves its side work back into app_sound for 1.
 *   app_beside(deadline)     on CPU0 in the interrupt, while CPU1 makes the block: the app's side work, to stop by
 *                            fm1_ticks() == deadline (85 % of the block)
 *   app_boundary()           at each block's start, nothing running on either core (double buffers change roles)
 * The second core is the felucca-bsp worker (fm1_cpu1.h) with its fallback: a misread, a fault or a job not back in
 * time retires it, that block is lost (the output repeats one block), and the sound goes on on CPU0 alone, as on the
 * plain schedule, until the next power-on. The load shown is the sound's own (CPU1's, or CPU0's in the interrupt); the
 * safety bypass is at 93 % on CPU1 (nothing else runs there), 85 % on CPU0.
 */
#pragma once
#if !DSP_CPU1
#error "base/audio_dual.h needs DSP_CPU1 1 in the app's file (or base/audio_plain.h for one core)"
#endif

static volatile uint8_t a_c1;                          /* the sound runs on CPU1 */
static uint8_t a_job;                                  /* a block is in progress on CPU1 */
static volatile uint32_t a_c0;                         /* CPU0's side work in the last block, % (app_beside) */

static void a_c1_off(void)                             /* one core from now on */
{
    a_c1 = 0;
    app_cores(1);
}
static void a_make(int run, uint32_t lim)              /* the block's sound: a_in -> a_out -> a_res, timed */
{
    const uint32_t t0 = fm1_ticks();
    if (run) app_sound(a_in, a_out);
    else for (uint32_t i = 0; i < A_OUT_N; i++) a_out[i] = a_in[i % A_FR];
    dsp_out();
    dsp_load(t0, lim);
}
static void a_job_fn(uint32_t run) { a_make((int)run, 93u); }   /* (CPU1) */
static void a_collect(void)                            /* the block in progress on CPU1, finished */
{
    if (!a_job) return;
    a_job = 0;
    if (fm1_cpu1_wait() < 0) a_c1_off();               /* retired: this block is lost, CPU0 from the next one */
}
static void a_block(const uint8_t *data, int len)      /* a guitar block: its sound started on CPU1, or made here */
{
    const uint32_t t_irq = fm1_ticks();
    a_collect();
    if (a_c1) app_boundary();
    a_input(data, len);
    const int run = !a_bypass && !a_hold;
    if (a_c1) {
        if (fm1_cpu1_run(a_job_fn, (uint32_t)run)) {
            a_job = 1;
            if (run) {
                app_beside(t_irq + A_BUDGET * 85u / 100u);
                a_c0 = (fm1_ticks() - t_irq) * 100u / A_BUDGET;
            }
            return;
        }
        a_c1_off();
    }
    a_make(run, 85u);
}
static void a_send(int32_t *d, uint32_t n) { a_collect(); a_output(d, n); }   /* an output block: that sound, finished */

static void a_dual_start(void)                         /* (base_go) CPU1 answered at the start: the sound goes there */
{
    a_c1 = fm1_cpu1_on;
    app_cores(a_c1 ? 2 : 1);
}
static void a_nan_poll(void)
{
    if (!a_nan) return;
    fm1_irq_mask(FM1_IRQ_ALNK0);
    a_collect();
    app_sound_reset();
    a_xf = 0.0f; a_xft = 1;                            /* (back from silence) */
    a_nan = 0; a_nan_count++;
    fm1_irq_unmask(FM1_IRQ_ALNK0);
}
static void a_change(void (*change)(void))             /* as base/audio_plain.h's, with CPU1 idle during the change */
{
    if (a_running) {
        const uint32_t t0 = fm1_ticks();
        a_xft = 0;
        while (a_xf != 0.0f && fm1_ticks() - t0 < 12u * A_BUDGET) fm1_wdt_feed();
    }
    fm1_irq_mask(FM1_IRQ_ALNK0);
    a_collect();
    change();
    fm1_irq_unmask(FM1_IRQ_ALNK0);
    a_xft = 1;
}
