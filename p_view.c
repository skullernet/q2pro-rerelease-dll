// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"
#include "m_player.h"

static edict_t   *current_player;
static gclient_t *current_client;

static vec3_t forward, right, up;
float         xyspeed;

static float bobmove;
static int   bobcycle, bobcycle_run;     // odd cycles are right foot going forward
static float bobfracsin; // sinf(bobfrac*M_PI)

/*
===============
SkipViewModifiers
===============
*/
static bool SkipViewModifiers(void)
{
    if (g_skipViewModifiers->integer && sv_cheats->integer)
        return true;

    // don't do bobbing, etc on grapple
    if (current_client->ctf_grapple && current_client->ctf_grapplestate > CTF_GRAPPLE_STATE_FLY)
        return true;

    // spectator mode
    if (current_client->resp.spectator || (G_TeamplayEnabled() && current_client->resp.ctf_team == CTF_NOTEAM))
        return true;

    return false;
}

/*
===============
SV_CalcRoll

===============
*/
static float SV_CalcRoll(const vec3_t angles, const vec3_t velocity)
{
    if (SkipViewModifiers())
        return 0.0f;

    float sign;
    float side;
    float value;

    side = DotProduct(velocity, right);
    sign = side < 0 ? -1 : 1;
    side = fabsf(side);

    value = sv_rollangle->value;

    if (side < sv_rollspeed->value)
        side = side * value / sv_rollspeed->value;
    else
        side = value;

    return side * sign;
}

/*
===============
P_DamageFeedback

Handles color blends and view kicks
===============
*/
static void P_DamageFeedback(edict_t *player)
{
    gclient_t   *client;
    float        side;
    float        realcount, count, kick;
    vec3_t       v;
    int          l;
    static const vec3_t armor_color = { 1.0f, 1.0f, 1.0f };
    static const vec3_t power_color = { 0.0f, 1.0f, 0.0f };
    static const vec3_t bcolor = { 1.0f, 0.0f, 0.0f };

    client = player->client;

    // flash the backgrounds behind the status numbers
    int want_flashes = 0;

    if (client->damage_blood)
        want_flashes |= 1;
    if (client->damage_armor && !(player->flags & FL_GODMODE) && (client->invincible_time <= level.time))
        want_flashes |= 2;

    if (want_flashes) {
        client->flash_time = level.time + HZ(10);
        client->ps.stats[STAT_FLASHES] = want_flashes;
    } else if (client->flash_time < level.time)
        client->ps.stats[STAT_FLASHES] = 0;

    // total points of damage shot at the player this frame
    count = client->damage_blood + client->damage_armor + client->damage_parmor;
    if (count == 0)
        return; // didn't take any damage

    // start a pain animation if still in the player model
    if (client->anim_priority < ANIM_PAIN && player->s.modelindex == MODELINDEX_PLAYER) {
        static int i;

        client->anim_priority = ANIM_PAIN;
        if (client->ps.pmove.pm_flags & PMF_DUCKED) {
            player->s.frame = FRAME_crpain1 - 1;
            client->anim_end = FRAME_crpain4;
        } else {
            i = (i + 1) % 3;
            switch (i) {
            case 0:
                player->s.frame = FRAME_pain101 - 1;
                client->anim_end = FRAME_pain104;
                break;
            case 1:
                player->s.frame = FRAME_pain201 - 1;
                client->anim_end = FRAME_pain204;
                break;
            case 2:
                player->s.frame = FRAME_pain301 - 1;
                client->anim_end = FRAME_pain304;
                break;
            }
        }

        client->anim_time = 0;
    }

    realcount = count;

    // if we took health damage, do a minimum clamp
    if (client->damage_blood) {
        if (count < 10)
            count = 10; // always make a visible effect
    } else {
        if (count > 2)
            count = 2; // don't go too deep
    }

    // play an appropriate pain sound
    if ((level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE) && (client->invincible_time <= level.time)) {
        player->pain_debounce_time = level.time + SEC(0.7f);

        static const char *const pain_sounds[] = {
            "*pain25_1.wav",
            "*pain25_2.wav",
            "*pain50_1.wav",
            "*pain50_2.wav",
            "*pain75_1.wav",
            "*pain75_2.wav",
            "*pain100_1.wav",
            "*pain100_2.wav",
        };

        if (player->health < 25)
            l = 0;
        else if (player->health < 50)
            l = 2;
        else if (player->health < 75)
            l = 4;
        else
            l = 6;

        l |= brandom();

        gi.sound(player, CHAN_VOICE, gi.soundindex(pain_sounds[l]), 1, ATTN_NORM, 0);
        // Paril: pain noises alert monsters
        PlayerNoise(player, player->s.origin, PNOISE_SELF);
    }

    // the total alpha of the blend is always proportional to count
    if (client->damage_alpha < 0)
        client->damage_alpha = 0;

    // [Paril-KEX] tweak the values to rely less on this
    // and more on damage indicators
    if (client->damage_blood || (client->damage_alpha + count * 0.06f) < 0.15f) {
        client->damage_alpha += count * 0.06f;
        client->damage_alpha = Q_clipf(client->damage_alpha, 0.06f, 0.4f); // don't go too saturated
    }

    // mix in colors
    VectorClear(v);

    if (client->damage_parmor)
        VectorMA(v, client->damage_parmor / realcount, power_color, v);
    if (client->damage_armor)
        VectorMA(v, client->damage_armor / realcount, armor_color, v);
    if (client->damage_blood) {
        float f = max(15.0f, client->damage_blood / realcount);
        VectorMA(v, f, bcolor, v);
    }
    VectorNormalize2(v, client->damage_blend);

    //
    // calculate view angle kicks
    //
    kick = abs(client->damage_knockback);
    if (kick && player->health > 0) { // kick of 0 means no view adjust at all
        kick = kick * 100 / player->health;

        if (kick < count * 0.5f)
            kick = count * 0.5f;
        if (kick > 50)
            kick = 50;

        VectorSubtract(client->damage_from, player->s.origin, v);
        VectorNormalize(v);

        side = DotProduct(v, right);
        client->v_dmg_roll = kick * side * 0.3f;

        side = -DotProduct(v, forward);
        client->v_dmg_pitch = kick * side * 0.3f;

        client->v_dmg_time = level.time + DAMAGE_TIME;
    }

    //
    // clear totals
    //
    client->damage_blood = 0;
    client->damage_armor = 0;
    client->damage_parmor = 0;
    client->damage_knockback = 0;
}

/*
===============
SV_CalcViewOffset

Auto pitching on slopes?

  fall from 128: 400 = 160000
  fall from 256: 580 = 336400
  fall from 384: 720 = 518400
  fall from 512: 800 = 640000
  fall from 640: 960 =

  damage = deltavelocity*deltavelocity  * 0.0001

===============
*/
static void SV_CalcViewOffset(edict_t *ent)
{
    float  bob;
    float  ratio;
    float  delta;
    vec3_t v;

    //===================================

    float kick_factor = 0;

    // [Paril-KEX] kicks in vanilla take place over 2 10hz server
    // frames; this is to mimic that visual behavior on any tickrate.
    if (ent->client->kick.time > level.time)
        kick_factor = (float)(ent->client->kick.time - level.time) / ent->client->kick.total;

    // base angles
    vec_t *angles = ent->client->ps.kick_angles;

    // if dead, fix the angle and don't add any kick
    if (ent->deadflag && !ent->client->resp.spectator) {
        VectorClear(angles);

        if (ent->flags & FL_SAM_RAIMI) {
            ent->client->ps.viewangles[ROLL] = 0;
            ent->client->ps.viewangles[PITCH] = 0;
        } else {
            ent->client->ps.viewangles[ROLL] = 40;
            ent->client->ps.viewangles[PITCH] = -15;
        }
        ent->client->ps.viewangles[YAW] = ent->client->killer_yaw;
    } else if (!ent->client->pers.bob_skip && !SkipViewModifiers()) {
        // add angles based on weapon kick
        VectorScale(ent->client->kick.angles, kick_factor, angles);

        // add angles based on damage kick
        if (ent->client->v_dmg_time > level.time) {
            ratio = (float)(ent->client->v_dmg_time - level.time) / DAMAGE_TIME;
            angles[PITCH] += ratio * ent->client->v_dmg_pitch;
            angles[ROLL] += ratio * ent->client->v_dmg_roll;
        }

        // add pitch based on fall kick
        if (ent->client->fall_time > level.time) {
            ratio = (float)(ent->client->fall_time - level.time) / FALL_TIME;
            angles[PITCH] += ratio * ent->client->fall_value;
        }

        // add angles based on velocity
        if (!ent->client->pers.bob_skip && !SkipViewModifiers()) {
            delta = DotProduct(ent->velocity, forward);
            angles[PITCH] += delta * run_pitch->value;

            delta = DotProduct(ent->velocity, right);
            angles[ROLL] += delta * run_roll->value;

            // add angles based on bob
            delta = bobfracsin * bob_pitch->value * xyspeed;
            if ((ent->client->ps.pmove.pm_flags & PMF_DUCKED) && ent->groundentity)
                delta *= 6; // crouching
            delta = min(delta, 1.2f);
            angles[PITCH] += delta;
            delta = bobfracsin * bob_roll->value * xyspeed;
            if ((ent->client->ps.pmove.pm_flags & PMF_DUCKED) && ent->groundentity)
                delta *= 6; // crouching
            delta = min(delta, 1.2f);
            if (bobcycle & 1)
                delta = -delta;
            angles[ROLL] += delta;
        }

        // add earthquake angles
        if (ent->client->quake_time > level.time) {
            float factor = ((float)ent->client->quake_time / level.time) * 0.25f;
            factor = min(1.0f, factor);

            angles[0] += crandom_open() * factor;
            angles[1] += crandom_open() * factor;
            angles[2] += crandom_open() * factor;
        }
    }

    // [Paril-KEX] clamp angles
    for (int i = 0; i < 3; i++)
        angles[i] = Q_clipf(angles[i], -31, 31);

    //===================================

    // base origin

    VectorClear(v);

    // add view height

    v[2] += ent->viewheight;

    if (!ent->client->pers.bob_skip && !SkipViewModifiers()) {
        // add fall height
        if (ent->client->fall_time > level.time) {
            ratio = (float)(ent->client->fall_time - level.time) / FALL_TIME;
            v[2] -= ratio * ent->client->fall_value * 0.4f;
        }

        // add bob height
        bob = bobfracsin * xyspeed * bob_up->value;
        if (bob > 6)
            bob = 6;
        v[2] += bob;

        // add kick offset
        VectorMA(v, kick_factor, ent->client->kick.origin, v);
    }

    // absolutely bound offsets
    // so the view can never be outside the player box

    ent->client->ps.viewoffset[0] = Q_clipf(v[0], -14, 14);
    ent->client->ps.viewoffset[1] = Q_clipf(v[1], -14, 14);
    ent->client->ps.viewoffset[2] = Q_clipf(v[2], -22, 30);
}

/*
==============
SV_CalcGunOffset
==============
*/
static void SV_CalcGunOffset(edict_t *ent)
{
    int   i;

    // ROGUE
    // ROGUE - heatbeam shouldn't bob so the beam looks right
    if (ent->client->pers.weapon &&
        !((ent->client->pers.weapon->id == IT_WEAPON_PLASMABEAM || ent->client->pers.weapon->id == IT_WEAPON_GRAPPLE) && ent->client->weaponstate == WEAPON_FIRING)
        && !SkipViewModifiers()) {
    // ROGUE
        // gun angles from bobbing
        ent->client->ps.gunangles[ROLL] = xyspeed * bobfracsin * 0.005f;
        ent->client->ps.gunangles[YAW] = xyspeed * bobfracsin * 0.01f;
        if (bobcycle & 1) {
            ent->client->ps.gunangles[ROLL] = -ent->client->ps.gunangles[ROLL];
            ent->client->ps.gunangles[YAW] = -ent->client->ps.gunangles[YAW];
        }

        ent->client->ps.gunangles[PITCH] = xyspeed * bobfracsin * 0.005f;

        vec3_t viewangles_delta;
        VectorSubtract(ent->client->oldviewangles, ent->client->ps.viewangles, viewangles_delta);

        VectorAdd(ent->client->slow_view_angles, viewangles_delta, ent->client->slow_view_angles);

        // gun angles from delta movement
        for (i = 0; i < 3; i++) {
            float d = ent->client->slow_view_angles[i];

            if (!d)
                continue;

            if (d > 180)
                d -= 360;
            if (d < -180)
                d += 360;

            d = Q_clipf(d, -45, 45);

            // [Sam-KEX] Apply only half-delta. Makes the weapons look less detatched from the player.
            if (i == ROLL)
                ent->client->ps.gunangles[i] += (0.1f * d) * 0.5f;
            else
                ent->client->ps.gunangles[i] += (0.2f * d) * 0.5f;

            float reduction_factor = viewangles_delta[i] ? 50 : 150;

            if (d > 0)
                d = max(0, d - FRAME_TIME_SEC * reduction_factor);
            else if (d < 0)
                d = min(0, d + FRAME_TIME_SEC * reduction_factor);

            ent->client->slow_view_angles[i] = d;
        }
    // ROGUE
    } else {
        VectorClear(ent->client->ps.gunangles);
    }
    // ROGUE

    // gun height
    VectorClear(ent->client->ps.gunoffset);

    // gun_x / gun_y / gun_z are development tools
    for (i = 0; i < 3; i++) {
        ent->client->ps.gunoffset[i] += forward[i] * gun_y->value;
        ent->client->ps.gunoffset[i] += right[i] * gun_x->value;
        ent->client->ps.gunoffset[i] += up[i] * -gun_z->value;
    }
}

/*
=============
G_AddBlend
=============
*/
static void G_AddBlend(float r, float g, float b, float a, vec4_t v_blend)
{
    if (a <= 0)
        return;

    float a2 = v_blend[3] + (1 - v_blend[3]) * a; // new total alpha
    float a3 = v_blend[3] / a2; // fraction of color from old

    v_blend[0] = v_blend[0] * a3 + r * (1 - a3);
    v_blend[1] = v_blend[1] * a3 + g * (1 - a3);
    v_blend[2] = v_blend[2] * a3 + b * (1 - a3);
    v_blend[3] = a2;
}

/*
=============
SV_CalcBlend
=============
*/
static void SV_CalcBlend(edict_t *ent)
{
    gtime_t remaining;

    Vector4Clear(ent->client->ps.blend);

    // add for contents
    vec3_t vieworg;
    VectorAdd(ent->s.origin, ent->client->ps.viewoffset, vieworg);
    contents_t contents = gi.pointcontents(vieworg);

    if (contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER))
        ent->client->ps.rdflags |= RDF_UNDERWATER;
    else
        ent->client->ps.rdflags &= ~RDF_UNDERWATER;

    if (contents & (CONTENTS_SOLID | CONTENTS_LAVA))
        G_AddBlend(1.0f, 0.3f, 0.0f, 0.6f, ent->client->ps.blend);
    else if (contents & CONTENTS_SLIME)
        G_AddBlend(0.0f, 0.1f, 0.05f, 0.6f, ent->client->ps.blend);
    else if (contents & CONTENTS_WATER)
        G_AddBlend(0.5f, 0.3f, 0.2f, 0.4f, ent->client->ps.blend);

    // add for powerups
    if (ent->client->quad_time > level.time) {
        remaining = ent->client->quad_time - level.time;
        if (remaining == SEC(3)) // beginning to fade
            gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage2.wav"), 1, ATTN_NORM, 0);
        if (G_PowerUpExpiringRelative(remaining))
            G_AddBlend(0, 0, 1, 0.08f, ent->client->ps.blend);
    // RAFAEL
    } else if (ent->client->quadfire_time > level.time) {
        remaining = ent->client->quadfire_time - level.time;
        if (remaining == SEC(3)) // beginning to fade
            gi.sound(ent, CHAN_ITEM, gi.soundindex("items/quadfire2.wav"), 1, ATTN_NORM, 0);
        if (G_PowerUpExpiringRelative(remaining))
            G_AddBlend(1, 0.2f, 0.5f, 0.08f, ent->client->ps.blend);
    // RAFAEL
    // PMM - double damage
    } else if (ent->client->double_time > level.time) {
        remaining = ent->client->double_time - level.time;
        if (remaining == SEC(3)) // beginning to fade
            gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/ddamage2.wav"), 1, ATTN_NORM, 0);
        if (G_PowerUpExpiringRelative(remaining))
            G_AddBlend(0.9f, 0.7f, 0, 0.08f, ent->client->ps.blend);
    // PMM
    } else if (ent->client->invincible_time > level.time) {
        remaining = ent->client->invincible_time - level.time;
        if (remaining == SEC(3)) // beginning to fade
            gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect2.wav"), 1, ATTN_NORM, 0);
        if (G_PowerUpExpiringRelative(remaining))
            G_AddBlend(1, 1, 0, 0.08f, ent->client->ps.blend);
    } else if (ent->client->invisible_time > level.time) {
        remaining = ent->client->invisible_time - level.time;
        if (remaining == SEC(3)) // beginning to fade
            gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect2.wav"), 1, ATTN_NORM, 0);
        if (G_PowerUpExpiringRelative(remaining))
            G_AddBlend(0.8f, 0.8f, 0.8f, 0.08f, ent->client->ps.blend);
    } else if (ent->client->enviro_time > level.time) {
        remaining = ent->client->enviro_time - level.time;
        if (remaining == SEC(3)) // beginning to fade
            gi.sound(ent, CHAN_ITEM, gi.soundindex("items/airout.wav"), 1, ATTN_NORM, 0);
        if (G_PowerUpExpiringRelative(remaining))
            G_AddBlend(0, 1, 0, 0.08f, ent->client->ps.blend);
    } else if (ent->client->breather_time > level.time) {
        remaining = ent->client->breather_time - level.time;
        if (remaining == SEC(3)) // beginning to fade
            gi.sound(ent, CHAN_ITEM, gi.soundindex("items/airout.wav"), 1, ATTN_NORM, 0);
        if (G_PowerUpExpiringRelative(remaining))
            G_AddBlend(0.4f, 1, 0.4f, 0.04f, ent->client->ps.blend);
    }

    // PGM
    if (ent->client->nuke_time > level.time) {
        float brightness = TO_SEC(ent->client->nuke_time - level.time) / 2.0f;
        G_AddBlend(1, 1, 1, brightness, ent->client->ps.blend);
    }
    if (ent->client->ir_time > level.time) {
        remaining = ent->client->ir_time - level.time;
        if (G_PowerUpExpiringRelative(remaining)) {
            ent->client->ps.rdflags |= RDF_IRGOGGLES;
            G_AddBlend(1, 0, 0, 0.2f, ent->client->ps.blend);
        } else
            ent->client->ps.rdflags &= ~RDF_IRGOGGLES;
    } else {
        ent->client->ps.rdflags &= ~RDF_IRGOGGLES;
    }
    // PGM

    // add for damage
    if (ent->client->damage_alpha > 0)
        G_AddBlend(ent->client->damage_blend[0], ent->client->damage_blend[1], ent->client->damage_blend[2], ent->client->damage_alpha, ent->client->ps.blend);

    // [Paril-KEX] drowning visual indicator
    if (ent->air_finished < level.time + SEC(9)) {
        float alpha = 1.0f;
        if (ent->air_finished > level.time)
            alpha = TO_SEC(ent->air_finished - level.time) / 9.0f;
        G_AddBlend(0.1f, 0.1f, 0.2f, alpha * 0.5f, ent->client->ps.blend);
    }

    if (ent->client->bonus_alpha > 0)
        G_AddBlend(0.85f, 0.7f, 0.3f, ent->client->bonus_alpha, ent->client->ps.blend);

    // drop the damage value
    ent->client->damage_alpha -= FRAME_TIME_SEC * 0.6f;
    if (ent->client->damage_alpha < 0)
        ent->client->damage_alpha = 0;

    // drop the bonus value
    ent->client->bonus_alpha -= FRAME_TIME_SEC;
    if (ent->client->bonus_alpha < 0)
        ent->client->bonus_alpha = 0;
}

/*
=================
P_FallingDamage
=================
*/
static void P_FallingDamage(edict_t *ent)
{
    float  delta;
    int    damage;

    // dead stuff can't crater
    if (ent->health <= 0 || ent->deadflag)
        return;

    if (ent->s.modelindex != MODELINDEX_PLAYER)
        return; // not in the player model

    if (ent->movetype == MOVETYPE_NOCLIP)
        return;

    // never take falling damage if completely underwater
    if (ent->waterlevel == WATER_UNDER)
        return;

    // ZOID
    //  never take damage if just release grapple or on grapple
    if (ent->client->ctf_grapplereleasetime >= level.time || (ent->client->ctf_grapple && ent->client->ctf_grapplestate > CTF_GRAPPLE_STATE_FLY))
        return;
    // ZOID

    if ((ent->client->oldvelocity[2] < 0) && (ent->velocity[2] > ent->client->oldvelocity[2]) && (!ent->groundentity)) {
        delta = ent->client->oldvelocity[2];
    } else {
        if (!ent->groundentity)
            return;
        delta = ent->velocity[2] - ent->client->oldvelocity[2];
    }

    delta = delta * delta * 0.0001f;

    if (ent->waterlevel == WATER_WAIST)
        delta *= 0.25f;
    if (ent->waterlevel == WATER_FEET)
        delta *= 0.5f;

    if (delta < 1)
        return;

    // restart footstep timer
    ent->client->bobtime = 0;

    if (ent->client->landmark_free_fall) {
        delta = min(30, delta);
        ent->client->landmark_free_fall = false;
        ent->client->landmark_noise_time = level.time + HZ(10);
    }

    if (delta < 15) {
        ent->s.event = EV_FOOTSTEP;
        return;
    }

    ent->client->fall_value = delta * 0.5f;
    if (ent->client->fall_value > 40)
        ent->client->fall_value = 40;
    ent->client->fall_time = level.time + FALL_TIME;

    if (delta > 30) {
        static const vec3_t dir = { 0, 0, 1 };

        if (delta >= 55)
            ent->s.event = EV_FALLFAR;
        else
            ent->s.event = EV_FALL;

        ent->pain_debounce_time = level.time + FRAME_TIME; // no normal pain sound
        damage = (int)((delta - 30) / 2);
        if (damage < 1)
            damage = 1;

        if (!deathmatch->integer || !g_dm_no_fall_damage->integer)
            T_Damage(ent, world, world, dir, ent->s.origin, vec3_origin, damage, 0, DAMAGE_NONE, (mod_t) { MOD_FALLING });
    } else
        ent->s.event = EV_FALLSHORT;

    // Paril: falling damage noises alert monsters
    if (ent->health)
        PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
}

/*
=============
P_WorldEffects
=============
*/
static void P_WorldEffects(void)
{
    bool          breather;
    bool          envirosuit;
    water_level_t waterlevel, old_waterlevel;

    if (current_player->movetype == MOVETYPE_NOCLIP) {
        current_player->air_finished = level.time + SEC(12); // don't need air
        return;
    }

    waterlevel = current_player->waterlevel;
    old_waterlevel = current_client->old_waterlevel;
    current_client->old_waterlevel = waterlevel;

    breather = current_client->breather_time > level.time;
    envirosuit = current_client->enviro_time > level.time;

    //
    // if just entered a water volume, play a sound
    //
    if (!old_waterlevel && waterlevel) {
        PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
        if (current_player->watertype & CONTENTS_LAVA)
            gi.sound(current_player, CHAN_BODY, gi.soundindex("player/lava_in.wav"), 1, ATTN_NORM, 0);
        else if (current_player->watertype & CONTENTS_SLIME)
            gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
        else if (current_player->watertype & CONTENTS_WATER)
            gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
        current_player->flags |= FL_INWATER;

        // clear damage_debounce, so the pain sound will play immediately
        current_player->damage_debounce_time = level.time - SEC(1);
    }

    //
    // if just completely exited a water volume, play a sound
    //
    if (old_waterlevel && !waterlevel) {
        PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
        gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_out.wav"), 1, ATTN_NORM, 0);
        current_player->flags &= ~FL_INWATER;
    }

    //
    // check for head just going under water
    //
    if (old_waterlevel != WATER_UNDER && waterlevel == WATER_UNDER) {
        gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_un.wav"), 1, ATTN_NORM, 0);
    }

    //
    // check for head just coming out of water
    //
    if (current_player->health > 0 && old_waterlevel == WATER_UNDER && waterlevel != WATER_UNDER) {
        if (current_player->air_finished < level.time) {
            // gasp for air
            gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/gasp1.wav"), 1, ATTN_NORM, 0);
            PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
        } else if (current_player->air_finished < level.time + SEC(11)) {
            // just break surface
            gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/gasp2.wav"), 1, ATTN_NORM, 0);
        }
    }

    //
    // check for drowning
    //
    if (waterlevel == WATER_UNDER) {
        // breather or envirosuit give air
        if (breather || envirosuit) {
            current_player->air_finished = level.time + SEC(10);

            if (((current_client->breather_time - level.time) % SEC(2.5f)) == 0) {
                if (!current_client->breather_sound)
                    gi.sound(current_player, CHAN_AUTO, gi.soundindex("player/u_breath1.wav"), 1, ATTN_NORM, 0);
                else
                    gi.sound(current_player, CHAN_AUTO, gi.soundindex("player/u_breath2.wav"), 1, ATTN_NORM, 0);
                current_client->breather_sound ^= 1;
                PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
                // FIXME: release a bubble?
            }
        }

        // if out of air, start drowning
        if (current_player->air_finished < level.time) {
            // drown!
            if (current_player->client->next_drown_time < level.time && current_player->health > 0) {
                current_player->client->next_drown_time = level.time + SEC(1);

                // take more damage the longer underwater
                current_player->dmg += 2;
                if (current_player->dmg > 15)
                    current_player->dmg = 15;

                // play a gurp sound instead of a normal pain sound
                if (current_player->health <= current_player->dmg)
                    gi.sound(current_player, CHAN_VOICE, gi.soundindex("*drown1.wav"), 1, ATTN_NORM, 0); // [Paril-KEX]
                else if (brandom())
                    gi.sound(current_player, CHAN_VOICE, gi.soundindex("*gurp1.wav"), 1, ATTN_NORM, 0);
                else
                    gi.sound(current_player, CHAN_VOICE, gi.soundindex("*gurp2.wav"), 1, ATTN_NORM, 0);

                current_player->pain_debounce_time = level.time;

                T_Damage(current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, current_player->dmg, 0, DAMAGE_NO_ARMOR, (mod_t) { MOD_WATER });
            }
        // Paril: almost-drowning sounds
        } else if (current_player->air_finished <= level.time + SEC(3)) {
            if (current_player->client->next_drown_time < level.time) {
                gi.sound(current_player, CHAN_VOICE, gi.soundindex(va("player/wade%d.wav", 1 + ((int)TO_SEC(level.time) % 3))), 1, ATTN_NORM, 0);
                current_player->client->next_drown_time = level.time + SEC(1);
            }
        }
    } else {
        current_player->air_finished = level.time + SEC(12);
        current_player->dmg = 2;
    }

    //
    // check for sizzle damage
    //
    if (waterlevel && (current_player->watertype & (CONTENTS_LAVA | CONTENTS_SLIME)) && current_player->slime_debounce_time <= level.time) {
        if (current_player->watertype & CONTENTS_LAVA) {
            if (current_player->health > 0 && current_player->pain_debounce_time <= level.time && current_client->invincible_time < level.time) {
                if (brandom())
                    gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/burn1.wav"), 1, ATTN_NORM, 0);
                else
                    gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/burn2.wav"), 1, ATTN_NORM, 0);
                current_player->pain_debounce_time = level.time + SEC(1);
            }

            int dmg = (envirosuit ? 1 : 3) * waterlevel; // take 1/3 damage with envirosuit

            T_Damage(current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, dmg, 0, DAMAGE_NONE, (mod_t) { MOD_LAVA });
            current_player->slime_debounce_time = level.time + HZ(10);
        }

        if (current_player->watertype & CONTENTS_SLIME) {
            if (!envirosuit) {
                // no damage from slime with envirosuit
                T_Damage(current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, 1 * waterlevel, 0, DAMAGE_NONE, (mod_t) { MOD_SLIME });
                current_player->slime_debounce_time = level.time + HZ(10);
            }
        }
    }
}

/*
===============
G_SetClientEffects
===============
*/
static void G_SetClientEffects(edict_t *ent)
{
    int pa_type;

    ent->s.effects = EF_NONE;
    ent->x.morefx = EFX_NONE;
    ent->s.renderfx &= RF_STAIR_STEP;
    ent->s.renderfx |= RF_IR_VISIBLE;
    ent->x.alpha = 1.0f;

    if (ent->health <= 0 || level.intermissiontime)
        return;

    if (ent->flags & FL_FLASHLIGHT)
        ent->x.morefx |= EFX_FLASHLIGHT;

    //=========
    // PGM
    if (ent->flags & FL_DISGUISED)
        ent->s.renderfx |= RF_USE_DISGUISE;

    if (gamerules->integer) {
        if (DMGame.PlayerEffects)
            DMGame.PlayerEffects(ent);
    }
    // PGM
    //=========

    if (ent->powerarmor_time > level.time) {
        pa_type = PowerArmorType(ent);
        if (pa_type == IT_ITEM_POWER_SCREEN) {
            ent->s.effects |= EF_POWERSCREEN;
        } else if (pa_type == IT_ITEM_POWER_SHIELD) {
            ent->s.effects |= EF_COLOR_SHELL;
            ent->s.renderfx |= RF_SHELL_GREEN;
        }
    }

    // ZOID
    CTFEffects(ent);
    // ZOID

    if (ent->client->quad_time > level.time) {
        if (G_PowerUpExpiring(ent->client->quad_time))
            CTFSetPowerUpEffect(ent, EF_QUAD);
    }

    // RAFAEL
    if (ent->client->quadfire_time > level.time) {
        if (G_PowerUpExpiring(ent->client->quadfire_time))
            //CTFSetPowerUpEffect(ent, EF_DUALFIRE);
            ent->x.morefx |= EFX_DUALFIRE;
    }
    // RAFAEL

    //=======
    // ROGUE
    if (ent->client->double_time > level.time) {
        if (G_PowerUpExpiring(ent->client->double_time))
            CTFSetPowerUpEffect(ent, EF_DOUBLE);
    }
    if ((ent->client->owned_sphere) && (ent->client->owned_sphere->spawnflags == SPHERE_DEFENDER))
        CTFSetPowerUpEffect(ent, EF_HALF_DAMAGE);

    if (ent->client->tracker_pain_time > level.time)
        ent->s.effects |= EF_TRACKERTRAIL;

    if (ent->client->invisible_time > level.time) {
        if (ent->client->invisibility_fade_time <= level.time)
            ent->x.alpha = 0.1f;
        else
            ent->x.alpha = Q_clipf((float)(ent->client->invisibility_fade_time - level.time) / INVISIBILITY_TIME, 0.1f, 1.0f);
    }
    // ROGUE
    //=======

    if (ent->client->invincible_time > level.time) {
        if (G_PowerUpExpiring(ent->client->invincible_time))
            CTFSetPowerUpEffect(ent, EF_PENT);
    }

    // show cheaters!!!
    if (ent->flags & FL_GODMODE) {
        ent->s.effects |= EF_COLOR_SHELL;
        ent->s.renderfx |= (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
    }
}

/*
===============
G_SetClientEvent
===============
*/
static void G_SetClientEvent(edict_t *ent)
{
    if (ent->s.event)
        return;

    if (ent->client->ps.pmove.pm_flags & PMF_ON_LADDER) {
        if (!deathmatch->integer &&
            current_client->last_ladder_sound < level.time &&
            Distance(current_client->last_ladder_pos, ent->s.origin) > 48) {
            ent->s.event = EV_LADDER_STEP;
            VectorCopy(ent->s.origin, current_client->last_ladder_pos);
            current_client->last_ladder_sound = level.time + LADDER_SOUND_TIME;
        }
    } else if (ent->groundentity && xyspeed > 225) {
        if ((int)(current_client->bobtime + bobmove) != bobcycle_run)
            ent->s.event = EV_FOOTSTEP;
    }
}

/*
===============
G_SetClientSound
===============
*/
static void G_SetClientSound(edict_t *ent)
{
    // help beep (no more than three times)
    if (ent->client->pers.helpchanged && ent->client->pers.helpchanged <= 3 && ent->client->pers.help_time < level.time) {
        if (ent->client->pers.helpchanged == 1) // [KEX] haleyjd: once only
            gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/pc_up.wav"), 1, ATTN_STATIC, 0);
        ent->client->pers.helpchanged++;
        ent->client->pers.help_time = level.time + SEC(5);
    }

    // reset defaults
    ent->s.sound = 0;
    ent->x.loop_attenuation = 0;
    ent->x.loop_volume = 0;

    if (ent->waterlevel && (ent->watertype & (CONTENTS_LAVA | CONTENTS_SLIME))) {
        ent->s.sound = level.snd_fry;
        return;
    }

    if (ent->deadflag || ent->client->resp.spectator)
        return;

    if (ent->client->weapon_sound)
        ent->s.sound = ent->client->weapon_sound;
    else if (ent->client->pers.weapon) {
        if (ent->client->pers.weapon->id == IT_WEAPON_RAILGUN)
            ent->s.sound = gi.soundindex("weapons/rg_hum.wav");
        else if (ent->client->pers.weapon->id == IT_WEAPON_BFG)
            ent->s.sound = gi.soundindex("weapons/bfg_hum.wav");
        // RAFAEL
        else if (ent->client->pers.weapon->id == IT_WEAPON_PHALANX)
            ent->s.sound = gi.soundindex("weapons/phaloop.wav");
        // RAFAEL
    }

    // [Paril-KEX] if no other sound is playing, play appropriate grapple sounds
    if (!ent->s.sound && ent->client->ctf_grapple) {
        if (ent->client->ctf_grapplestate == CTF_GRAPPLE_STATE_PULL)
            ent->s.sound = gi.soundindex("weapons/grapple/grpull.wav");
        else if (ent->client->ctf_grapplestate == CTF_GRAPPLE_STATE_FLY)
            ent->s.sound = gi.soundindex("weapons/grapple/grfly.wav");
        else if (ent->client->ctf_grapplestate == CTF_GRAPPLE_STATE_HANG)
            ent->s.sound = gi.soundindex("weapons/grapple/grhang.wav");
    }

    // weapon sounds play at a higher attn
    ent->x.loop_attenuation = ATTN_NORM;
}

/*
===============
G_SetClientFrame
===============
*/
void G_SetClientFrame(edict_t *ent)
{
    gclient_t *client;
    bool       duck, run;

    if (ent->s.modelindex != MODELINDEX_PLAYER)
        return; // not in the player model

    client = ent->client;

    if (client->ps.pmove.pm_flags & PMF_DUCKED)
        duck = true;
    else
        duck = false;
    if (xyspeed)
        run = true;
    else
        run = false;

    // check for stand/duck and stop/go transitions
    if (duck != client->anim_duck && client->anim_priority < ANIM_DEATH)
        goto newanim;
    if (run != client->anim_run && client->anim_priority == ANIM_BASIC)
        goto newanim;
    if (!ent->groundentity && client->anim_priority <= ANIM_WAVE)
        goto newanim;

    if (client->anim_time > level.time)
        return;
    if ((client->anim_priority & ANIM_REVERSED) && (ent->s.frame > client->anim_end)) {
        if (client->anim_time <= level.time) {
            ent->s.frame--;
            client->anim_time = level.time + HZ(10);
        }
        return;
    }
    if (!(client->anim_priority & ANIM_REVERSED) && (ent->s.frame < client->anim_end)) {
        // continue an animation
        if (client->anim_time <= level.time) {
            ent->s.frame++;
            client->anim_time = level.time + HZ(10);
        }
        return;
    }

    if (client->anim_priority == ANIM_DEATH)
        return; // stay there
    if (client->anim_priority == ANIM_JUMP) {
        if (!ent->groundentity)
            return; // stay there
        ent->client->anim_priority = ANIM_WAVE;

        if (duck) {
            ent->s.frame = FRAME_jump6;
            ent->client->anim_end = FRAME_jump4;
            ent->client->anim_priority |= ANIM_REVERSED;
        } else {
            ent->s.frame = FRAME_jump3;
            ent->client->anim_end = FRAME_jump6;
        }
        ent->client->anim_time = level.time + HZ(10);
        return;
    }

newanim:
    // return to either a running or standing frame
    client->anim_priority = ANIM_BASIC;
    client->anim_duck = duck;
    client->anim_run = run;
    client->anim_time = level.time + HZ(10);

    if (!ent->groundentity) {
        // ZOID: if on grapple, don't go into jump frame, go into standing
        // frame
        if (client->ctf_grapple) {
            if (duck) {
                ent->s.frame = FRAME_crstnd01;
                client->anim_end = FRAME_crstnd19;
            } else {
                ent->s.frame = FRAME_stand01;
                client->anim_end = FRAME_stand40;
            }
        } else {
            // ZOID
            client->anim_priority = ANIM_JUMP;

            if (duck) {
                if (ent->s.frame != FRAME_crwalk2)
                    ent->s.frame = FRAME_crwalk1;
                client->anim_end = FRAME_crwalk2;
            } else {
                if (ent->s.frame != FRAME_jump2)
                    ent->s.frame = FRAME_jump1;
                client->anim_end = FRAME_jump2;
            }
        }
    } else if (run) {
        // running
        if (duck) {
            ent->s.frame = FRAME_crwalk1;
            client->anim_end = FRAME_crwalk6;
        } else {
            ent->s.frame = FRAME_run1;
            client->anim_end = FRAME_run6;
        }
    } else {
        // standing
        if (duck) {
            ent->s.frame = FRAME_crstnd01;
            client->anim_end = FRAME_crstnd19;
        } else {
            ent->s.frame = FRAME_stand01;
            client->anim_end = FRAME_stand40;
        }
    }
}

// [Paril-KEX]
static void P_RunMegaHealth(edict_t *ent)
{
    if (!ent->client->pers.megahealth_time)
        return;
    if (ent->health <= ent->max_health) {
        ent->client->pers.megahealth_time = 0;
        return;
    }

    ent->client->pers.megahealth_time -= FRAME_TIME;

    if (ent->client->pers.megahealth_time <= 0) {
        ent->health--;

        if (ent->health > ent->max_health)
            ent->client->pers.megahealth_time = SEC(1);
        else
            ent->client->pers.megahealth_time = 0;
    }
}

/*
=================
ClientEndServerFrame

Called for each player at the end of the server frame
and right after spawning
=================
*/
void ClientEndServerFrame(edict_t *ent)
{
    // no player exists yet (load game)
    if (!ent->client->pers.spawned)
        return;

    float bobtime, bobtime_run;

    current_player = ent;
    current_client = ent->client;

    // check goals
    G_PlayerNotifyGoal(ent);

    // mega health
    P_RunMegaHealth(ent);

    //
    // If the origin or velocity have changed since ClientThink(),
    // update the pmove values.  This will happen when the client
    // is pushed by a bmodel or kicked by an explosion.
    //
    // If it wasn't updated here, the view position would lag a frame
    // behind the body position when pushed -- "sinking into plats"
    //
    for (int i = 0; i < 3; i++) {
        current_client->ps.pmove.origin[i] = COORD2SHORT(ent->s.origin[i]);
        current_client->ps.pmove.velocity[i] = COORD2SHORT(ent->velocity[i]);
    }

    //
    // If the end of unit layout is displayed, don't give
    // the player any normal movement attributes
    //
    if (level.intermissiontime || ent->client->awaiting_respawn) {
        if (ent->client->awaiting_respawn || (level.intermission_eou || level.is_n64 || (deathmatch->integer && level.intermissiontime))) {
            current_client->ps.blend[3] = 0;
            current_client->ps.fov = 90;
            current_client->ps.gunindex = 0;
        }
        G_SetStats(ent);
        G_SetCoopStats(ent);

        // if the scoreboard is up, update it if a client leaves
        if (deathmatch->integer && ent->client->showscores && ent->client->menutime) {
            DeathmatchScoreboardMessage(ent, ent->enemy);
            gi.unicast(ent, false);
            ent->client->menutime = 0;
        }

        return;
    }

    // ZOID
    // regen tech
    CTFApplyRegeneration(ent);
    // ZOID

    AngleVectors(ent->client->v_angle, forward, right, up);

    // burn from lava, etc
    P_WorldEffects();

    //
    // set model angles from view angles so other things in
    // the world can tell which direction you are looking
    //
    if (ent->client->v_angle[PITCH] > 180)
        ent->s.angles[PITCH] = (-360 + ent->client->v_angle[PITCH]) / 3;
    else
        ent->s.angles[PITCH] = ent->client->v_angle[PITCH] / 3;

    ent->s.angles[YAW] = ent->client->v_angle[YAW];
    ent->s.angles[ROLL] = 0;
    ent->s.angles[ROLL] = SV_CalcRoll(ent->s.angles, ent->velocity) * 4;

    //
    // calculate speed and cycle to be used for
    // all cyclic walking effects
    //
    xyspeed = sqrtf(ent->velocity[0] * ent->velocity[0] + ent->velocity[1] * ent->velocity[1]);

    if (xyspeed < 5) {
        bobmove = 0;
        current_client->bobtime = 0; // start at beginning of cycle again
    } else if (ent->groundentity) {
        // so bobbing only cycles when on ground
        if (xyspeed > 210)
            bobmove = FRAME_TIME_SEC / 0.4f;
        else if (xyspeed > 100)
            bobmove = FRAME_TIME_SEC / 0.8f;
        else
            bobmove = FRAME_TIME_SEC / 1.6f;
    }

    bobtime = (current_client->bobtime += bobmove);
    bobtime_run = bobtime;

    if ((current_client->ps.pmove.pm_flags & PMF_DUCKED) && ent->groundentity)
        bobtime *= 4;

    bobcycle = (int)bobtime;
    bobcycle_run = (int)bobtime_run;
    bobfracsin = fabsf(sinf(bobtime * M_PI));

    // detect hitting the floor
    P_FallingDamage(ent);

    // apply all the damage taken this frame
    P_DamageFeedback(ent);

    // determine the view offsets
    SV_CalcViewOffset(ent);

    // determine the gun offsets
    SV_CalcGunOffset(ent);

    // determine the full screen color blend
    // must be after viewoffset, so eye contents can be
    // accurately determined
    SV_CalcBlend(ent);

    // chase cam stuff
    if (ent->client->resp.spectator)
        G_SetSpectatorStats(ent);
    else
        G_SetStats(ent);

    G_CheckChaseStats(ent);

    G_SetCoopStats(ent);

    G_SetClientEvent(ent);

    G_SetClientEffects(ent);

    G_SetClientSound(ent);

    G_SetClientFrame(ent);

    VectorCopy(ent->velocity, ent->client->oldvelocity);
    VectorCopy(ent->client->ps.viewangles, ent->client->oldviewangles);
    ent->client->oldgroundentity = ent->groundentity;

    // ZOID
    if (ent->client->menudirty && ent->client->menutime <= level.time) {
        if (ent->client->menu) {
            PMenu_Do_Update(ent);
            gi.unicast(ent, true);
        }
        ent->client->menutime = level.time;
        ent->client->menudirty = false;
    }
    // ZOID

    // if the scoreboard is up, update it
    if (ent->client->showscores && ent->client->menutime <= level.time) {
        // ZOID
        if (ent->client->menu) {
            PMenu_Do_Update(ent);
            ent->client->menudirty = false;
        } else
        // ZOID
            DeathmatchScoreboardMessage(ent, ent->enemy);
        gi.unicast(ent, false);
        ent->client->menutime = level.time + SEC(3);
    }

    P_AssignClientSkinnum(ent);

    // [Paril-KEX] in coop, if player collision is enabled and
    // we are currently in no-player-collision mode, check if
    // it's safe.
    if (coop->integer && G_ShouldPlayersCollide(false) && !(ent->clipmask & CONTENTS_PLAYER) && ent->takedamage) {
        bool clipped_player = false;

        for (int i = 1; i <= game.maxclients; i++) {
            edict_t *player = &g_edicts[i];

            if (!player->inuse)
                continue;
            if (player == ent)
                continue;

            trace_t clip = gix.clip(ent->s.origin, ent->mins, ent->maxs, ent->s.origin, player, CONTENTS_MONSTER | CONTENTS_PLAYER);

            if (clip.startsolid || clip.allsolid) {
                clipped_player = true;
                break;
            }
        }

        // safe!
        if (!clipped_player)
            ent->clipmask |= CONTENTS_PLAYER;
    }
}
