/* SPDX-License-Identifier: GPL-3.0-only */
/* POCKET METAL's memory regions (firmware/app.ld includes this file; docs/BASE-FIRMWARE.md): the 432 KiB of SRAM from
 * 0x01C08000 up to the stacks, split as OpenPocket's (firmware/memory.ld), whose engine this app runs: the engine's state
 * (px_engine, 162 KiB) and the stereo stage in the POOL, the audio path in RAMTEXT. Without NAM the POOL has room left. */
MEMORY
{
    RAM    (rwx) : ORIGIN = 0x01C08000, LENGTH = 96K
    POOL   (rw)  : ORIGIN = 0x01C20000, LENGTH = 0x3E000   /* 248 KiB: the engine, the stereo stage */
    RAMTEXT (rwx): ORIGIN = 0x01C5E000, LENGTH = 0x16000   /* 88 KiB of RAM code, up to the stacks */
}
