# Credits and licences

POCKET METAL is free software under the GPL-3.0 (`LICENSE`), except the parts below, which keep their own licences.

**POCKET METAL is built on [OpenPocket](https://github.com/isod89/openpocket)**, the open-source firmware for the
Donner Pocket X and the M-VAVE BlackBox. Its base firmware (`firmware/base/`: the start, the boot guard, USB-MIDI, the
update entry, the audio path, the controls, the store), its build, its update loader, its simulator and its installer
are OpenPocket's, copied here as OpenPocket has them (7ea83be). The base firmware shared by OpenPocket
and its alternative firmwares is by **Vince Speelman ([VinSpee](https://github.com/VinSpee))** with OpenPocket
(pull request #27). OpenPocket's other contributors are credited in its own
[NOTICE.md](https://github.com/isod89/openpocket/blob/main/NOTICE.md).

**POCKET METAL** (`firmware/apps/metal`): its amps and pedals are OpenPocket's fitted models, ten amps and five pedals fitted for it to captures of the Neural Amp Modeler community collection ([pelennor2170/NAM_models](https://github.com/pelennor2170/NAM_models), GNU GPL v3, each capture's author named in `docs/METAL.md`), on DI guitar from [Alec-Wright/Automated-GuitarAmpModelling](https://github.com/Alec-Wright/Automated-GuitarAmpModelling) (GNU GPL v3). Its screen fonts are bitmaps rendered from three SIL Open Font License 1.1 fonts, kept with their licences in `assets/metal/fonts/`: Pirata One (Rodrigo Fuenzalida, Nicolas Massi), UnifrakturMaguntia (j. 'mach' wust, Peter Wiegel) and Barlow Condensed (the Barlow Project Authors). The installer page uses Pirata One and Barlow Condensed from Google Fonts.

| Part | Origin | Licence |
| --- | --- | --- |
| `firmware/bsp/` (chip support: startup, interrupts, flash, USB-MIDI device, M-UPGRADE update entry `ota.c`) | [felucca-bsp](https://github.com/hugelton/felucca-bsp), Leo Kuroshita / Hügelton Instruments and contributors, changed for OpenPocket | MPL-2.0 (`LICENSES/MPL-2.0.txt`) |
| `loader/` (update loader) | felucca-bsp `loader/`, changed for the Pocket X flash layout | MPL-2.0 |
| `docs/pxota.js`, `docs/pxpkg.js`, `tools/pxpkg.py`, `tools/mkloader.py`, `tools/lz4blk.py` | adapted from [Felucca](https://github.com/hugelton/Felucca)'s web installer and build tools | GPL-3.0 |
| `firmware/sdk/` (I2S and DAC drivers as LLVM bitcode, SPL, cfg files) | JieLi AC79 SDK V1.2.0 (fw-AC79_AIoT_SDK) | Apache-2.0 (`LICENSES/Apache-2.0.txt`) |
| `firmware/fonts.h` (bitmaps generated from Inter) | [Inter](https://rsms.me/inter/), Rasmus Andersson | SIL OFL 1.1 (`LICENSES/OFL-1.1-Inter.txt`) |
| `assets/loops/*.wav` (guitar loops for the tests and the simulator) | cut from [Guitar-TECHS](https://zenodo.org/records/14963133), credits in `assets/loops/ATTRIBUTION.md` | CC BY 4.0 |

No part of the official Pocket X / BlackBox firmware is in this repository. The installer's way back to the official
firmware uses the file you download from the maker; it is checked in your browser and never uploaded. All trademarks
belong to their owners; POCKET METAL is not affiliated with Donner or M-VAVE.
