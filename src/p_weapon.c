// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_weapon.c

#include "g_local.h"
#include "m_player.h"

bool is_quad;
// RAFAEL
bool is_quadfire;
// RAFAEL
player_muzzle_t is_silenced;

// PGM
int damage_multiplier;
// PGM

//========
// [Kex]
bool G_CheckInfiniteAmmo(const gitem_t *item)
{
    if (item->flags & IF_NO_INFINITE_AMMO)
        return false;

    return g_infinite_ammo->integer || (deathmatch->integer && g_instagib->integer);
}

//========
// ROGUE
int P_DamageModifier(edict_t *ent)
{
    is_quad = 0;
    damage_multiplier = 1;

    if (ent->client->quad_time > level.time) {
        damage_multiplier *= 4;
        is_quad = 1;

        // if we're quad and DF_NO_STACK_DOUBLE is on, return now.
        if (g_dm_no_stack_double->integer)
            return damage_multiplier;
    }

    if (ent->client->double_time > level.time) {
        damage_multiplier *= 2;
        is_quad = 1;
    }

    return damage_multiplier;
}
// ROGUE
//========

void P_AddWeaponKick(edict_t *ent, float scale, float pitch)
{
    VectorScale(ent->client->v_forward, scale, ent->client->kick.origin);
    VectorSet(ent->client->kick.angles, pitch, 0, 0);
    ent->client->kick.total = KICK_TIME;
    ent->client->kick.time = level.time + ent->client->kick.total;
}

void P_ProjectSource(edict_t *ent, const vec3_t angles, const vec3_t g_distance, vec3_t result_start, vec3_t result_dir)
{
    vec3_t distance;
    VectorCopy(g_distance, distance);

    if (ent->client->pers.hand == LEFT_HANDED)
        distance[1] = -distance[1];
    else if (ent->client->pers.hand == CENTER_HANDED)
        distance[1] = 0;

    vec3_t eye_position;
    VectorCopy(ent->s.origin, eye_position);
    eye_position[2] += ent->viewheight;

    vec3_t forward, right, up;
    AngleVectors(angles, forward, right, up);

    G_ProjectSource2(eye_position, distance, forward, right, up, result_start);

    vec3_t end;
    VectorMA(eye_position, 8192, forward, end);

    contents_t mask = MASK_PROJECTILE & ~CONTENTS_DEADMONSTER;

    // [Paril-KEX]
    if (!G_ShouldPlayersCollide(true))
        mask &= ~CONTENTS_PLAYER;

    trace_t tr = gi.trace(eye_position, NULL, NULL, end, ent, mask);

    // if the point was a monster & close to us, use raw forward
    // so railgun pierces properly
    if (tr.startsolid || ((tr.contents & (CONTENTS_MONSTER | CONTENTS_PLAYER)) && (tr.fraction * 8192) < 128))
        VectorCopy(forward, result_dir);
    else {
        VectorSubtract(tr.endpos, result_start, result_dir);
        VectorNormalize(result_dir);
    }
}

/*
===============
PlayerNoise

Each player can have two noise objects associated with it:
a personal noise (jumping, pain, weapon firing), and a weapon
target noise (bullet wall impacts)

Monsters that don't directly see the player can move
to a noise in hopes of seeing the player from there.
===============
*/
void PlayerNoise(edict_t *who, const vec3_t where, player_noise_t type)
{
    edict_t *noise;

    if (type == PNOISE_WEAPON) {
        if (who->client->silencer_shots)
            who->client->invisibility_fade_time = level.time + (INVISIBILITY_TIME / 5);
        else
            who->client->invisibility_fade_time = level.time + INVISIBILITY_TIME;

        if (who->client->silencer_shots) {
            who->client->silencer_shots--;
            return;
        }
    }

    if (deathmatch->integer)
        return;

    if (who->flags & FL_NOTARGET)
        return;

    if (type == PNOISE_SELF && (who->client->landmark_free_fall || who->client->landmark_noise_time >= level.time))
        return;

    // ROGUE
    if (who->flags & FL_DISGUISED) {
        if (type == PNOISE_WEAPON) {
            level.disguise_violator = who;
            level.disguise_violation_time = level.time + SEC(0.5f);
        } else
            return;
    }
    // ROGUE

    if (!who->mynoise) {
        noise = G_Spawn();
        noise->classname = "player_noise";
        VectorSet(noise->mins, -8, -8, -8);
        VectorSet(noise->maxs, 8, 8, 8);
        noise->owner = who;
        noise->svflags = SVF_NOCLIENT;
        who->mynoise = noise;

        noise = G_Spawn();
        noise->classname = "player_noise";
        VectorSet(noise->mins, -8, -8, -8);
        VectorSet(noise->maxs, 8, 8, 8);
        noise->owner = who;
        noise->svflags = SVF_NOCLIENT;
        who->mynoise2 = noise;
    }

    if (type == PNOISE_SELF || type == PNOISE_WEAPON) {
        noise = who->mynoise;
        who->client->sound_entity = noise;
        who->client->sound_entity_time = level.time;
    } else { // type == PNOISE_IMPACT
        noise = who->mynoise2;
        who->client->sound2_entity = noise;
        who->client->sound2_entity_time = level.time;
    }

    VectorCopy(where, noise->s.origin);
    noise->teleport_time = level.time;
    gi.linkentity(noise);
}

static bool G_WeaponShouldStay(void)
{
    if (deathmatch->integer)
        return g_dm_weapons_stay->integer;
    if (coop->integer)
        return !P_UseCoopInstancedItems();

    return false;
}

bool Pickup_Weapon(edict_t *ent, edict_t *other)
{
    item_id_t index;
    const gitem_t *ammo;

    index = ent->item->id;

    if (G_WeaponShouldStay() && other->client->pers.inventory[index] &&
        !(ent->spawnflags & (SPAWNFLAG_ITEM_DROPPED | SPAWNFLAG_ITEM_DROPPED_PLAYER)))
        return false; // leave the weapon for others to pickup

    bool is_new = !other->client->pers.inventory[index];

    other->client->pers.inventory[index]++;

    if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED)) {
        // give them some ammo with it
        // PGM -- IF APPROPRIATE!
        if (ent->item->ammo) { // PGM
            ammo = GetItemByIndex(ent->item->ammo);
            // RAFAEL: Don't get infinite ammo with trap
            if (G_CheckInfiniteAmmo(ammo))
                Add_Ammo(other, ammo, 1000);
            else
                Add_Ammo(other, ammo, ammo->quantity);
        }

        if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED_PLAYER)) {
            if (deathmatch->integer) {
                if (g_dm_weapons_stay->integer)
                    ent->flags |= FL_RESPAWN;

                SetRespawnEx(ent, SEC(g_weapon_respawn_time->integer), !g_dm_weapons_stay->integer);
            }
            if (coop->integer)
                ent->flags |= FL_RESPAWN;
        }
    }

    G_CheckAutoSwitch(other, ent->item, is_new);

    return true;
}

static void Weapon_RunThink(edict_t *ent)
{
    // call active weapon think routine
    if (!ent->client->pers.weapon->weaponthink)
        return;

    P_DamageModifier(ent);
    // RAFAEL
    is_quadfire = (ent->client->quadfire_time > level.time);
    // RAFAEL
    if (ent->client->silencer_shots)
        is_silenced = MZ_SILENCED;
    else
        is_silenced = MZ_NONE;
    ent->client->pers.weapon->weaponthink(ent);
}

/*
===============
ChangeWeapon

The old weapon has been dropped all the way, so make the new one
current
===============
*/
void ChangeWeapon(edict_t *ent)
{
    // [Paril-KEX]
    if (ent->health > 0 && !g_instant_weapon_switch->integer && ((ent->client->latched_buttons | ent->client->buttons) & BUTTON_HOLSTER))
        return;

    if (ent->client->grenade_time) {
        // force a weapon think to drop the held grenade
        ent->client->weapon_sound = 0;
        Weapon_RunThink(ent);
        ent->client->grenade_time = 0;
    }

    if (ent->client->pers.weapon) {
        ent->client->pers.lastweapon = ent->client->pers.weapon;

        if (ent->client->newweapon && ent->client->newweapon != ent->client->pers.weapon)
            gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/change.wav"), 1, ATTN_NORM, 0);
    }

    ent->client->pers.weapon = ent->client->newweapon;
    ent->client->newweapon = NULL;

    // set visible model
    if (ent->s.modelindex == MODELINDEX_PLAYER)
        P_AssignClientSkinnum(ent);

    if (!ent->client->pers.weapon) {
        // dead
        ent->client->ps.gunindex = 0;
        return;
    }

    ent->client->weaponstate = WEAPON_ACTIVATING;
    ent->client->ps.gunframe = 0;
    ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);
    ent->client->weapon_sound = 0;

    ent->client->anim_priority = ANIM_PAIN;
    if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
        ent->s.frame = FRAME_crpain1;
        ent->client->anim_end = FRAME_crpain4;
    } else {
        ent->s.frame = FRAME_pain301;
        ent->client->anim_end = FRAME_pain304;
    }
    ent->client->anim_time = 0;

    // for instantweap, run think immediately
    // to set up correct start frame
    if (g_instant_weapon_switch->integer)
        Weapon_RunThink(ent);
}

/*
=================
NoAmmoWeaponChange
=================
*/
void NoAmmoWeaponChange(edict_t *ent, bool sound)
{
    if (sound && level.time >= ent->client->empty_click_sound) {
        gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
        ent->client->empty_click_sound = level.time + SEC(1);
    }

    static const uint8_t no_ammo_order[] = {
        IT_WEAPON_DISRUPTOR,
        IT_WEAPON_RAILGUN,
        IT_WEAPON_PLASMABEAM,
        IT_WEAPON_IONRIPPER,
        IT_WEAPON_HYPERBLASTER,
        IT_WEAPON_ETF_RIFLE,
        IT_WEAPON_CHAINGUN,
        IT_WEAPON_MACHINEGUN,
        IT_WEAPON_SSHOTGUN,
        IT_WEAPON_SHOTGUN,
        IT_WEAPON_PHALANX,
        IT_WEAPON_RLAUNCHER,
        IT_WEAPON_GLAUNCHER,
        IT_WEAPON_PROXLAUNCHER,
        IT_WEAPON_CHAINFIST,
        IT_WEAPON_BLASTER
    };

    for (int i = 0; i < q_countof(no_ammo_order); i++) {
        const gitem_t *item = GetItemByIndex(no_ammo_order[i]);

        if (!item)
            gi.error("Invalid no ammo weapon switch weapon %d\n", no_ammo_order[i]);

        if (!ent->client->pers.inventory[item->id])
            continue;

        if (item->ammo && ent->client->pers.inventory[item->ammo] < item->quantity)
            continue;

        ent->client->newweapon = item;
        return;
    }
}

void G_RemoveAmmoEx(edict_t *ent, int quantity)
{
    if (G_CheckInfiniteAmmo(ent->client->pers.weapon))
        return;

    bool pre_warning = ent->client->pers.inventory[ent->client->pers.weapon->ammo] <=
                       ent->client->pers.weapon->quantity_warn;

    ent->client->pers.inventory[ent->client->pers.weapon->ammo] -= quantity;

    bool post_warning = ent->client->pers.inventory[ent->client->pers.weapon->ammo] <=
                        ent->client->pers.weapon->quantity_warn;

    if (!pre_warning && post_warning)
        gix.local_sound(ent, NULL, ent, CHAN_AUTO, gi.soundindex("weapons/lowammo.wav"), 1, ATTN_NORM, 0);

    if (ent->client->pers.weapon->ammo == IT_AMMO_CELLS)
        G_CheckPowerArmor(ent);
}

void G_RemoveAmmo(edict_t *ent)
{
    G_RemoveAmmoEx(ent, ent->client->pers.weapon->quantity);
}

// [Paril-KEX] get time per animation frame
static gtime_t Weapon_AnimationTime(edict_t *ent)
{
    int gunrate;

    if (g_quick_weapon_switch->integer && ent->client->ps.gunframe != 0 &&
        (ent->client->weaponstate == WEAPON_ACTIVATING || ent->client->weaponstate == WEAPON_DROPPING))
        gunrate = 20;
    else
        gunrate = 10;

    if (ent->client->ps.gunframe != 0 && (!(ent->client->pers.weapon->flags & IF_NO_HASTE) || ent->client->weaponstate != WEAPON_FIRING)) {
        if (is_quadfire)
            gunrate *= 2;
        if (CTFApplyHaste(ent))
            gunrate *= 2;
    }

    return SEC(1.0f / gunrate);
}

/*
=================
Think_Weapon

Called by ClientBeginServerFrame and ClientThink
=================
*/
void Think_Weapon(edict_t *ent)
{
    if (ent->client->resp.spectator)
        return;

    // if just died, put the weapon away
    if (ent->health < 1) {
        ent->client->newweapon = NULL;
        ChangeWeapon(ent);
    }

    if (!ent->client->pers.weapon) {
        if (ent->client->newweapon)
            ChangeWeapon(ent);
        return;
    }

    // call active weapon think routine
    Weapon_RunThink(ent);

    // check remainder from haste; on 100ms/50ms server frames we may have
    // 'run next frame in' times that we can't possibly catch up to,
    // so we have to run them now.
    if (MSEC(33) < FRAME_TIME) {
        gtime_t relative_time = Weapon_AnimationTime(ent);

        if (relative_time < FRAME_TIME) {
            // check how many we can't run before the next server tick
            gtime_t next_frame = level.time + FRAME_TIME;
            int64_t remaining_ms = TO_MSEC(next_frame - ent->client->weapon_think_time);

            while (remaining_ms > 0) {
                ent->client->weapon_think_time -= relative_time;
                ent->client->weapon_fire_finished -= relative_time;
                Weapon_RunThink(ent);
                remaining_ms -= TO_MSEC(relative_time);
            }
        }
    }
}

typedef enum {
    WEAP_SWITCH_ALREADY_USING,
    WEAP_SWITCH_NO_WEAPON,
    WEAP_SWITCH_NO_AMMO,
    WEAP_SWITCH_NOT_ENOUGH_AMMO,
    WEAP_SWITCH_VALID
} weap_switch_t;

static weap_switch_t Weapon_AttemptSwitch(edict_t *ent, const gitem_t *item, bool silent)
{
    if (ent->client->pers.weapon == item)
        return WEAP_SWITCH_ALREADY_USING;
    if (!ent->client->pers.inventory[item->id])
        return WEAP_SWITCH_NO_WEAPON;

    if (item->ammo && !g_select_empty->integer && !(item->flags & IF_AMMO)) {
        const gitem_t *ammo_item = GetItemByIndex(item->ammo);

        if (!ent->client->pers.inventory[item->ammo]) {
            if (!silent)
                gi.cprintf(ent, PRINT_HIGH, "No %s for %s.\n", ammo_item->pickup_name, item->pickup_name_definite);
            return WEAP_SWITCH_NO_AMMO;
        }
        if (ent->client->pers.inventory[item->ammo] < item->quantity) {
            if (!silent)
                gi.cprintf(ent, PRINT_HIGH, "Not enough %s for %s.\n", ammo_item->pickup_name, item->pickup_name_definite);
            return WEAP_SWITCH_NOT_ENOUGH_AMMO;
        }
    }

    return WEAP_SWITCH_VALID;
}

static bool Weapon_IsPartOfChain(const gitem_t *item, const gitem_t *other)
{
    if (!other || !other->chain || !item->chain)
        return false;

    const gitem_t *root = other;
    while (1) {
        if (other == item)
            return true;
        other = &itemlist[other->chain];
        if (other == root)
            return false;
    }
}

/*
================
Use_Weapon

Make the weapon ready if there is ammo
================
*/
void Use_Weapon(edict_t *ent, const gitem_t *item)
{
    const gitem_t *wanted, *root;
    weap_switch_t  result = WEAP_SWITCH_NO_WEAPON;

    // if we're switching to a weapon in this chain already,
    // start from the weapon after this one in the chain
    if (!ent->client->no_weapon_chains && Weapon_IsPartOfChain(item, ent->client->newweapon)) {
        root = ent->client->newweapon;
        wanted = &itemlist[root->chain];
    // if we're already holding a weapon in this chain,
    // start from the weapon after that one
    } else if (!ent->client->no_weapon_chains && Weapon_IsPartOfChain(item, ent->client->pers.weapon)) {
        root = ent->client->pers.weapon;
        wanted = &itemlist[root->chain];
    // start from beginning of chain (if any)
    } else
        wanted = root = item;

    while (true) {
        // try the weapon currently in the chain
        if ((result = Weapon_AttemptSwitch(ent, wanted, false)) == WEAP_SWITCH_VALID)
            break;

        // no chains
        if (!wanted->chain || ent->client->no_weapon_chains)
            break;

        wanted = &itemlist[wanted->chain];

        // we wrapped back to the root item
        if (wanted == root)
            break;
    }

    if (result == WEAP_SWITCH_VALID)
        ent->client->newweapon = wanted; // change to this weapon when down
    else if ((result = Weapon_AttemptSwitch(ent, wanted, true)) == WEAP_SWITCH_NO_WEAPON && wanted != ent->client->pers.weapon && wanted != ent->client->newweapon)
        gi.cprintf(ent, PRINT_HIGH, "Out of item: %s\n", wanted->pickup_name);
}

/*
================
Drop_Weapon
================
*/
void Drop_Weapon(edict_t *ent, const gitem_t *item)
{
    item_id_t index = item->id;
    // see if we're already using it
    if (((item == ent->client->pers.weapon) || (item == ent->client->newweapon)) && (ent->client->pers.inventory[index] == 1)) {
        gi.cprintf(ent, PRINT_HIGH, "Can't drop current weapon\n");
        return;
    }

    edict_t *drop = Drop_Item(ent, item);
    drop->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
    drop->svflags &= ~SVF_INSTANCED;
    ent->client->pers.inventory[index]--;
}

void Weapon_PowerupSound(edict_t *ent)
{
    if (!CTFApplyStrengthSound(ent)) {
        if (ent->client->quad_time > level.time && ent->client->double_time > level.time)
            gi.sound(ent, CHAN_ITEM, gi.soundindex("ctf/tech2x.wav"), 1, ATTN_NORM, 0);
        else if (ent->client->quad_time > level.time)
            gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);
        else if (ent->client->double_time > level.time)
            gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/ddamage3.wav"), 1, ATTN_NORM, 0);
        else if (ent->client->quadfire_time > level.time
                 && ent->client->ctf_techsndtime < level.time) {
            ent->client->ctf_techsndtime = level.time + SEC(1);
            gi.sound(ent, CHAN_ITEM, gi.soundindex("ctf/tech3.wav"), 1, ATTN_NORM, 0);
        }
    }

    CTFApplyHasteSound(ent);
}

static bool Weapon_CanAnimate(edict_t *ent)
{
    // VWep animations screw up corpses
    return !ent->deadflag && ent->s.modelindex == MODELINDEX_PLAYER;
}

// [Paril-KEX] called when finished to set time until
// we're allowed to switch to fire again
static void Weapon_SetFinished(edict_t *ent)
{
    ent->client->weapon_fire_finished = level.time + Weapon_AnimationTime(ent);
}

static bool Weapon_HandleDropping(edict_t *ent, int FRAME_DEACTIVATE_LAST)
{
    if (ent->client->weaponstate == WEAPON_DROPPING) {
        if (ent->client->weapon_think_time <= level.time) {
            if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST) {
                ChangeWeapon(ent);
                return true;
            }
            if ((FRAME_DEACTIVATE_LAST - ent->client->ps.gunframe) == 4) {
                ent->client->anim_priority = ANIM_ATTACK | ANIM_REVERSED;
                if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
                    ent->s.frame = FRAME_crpain4 + 1;
                    ent->client->anim_end = FRAME_crpain1;
                } else {
                    ent->s.frame = FRAME_pain304 + 1;
                    ent->client->anim_end = FRAME_pain301;
                }
                ent->client->anim_time = 0;
            }

            ent->client->ps.gunframe++;
            ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);
        }
        return true;
    }

    return false;
}

static bool Weapon_HandleActivating(edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_IDLE_FIRST)
{
    if (ent->client->weaponstate == WEAPON_ACTIVATING) {
        if (ent->client->weapon_think_time <= level.time || g_instant_weapon_switch->integer) {
            ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);

            if (ent->client->ps.gunframe == FRAME_ACTIVATE_LAST || g_instant_weapon_switch->integer) {
                ent->client->weaponstate = WEAPON_READY;
                ent->client->ps.gunframe = FRAME_IDLE_FIRST;
                ent->client->weapon_fire_buffered = false;
                if (!g_instant_weapon_switch->integer)
                    Weapon_SetFinished(ent);
                else
                    ent->client->weapon_fire_finished = 0;
                return true;
            }

            ent->client->ps.gunframe++;
            return true;
        }
    }

    return false;
}

static bool Weapon_HandleNewWeapon(edict_t *ent, int FRAME_DEACTIVATE_FIRST, int FRAME_DEACTIVATE_LAST)
{
    bool is_holstering = false;

    if (!g_instant_weapon_switch->integer)
        is_holstering = ((ent->client->latched_buttons | ent->client->buttons) & BUTTON_HOLSTER);

    if ((ent->client->newweapon || is_holstering) && (ent->client->weaponstate != WEAPON_FIRING)) {
        if (g_instant_weapon_switch->integer || ent->client->weapon_think_time <= level.time) {
            if (!ent->client->newweapon)
                ent->client->newweapon = ent->client->pers.weapon;

            ent->client->weaponstate = WEAPON_DROPPING;

            if (g_instant_weapon_switch->integer) {
                ChangeWeapon(ent);
                return true;
            }

            ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;

            if ((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4) {
                ent->client->anim_priority = ANIM_ATTACK | ANIM_REVERSED;
                if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
                    ent->s.frame = FRAME_crpain4 + 1;
                    ent->client->anim_end = FRAME_crpain1;
                } else {
                    ent->s.frame = FRAME_pain304 + 1;
                    ent->client->anim_end = FRAME_pain301;
                }
                ent->client->anim_time = 0;
            }

            ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);
        }
        return true;
    }

    return false;
}

typedef enum {
    READY_NONE,
    READY_CHANGING,
    READY_FIRING
} weapon_ready_state_t;

static weapon_ready_state_t Weapon_HandleReady(edict_t *ent, int FRAME_FIRE_FIRST, int FRAME_IDLE_FIRST, int FRAME_IDLE_LAST, const int *pause_frames)
{
    if (ent->client->weaponstate == WEAPON_READY) {
        bool request_firing = ent->client->weapon_fire_buffered || ((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK);

        if (request_firing && ent->client->weapon_fire_finished <= level.time) {
            ent->client->latched_buttons &= ~BUTTON_ATTACK;
            ent->client->weapon_think_time = level.time;

            if ((!ent->client->pers.weapon->ammo) ||
                (ent->client->pers.inventory[ent->client->pers.weapon->ammo] >= ent->client->pers.weapon->quantity)) {
                ent->client->weaponstate = WEAPON_FIRING;
                ent->client->last_firing_time = level.time + COOP_DAMAGE_FIRING_TIME;
                return READY_FIRING;
            }
            NoAmmoWeaponChange(ent, true);
            return READY_CHANGING;
        }
        if (ent->client->weapon_think_time <= level.time) {
            ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);

            if (ent->client->ps.gunframe == FRAME_IDLE_LAST) {
                ent->client->ps.gunframe = FRAME_IDLE_FIRST;
                return READY_CHANGING;
            }

            if (pause_frames)
                for (int n = 0; pause_frames[n]; n++)
                    if (ent->client->ps.gunframe == pause_frames[n])
                        if (irandom1(16))
                            return READY_CHANGING;

            ent->client->ps.gunframe++;
            return READY_CHANGING;
        }
    }

    return READY_NONE;
}

static void Weapon_HandleFiring(edict_t *ent, int FRAME_IDLE_FIRST, const int *fire_frames, void (*fire)(edict_t *ent))
{
    Weapon_SetFinished(ent);

    if (ent->client->weapon_fire_buffered) {
        ent->client->buttons |= BUTTON_ATTACK;
        ent->client->weapon_fire_buffered = false;
    }

    if (fire_frames) {
        for (int n = 0; fire_frames[n]; n++) {
            if (ent->client->ps.gunframe == fire_frames[n]) {
                Weapon_PowerupSound(ent);
                fire(ent);
                break;
            }
        }
    } else {
        fire(ent);
    }

    if (ent->client->ps.gunframe == FRAME_IDLE_FIRST) {
        ent->client->weaponstate = WEAPON_READY;
        ent->client->weapon_fire_buffered = false;
    }

    ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);
}

void Weapon_Generic(edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, const int *pause_frames, const int *fire_frames, void (*fire)(edict_t *ent))
{
    int FRAME_FIRE_FIRST = (FRAME_ACTIVATE_LAST + 1);
    int FRAME_IDLE_FIRST = (FRAME_FIRE_LAST + 1);
    int FRAME_DEACTIVATE_FIRST = (FRAME_IDLE_LAST + 1);

    if (!Weapon_CanAnimate(ent))
        return;

    if (Weapon_HandleDropping(ent, FRAME_DEACTIVATE_LAST))
        return;
    if (Weapon_HandleActivating(ent, FRAME_ACTIVATE_LAST, FRAME_IDLE_FIRST))
        return;
    if (Weapon_HandleNewWeapon(ent, FRAME_DEACTIVATE_FIRST, FRAME_DEACTIVATE_LAST))
        return;
    weapon_ready_state_t state = Weapon_HandleReady(ent, FRAME_FIRE_FIRST, FRAME_IDLE_FIRST, FRAME_IDLE_LAST, pause_frames);
    if (state) {
        if (state == READY_FIRING) {
            ent->client->ps.gunframe = FRAME_FIRE_FIRST;
            ent->client->weapon_fire_buffered = false;

            if (ent->client->weapon_thunk)
                ent->client->weapon_think_time += FRAME_TIME;

            ent->client->weapon_think_time += Weapon_AnimationTime(ent);
            Weapon_SetFinished(ent);

            for (int n = 0; fire_frames[n]; n++) {
                if (ent->client->ps.gunframe == fire_frames[n]) {
                    Weapon_PowerupSound(ent);
                    fire(ent);
                    break;
                }
            }

            // start the animation
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

        return;
    }

    if (ent->client->weaponstate == WEAPON_FIRING && ent->client->weapon_think_time <= level.time) {
        ent->client->last_firing_time = level.time + COOP_DAMAGE_FIRING_TIME;
        ent->client->ps.gunframe++;
        Weapon_HandleFiring(ent, FRAME_IDLE_FIRST, fire_frames, fire);
    }
}

void Weapon_Repeating(edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, const int *pause_frames, void (*fire)(edict_t *ent))
{
    int FRAME_FIRE_FIRST = (FRAME_ACTIVATE_LAST + 1);
    int FRAME_IDLE_FIRST = (FRAME_FIRE_LAST + 1);
    int FRAME_DEACTIVATE_FIRST = (FRAME_IDLE_LAST + 1);

    if (!Weapon_CanAnimate(ent))
        return;

    if (Weapon_HandleDropping(ent, FRAME_DEACTIVATE_LAST))
        return;
    if (Weapon_HandleActivating(ent, FRAME_ACTIVATE_LAST, FRAME_IDLE_FIRST))
        return;
    if (Weapon_HandleNewWeapon(ent, FRAME_DEACTIVATE_FIRST, FRAME_DEACTIVATE_LAST))
        return;
    if (Weapon_HandleReady(ent, FRAME_FIRE_FIRST, FRAME_IDLE_FIRST, FRAME_IDLE_LAST, pause_frames) == READY_CHANGING)
        return;

    if (ent->client->weaponstate == WEAPON_FIRING && ent->client->weapon_think_time <= level.time) {
        ent->client->last_firing_time = level.time + COOP_DAMAGE_FIRING_TIME;
        Weapon_HandleFiring(ent, FRAME_IDLE_FIRST, NULL, fire);

        if (ent->client->weapon_thunk)
            ent->client->weapon_think_time += FRAME_TIME;
    }
}

/*
======================================================================

GRENADE

======================================================================
*/

static void weapon_grenade_fire(edict_t *ent, bool held)
{
    int   damage = 125;
    int   speed;
    float radius;

    radius = (float)(damage + 40);
    if (is_quad)
        damage *= damage_multiplier;

    // Paril: kill sideways angle on grenades
    // limit upwards angle so you don't throw behind you
    vec3_t angles;
    P_GetThrowAngles(ent, angles);

    vec3_t start, dir;
    P_ProjectSource(ent, angles, (const vec3_t) { 2, 0, -14 }, start, dir);

    gtime_t timer = ent->client->grenade_time - level.time;

    if (ent->health > 0) {
        float frac = 1.0f - TO_SEC(timer) / GRENADE_TIMER_SEC;
        speed = lerp(GRENADE_MINSPEED, GRENADE_MAXSPEED, min(frac, 1.0f));
    } else
        speed = GRENADE_MINSPEED;

    ent->client->grenade_time = 0;

    fire_grenade2(ent, start, dir, damage, speed, timer, radius, held);

    G_RemoveAmmoEx(ent, 1);
}

void Throw_Generic(edict_t *ent, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_PRIME_SOUND,
                   const char *prime_sound, int FRAME_THROW_HOLD, int FRAME_THROW_FIRE, const int *pause_frames,
                   bool explode, const char *primed_sound, void (*fire)(edict_t *ent, bool held), bool extra_idle_frame)
{
    // when we die, just toss what we had in our hands.
    if (ent->health <= 0) {
        fire(ent, true);
        return;
    }

    int n;
    int FRAME_IDLE_FIRST = (FRAME_FIRE_LAST + 1);

    if (ent->client->newweapon && (ent->client->weaponstate == WEAPON_READY)) {
        if (ent->client->weapon_think_time <= level.time) {
            ChangeWeapon(ent);
            ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);
        }
        return;
    }

    if (ent->client->weaponstate == WEAPON_ACTIVATING) {
        if (ent->client->weapon_think_time <= level.time) {
            ent->client->weaponstate = WEAPON_READY;
            if (!extra_idle_frame)
                ent->client->ps.gunframe = FRAME_IDLE_FIRST;
            else
                ent->client->ps.gunframe = FRAME_IDLE_LAST + 1;
            ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);
            Weapon_SetFinished(ent);
        }
        return;
    }

    if (ent->client->weaponstate == WEAPON_READY) {
        bool request_firing = ent->client->weapon_fire_buffered || ((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK);

        if (request_firing && ent->client->weapon_fire_finished <= level.time) {
            ent->client->latched_buttons &= ~BUTTON_ATTACK;

            if (ent->client->pers.inventory[ent->client->pers.weapon->ammo]) {
                ent->client->ps.gunframe = 1;
                ent->client->weaponstate = WEAPON_FIRING;
                ent->client->grenade_time = 0;
                ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);
            } else
                NoAmmoWeaponChange(ent, true);
            return;
        } else if (ent->client->weapon_think_time <= level.time) {
            ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);

            if (ent->client->ps.gunframe >= FRAME_IDLE_LAST) {
                ent->client->ps.gunframe = FRAME_IDLE_FIRST;
                return;
            }

            if (pause_frames) {
                for (n = 0; pause_frames[n]; n++) {
                    if (ent->client->ps.gunframe == pause_frames[n]) {
                        if (irandom1(16))
                            return;
                    }
                }
            }

            ent->client->ps.gunframe++;
        }
        return;
    }

    if (ent->client->weaponstate == WEAPON_FIRING) {
        ent->client->last_firing_time = level.time + COOP_DAMAGE_FIRING_TIME;

        if (ent->client->weapon_think_time <= level.time) {
            if (prime_sound && ent->client->ps.gunframe == FRAME_PRIME_SOUND)
                gi.sound(ent, CHAN_WEAPON, gi.soundindex(prime_sound), 1, ATTN_NORM, 0);

            // [Paril-KEX] dualfire/time accel
            gtime_t grenade_wait_time = SEC(1);

            if (CTFApplyHaste(ent))
                grenade_wait_time *= 0.5f;
            if (is_quadfire)
                grenade_wait_time *= 0.5f;

            if (ent->client->ps.gunframe == FRAME_THROW_HOLD) {
                if (!ent->client->grenade_time && !ent->client->grenade_finished_time)
                    ent->client->grenade_time = level.time + SEC(GRENADE_TIMER_SEC + 0.2f);

                if (primed_sound && !ent->client->grenade_blew_up)
                    ent->client->weapon_sound = gi.soundindex(primed_sound);

                // they waited too long, detonate it in their hand
                if (explode && !ent->client->grenade_blew_up && level.time >= ent->client->grenade_time) {
                    Weapon_PowerupSound(ent);
                    ent->client->weapon_sound = 0;
                    fire(ent, true);
                    ent->client->grenade_blew_up = true;

                    ent->client->grenade_finished_time = level.time + grenade_wait_time;
                }

                if (ent->client->buttons & BUTTON_ATTACK) {
                    ent->client->weapon_think_time = level.time + FRAME_TIME;
                    return;
                }

                if (ent->client->grenade_blew_up) {
                    if (level.time >= ent->client->grenade_finished_time) {
                        ent->client->ps.gunframe = FRAME_FIRE_LAST;
                        ent->client->grenade_blew_up = false;
                        ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);
                    } else {
                        return;
                    }
                } else {
                    ent->client->ps.gunframe++;

                    Weapon_PowerupSound(ent);
                    ent->client->weapon_sound = 0;
                    fire(ent, false);

                    if (!explode || !ent->client->grenade_blew_up)
                        ent->client->grenade_finished_time = level.time + grenade_wait_time;

                    if (!ent->deadflag && ent->s.modelindex == MODELINDEX_PLAYER && ent->health > 0) { // VWep animations screw up corpses
                        if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
                            ent->client->anim_priority = ANIM_ATTACK;
                            ent->s.frame = FRAME_crattak1 - 1;
                            ent->client->anim_end = FRAME_crattak3;
                        } else {
                            ent->client->anim_priority = ANIM_ATTACK | ANIM_REVERSED;
                            ent->s.frame = FRAME_wave08;
                            ent->client->anim_end = FRAME_wave01;
                        }
                        ent->client->anim_time = 0;
                    }
                }
            }

            ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);

            if ((ent->client->ps.gunframe == FRAME_FIRE_LAST) && (level.time < ent->client->grenade_finished_time))
                return;

            ent->client->ps.gunframe++;

            if (ent->client->ps.gunframe == FRAME_IDLE_FIRST) {
                ent->client->grenade_finished_time = 0;
                ent->client->weaponstate = WEAPON_READY;
                ent->client->weapon_fire_buffered = false;
                Weapon_SetFinished(ent);

                if (extra_idle_frame)
                    ent->client->ps.gunframe = FRAME_IDLE_LAST + 1;

                // Paril: if we ran out of the throwable, switch
                // so we don't appear to be holding one that we
                // can't throw
                if (!ent->client->pers.inventory[ent->client->pers.weapon->ammo]) {
                    NoAmmoWeaponChange(ent, false);
                    ChangeWeapon(ent);
                }
            }
        }
    }
}

void Weapon_Grenade(edict_t *ent)
{
    static const int pause_frames[] = { 29, 34, 39, 48, 0 };

    Throw_Generic(ent, 15, 48, 5, "weapons/hgrena1b.wav", 11, 12, pause_frames, true, "weapons/hgrenc1b.wav", weapon_grenade_fire, true);

    // [Paril-KEX] skip the duped frame
    if (ent->client->ps.gunframe == 1)
        ent->client->ps.gunframe = 2;
}

/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

void P_GetThrowAngles(edict_t *ent, vec3_t angles)
{
    VectorCopy(ent->client->v_angle, angles);
    angles[0] = max(angles[0], -62.5f);
}

static void weapon_grenadelauncher_fire(edict_t *ent)
{
    int   damage = 120;
    float radius;

    radius = (float)(damage + 40);
    if (is_quad)
        damage *= damage_multiplier;

    // Paril: kill sideways angle on grenades
    // limit upwards angle so you don't fire it behind you
    vec3_t angles;
    P_GetThrowAngles(ent, angles);

    vec3_t start, dir;
    P_ProjectSource(ent, angles, (const vec3_t) { 8, 0, -8 }, start, dir);

    P_AddWeaponKick(ent, -2, -1);

    fire_grenade(ent, start, dir, damage, 600, SEC(2.5f), radius, (crandom_open() * 10.0f), (200 + crandom_open() * 10.0f), false);

    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(ent - g_edicts);
    gi.WriteByte(MZ_GRENADE | is_silenced);
    gi.multicast(ent->s.origin, MULTICAST_PVS);

    PlayerNoise(ent, start, PNOISE_WEAPON);

    G_RemoveAmmo(ent);
}

void Weapon_GrenadeLauncher(edict_t *ent)
{
    static const int pause_frames[] = { 34, 51, 59, 0 };
    static const int fire_frames[] = { 6, 0 };

    Weapon_Generic(ent, 5, 16, 59, 64, pause_frames, fire_frames, weapon_grenadelauncher_fire);
}

/*
======================================================================

ROCKET

======================================================================
*/

static void Weapon_RocketLauncher_Fire(edict_t *ent)
{
    int   damage;
    float damage_radius;
    int   radius_damage;

    damage = irandom2(100, 120);
    radius_damage = 120;
    damage_radius = 120;
    if (is_quad) {
        damage *= damage_multiplier;
        radius_damage *= damage_multiplier;
    }

    vec3_t start, dir;
    P_ProjectSource(ent, ent->client->v_angle, (const vec3_t) { 8, 8, -8 }, start, dir);
    fire_rocket(ent, start, dir, damage, 650, damage_radius, radius_damage);

    P_AddWeaponKick(ent, -2, -1);

    // send muzzle flash
    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(ent - g_edicts);
    gi.WriteByte(MZ_ROCKET | is_silenced);
    gi.multicast(ent->s.origin, MULTICAST_PVS);

    PlayerNoise(ent, start, PNOISE_WEAPON);

    G_RemoveAmmo(ent);
}

void Weapon_RocketLauncher(edict_t *ent)
{
    static const int pause_frames[] = { 25, 33, 42, 50, 0 };
    static const int fire_frames[] = { 5, 0 };

    Weapon_Generic(ent, 4, 12, 50, 54, pause_frames, fire_frames, Weapon_RocketLauncher_Fire);
}

/*
======================================================================

BLASTER / HYPERBLASTER

======================================================================
*/

static void Blaster_Fire(edict_t *ent, const vec3_t g_offset, int damage, bool hyper, effects_t effect)
{
    if (is_quad)
        damage *= damage_multiplier;

    vec3_t offset = { 24, 8, -8 };
    VectorAdd(offset, g_offset, offset);

    vec3_t start, dir;
    P_ProjectSource(ent, ent->client->v_angle, offset, start, dir);

    P_AddWeaponKick(ent, -2, -1);
    if (hyper)
        crandom_vec(ent->client->kick.angles, 0.7f);

    // let the regular blaster projectiles travel a bit faster because it is a completely useless gun
    int speed = hyper ? 1000 : 1500;

    fire_blaster(ent, start, dir, damage, speed, effect, (mod_t) { hyper ? MOD_HYPERBLASTER : MOD_BLASTER });

    // send muzzle flash
    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(ent - g_edicts);
    if (hyper)
        gi.WriteByte(MZ_HYPERBLASTER | is_silenced);
    else
        gi.WriteByte(MZ_BLASTER | is_silenced);
    gi.multicast(ent->s.origin, MULTICAST_PVS);

    PlayerNoise(ent, start, PNOISE_WEAPON);
}

static void Weapon_Blaster_Fire(edict_t *ent)
{
    // give the blaster 15 across the board instead of just in dm
    int damage = 15;
    Blaster_Fire(ent, vec3_origin, damage, false, EF_BLASTER);
}

void Weapon_Blaster(edict_t *ent)
{
    static const int pause_frames[] = { 19, 32, 0 };
    static const int fire_frames[] = { 5, 0 };

    Weapon_Generic(ent, 4, 8, 52, 55, pause_frames, fire_frames, Weapon_Blaster_Fire);
}

static void Weapon_HyperBlaster_Fire(edict_t *ent)
{
    float     rotation;
    vec3_t    offset;
    int       damage;

    // start on frame 6
    if (ent->client->ps.gunframe > 20)
        ent->client->ps.gunframe = 6;
    else
        ent->client->ps.gunframe++;

    // if we reached end of loop, have ammo & holding attack, reset loop
    // otherwise play wind down
    if (ent->client->ps.gunframe == 12) {
        if (ent->client->pers.inventory[ent->client->pers.weapon->ammo] && (ent->client->buttons & BUTTON_ATTACK))
            ent->client->ps.gunframe = 6;
        else
            gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/hyprbd1a.wav"), 1, ATTN_NORM, 0);
    }

    // play weapon sound for firing loop
    if (ent->client->ps.gunframe >= 6 && ent->client->ps.gunframe <= 11)
        ent->client->weapon_sound = gi.soundindex("weapons/hyprbl1a.wav");
    else
        ent->client->weapon_sound = 0;

    // fire frames
    bool request_firing = ent->client->weapon_fire_buffered || (ent->client->buttons & BUTTON_ATTACK);

    if (request_firing) {
        if (ent->client->ps.gunframe >= 6 && ent->client->ps.gunframe <= 11) {
            ent->client->weapon_fire_buffered = false;

            if (!ent->client->pers.inventory[ent->client->pers.weapon->ammo]) {
                NoAmmoWeaponChange(ent, true);
                return;
            }

            rotation = (ent->client->ps.gunframe - 5) * 2 * M_PIf / 6;
            offset[0] = -4 * sinf(rotation);
            offset[2] = 0;
            offset[1] = 4 * cosf(rotation);

            if (deathmatch->integer)
                damage = 15;
            else
                damage = 20;
            Blaster_Fire(ent, offset, damage, true, (ent->client->ps.gunframe % 4) ? EF_NONE : EF_HYPERBLASTER);
            Weapon_PowerupSound(ent);

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
    }
}

void Weapon_HyperBlaster(edict_t *ent)
{
    static const int pause_frames[] = { 0 };

    Weapon_Repeating(ent, 5, 20, 49, 53, pause_frames, Weapon_HyperBlaster_Fire);
}

/*
======================================================================

MACHINEGUN / CHAINGUN

======================================================================
*/

static void Machinegun_Fire(edict_t *ent)
{
    int i;
    int damage = 8;
    int kick = 2;

    if (!(ent->client->buttons & BUTTON_ATTACK)) {
        ent->client->ps.gunframe = 6;
        return;
    }

    if (ent->client->ps.gunframe == 4)
        ent->client->ps.gunframe = 5;
    else
        ent->client->ps.gunframe = 4;

    if (ent->client->pers.inventory[ent->client->pers.weapon->ammo] < 1) {
        ent->client->ps.gunframe = 6;
        NoAmmoWeaponChange(ent, true);
        return;
    }

    if (is_quad) {
        damage *= damage_multiplier;
        kick *= damage_multiplier;
    }

    for (i = 0; i < 3; i++) {
        ent->client->kick.origin[i] = crandom() * 0.35f;
        ent->client->kick.angles[i] = crandom() * 0.7f;
    }
    ent->client->kick.total = KICK_TIME;
    ent->client->kick.time = level.time + ent->client->kick.total;

    // get start / end positions
    vec3_t start, dir;
    // Paril: kill sideways angle on hitscan
    P_ProjectSource(ent, ent->client->v_angle, (const vec3_t) { 0, 0, -8 }, start, dir);
    fire_bullet(ent, start, dir, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, (mod_t) { MOD_MACHINEGUN });
    Weapon_PowerupSound(ent);

    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(ent - g_edicts);
    gi.WriteByte(MZ_MACHINEGUN | is_silenced);
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

void Weapon_Machinegun(edict_t *ent)
{
    static const int pause_frames[] = { 23, 45, 0 };

    Weapon_Repeating(ent, 3, 5, 45, 49, pause_frames, Machinegun_Fire);
}

static void Chaingun_Fire(edict_t *ent)
{
    int   i;
    int   shots;
    float r, u;
    int   damage;
    int   kick = 2;

    if (deathmatch->integer)
        damage = 6;
    else
        damage = 8;

    if (ent->client->ps.gunframe > 31) {
        ent->client->ps.gunframe = 5;
        gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);
    } else if ((ent->client->ps.gunframe == 14) && !(ent->client->buttons & BUTTON_ATTACK)) {
        ent->client->ps.gunframe = 32;
        ent->client->weapon_sound = 0;
        return;
    } else if ((ent->client->ps.gunframe == 21) && (ent->client->buttons & BUTTON_ATTACK) && ent->client->pers.inventory[ent->client->pers.weapon->ammo]) {
        ent->client->ps.gunframe = 15;
    } else {
        ent->client->ps.gunframe++;
    }

    if (ent->client->ps.gunframe == 22) {
        ent->client->weapon_sound = 0;
        gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnd1a.wav"), 1, ATTN_IDLE, 0);
    }

    if (ent->client->ps.gunframe < 5 || ent->client->ps.gunframe > 21)
        return;

    ent->client->weapon_sound = gi.soundindex("weapons/chngnl1a.wav");

    ent->client->anim_priority = ANIM_ATTACK;
    if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
        ent->s.frame = FRAME_crattak1 - (ent->client->ps.gunframe & 1);
        ent->client->anim_end = FRAME_crattak9;
    } else {
        ent->s.frame = FRAME_attack1 - (ent->client->ps.gunframe & 1);
        ent->client->anim_end = FRAME_attack8;
    }
    ent->client->anim_time = 0;

    if (ent->client->ps.gunframe <= 9)
        shots = 1;
    else if (ent->client->ps.gunframe <= 14) {
        if (ent->client->buttons & BUTTON_ATTACK)
            shots = 2;
        else
            shots = 1;
    } else
        shots = 3;

    if (ent->client->pers.inventory[ent->client->pers.weapon->ammo] < shots)
        shots = ent->client->pers.inventory[ent->client->pers.weapon->ammo];

    if (!shots) {
        NoAmmoWeaponChange(ent, true);
        return;
    }

    if (is_quad) {
        damage *= damage_multiplier;
        kick *= damage_multiplier;
    }

    for (i = 0; i < 3; i++) {
        ent->client->kick.origin[i] = crandom() * 0.35f;
        ent->client->kick.angles[i] = crandom() * (0.5f + (shots * 0.15f));
    }
    ent->client->kick.total = KICK_TIME;
    ent->client->kick.time = level.time + ent->client->kick.total;

    vec3_t start, dir;
    P_ProjectSource(ent, ent->client->v_angle, (const vec3_t) { 0, 0, -8 }, start, dir);

    for (i = 0; i < shots; i++) {
        // get start / end positions
        // Paril: kill sideways angle on hitscan
        r = crandom() * 4;
        u = crandom() * 4;
        P_ProjectSource(ent, ent->client->v_angle, (const vec3_t) { 0, r, u - 8 }, start, dir);

        fire_bullet(ent, start, dir, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, (mod_t) { MOD_CHAINGUN });
    }

    Weapon_PowerupSound(ent);

    // send muzzle flash
    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(ent - g_edicts);
    gi.WriteByte((MZ_CHAINGUN1 + shots - 1) | is_silenced);
    gi.multicast(ent->s.origin, MULTICAST_PVS);

    PlayerNoise(ent, start, PNOISE_WEAPON);

    G_RemoveAmmoEx(ent, shots);
}

void Weapon_Chaingun(edict_t *ent)
{
    static const int pause_frames[] = { 38, 43, 51, 61, 0 };

    Weapon_Repeating(ent, 4, 31, 61, 64, pause_frames, Chaingun_Fire);
}

/*
======================================================================

SHOTGUN / SUPERSHOTGUN

======================================================================
*/

static void weapon_shotgun_fire(edict_t *ent)
{
    int damage = 4;
    int kick = 8;

    vec3_t start, dir;
    // Paril: kill sideways angle on hitscan
    P_ProjectSource(ent, ent->client->v_angle, (const vec3_t) { 0, 0, -8 }, start, dir);

    P_AddWeaponKick(ent, -2, -2);

    if (is_quad) {
        damage *= damage_multiplier;
        kick *= damage_multiplier;
    }

    if (deathmatch->integer)
        fire_shotgun(ent, start, dir, damage, kick, 500, 500, DEFAULT_DEATHMATCH_SHOTGUN_COUNT, (mod_t) { MOD_SHOTGUN });
    else
        fire_shotgun(ent, start, dir, damage, kick, 500, 500, DEFAULT_SHOTGUN_COUNT, (mod_t) { MOD_SHOTGUN });

    // send muzzle flash
    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(ent - g_edicts);
    gi.WriteByte(MZ_SHOTGUN | is_silenced);
    gi.multicast(ent->s.origin, MULTICAST_PVS);

    PlayerNoise(ent, start, PNOISE_WEAPON);

    G_RemoveAmmo(ent);
}

void Weapon_Shotgun(edict_t *ent)
{
    static const int pause_frames[] = { 22, 28, 34, 0 };
    static const int fire_frames[] = { 8, 0 };

    Weapon_Generic(ent, 7, 18, 36, 39, pause_frames, fire_frames, weapon_shotgun_fire);
}

static void weapon_supershotgun_fire(edict_t *ent)
{
    int damage = 6;
    int kick = 12;

    if (is_quad) {
        damage *= damage_multiplier;
        kick *= damage_multiplier;
    }

    vec3_t start, dir, v;
    v[PITCH] = ent->client->v_angle[PITCH];
    v[YAW] = ent->client->v_angle[YAW] - 5;
    v[ROLL] = ent->client->v_angle[ROLL];
    // Paril: kill sideways angle on hitscan
    P_ProjectSource(ent, v, (const vec3_t) { 0, 0, -8 }, start, dir);
    fire_shotgun(ent, start, dir, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, (mod_t) { MOD_SSHOTGUN });
    v[YAW] = ent->client->v_angle[YAW] + 5;
    P_ProjectSource(ent, v, (const vec3_t) { 0, 0, -8 }, start, dir);
    fire_shotgun(ent, start, dir, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, (mod_t) { MOD_SSHOTGUN });

    P_AddWeaponKick(ent, -2, -2);

    // send muzzle flash
    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(ent - g_edicts);
    gi.WriteByte(MZ_SSHOTGUN | is_silenced);
    gi.multicast(ent->s.origin, MULTICAST_PVS);

    PlayerNoise(ent, start, PNOISE_WEAPON);

    G_RemoveAmmo(ent);
}

void Weapon_SuperShotgun(edict_t *ent)
{
    static const int pause_frames[] = { 29, 42, 57, 0 };
    static const int fire_frames[] = { 7, 0 };

    Weapon_Generic(ent, 6, 17, 57, 61, pause_frames, fire_frames, weapon_supershotgun_fire);
}

/*
======================================================================

RAILGUN

======================================================================
*/

static void weapon_railgun_fire(edict_t *ent)
{
    int damage, kick;

    // normal damage too extreme for DM
    if (deathmatch->integer) {
        damage = 100;
        kick = 200;
    } else {
        damage = 125;
        kick = 225;
    }

    if (is_quad) {
        damage *= damage_multiplier;
        kick *= damage_multiplier;
    }

    vec3_t start, dir;
    P_ProjectSource(ent, ent->client->v_angle, (const vec3_t) { 0, 7, -8 }, start, dir);
    fire_rail(ent, start, dir, damage, kick);

    P_AddWeaponKick(ent, -3, -3);

    // send muzzle flash
    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(ent - g_edicts);
    gi.WriteByte(MZ_RAILGUN | is_silenced);
    gi.multicast(ent->s.origin, MULTICAST_PVS);

    PlayerNoise(ent, start, PNOISE_WEAPON);

    G_RemoveAmmo(ent);
}

void Weapon_Railgun(edict_t *ent)
{
    static const int pause_frames[] = { 56, 0 };
    static const int fire_frames[] = { 4, 0 };

    Weapon_Generic(ent, 3, 18, 56, 61, pause_frames, fire_frames, weapon_railgun_fire);
}

/*
======================================================================

BFG10K

======================================================================
*/

static void weapon_bfg_fire(edict_t *ent)
{
    int   damage;
    float damage_radius = 1000;

    if (deathmatch->integer)
        damage = 200;
    else
        damage = 500;

    if (ent->client->ps.gunframe == 9) {
        // send muzzle flash
        gi.WriteByte(svc_muzzleflash);
        gi.WriteShort(ent - g_edicts);
        gi.WriteByte(MZ_BFG | is_silenced);
        gi.multicast(ent->s.origin, MULTICAST_PVS);

        PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);
        return;
    }

    // cells can go down during windup (from power armor hits), so
    // check again and abort firing if we don't have enough now
    if (ent->client->pers.inventory[ent->client->pers.weapon->ammo] < 50)
        return;

    if (is_quad)
        damage *= damage_multiplier;

    vec3_t start, dir;
    P_ProjectSource(ent, ent->client->v_angle, (const vec3_t) { 8, 8, -8 }, start, dir);
    fire_bfg(ent, start, dir, damage, 400, damage_radius);

    VectorScale(ent->client->v_forward, -2, ent->client->kick.origin);
    VectorSet(ent->client->kick.angles, -20, 0, crandom() * 8);
    ent->client->kick.total = DAMAGE_TIME;
    ent->client->kick.time = level.time + ent->client->kick.total;

    // send muzzle flash
    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(ent - g_edicts);
    gi.WriteByte(MZ_BFG2 | is_silenced);
    gi.multicast(ent->s.origin, MULTICAST_PVS);

    PlayerNoise(ent, start, PNOISE_WEAPON);

    G_RemoveAmmo(ent);
}

void Weapon_BFG(edict_t *ent)
{
    static const int pause_frames[] = { 39, 45, 50, 55, 0 };
    static const int fire_frames[] = { 9, 17, 0 };

    Weapon_Generic(ent, 8, 32, 54, 58, pause_frames, fire_frames, weapon_bfg_fire);
}

//======================================================================

static void weapon_disint_fire(edict_t *self)
{
    vec3_t start, dir;
    P_ProjectSource(self, self->client->v_angle, (const vec3_t) { 24, 8, -8 }, start, dir);

    P_AddWeaponKick(self, -2, -1);

    fire_disintegrator(self, start, dir, 800);

    // send muzzle flash
    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(self - g_edicts);
    gi.WriteByte(MZ_BLASTER2);
    gi.multicast(self->s.origin, MULTICAST_PVS);

    PlayerNoise(self, start, PNOISE_WEAPON);

    G_RemoveAmmo(self);
}

void Weapon_Beta_Disintegrator(edict_t *ent)
{
    static const int pause_frames[] = { 30, 37, 45, 0 };
    static const int fire_frames[] = { 17, 0 };

    Weapon_Generic(ent, 16, 23, 46, 50, pause_frames, fire_frames, weapon_disint_fire);
}
