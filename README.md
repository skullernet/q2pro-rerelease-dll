# Port of Quake 2 re-release game source to Q2PRO

Port of Quake 2 re-release game source designed to run with Q2PRO, with all
KEX-specific APIs removed. The goal of this project is to make re-release
single player campaign playable in Q2PRO.

This game library uses custom game API version and will *not* run on standard
Quake 2 server. Latest version of Q2PRO client and server is required!

## Compatibility

All single- and multiplayer maps that come with Quake 2 re-release should
work without issues. Community made maps should work too, unless they have
bugs, or are incompatible with classic Quake 2 player movement code.

## Removed features

This project aims to recreate mostly vanilla Quake 2 experience. For this sake,
the following remaster-specific features have been removed and *won't be*
implemented.

* Cgame module
* Bots
* Custom pmove
* Lag compensation
* Shadow lights
* Weapon wheel
* Damage indicators
* POIs
* Achievements
* Localization

Variable tick rate support is currently broken. It may be fixed later, but this
is not a priority since variable tick rate is mostly useless for single player.

## Other notable changes

* Source code converted back to C from C++ for faster compilation and better
  portability.

* Savegames use custom text format rather than JSON for easier parsing and much
  more compact representation.

## Building

This project uses Meson build system. To build:

    meson setup build
    meson compile -C build

## Binaries

Precompiled binaries for Windows are available for downloading as CI artifacts.
See Actions → Workflow Runs → Artifacts.

Nightly Q2PRO re-release builds also include this game library built from its
latest commit.
