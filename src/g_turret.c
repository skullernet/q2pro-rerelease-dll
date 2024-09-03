// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_turret.c

#include "g_local.h"

#define SPAWNFLAG_TURRET_BREACH_FIRE    65536

static void AnglesNormalize(vec3_t vec)
{
    while (vec[0] > 360)
        vec[0] -= 360;
    while (vec[0] < 0)
        vec[0] += 360;
    while (vec[1] > 360)
        vec[1] -= 360;
    while (vec[1] < 0)
        vec[1] += 360;
}

float SnapToEights(float x)
{
    x *= 8.0f;
    if (x > 0.0f)
        x += 0.5f;
    else
        x -= 0.5f;
    return 0.125f * (int)x;
}

void MOVEINFO_BLOCKED(turret_blocked)(edict_t *self, edict_t *other)
{
    edict_t *attacker;

    if (other->takedamage) {
        if (self->teammaster->owner)
            attacker = self->teammaster->owner;
        else
            attacker = self->teammaster;
        T_Damage(other, self, attacker, vec3_origin, other->s.origin, vec3_origin, self->teammaster->dmg, 10, DAMAGE_NONE, (mod_t) { MOD_CRUSH });
    }
}

/*QUAKED turret_breach (0 0 0) ?
This portion of the turret can change both pitch and yaw.
The model  should be made with a flat pitch.
It (and the associated base) need to be oriented towards 0.
Use "angle" to set the starting angle.

"speed"     default 50
"dmg"       default 10
"angle"     point this forward
"target"    point this at an info_notnull at the muzzle tip
"minpitch"  min acceptable pitch angle : default -30
"maxpitch"  max acceptable pitch angle : default 30
"minyaw"    min acceptable yaw angle   : default 0
"maxyaw"    max acceptable yaw angle   : default 360
*/

static void turret_breach_fire(edict_t *self)
{
    vec3_t f, r, u;
    vec3_t start;
    int    damage;
    int    speed;

    AngleVectors(self->s.angles, f, r, u);
    VectorMA(self->s.origin, self->move_origin[0], f, start);
    VectorMA(start, self->move_origin[1], r, start);
    VectorMA(start, self->move_origin[2], u, start);

    if (self->count)
        damage = self->count;
    else
        damage = irandom2(100, 150);
    speed = 550 + 50 * skill->integer;
    edict_t *rocket = fire_rocket(self->teammaster->owner->activator ? self->teammaster->owner->activator : self->teammaster->owner, start, f, damage, speed, 150, damage);
    rocket->x.scale = self->teammaster->dmg_radius;

    gi.positioned_sound(start, self, CHAN_WEAPON, gi.soundindex("weapons/rocklf1a.wav"), 1, ATTN_NORM, 0);
}

void THINK(turret_breach_think)(edict_t *self)
{
    edict_t *ent;
    vec3_t   current_angles;
    vec3_t   delta;

    VectorCopy(self->s.angles, current_angles);
    AnglesNormalize(current_angles);

    AnglesNormalize(self->move_angles);
    if (self->move_angles[PITCH] > 180)
        self->move_angles[PITCH] -= 360;

    // clamp angles to mins & maxs
    if (self->move_angles[PITCH] > self->pos1[PITCH])
        self->move_angles[PITCH] = self->pos1[PITCH];
    else if (self->move_angles[PITCH] < self->pos2[PITCH])
        self->move_angles[PITCH] = self->pos2[PITCH];

    if ((self->move_angles[YAW] < self->pos1[YAW]) || (self->move_angles[YAW] > self->pos2[YAW])) {
        float dmin, dmax;

        dmin = fabsf(self->pos1[YAW] - self->move_angles[YAW]);
        if (dmin < -180)
            dmin += 360;
        else if (dmin > 180)
            dmin -= 360;
        dmax = fabsf(self->pos2[YAW] - self->move_angles[YAW]);
        if (dmax < -180)
            dmax += 360;
        else if (dmax > 180)
            dmax -= 360;
        if (fabsf(dmin) < fabsf(dmax))
            self->move_angles[YAW] = self->pos1[YAW];
        else
            self->move_angles[YAW] = self->pos2[YAW];
    }

    VectorSubtract(self->move_angles, current_angles, delta);
    if (delta[0] < -180)
        delta[0] += 360;
    else if (delta[0] > 180)
        delta[0] -= 360;
    if (delta[1] < -180)
        delta[1] += 360;
    else if (delta[1] > 180)
        delta[1] -= 360;
    delta[2] = 0;

    if (delta[0] > self->speed * FRAME_TIME_SEC)
        delta[0] = self->speed * FRAME_TIME_SEC;
    if (delta[0] < -1 * self->speed * FRAME_TIME_SEC)
        delta[0] = -1 * self->speed * FRAME_TIME_SEC;
    if (delta[1] > self->speed * FRAME_TIME_SEC)
        delta[1] = self->speed * FRAME_TIME_SEC;
    if (delta[1] < -1 * self->speed * FRAME_TIME_SEC)
        delta[1] = -1 * self->speed * FRAME_TIME_SEC;

    for (ent = self->teammaster; ent; ent = ent->teamchain) {
        if (ent->noise_index) {
            if (delta[0] || delta[1]) {
                ent->s.sound = ent->noise_index;
                ent->x.loop_attenuation = ATTN_NORM;
            } else
                ent->s.sound = 0;
        }
    }

    VectorScale(delta, 1.0f / FRAME_TIME_SEC, self->avelocity);

    self->nextthink = level.time + FRAME_TIME;

    for (ent = self->teammaster; ent; ent = ent->teamchain)
        ent->avelocity[1] = self->avelocity[1];

    // if we have a driver, adjust his velocities
    if (self->owner) {
        float  angle;
        float  target_z;
        float  diff;
        vec3_t target;
        vec3_t dir;

        // angular is easy, just copy ours
        self->owner->avelocity[0] = self->avelocity[0];
        self->owner->avelocity[1] = self->avelocity[1];

        // x & y
        angle = DEG2RAD(self->s.angles[1] + self->owner->move_origin[1]);
        target[0] = SnapToEights(self->s.origin[0] + cosf(angle) * self->owner->move_origin[0]);
        target[1] = SnapToEights(self->s.origin[1] + sinf(angle) * self->owner->move_origin[0]);
        target[2] = self->owner->s.origin[2];

        VectorSubtract(target, self->owner->s.origin, dir);
        self->owner->velocity[0] = dir[0] * 1.0f / FRAME_TIME_SEC;
        self->owner->velocity[1] = dir[1] * 1.0f / FRAME_TIME_SEC;

        // z
        angle = DEG2RAD(self->s.angles[PITCH]);
        target_z = SnapToEights(self->s.origin[2] + self->owner->move_origin[0] * tanf(angle) + self->owner->move_origin[2]);

        diff = target_z - self->owner->s.origin[2];
        self->owner->velocity[2] = diff * 1.0f / FRAME_TIME_SEC;

        if (self->spawnflags & SPAWNFLAG_TURRET_BREACH_FIRE) {
            turret_breach_fire(self);
            self->spawnflags &= ~SPAWNFLAG_TURRET_BREACH_FIRE;
        }
    }
}

void THINK(turret_breach_finish_init)(edict_t *self)
{
    // get and save info for muzzle location
    if (!self->target) {
        gi.dprintf("%s: needs a target\n", etos(self));
    } else {
        self->target_ent = G_PickTarget(self->target);
        if (self->target_ent) {
            VectorSubtract(self->target_ent->s.origin, self->s.origin, self->move_origin);
            G_FreeEdict(self->target_ent);
        } else
            gi.dprintf("%s: could not find target entity \"%s\"\n", etos(self), self->target);
    }

    self->teammaster->dmg = self->dmg;
    self->teammaster->dmg_radius = self->dmg_radius; // scale
    self->think = turret_breach_think;
    self->think(self);
}

void SP_turret_breach(edict_t *self)
{
    self->solid = SOLID_BSP;
    self->movetype = MOVETYPE_PUSH;

    if (st.noise)
        self->noise_index = gi.soundindex(st.noise);

    gi.setmodel(self, self->model);

    if (!self->speed)
        self->speed = 50;
    if (!self->dmg)
        self->dmg = 10;

    if (!st.minpitch)
        st.minpitch = -30;
    if (!st.maxpitch)
        st.maxpitch = 30;
    if (!st.maxyaw)
        st.maxyaw = 360;

    self->pos1[PITCH] = -1 * st.minpitch;
    self->pos1[YAW] = st.minyaw;
    self->pos2[PITCH] = -1 * st.maxpitch;
    self->pos2[YAW] = st.maxyaw;

    // scale used for rocket scale
    self->dmg_radius = self->x.scale;
    self->x.scale = 0;

    self->ideal_yaw = self->s.angles[YAW];
    self->move_angles[YAW] = self->ideal_yaw;

    self->moveinfo.blocked = turret_blocked;

    self->think = turret_breach_finish_init;
    self->nextthink = level.time + FRAME_TIME;
    gi.linkentity(self);
}

/*QUAKED turret_base (0 0 0) ?
This portion of the turret changes yaw only.
MUST be teamed with a turret_breach.
*/

void SP_turret_base(edict_t *self)
{
    self->solid = SOLID_BSP;
    self->movetype = MOVETYPE_PUSH;

    if (st.noise)
        self->noise_index = gi.soundindex(st.noise);

    gi.setmodel(self, self->model);
    self->moveinfo.blocked = turret_blocked;
    gi.linkentity(self);
}

/*QUAKED turret_driver (1 .5 0) (-16 -16 -24) (16 16 32)
Must NOT be on the team with the rest of the turret parts.
Instead it must target the turret_breach.
*/

void infantry_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod);
void infantry_stand(edict_t *self);
void infantry_pain(edict_t *self, edict_t *other, float kick, int damage, mod_t mod);
void infantry_setskin(edict_t *self);

void DIE(turret_driver_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    if (!self->deadflag) {
        edict_t *ent;

        // level the gun
        self->target_ent->move_angles[0] = 0;

        // remove the driver from the end of them team chain
        for (ent = self->target_ent->teammaster; ent->teamchain != self; ent = ent->teamchain)
            ;
        ent->teamchain = NULL;
        self->teammaster = NULL;
        self->flags &= ~FL_TEAMSLAVE;

        self->target_ent->owner = NULL;
        self->target_ent->teammaster->owner = NULL;

        self->target_ent->moveinfo.blocked = NULL;

        // clear pitch
        self->s.angles[0] = 0;
        self->movetype = MOVETYPE_STEP;

        self->think = monster_think;
    }

    infantry_die(self, inflictor, attacker, damage, point, mod);

    G_FixStuckObject(self, self->s.origin);
    AngleVectors(self->s.angles, self->velocity, NULL, NULL);
    VectorScale(self->velocity, -50, self->velocity);
    self->velocity[2] += 110;
}

void THINK(turret_driver_think)(edict_t *self)
{
    vec3_t target;
    vec3_t dir;

    self->nextthink = level.time + FRAME_TIME;

    if (self->enemy && (!self->enemy->inuse || self->enemy->health <= 0))
        self->enemy = NULL;

    if (!self->enemy) {
        if (!FindTarget(self))
            return;
        self->monsterinfo.trail_time = level.time;
        self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
    } else if (visible(self, self->enemy)) {
        if (self->monsterinfo.aiflags & AI_LOST_SIGHT) {
            self->monsterinfo.trail_time = level.time;
            self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
        }
    } else {
        self->monsterinfo.aiflags |= AI_LOST_SIGHT;
        return;
    }

    // let the turret know where we want it to aim
    VectorCopy(self->enemy->s.origin, target);
    target[2] += self->enemy->viewheight;
    VectorSubtract(target, self->target_ent->s.origin, dir);
    vectoangles(dir, self->target_ent->move_angles);

    // decide if we should shoot
    if (level.time < self->monsterinfo.attack_finished)
        return;

    gtime_t reaction_time = SEC(3 - skill->integer);
    if ((level.time - self->monsterinfo.trail_time) < reaction_time)
        return;

    self->monsterinfo.attack_finished = level.time + reaction_time + SEC(1);
    // FIXME how do we really want to pass this along?
    self->target_ent->spawnflags |= SPAWNFLAG_TURRET_BREACH_FIRE;
}

void THINK(turret_driver_link)(edict_t *self)
{
    vec3_t   vec;
    edict_t *ent;

    self->think = turret_driver_think;
    self->nextthink = level.time + FRAME_TIME;

    self->target_ent = G_PickTarget(self->target);
    if (!self->target_ent) {
        G_FreeEdict(self);
        return;
    }
    self->target_ent->owner = self;
    self->target_ent->teammaster->owner = self;
    VectorCopy(self->target_ent->s.angles, self->s.angles);

    vec[0] = self->target_ent->s.origin[0] - self->s.origin[0];
    vec[1] = self->target_ent->s.origin[1] - self->s.origin[1];
    vec[2] = 0;
    self->move_origin[0] = VectorLength(vec);

    VectorSubtract(self->s.origin, self->target_ent->s.origin, vec);
    vectoangles(vec, vec);
    AnglesNormalize(vec);
    self->move_origin[1] = vec[1];

    self->move_origin[2] = self->s.origin[2] - self->target_ent->s.origin[2];

    // add the driver to the end of them team chain
    for (ent = self->target_ent->teammaster; ent->teamchain; ent = ent->teamchain)
        ;
    ent->teamchain = self;
    self->teammaster = self->target_ent->teammaster;
    self->flags |= FL_TEAMSLAVE;
}

void InfantryPrecache(void);

void SP_turret_driver(edict_t *self)
{
    if (deathmatch->integer) {
        G_FreeEdict(self);
        return;
    }

    InfantryPrecache();

    self->movetype = MOVETYPE_PUSH;
    self->solid = SOLID_BBOX;
    self->s.modelindex = gi.modelindex("models/monsters/infantry/tris.md2");
    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, 32);

    self->health = self->max_health = 100;
    self->gib_health = -40;
    self->mass = 200;
    self->viewheight = 24;

    self->pain = infantry_pain;
    self->die = turret_driver_die;
    self->monsterinfo.stand = infantry_stand;

    self->flags |= FL_NO_KNOCKBACK;

    if (g_debug_monster_kills->integer)
        level.monsters_registered[level.total_monsters] = self;
    level.total_monsters++;

    self->svflags |= SVF_MONSTER;
    self->takedamage = true;
    self->use = monster_use;
    self->clipmask = MASK_MONSTERSOLID;
    VectorCopy(self->s.origin, self->s.old_origin);
    self->monsterinfo.aiflags |= AI_STAND_GROUND;
    self->monsterinfo.setskin = infantry_setskin;

    if (st.item) {
        self->item = FindItemByClassname(st.item);
        if (!self->item)
            gi.dprintf("%s: bad item: %s\n", etos(self), st.item);
    }

    self->think = turret_driver_link;
    self->nextthink = level.time + FRAME_TIME;

    gi.linkentity(self);
}

//============
// ROGUE

// invisible turret drivers so we can have unmanned turrets.
// originally designed to shoot at func_trains and such, so they
// fire at the center of the bounding box, rather than the entity's
// origin.

#define SPAWNFLAG_TURRET_BRAIN_IGNORE_SIGHT 1

void THINK(turret_brain_think)(edict_t *self)
{
    vec3_t  target;
    vec3_t  dir;
    trace_t trace;

    self->nextthink = level.time + FRAME_TIME;

    if (self->enemy) {
        if (!self->enemy->inuse)
            self->enemy = NULL;
        else if (self->enemy->takedamage && self->enemy->health <= 0)
            self->enemy = NULL;
    }

    if (!self->enemy) {
        if (!FindTarget(self))
            return;
        self->monsterinfo.trail_time = level.time;
        self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
    }

    VectorAvg(self->enemy->absmax, self->enemy->absmin, target);

    if (!(self->spawnflags & SPAWNFLAG_TURRET_BRAIN_IGNORE_SIGHT)) {
        trace = gi.trace(self->target_ent->s.origin, NULL, NULL, target, self->target_ent, MASK_SHOT);
        if (trace.fraction == 1 || trace.ent == self->enemy) {
            if (self->monsterinfo.aiflags & AI_LOST_SIGHT) {
                self->monsterinfo.trail_time = level.time;
                self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
            }
        } else {
            self->monsterinfo.aiflags |= AI_LOST_SIGHT;
            return;
        }
    }

    // let the turret know where we want it to aim
    VectorSubtract(target, self->target_ent->s.origin, dir);
    vectoangles(dir, self->target_ent->move_angles);

    // decide if we should shoot
    if (level.time < self->monsterinfo.attack_finished)
        return;

    gtime_t reaction_time = self->delay ? SEC(self->delay) : SEC(3 - skill->integer);
    if ((level.time - self->monsterinfo.trail_time) < reaction_time)
        return;

    self->monsterinfo.attack_finished = level.time + reaction_time + SEC(1);
    // FIXME how do we really want to pass this along?
    self->target_ent->spawnflags |= SPAWNFLAG_TURRET_BREACH_FIRE;
}

void THINK(turret_brain_link)(edict_t *self)
{
    vec3_t   vec;
    edict_t *ent;

    if (self->killtarget)
        self->enemy = G_PickTarget(self->killtarget);

    self->think = turret_brain_think;
    self->nextthink = level.time + FRAME_TIME;

    self->target_ent = G_PickTarget(self->target);
    if (!self->target_ent) {
        G_FreeEdict(self);
        return;
    }
    self->target_ent->owner = self;
    self->target_ent->teammaster->owner = self;
    VectorCopy(self->target_ent->s.angles, self->s.angles);

    vec[0] = self->target_ent->s.origin[0] - self->s.origin[0];
    vec[1] = self->target_ent->s.origin[1] - self->s.origin[1];
    vec[2] = 0;
    self->move_origin[0] = VectorLength(vec);

    VectorSubtract(self->s.origin, self->target_ent->s.origin, vec);
    vectoangles(vec, vec);
    AnglesNormalize(vec);
    self->move_origin[1] = vec[1];

    self->move_origin[2] = self->s.origin[2] - self->target_ent->s.origin[2];

    // add the driver to the end of them team chain
    for (ent = self->target_ent->teammaster; ent->teamchain; ent = ent->teamchain)
        ent->activator = self->activator; // pass along activator to breach, etc

    ent->teamchain = self;
    self->teammaster = self->target_ent->teammaster;
    self->flags |= FL_TEAMSLAVE;
}

void USE(turret_brain_deactivate)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->think = NULL;
    self->nextthink = 0;
}

void USE(turret_brain_activate)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (!self->enemy)
        self->enemy = activator;

    // wait at least 3 seconds to fire.
    self->monsterinfo.attack_finished = level.time + SEC(self->wait ? self->wait : 3);
    self->use = turret_brain_deactivate;

    // Paril NOTE: rhangar1 has a turret_invisible_brain that breaks the
    // hangar ceiling; once the final rocket explodes the barrier,
    // it attempts to print "Barrier neutralized." to the rocket owner
    // who happens to be this brain rather than the player that activated
    // the turret. this resolves this by passing it along to fire_rocket.
    self->activator = activator;

    self->think = turret_brain_link;
    self->nextthink = level.time + FRAME_TIME;
}

/*QUAKED turret_invisible_brain (1 .5 0) (-16 -16 -16) (16 16 16)
Invisible brain to drive the turret.

Does not search for targets. If targeted, can only be turned on once
and then off once. After that they are completely disabled.

"delay" the delay between firing (default ramps for skill level)
"Target" the turret breach
"Killtarget" the item you want it to attack.
Target the brain if you want it activated later, instead of immediately. It will wait 3 seconds
before firing to acquire the target.
*/
void SP_turret_invisible_brain(edict_t *self)
{
    if (!self->killtarget) {
        gi.dprintf("%s with no killtarget!\n", etos(self));
        G_FreeEdict(self);
        return;
    }
    if (!self->target) {
        gi.dprintf("%s with no target!\n", etos(self));
        G_FreeEdict(self);
        return;
    }

    if (self->targetname) {
        self->use = turret_brain_activate;
    } else {
        self->think = turret_brain_link;
        self->nextthink = level.time + FRAME_TIME;
    }

    self->movetype = MOVETYPE_PUSH;
    gi.linkentity(self);
}

// ROGUE
//============
