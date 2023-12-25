// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_newdm.c
// pmack
// june 1998

#include "../g_local.h"
#include "../m_player.h"

dm_game_rt DMGame;

#define IF_TYPE_MASK    (IF_WEAPON | IF_AMMO | IF_POWERUP | IF_ARMOR | IF_KEY)

static item_flags_t GetSubstituteItemFlags(item_id_t id)
{
    const gitem_t *item = GetItemByIndex(id);

    // we want to stay within the item class
    item_flags_t flags = item->flags & IF_TYPE_MASK;

    if ((flags & (IF_WEAPON | IF_AMMO)) == (IF_WEAPON | IF_AMMO))
        flags = IF_AMMO;
    // Adrenaline and Mega Health count as powerup
    else if (id == IT_ITEM_ADRENALINE || id == IT_HEALTH_MEGA)
        flags = IF_POWERUP;

    return flags;
}

static item_id_t FindSubstituteItem(edict_t *ent)
{
    item_id_t id = ent->item->id;

    // never replace flags
    if (id == IT_FLAG1 || id == IT_FLAG2 || id == IT_ITEM_TAG_TOKEN)
        return IT_NULL;

    // stimpack/shard randomizes
    if (id == IT_HEALTH_SMALL || id == IT_ARMOR_SHARD)
        return brandom() ? IT_HEALTH_SMALL : IT_ARMOR_SHARD;

    // health is special case
    if (id == IT_HEALTH_MEDIUM || id == IT_HEALTH_LARGE) {
        if (frandom() < 0.6f)
            return IT_HEALTH_MEDIUM;
        else
            return IT_HEALTH_LARGE;
    }

    // armor is also special case
    if (id == IT_ARMOR_JACKET || id == IT_ARMOR_COMBAT || id == IT_ARMOR_BODY ||
        id == IT_ITEM_POWER_SCREEN || id == IT_ITEM_POWER_SHIELD) {
        float rnd = frandom();

        if (rnd < 0.4f)
            return IT_ARMOR_JACKET;
        if (rnd < 0.6f)
            return IT_ARMOR_COMBAT;
        if (rnd < 0.8f)
            return IT_ARMOR_BODY;
        if (rnd < 0.9f)
            return IT_ITEM_POWER_SCREEN;

        return IT_ITEM_POWER_SHIELD;
    }

    item_flags_t myflags = GetSubstituteItemFlags(id);

    item_id_t possible_items[MAX_ITEMS];
    int possible_item_count = 0;

    // gather matching items
    for (item_id_t i = IT_NULL + 1; i < IT_TOTAL; i++) {
        const gitem_t *it = GetItemByIndex(i);
        item_flags_t itflags = it->flags;

        if (!itflags || (itflags & (IF_NOT_GIVEABLE | IF_TECH | IF_NOT_RANDOM)) || !it->pickup || !it->world_model)
            continue;

        // don't respawn spheres if they're dmflag disabled.
        if (g_no_spheres->integer && (i == IT_ITEM_SPHERE_VENGEANCE || i == IT_ITEM_SPHERE_HUNTER || i == IT_ITEM_SPHERE_DEFENDER))
            continue;

        if (g_no_nukes->integer && i == IT_AMMO_NUKE)
            continue;

        if (g_no_mines->integer && (i == IT_AMMO_PROX || i == IT_AMMO_TESLA || i == IT_AMMO_TRAP))
            continue;

        itflags = GetSubstituteItemFlags(i);

        if ((itflags & IF_TYPE_MASK) == (myflags & IF_TYPE_MASK))
            possible_items[possible_item_count++] = i;
    }

    if (!possible_item_count)
        return IT_NULL;

    return possible_items[irandom1(possible_item_count)];
}

item_id_t DoRandomRespawn(edict_t *ent)
{
    if (!ent->item)
        return IT_NULL; // why

    return FindSubstituteItem(ent);
}

void PrecacheForRandomRespawn(void)
{
    const gitem_t *it;
    int            i;
    int            itflags;

    it = itemlist;
    for (i = 0; i < IT_TOTAL; i++, it++) {
        itflags = it->flags;

        if (!itflags || (itflags & (IF_NOT_GIVEABLE | IF_TECH | IF_NOT_RANDOM)) || !it->pickup || !it->world_model)
            continue;

        PrecacheItem(it);
    }
}

// ***************************
//  DOPPLEGANGER
// ***************************

edict_t *Sphere_Spawn(edict_t *owner, spawnflags_t spawnflags);

void DIE(doppleganger_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, const mod_t mod)
{
    edict_t *sphere;
    float    dist;

    if ((self->enemy) && (self->enemy != self->teammaster)) {
        dist = Distance(self->enemy->s.origin, self->s.origin);

        if (dist > 768) {
            sphere = Sphere_Spawn(self, SPHERE_HUNTER | SPHERE_DOPPLEGANGER);
            sphere->pain(sphere, attacker, 0, 0, mod);
        } else if (dist > 80) {
            sphere = Sphere_Spawn(self, SPHERE_VENGEANCE | SPHERE_DOPPLEGANGER);
            sphere->pain(sphere, attacker, 0, 0, mod);
        }
    }

    self->takedamage = DAMAGE_NONE;

    // [Paril-KEX]
    T_RadiusDamage(self, self->teammaster, 160, self, 140, DAMAGE_NONE, (mod_t) { MOD_DOPPLE_EXPLODE });

    if (self->teamchain)
        BecomeExplosion1(self->teamchain);
    BecomeExplosion1(self);
}

void PAIN(doppleganger_pain)(edict_t *self, edict_t *other, float kick, int damage, const mod_t mod)
{
    self->enemy = other;
}

void THINK(doppleganger_timeout)(edict_t *self)
{
    doppleganger_die(self, self, self, 9999, self->s.origin, (mod_t) { MOD_UNKNOWN });
}

void THINK(body_think)(edict_t *self)
{
    float r;

    if (fabsf(self->ideal_yaw - anglemod(self->s.angles[YAW])) < 2 && self->timestamp < level.time) {
        r = frandom();
        if (r < 0.10f) {
            self->ideal_yaw = frandom1(350.0f);
            self->timestamp = level.time + SEC(1);
        }
    } else
        M_ChangeYaw(self);

    if (self->teleport_time <= level.time) {
        self->s.frame++;
        if (self->s.frame > FRAME_stand40)
            self->s.frame = FRAME_stand01;

        self->teleport_time = level.time + HZ(10);
    }

    self->nextthink = level.time + FRAME_TIME;
}

void fire_doppleganger(edict_t *ent, const vec3_t start, const vec3_t aimdir)
{
    edict_t *base;
    edict_t *body;
    vec3_t   dir;
    vec3_t   forward, right, up;
    int      number;

    vectoangles(aimdir, dir);
    AngleVectors(dir, forward, right, up);

    base = G_Spawn();
    VectorCopy(start, base->s.origin);
    VectorCopy(dir, base->s.angles);
    base->movetype = MOVETYPE_TOSS;
    base->solid = SOLID_BBOX;
    base->s.renderfx |= RF_IR_VISIBLE;
    base->s.angles[PITCH] = 0;
    VectorSet(base->mins, -16, -16, -24);
    VectorSet(base->maxs, 16, 16, 32);
    base->s.modelindex = gi.modelindex("models/objects/dopplebase/tris.md2");
    base->x.alpha = 0.1f;
    base->teammaster = ent;
    base->flags |= (FL_DAMAGEABLE | FL_TRAP);
    base->takedamage = true;
    base->health = 30;
    base->pain = doppleganger_pain;
    base->die = doppleganger_die;

    base->nextthink = level.time + SEC(30);
    base->think = doppleganger_timeout;

    base->classname = "doppleganger";

    gi.linkentity(base);

    body = G_Spawn();
    number = body->s.number;
    body->s = ent->s;
    body->s.sound = 0;
    body->s.event = EV_NONE;
    body->s.number = number;
    body->yaw_speed = 30;
    body->ideal_yaw = 0;
    VectorCopy(start, body->s.origin);
    body->s.origin[2] += 8;
    body->teleport_time = level.time + HZ(10);
    body->think = body_think;
    body->nextthink = level.time + FRAME_TIME;
    gi.linkentity(body);

    base->teamchain = body;
    body->teammaster = base;

    // [Paril-KEX]
    body->owner = ent;
    gi.sound(body, CHAN_AUTO, gi.soundindex("medic_commander/monsterspawn1.wav"), 1, ATTN_NORM, 0);
}

// ****************************
// General DM Stuff
// ****************************

extern const dm_game_rt DMGame_Tag;
extern const dm_game_rt DMGame_DBall;

void InitGameRules(void)
{
    // clear out the game rule structure before we start
    memset(&DMGame, 0, sizeof(DMGame));

    if (gamerules->integer) {
        switch (gamerules->integer) {
        case RDM_TAG:
            DMGame = DMGame_Tag;
            break;
        case RDM_DEATHBALL:
            DMGame = DMGame_DBall;
            break;
        // reset gamerules if it's not a valid number
        default:
            gi.cvar_forceset("gamerules", "0");
            break;
        }
    }

    // if we're set up to play, initialize the game as needed.
    if (DMGame.GameInit)
        DMGame.GameInit();
}
