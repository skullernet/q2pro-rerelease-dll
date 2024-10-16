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
the following remaster-specific features have been removed / not implemented.

* Cgame module
* Bots
* Custom pmove
* Lag compensation
* Variable tick rate
* Shadow lights
* Weapon wheel
* Damage indicators
* POIs
* Achievements
* Localization

## Other notable changes

* Source code converted back to C from C++ for faster compilation.

* Savegames use custom text format rather than JSON for easier parsing.

## Building

This project uses Meson build system. To build:

    meson setup build
    meson compile -C build

## Binaries

Precompiled binaries for Windows are available for downloading as CI artifacts.
See Actions → Workflow Runs → Artifacts.
