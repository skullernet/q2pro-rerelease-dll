// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "../g_local.h"

// RAFAEL
/*
    RipperGun
*/

static void weapon_ionripper_fire(edict_t *ent)
{
    vec3_t tempang;
    int    damage;

    if (deathmatch->integer)
        // tone down for deathmatch
        damage = 30;
    else
        damage = 50;

    if (is_quad)
        damage *= damage_multiplier;

    VectorCopy(ent->client->v_angle, tempang);
    tempang[YAW] += crandom();

    vec3_t start, dir;
    P_ProjectSource(ent, tempang, (const vec3_t) { 16, 7, -8 }, start, dir);

    P_AddWeaponKick(ent, -3, -3);

    fire_ionripper(ent, start, dir, damage, 500, EF_IONRIPPER);

    // send muzzle flash
    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(ent - g_edicts);
    gi.WriteByte(MZ_IONRIPPER | is_silenced);
    gi.multicast(ent->s.origin, MULTICAST_PVS);

    PlayerNoise(ent, start, PNOISE_WEAPON);

    G_RemoveAmmo(ent);
}

void Weapon_Ionripper(edict_t *ent)
{
    static const int pause_frames[] = { 36, 0 };
    static const int fire_frames[] = { 6, 0 };

    Weapon_Generic(ent, 5, 7, 36, 39, pause_frames, fire_frames, weapon_ionripper_fire);
}

//
//  Phalanx
//

static void weapon_phalanx_fire(edict_t *ent)
{
    vec3_t v, start, dir;
    int    damage;
    float  damage_radius;
    int    radius_damage;

    damage = irandom2(70, 80);
    radius_damage = 120;
    damage_radius = 120;

    if (is_quad) {
        damage *= damage_multiplier;
        radius_damage *= damage_multiplier;
    }

    VectorCopy(ent->client->v_angle, v);

    if (ent->client->ps.gunframe == 8) {
        v[YAW] -= 1.5f;
        P_ProjectSource(ent, v, (const vec3_t) { 0, 8, -8 }, start, dir);

        radius_damage = 30;
        damage_radius = 120;

        fire_plasma(ent, start, dir, damage, 725, damage_radius, radius_damage);

        // send muzzle flash
        gi.WriteByte(svc_muzzleflash);
        gi.WriteShort(ent - g_edicts);
        gi.WriteByte(MZ_PHALANX2 | is_silenced);
        gi.multicast(ent->s.origin, MULTICAST_PVS);

        G_RemoveAmmo(ent);
    } else {
        v[YAW] += 1.5f;
        P_ProjectSource(ent, v, (const vec3_t) { 0, 8, -8 }, start, dir);

        fire_plasma(ent, start, dir, damage, 725, damage_radius, radius_damage);

        // send muzzle flash
        gi.WriteByte(svc_muzzleflash);
        gi.WriteShort(ent - g_edicts);
        gi.WriteByte(MZ_PHALANX | is_silenced);
        gi.multicast(ent->s.origin, MULTICAST_PVS);

        PlayerNoise(ent, start, PNOISE_WEAPON);
    }

    P_AddWeaponKick(ent, -2, -2);
}

void Weapon_Phalanx(edict_t *ent)
{
    static const int pause_frames[] = { 29, 42, 55, 0 };
    static const int fire_frames[] = { 7, 8, 0 };

    Weapon_Generic(ent, 5, 20, 58, 63, pause_frames, fire_frames, weapon_phalanx_fire);
}

/*
======================================================================

TRAP

======================================================================
*/

#define TRAP_TIMER_SEC  5.0f
#define TRAP_MINSPEED   300
#define TRAP_MAXSPEED   700

static void weapon_trap_fire(edict_t *ent, bool held)
{
    int speed;

    // Paril: kill sideways angle on grenades
    // limit upwards angle so you don't throw behind you
    vec3_t angles;
    P_GetThrowAngles(ent, angles);

    vec3_t start, dir;
    P_ProjectSource(ent, angles, (const vec3_t) { 8, 0, -8 }, start, dir);

    if (ent->health > 0) {
        float frac = 1.0f - TO_SEC(ent->client->grenade_time - level.time) / TRAP_TIMER_SEC;
        speed = lerp(TRAP_MINSPEED, TRAP_MAXSPEED, min(frac, 1.0f));
    } else
        speed = TRAP_MINSPEED;

    ent->client->grenade_time = 0;

    fire_trap(ent, start, dir, speed);

    G_RemoveAmmoEx(ent, 1);
}

void Weapon_Trap(edict_t *ent)
{
    static const int pause_frames[] = { 29, 34, 39, 48, 0 };

    Throw_Generic(ent, 15, 48, 5, "weapons/trapcock.wav", 11, 12, pause_frames, false, "weapons/traploop.wav", weapon_trap_fire, false);
}
