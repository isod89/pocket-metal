# POCKET METAL (metal.c): what firmware/build.sh needs to build it on the base firmware (docs/BASE-FIRMWARE.md, docs/METAL.md).
APP_MAIN=metal.c           # the app's C file
APP_ENGINE=mx_engine.c     # the engine: OpenPocket's px_dsp.c without NAM, with the metal types, and the stereo stage
APP_DEFS="-DPX_SR=44100.0f -DPX_OVERSAMPLE=4 -DPX_NO_NAM=1 -DPX_METAL=1"
APP_SIM_SRC="firmware/px_math.c"   # (the simulator: the pedal's mini libm, as OpenPocket's simulator builds the engine)
# the engine's per-block functions run from RAM, as OpenPocket's do (firmware/build.sh RAM_DSP, without the NAM network's)
APP_RAM_DSP="px_engine_process px_engine_part dly_block rv_block rv_front clip_run os_up os_down ir_run fft64 rfft128
  cab_detail ps_voice_al ps_align amp_stage bq_block bq_block2 drive_block fq_loop_c ps_voice_run pitch_run cab_detail_run
  gate_run comp_run bq_chain mx_stereo_run px_wet_run px_wet_swap"
APP_RAM_FW="app_sound app_beside app_boundary a_make a_job_fn mx_ticks"   # metal.c's sound entry and its side work, the two-core schedule's (base/audio_dual.h)
