# vip_fp (Plugify)

[VIP] Fast Plant: a bomb_beginplant hook sets the held C4's m_fArmedTime to the current game time for a VIP with the 'fp' feature, so the plant counts as already armed. Ported from the Pisex VIP core module VIP_FastPlant (vip_fp). Requires the vip core.

Plugify port (fork) of **VIP_FastPlant ([VIP] Fast Plant)** by Pisex (Metamod (C++)).
Original: https://github.com/Pisex/cs2-vip-modules
The code was rewritten for Plugify / s2sdk; behaviour follows the original.

## Requirements
- [Plugify](https://github.com/untrustedmodders/plugify) with [s2sdk](https://github.com/untrustedmodders/plugify-plugin-s2sdk)
- Plugify dependencies: s2sdk, vip, hudmenu (optional)

## Installation
Download `vip_fp-1.0.0.zip` from Releases and extract it into `game/csgo/`.

## Building
Requires Docker. `./build.sh` builds in the Steam Runtime 3 (sniper) SDK image; the package
is written to `build/package/vip_fp`.

`plugins/vip` carry only the API headers of the plugins this one uses.

## License
GPL-3.0, see LICENSE.

