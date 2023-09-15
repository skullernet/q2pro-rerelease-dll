# Port of Quake 2 re-release game source to Q2PRO

Port of Quake 2 re-release game source designed to run with modified version of
Q2PRO, with all KEX-specific APIs removed. The goal of this project is to make
re-release single player campaign playable in Q2PRO.

Although this game library uses game API version 3, it will *not* run on
standard Quake 2 server, modified Q2PRO server and client built from
experimental `protocol-limits` branch is required!

This is a work in progress, don't expect it to be 100% playable yet (although
many maps already play fine).

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

## Notes

* `localization/loc_english.txt` must be manually extracted from `Q2Game.kpf`
  archive and put into `baseq2` for map messages to work.
