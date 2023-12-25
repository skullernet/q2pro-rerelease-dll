// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"
#include "../m_player.h"

static void weapon_prox_fire(edict_t *ent)
{
    // Paril: kill sideways angle on grenades
    // limit upwards angle so you don't fire behind you
    vec3_t angles;
    VectorCopy(ent->client->v_angle, angles);
    angles[0] = max(angles[0], -62.5f);

    vec3_t start, dir;
    P_ProjectSource(ent, angles, (const vec3_t) { 8, 0, -8 }, start, dir);

    P_AddWeaponKick(ent, -2, -1);

    fire_prox(ent, start, dir, damage_multiplier, 600);

    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(ent - g_edicts);
    gi.WriteByte(MZ_PROX | is_silenced);
    gi.multicast(ent->s.origin, MULTICAST_PVS);

    PlayerNoise(ent, start, PNOISE_WEAPON);

    G_RemoveAmmo(ent);
}

void Weapon_ProxLauncher(edict_t *ent)
{
    static const int pause_frames[] = { 34, 51, 59, 0 };
    static const int fire_frames[] = { 6, 0 };

    Weapon_Generic(ent, 5, 16, 59, 64, pause_frames, fire_frames, weapon_prox_fire);
}

static void weapon_tesla_fire(edict_t *ent, bool held)
{
    int speed;

    // Paril: kill sideways angle on grenades
    // limit upwards angle so you don't throw behind you
    vec3_t angles;
    VectorCopy(ent->client->v_angle, angles);
    angles[0] = max(angles[0], -62.5f);

    vec3_t start, dir;
    P_ProjectSource(ent, angles, (const vec3_t) { 0, 0, -22 }, start, dir);

    if (ent->health > 0) {
        float timer = TO_SEC(ent->client->grenade_time - level.time);
        speed = GRENADE_MINSPEED + (GRENADE_TIMER_SEC - timer) * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER_SEC);
        speed = min(speed, GRENADE_MAXSPEED);
    } else
        speed = GRENADE_MINSPEED;

    ent->client->grenade_time = 0;

    fire_tesla(ent, start, dir, damage_multiplier, speed);

    G_RemoveAmmoEx(ent, 1);
}

void Weapon_Tesla(edict_t *ent)
{
    static const int pause_frames[] = { 21, 0 };

    Throw_Generic(ent, 8, 32, -1, NULL, 1, 2, pause_frames, false, NULL, weapon_tesla_fire, false);
}

//======================================================================
// ROGUE MODS BELOW
//======================================================================

//
// CHAINFIST
//
#define CHAINFIST_REACH 24

static void weapon_chainfist_fire(edict_t *ent)
{
    if (!(ent->client->buttons & BUTTON_ATTACK)) {
        if (ent->client->ps.gunframe == 13 ||
            ent->client->ps.gunframe == 23 ||
            ent->client->ps.gunframe >= 32) {
            ent->client->ps.gunframe = 33;
            return;
        }
    }

    int damage = 7;

    if (deathmatch->integer)
        damage = 15;

    if (is_quad)
        damage *= damage_multiplier;

    // set start point
    vec3_t start, dir;

    P_ProjectSource(ent, ent->client->v_angle, (const vec3_t) { 0, 0, -4 }, start, dir);

    if (fire_player_melee(ent, start, dir, CHAINFIST_REACH, damage, 100, (mod_t) { MOD_CHAINFIST })) {
        if (ent->client->empty_click_sound < level.time) {
            ent->client->empty_click_sound = level.time + SEC(0.5f);
            gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/sawslice.wav"), 1, ATTN_NORM, 0);
        }
    }

    PlayerNoise(ent, start, PNOISE_WEAPON);

    ent->client->ps.gunframe++;

    if (ent->client->buttons & BUTTON_ATTACK) {
        if (ent->client->ps.gunframe == 12)
            ent->client->ps.gunframe = 14;
        else if (ent->client->ps.gunframe == 22)
            ent->client->ps.gunframe = 24;
        else if (ent->client->ps.gunframe >= 32)
            ent->client->ps.gunframe = 7;
    }

    // start the animation
    if (ent->client->anim_priority != ANIM_ATTACK || frandom() < 0.25f) {
        ent->client->anim_priority = ANIM_ATTACK;
        if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
            ent->s.frame = FRAME_crattak1 - 1;
            ent->client->anim_end = FRAME_crattak9;
        } else {
            ent->s.frame = FRAME_attack1 - 1;
            ent->client->anim_end = FRAME_attack8;
        }
        ent->client->anim_time = 0;
    }
}

// this spits out some smoke from the motor. it's a two-stroke, you know.
static void chainfist_smoke(edict_t *ent)
{
    vec3_t start, dir;
    P_ProjectSource(ent, ent->client->v_angle, (const vec3_t) { 8, 8, -4 }, start, dir);

    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_CHAINFIST_SMOKE);
    gi.WritePosition(start);
    gi.unicast(ent, 0);
}

void Weapon_ChainFist(edict_t *ent)
{
    static const int pause_frames[] = { 0 };

    Weapon_Repeating(ent, 4, 32, 57, 60, pause_frames, weapon_chainfist_fire);

    // smoke on idle sequence
    if (ent->client->ps.gunframe == 42 && irandom1(8)) {
        if ((ent->client->pers.hand != CENTER_HANDED) && frandom() < 0.4f)
            chainfist_smoke(ent);
    } else if (ent->client->ps.gunframe == 51 && irandom1(8)) {
        if ((ent->client->pers.hand != CENTER_HANDED) && frandom() < 0.4f)
            chainfist_smoke(ent);
    }

    // set the appropriate weapon sound.
    if (ent->client->weaponstate == WEAPON_FIRING)
        ent->client->weapon_sound = gi.soundindex("weapons/sawhit.wav");
    else if (ent->client->weaponstate == WEAPON_DROPPING)
        ent->client->weapon_sound = 0;
    else if (ent->client->pers.weapon->id == IT_WEAPON_CHAINFIST)
        ent->client->weapon_sound = gi.soundindex("weapons/sawidle.wav");
}

//
// Disintegrator
//

static void weapon_tracker_fire(edict_t *self)
{
    vec3_t   end;
    edict_t *enemy = NULL;
    trace_t  tr;
    int      damage;
    contents_t mask = MASK_PROJECTILE;

    // [Paril-KEX]
    if (!G_ShouldPlayersCollide(true))
        mask &= ~CONTENTS_PLAYER;

    // PMM - felt a little high at 25
    if (deathmatch->integer)
        damage = 45;
    else
        damage = 135;

    if (is_quad)
        damage *= damage_multiplier; // pgm

    vec3_t start, dir;
    P_ProjectSource(self, self->client->v_angle, (const vec3_t) { 24, 8, -8 }, start, dir);

    VectorMA(start, 8192, dir, end);

    // PMM - doing two traces .. one point and one box.
    tr = gi.trace(start, NULL, NULL, end, self, mask);
    if (tr.ent == world)
        tr = gi.trace(start, (const vec3_t) { -16, -16, -16 }, (const vec3_t) { 16, 16, 16 }, end, self, mask);

    if (tr.ent != world && ((tr.ent->svflags & SVF_MONSTER) || tr.ent->client || (tr.ent->flags & FL_DAMAGEABLE)) && tr.ent->health > 0)
        enemy = tr.ent;

    P_AddWeaponKick(self, -2, -1);

    fire_tracker(self, start, dir, damage, 1000, enemy);

    // send muzzle flash
    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(self - g_edicts);
    gi.WriteByte(MZ_TRACKER | is_silenced);
    gi.multicast(self->s.origin, MULTICAST_PVS);

    PlayerNoise(self, start, PNOISE_WEAPON);

    G_RemoveAmmo(self);
}

void Weapon_Disintegrator(edict_t *ent)
{
    static const int pause_frames[] = { 14, 19, 23, 0 };
    static const int fire_frames[] = { 5, 0 };

    Weapon_Generic(ent, 4, 9, 29, 34, pause_frames, fire_frames, weapon_tracker_fire);
}

/*
======================================================================

ETF RIFLE

======================================================================
*/
static void weapon_etf_rifle_fire(edict_t *ent)
{
    int    damage;
    int    kick = 3;
    int    i;
    vec3_t offset;

    if (deathmatch->integer)
        damage = 10;
    else
        damage = 10;

    if (!(ent->client->buttons & BUTTON_ATTACK)) {
        ent->client->ps.gunframe = 8;
        return;
    }

    if (ent->client->ps.gunframe == 6)
        ent->client->ps.gunframe = 7;
    else
        ent->client->ps.gunframe = 6;

    // PGM - adjusted to use the quantity entry in the weapon structure.
    if (ent->client->pers.inventory[ent->client->pers.weapon->ammo] < ent->client->pers.weapon->quantity) {
        ent->client->ps.gunframe = 8;
        NoAmmoWeaponChange(ent, true);
        return;
    }

    if (is_quad) {
        damage *= damage_multiplier;
        kick *= damage_multiplier;
    }

    for (i = 0; i < 3; i++) {
        ent->client->kick.origin[i] = crandom() * 0.85f;
        ent->client->kick.angles[i] = crandom() * 0.85f;
    }
    ent->client->kick.total = KICK_TIME;
    ent->client->kick.time = level.time + ent->client->kick.total;

    // get start / end positions
    if (ent->client->ps.gunframe == 6)
        VectorSet(offset, 15, 8, -8);
    else
        VectorSet(offset, 15, 6, -8);

    vec3_t start, dir, angles;
    VectorAdd(ent->client->v_angle, ent->client->kick.angles, angles);
    P_ProjectSource(ent, angles, offset, start, dir);
    fire_flechette(ent, start, dir, damage, 1150, kick);
    Weapon_PowerupSound(ent);

    // send muzzle flash
    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(ent - g_edicts);
    gi.WriteByte((ent->client->ps.gunframe == 6 ? MZ_ETF_RIFLE : MZ_ETF_RIFLE_2) | is_silenced);
    gi.multicast(ent->s.origin, MULTICAST_PVS);

    PlayerNoise(ent, start, PNOISE_WEAPON);

    G_RemoveAmmo(ent);

    ent->client->anim_priority = ANIM_ATTACK;
    if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
        ent->s.frame = FRAME_crattak1 - (int)(frandom() + 0.25f);
        ent->client->anim_end = FRAME_crattak9;
    } else {
        ent->s.frame = FRAME_attack1 - (int)(frandom() + 0.25f);
        ent->client->anim_end = FRAME_attack8;
    }
    ent->client->anim_time = 0;
}

void Weapon_ETF_Rifle(edict_t *ent)
{
    static const int pause_frames[] = { 18, 28, 0 };

    Weapon_Repeating(ent, 4, 7, 37, 41, pause_frames, weapon_etf_rifle_fire);
}

#define HEATBEAM_DM_DMG 15
#define HEATBEAM_SP_DMG 15

static void Heatbeam_Fire(edict_t *ent)
{
    bool firing = (ent->client->buttons & BUTTON_ATTACK);
    bool has_ammo = ent->client->pers.inventory[ent->client->pers.weapon->ammo] >= ent->client->pers.weapon->quantity;

    if (!firing || !has_ammo) {
        ent->client->ps.gunframe = 13;
        ent->client->weapon_sound = 0;
        //ent->client->ps.gunskin = 0;

        if (firing && !has_ammo)
            NoAmmoWeaponChange(ent, true);
        return;
    }

    // start on frame 8
    if (ent->client->ps.gunframe > 12)
        ent->client->ps.gunframe = 8;
    else
        ent->client->ps.gunframe++;

    if (ent->client->ps.gunframe == 12)
        ent->client->ps.gunframe = 8;

    // play weapon sound for firing
    ent->client->weapon_sound = gi.soundindex("weapons/bfg__l1a.wav");
    //ent->client->ps.gunskin = 1;

    int damage;
    int kick;

    // for comparison, the hyperblaster is 15/20
    // jim requested more damage, so try 15/15 --- PGM 07/23/98
    if (deathmatch->integer)
        damage = HEATBEAM_DM_DMG;
    else
        damage = HEATBEAM_SP_DMG;

    if (deathmatch->integer) // really knock 'em around in deathmatch
        kick = 75;
    else
        kick = 30;

    if (is_quad) {
        damage *= damage_multiplier;
        kick *= damage_multiplier;
    }

    ent->client->kick.time = 0;

    // This offset is the "view" offset for the beam start (used by trace)
    vec3_t start, dir;
    P_ProjectSource(ent, ent->client->v_angle, (const vec3_t) { 7, 2, -3 }, start, dir);

    // This offset is the entity offset
    fire_heatbeam(ent, start, dir, (const vec3_t) { 2, 7, -3 }, damage, kick, false);
    Weapon_PowerupSound(ent);

    // send muzzle flash
    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(ent - g_edicts);
    gi.WriteByte(MZ_HEATBEAM | is_silenced);
    gi.multicast(ent->s.origin, MULTICAST_PVS);

    PlayerNoise(ent, start, PNOISE_WEAPON);

    G_RemoveAmmo(ent);

    ent->client->anim_priority = ANIM_ATTACK;
    if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
        ent->s.frame = FRAME_crattak1 - (int)(frandom() + 0.25f);
        ent->client->anim_end = FRAME_crattak9;
    } else {
        ent->s.frame = FRAME_attack1 - (int)(frandom() + 0.25f);
        ent->client->anim_end = FRAME_attack8;
    }
    ent->client->anim_time = 0;
}

void Weapon_Heatbeam(edict_t *ent)
{
    static const int pause_frames[] = { 35, 0 };

    Weapon_Repeating(ent, 8, 12, 42, 47, pause_frames, Heatbeam_Fire);
}
