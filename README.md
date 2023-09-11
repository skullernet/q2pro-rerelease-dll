# Port of Quake 2 re-release game source to Q2PRO

Port of Quake 2 re-release game source designed to run with modified version of
Q2PRO, with all KEX-specific APIs removed. The goal of this project is to make
re-release single player campaign playable in Q2PRO.

This game library will *not* run on standard Quake 2 server, modified Q2PRO
server and client built from experimental `protocol-limits` branch is required!
This is a work in progress, don't expect it to function properly yet.

## Notes

* `localization/loc_english.txt` must be manually extracted from `Q2Game.kpf`
  archive and put into `baseq2` for map messages to work.

## Known bugs

* mgu6m1 requires `map_allsolid_bug 0` to be complete-able, otherwise
  `func_object` falls and blocks passage.

* Berserk jump is broken. Seems to be related to 10 FPS tick rate, because the
  same thing happens in KEX at 10 FPS.
