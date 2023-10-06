# Port of Quake 2 re-release game source to Q2PRO

Port of Quake 2 re-release game source designed to run with modified version of
Q2PRO, with all KEX-specific APIs removed. The goal of this project is to make
re-release single player campaign playable in Q2PRO.

Although this game library uses game API version 3, it will *not* run on
standard Quake 2 server, latest version of Q2PRO server and client is required!

This is a work in progress, but it's quite playable already. It has been used
to play through Call of the Machine episode and N64 campaign in Q2PRO without
any major issues.

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

## Building

This project uses Meson build system. To build:

    meson setup build
    meson compile -C build

## Binaries

Precompiled binaries for Windows are available for downloading as CI artifacts.
See Actions → Workflow Runs → Artifacts.

## Notes

* Focus is on single player support first and coop second. Don't use this game
  library for CTF or teamplay, that's not expected to work. Regular deathmatch
  may work, but is untested.
