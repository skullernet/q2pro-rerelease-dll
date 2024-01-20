# Port of Quake 2 re-release game source to Q2PRO

Port of Quake 2 re-release game source designed to run with Q2PRO, with all
KEX-specific APIs removed. The goal of this project is to make re-release
single player campaign playable in Q2PRO.

Although this game library uses game API version 3, it will *not* run on
standard Quake 2 server, latest version of Q2PRO server and client is required!

All single player maps that come with Quake 2 re-release should work out of the
box. Many community made maps should work too, but some are known to have bugs.

## Removed features

This project aims to recreate mostly vanilla Quake 2 experience. For this sake,
the following remaster-specific features have been removed.

* Cgame module
* Bots
* AI navigation using bot pathfinding
* Custom pmove
* Lag compensation
* Variable tick rate
* Shadow lights
* Fog
* Weapon wheel
* Health bars
* Hit/damage indicators
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
