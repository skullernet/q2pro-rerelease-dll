// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

TURRET

==============================================================================
*/

#include "../g_local.h"
#include "m_rogue_turret.h"

#define SPAWNFLAG_TURRET_BLASTER        0x0008
#define SPAWNFLAG_TURRET_MACHINEGUN     0x0010
#define SPAWNFLAG_TURRET_ROCKET         0x0020
#define SPAWNFLAG_TURRET_HEATBEAM       0x0040
#define SPAWNFLAG_TURRET_WEAPONCHOICE   (SPAWNFLAG_TURRET_HEATBEAM | SPAWNFLAG_TURRET_ROCKET | SPAWNFLAG_TURRET_MACHINEGUN | SPAWNFLAG_TURRET_BLASTER)
#define SPAWNFLAG_TURRET_WALL_UNIT      0x0080
#define SPAWNFLAG_TURRET_NO_LASERSIGHT  BIT(18)

static void turret_ready_gun(edict_t *self);
void turret_run(edict_t *self);

const mmove_t turret_move_fire;
const mmove_t turret_move_fire_blind;

static int sound_moved, sound_moving;

static void TurretAim(edict_t *self)
{
    vec3_t end, dir;
    vec3_t ang;
    float  move, idealPitch, idealYaw, current, speed;
    int    orientation;

    if (!self->enemy || self->enemy == world) {
        if (!FindTarget(self))
            return;
    }

    // if turret is still in inactive mode, ready the gun, but don't aim
    if (self->s.frame < FRAME_active01) {
        turret_ready_gun(self);
        return;
    }
    // if turret is still readying, don't aim.
    if (self->s.frame < FRAME_run01)
        return;

    // PMM - blindfire aiming here
    if (self->monsterinfo.active_move == &turret_move_fire_blind) {
        VectorCopy(self->monsterinfo.blind_fire_target, end);
        if (self->enemy->s.origin[2] < self->monsterinfo.blind_fire_target[2])
            end[2] += self->enemy->viewheight + 10;
        else
            end[2] += self->enemy->mins[2] - 10;
    } else {
        VectorCopy(self->enemy->s.origin, end);
        if (self->enemy->client)
            end[2] += self->enemy->viewheight;
    }

    VectorSubtract(end, self->s.origin, dir);
    vectoangles(dir, ang);

    //
    // Clamp first
    //

    idealPitch = ang[PITCH];
    idealYaw = ang[YAW];

    orientation = (int)self->offset[1];
    switch (orientation) {
    case -1: // up      pitch: 0 to 90
        if (idealPitch < -90)
            idealPitch += 360;
        if (idealPitch > -5)
            idealPitch = -5;
        break;
    case -2: // down        pitch: -180 to -360
        if (idealPitch > -90)
            idealPitch -= 360;
        if (idealPitch < -355)
            idealPitch = -355;
        else if (idealPitch > -185)
            idealPitch = -185;
        break;
    case 0: // +X       pitch: 0 to -90, -270 to -360 (or 0 to 90)
        if (idealPitch < -180)
            idealPitch += 360;

        if (idealPitch > 85)
            idealPitch = 85;
        else if (idealPitch < -85)
            idealPitch = -85;

        //          yaw: 270 to 360, 0 to 90
        //          yaw: -90 to 90 (270-360 == -90-0)
        if (idealYaw > 180)
            idealYaw -= 360;
        if (idealYaw > 85)
            idealYaw = 85;
        else if (idealYaw < -85)
            idealYaw = -85;
        break;
    case 90: // +Y  pitch: 0 to 90, -270 to -360 (or 0 to 90)
        if (idealPitch < -180)
            idealPitch += 360;

        if (idealPitch > 85)
            idealPitch = 85;
        else if (idealPitch < -85)
            idealPitch = -85;

        //          yaw: 0 to 180
        if (idealYaw > 270)
            idealYaw -= 360;
        if (idealYaw > 175)
            idealYaw = 175;
        else if (idealYaw < 5)
            idealYaw = 5;

        break;
    case 180: // -X pitch: 0 to 90, -270 to -360 (or 0 to 90)
        if (idealPitch < -180)
            idealPitch += 360;

        if (idealPitch > 85)
            idealPitch = 85;
        else if (idealPitch < -85)
            idealPitch = -85;

        //          yaw: 90 to 270
        if (idealYaw > 265)
            idealYaw = 265;
        else if (idealYaw < 95)
            idealYaw = 95;

        break;
    case 270: // -Y pitch: 0 to 90, -270 to -360 (or 0 to 90)
        if (idealPitch < -180)
            idealPitch += 360;

        if (idealPitch > 85)
            idealPitch = 85;
        else if (idealPitch < -85)
            idealPitch = -85;

        //          yaw: 180 to 360
        if (idealYaw < 90)
            idealYaw += 360;
        if (idealYaw > 355)
            idealYaw = 355;
        else if (idealYaw < 185)
            idealYaw = 185;
        break;
    }

    //
    // adjust pitch
    //
    current = self->s.angles[PITCH];
    speed = self->yaw_speed / (TICK_RATE / 10);

    if (idealPitch != current) {
        move = idealPitch - current;

        while (move >= 360)
            move -= 360;
        if (move >= 90)
            move = move - 360;

        while (move <= -360)
            move += 360;
        if (move <= -90)
            move = move + 360;

        if (move > 0) {
            if (move > speed)
                move = speed;
        } else {
            if (move < -speed)
                move = -speed;
        }

        self->s.angles[PITCH] = anglemod(current + move);
    }

    //
    // adjust yaw
    //
    current = self->s.angles[YAW];

    if (idealYaw != current) {
        move = idealYaw - current;

        //      while(move >= 360)
        //          move -= 360;
        if (move >= 180)
            move = move - 360;

        //      while(move <= -360)
        //          move += 360;
        if (move <= -180)
            move = move + 360;

        if (move > 0) {
            if (move > speed)
                move = speed;
        } else {
            if (move < -speed)
                move = -speed;
        }

        self->s.angles[YAW] = anglemod(current + move);
    }

    if (self->spawnflags & SPAWNFLAG_TURRET_NO_LASERSIGHT)
        return;

    // Paril: improved turrets; draw lasersight
    if (!self->target_ent) {
        self->target_ent = G_Spawn();
        self->target_ent->s.modelindex = MODELINDEX_WORLD;
        self->target_ent->s.renderfx = RF_BEAM;
        self->target_ent->s.frame = 1;
        self->target_ent->s.skinnum = 0xf0f0f0f0;
        self->target_ent->classname = "turret_lasersight";
        VectorCopy(self->s.origin, self->target_ent->s.origin);
    }

    vec3_t forward;
    AngleVectors(self->s.angles, forward, NULL, NULL);
    VectorMA(self->s.origin, 8192, forward, end);
    trace_t tr = gi.trace(self->s.origin, NULL, NULL, end, self, MASK_SOLID);

    float scan_range = 64;

    if (visible(self, self->enemy))
        scan_range = 12;

    float sec = TO_SEC(level.time);
    tr.endpos[0] += sinf(sec + self->s.number) * scan_range;
    tr.endpos[1] += cosf((sec - self->s.number) * 3.0f) * scan_range;
    tr.endpos[2] += sinf((sec - self->s.number) * 2.5f) * scan_range;

    VectorSubtract(tr.endpos, self->s.origin, forward);
    VectorNormalize(forward);

    VectorMA(self->s.origin, 8192, forward, end);
    tr = gi.trace(self->s.origin, NULL, NULL, end, self, MASK_SOLID);

    VectorCopy(tr.endpos, self->target_ent->s.old_origin);
    gi.linkentity(self->target_ent);
}

void MONSTERINFO_SIGHT(turret_sight)(edict_t *self, edict_t *other)
{
}

void MONSTERINFO_SEARCH(turret_search)(edict_t *self)
{
}

static const mframe_t turret_frames_stand[] = {
    { ai_stand },
    { ai_stand }
};
const mmove_t MMOVE_T(turret_move_stand) = { FRAME_stand01, FRAME_stand02, turret_frames_stand, NULL };

void MONSTERINFO_STAND(turret_stand)(edict_t *self)
{
    M_SetAnimation(self, &turret_move_stand);
    if (self->target_ent) {
        G_FreeEdict(self->target_ent);
        self->target_ent = NULL;
    }
}

static const mframe_t turret_frames_ready_gun[] = {
    { ai_stand },
    { ai_stand },
    { ai_stand },

    { ai_stand },
    { ai_stand },
    { ai_stand },

    { ai_stand }
};
const mmove_t MMOVE_T(turret_move_ready_gun) = { FRAME_active01, FRAME_run01, turret_frames_ready_gun, turret_run };

static void turret_ready_gun(edict_t *self)
{
    if (self->monsterinfo.active_move != &turret_move_ready_gun) {
        M_SetAnimation(self, &turret_move_ready_gun);
        self->monsterinfo.weapon_sound = sound_moving;
    }
}

static const mframe_t turret_frames_seek[] = {
    { ai_walk, 0, TurretAim },
    { ai_walk, 0, TurretAim }
};
const mmove_t MMOVE_T(turret_move_seek) = { FRAME_run01, FRAME_run02, turret_frames_seek, NULL };

void MONSTERINFO_WALK(turret_walk)(edict_t *self)
{
    if (self->s.frame < FRAME_run01)
        turret_ready_gun(self);
    else
        M_SetAnimation(self, &turret_move_seek);
}

static const mframe_t turret_frames_run[] = {
    { ai_run, 0, TurretAim },
    { ai_run, 0, TurretAim }
};
const mmove_t MMOVE_T(turret_move_run) = { FRAME_run01, FRAME_run02, turret_frames_run, turret_run };

void MONSTERINFO_RUN(turret_run)(edict_t *self)
{
    if (self->s.frame < FRAME_run01)
        turret_ready_gun(self);
    else {
        self->monsterinfo.aiflags |= AI_HIGH_TICK_RATE;
        M_SetAnimation(self, &turret_move_run);

        if (self->monsterinfo.weapon_sound) {
            self->monsterinfo.weapon_sound = 0;
            gi.sound(self, CHAN_WEAPON, sound_moved, 1.0f, ATTN_NORM, 0.0f);
        }
    }
}

// **********************
//  ATTACK
// **********************

#define TURRET_BLASTER_DAMAGE   8
#define TURRET_BULLET_DAMAGE    2
// unused
//#define TURRET_HEAT_DAMAGE    4

static void TurretFire(edict_t *self)
{
    vec3_t  forward;
    vec3_t  start, end, dir;
    float   dist;
    trace_t trace;
    int     rocketSpeed;

    TurretAim(self);

    if (!self->enemy || !self->enemy->inuse)
        return;

    if (self->monsterinfo.aiflags & AI_LOST_SIGHT)
        VectorCopy(self->monsterinfo.blind_fire_target, end);
    else
        VectorCopy(self->enemy->s.origin, end);
    VectorSubtract(end, self->s.origin, dir);
    VectorNormalize(dir);
    AngleVectors(self->s.angles, forward, NULL, NULL);
    if (DotProduct(dir, forward) < 0.98f)
        return;

    if (self->spawnflags & SPAWNFLAG_TURRET_ROCKET)
        rocketSpeed = 650;
    else if (self->spawnflags & SPAWNFLAG_TURRET_BLASTER)
        rocketSpeed = 800;
    else
        rocketSpeed = 0;

    if ((self->spawnflags & SPAWNFLAG_TURRET_MACHINEGUN) || visible(self, self->enemy)) {
        VectorCopy(self->s.origin, start);

        // aim for the head.
        if (!(self->monsterinfo.aiflags & AI_LOST_SIGHT)) {
            if ((self->enemy) && (self->enemy->client))
                end[2] += self->enemy->viewheight;
            else
                end[2] += 22;
        }

        VectorSubtract(end, start, dir);
        dist = VectorNormalize(dir);

        // check for predictive fire
        // Paril: adjusted to be a bit more fair
        if (!(self->monsterinfo.aiflags & AI_LOST_SIGHT)) {
            // on harder difficulties, randomly fire directly at enemy
            // more often; makes them more unpredictable
            if (self->spawnflags & SPAWNFLAG_TURRET_MACHINEGUN)
                PredictAim(self, self->enemy, start, 0, true, 0.3f, dir, NULL);
            else if (frandom() < skill->integer / 5.0f)
                PredictAim(self, self->enemy, start, rocketSpeed, true, (frandom1(3.0f - skill->integer) / 3.0f) - frandom1(0.05f * (3.0f - skill->integer)), dir, NULL);
        }

        trace = gi.trace(start, NULL, NULL, end, self, MASK_PROJECTILE);
        if (trace.ent == self->enemy || trace.ent == world) {
            if (self->spawnflags & SPAWNFLAG_TURRET_BLASTER)
                monster_fire_blaster(self, start, dir, TURRET_BLASTER_DAMAGE, rocketSpeed, MZ2_TURRET_BLASTER, EF_BLASTER);
            else if (self->spawnflags & SPAWNFLAG_TURRET_MACHINEGUN) {
                if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME)) {
                    self->monsterinfo.aiflags |= AI_HOLD_FRAME;
                    self->monsterinfo.duck_wait_time = level.time + SEC(2 + frandom1(skill->value));
                    self->monsterinfo.next_duck_time = level.time + SEC(1);
                    gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_NORM, 0);
                } else {
                    if (self->monsterinfo.next_duck_time < level.time &&
                        self->monsterinfo.melee_debounce_time <= level.time) {
                        monster_fire_bullet(self, start, dir, TURRET_BULLET_DAMAGE, 0, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_TURRET_MACHINEGUN);
                        self->monsterinfo.melee_debounce_time = level.time + HZ(10);
                    }

                    if (self->monsterinfo.duck_wait_time < level.time)
                        self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
                }
            } else if (self->spawnflags & SPAWNFLAG_TURRET_ROCKET) {
                if (dist * trace.fraction > 72)
                    monster_fire_rocket(self, start, dir, 40, rocketSpeed, MZ2_TURRET_ROCKET);
            }
        }
    }
}

// PMM
static void TurretFireBlind(edict_t *self)
{
    vec3_t forward;
    vec3_t start, end, dir;
    int    rocketSpeed = 550;

    TurretAim(self);

    if (!self->enemy || !self->enemy->inuse)
        return;

    VectorSubtract(self->monsterinfo.blind_fire_target, self->s.origin, dir);
    VectorNormalize(dir);
    AngleVectors(self->s.angles, forward, NULL, NULL);
    if (DotProduct(dir, forward) < 0.98f)
        return;

    if (self->spawnflags & SPAWNFLAG_TURRET_ROCKET)
        rocketSpeed = 650;
    else if (self->spawnflags & SPAWNFLAG_TURRET_BLASTER)
        rocketSpeed = 800;
    else
        rocketSpeed = 0;

    VectorCopy(self->s.origin, start);
    VectorCopy(self->monsterinfo.blind_fire_target, end);

    if (self->enemy->s.origin[2] < self->monsterinfo.blind_fire_target[2])
        end[2] += self->enemy->viewheight + 10;
    else
        end[2] += self->enemy->mins[2] - 10;

    VectorSubtract(end, start, dir);
    VectorNormalize(dir);

    if (self->spawnflags & SPAWNFLAG_TURRET_BLASTER)
        monster_fire_blaster(self, start, dir, TURRET_BLASTER_DAMAGE, rocketSpeed, MZ2_TURRET_BLASTER, EF_BLASTER);
    else if (self->spawnflags & SPAWNFLAG_TURRET_ROCKET)
        monster_fire_rocket(self, start, dir, 40, rocketSpeed, MZ2_TURRET_ROCKET);
}
// pmm

static const mframe_t turret_frames_fire[] = {
    { ai_run, 0, TurretFire },
    { ai_run, 0, TurretAim },
    { ai_run, 0, TurretAim },
    { ai_run, 0, TurretAim }
};
const mmove_t MMOVE_T(turret_move_fire) = { FRAME_pow01, FRAME_pow04, turret_frames_fire, turret_run };

// PMM

// the blind frames need to aim first
static const mframe_t turret_frames_fire_blind[] = {
    { ai_run, 0, TurretAim },
    { ai_run, 0, TurretAim },
    { ai_run, 0, TurretAim },
    { ai_run, 0, TurretFireBlind }
};
const mmove_t MMOVE_T(turret_move_fire_blind) = { FRAME_pow01, FRAME_pow04, turret_frames_fire_blind, turret_run };
// pmm

void MONSTERINFO_ATTACK(turret_attack)(edict_t *self)
{
    if (self->s.frame < FRAME_run01)
        turret_ready_gun(self);
    // PMM
    else if (self->monsterinfo.attack_state != AS_BLIND)
        M_SetAnimation(self, &turret_move_fire);
    else {
        float chance;

        // setup shot probabilities
        if (self->monsterinfo.blind_fire_delay < SEC(1))
            chance = 1.0f;
        else if (self->monsterinfo.blind_fire_delay < SEC(7.5f))
            chance = 0.4f;
        else
            chance = 0.1f;

        // minimum of 3 seconds, plus 0-4, after the shots are done - total time should be max less than 7.5
        self->monsterinfo.blind_fire_delay += random_time_sec(3.4f, 7.4f);

        // don't shoot at the origin
        if (VectorEmpty(self->monsterinfo.blind_fire_target))
            return;

        // don't shoot if the dice say not to
        if (frandom() > chance)
            return;

        M_SetAnimation(self, &turret_move_fire_blind);
    }
    // pmm
}

// **********************
//  PAIN
// **********************

void PAIN(turret_pain)(edict_t *self, edict_t *other, float kick, int damage, mod_t mod)
{
}

// **********************
//  DEATH
// **********************

void DIE(turret_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    vec3_t   forward;
    edict_t *base;

    AngleVectors(self->s.angles, forward, NULL, NULL);
    VectorAdd(self->s.origin, forward, self->s.origin);

    for (int i = 0; i < 2; i++) {
        ThrowGib(self, "models/objects/debris1/tris.md2", 2, GIB_METALLIC | GIB_DEBRIS, self->x.scale);
        ThrowGib(self, "models/objects/debris1/tris.md2", 1, GIB_METALLIC | GIB_DEBRIS, self->x.scale);
    }

    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_PLAIN_EXPLOSION);
    gi.WritePosition(self->s.origin);
    gi.multicast(self->s.origin, MULTICAST_PHS);

    if (self->teamchain) {
        base = self->teamchain;
        base->solid = SOLID_NOT;
        base->takedamage = false;
        base->movetype = MOVETYPE_NONE;
        base->teammaster = base;
        base->teamchain = NULL;
        base->flags &= ~FL_TEAMSLAVE;
        base->flags |= FL_TEAMMASTER;
        gi.linkentity(base);

        self->teammaster = self->teamchain = NULL;
        self->flags &= ~(FL_TEAMSLAVE | FL_TEAMMASTER);
    }

    if (self->target) {
        if (self->enemy && self->enemy->inuse)
            G_UseTargets(self, self->enemy);
        else
            G_UseTargets(self, self);
    }

    if (self->target_ent) {
        G_FreeEdict(self->target_ent);
        self->target_ent = NULL;
    }

    edict_t *gib = ThrowGib(self, "models/monsters/turret/tris.md2", damage, GIB_SKINNED | GIB_METALLIC | GIB_HEAD | GIB_DEBRIS, self->x.scale);
    gib->s.frame = 14;
}

// **********************
//  WALL SPAWN
// **********************

static void turret_wall_spawn(edict_t *turret)
{
    edict_t *ent;
    int      angle;

    ent = G_Spawn();
    VectorCopy(turret->s.origin, ent->s.origin);
    VectorCopy(turret->s.angles, ent->s.angles);

    angle = (int)ent->s.angles[1];
    if (ent->s.angles[0] == 90)
        angle = -1;
    else if (ent->s.angles[0] == 270)
        angle = -2;
    switch (angle) {
    case -1:
        VectorSet(ent->mins, -16, -16, -8);
        VectorSet(ent->maxs, 16, 16, 0);
        break;
    case -2:
        VectorSet(ent->mins, -16, -16, 0);
        VectorSet(ent->maxs, 16, 16, 8);
        break;
    case 0:
        VectorSet(ent->mins, -8, -16, -16);
        VectorSet(ent->maxs, 0, 16, 16);
        break;
    case 90:
        VectorSet(ent->mins, -16, -8, -16);
        VectorSet(ent->maxs, 16, 0, 16);
        break;
    case 180:
        VectorSet(ent->mins, 0, -16, -16);
        VectorSet(ent->maxs, 8, 16, 16);
        break;
    case 270:
        VectorSet(ent->mins, -16, 0, -16);
        VectorSet(ent->maxs, 16, 8, 16);
        break;
    }

    ent->movetype = MOVETYPE_PUSH;
    ent->solid = SOLID_NOT;

    ent->teammaster = turret;
    turret->flags |= FL_TEAMMASTER;
    turret->teammaster = turret;
    turret->teamchain = ent;
    ent->teamchain = NULL;
    ent->flags |= FL_TEAMSLAVE;
    ent->owner = turret;

    ent->s.modelindex = gi.modelindex("models/monsters/turretbase/tris.md2");

    gi.linkentity(ent);
}

void MOVEINFO_ENDFUNC(turret_wake)(edict_t *ent)
{
    // the wall section will call this when it stops moving.
    // just return without doing anything. easiest way to have a null function.
    if (ent->flags & FL_TEAMSLAVE) {
        ent->s.sound = 0;
        return;
    }

    ent->monsterinfo.stand = turret_stand;
    ent->monsterinfo.walk = turret_walk;
    ent->monsterinfo.run = turret_run;
    ent->monsterinfo.dodge = NULL;
    ent->monsterinfo.attack = turret_attack;
    ent->monsterinfo.melee = NULL;
    ent->monsterinfo.sight = turret_sight;
    ent->monsterinfo.search = turret_search;
    M_SetAnimation(ent, &turret_move_stand);
    ent->takedamage = true;
    ent->movetype = MOVETYPE_NONE;
    // prevent counting twice
    ent->monsterinfo.aiflags |= AI_DO_NOT_COUNT;

    gi.linkentity(ent);

    stationarymonster_start(ent);

    if (ent->spawnflags & SPAWNFLAG_TURRET_MACHINEGUN)
        ent->s.skinnum = 1;
    else if (ent->spawnflags & SPAWNFLAG_TURRET_ROCKET)
        ent->s.skinnum = 2;

    // but we do want the death to count
    ent->monsterinfo.aiflags &= ~AI_DO_NOT_COUNT;
}

void USE(turret_activate)(edict_t *self, edict_t *other, edict_t *activator)
{
    vec3_t   endpos;
    vec3_t   forward;
    edict_t *base;

    self->movetype = MOVETYPE_PUSH;
    if (!self->speed)
        self->speed = 15;
    self->moveinfo.speed = self->speed;
    self->moveinfo.accel = self->speed;
    self->moveinfo.decel = self->speed;

    if (self->s.angles[0] == 270)
        VectorSet(forward, 0, 0, 1);
    else if (self->s.angles[0] == 90)
        VectorSet(forward, 0, 0, -1);
    else if (self->s.angles[1] == 0)
        VectorSet(forward, 1, 0, 0);
    else if (self->s.angles[1] == 90)
        VectorSet(forward, 0, 1, 0);
    else if (self->s.angles[1] == 180)
        VectorSet(forward, -1, 0, 0);
    else if (self->s.angles[1] == 270)
        VectorSet(forward, 0, -1, 0);
    else
        return;

    // start up the turret
    VectorMA(self->s.origin, 32, forward, endpos);
    Move_Calc(self, endpos, turret_wake);

    base = self->teamchain;
    if (base) {
        base->movetype = MOVETYPE_PUSH;
        base->speed = self->speed;
        base->moveinfo.speed = base->speed;
        base->moveinfo.accel = base->speed;
        base->moveinfo.decel = base->speed;

        // start up the wall section
        VectorMA(self->teamchain->s.origin, 32, forward, endpos);
        Move_Calc(self->teamchain, endpos, turret_wake);

        base->s.sound = sound_moving;
        base->x.loop_attenuation = ATTN_NORM;
    }
}

// PMM
// checkattack .. ignore range, just attack if available
bool MONSTERINFO_CHECKATTACK(turret_checkattack)(edict_t *self)
{
    vec3_t  spot1, spot2;
    float   chance;
    trace_t tr;

    if (level.time < self->monsterinfo.attack_finished)
        return false;

    if (self->enemy->health > 0) {
        // see if any entities are in the way of the shot
        VectorCopy(self->s.origin, spot1);
        spot1[2] += self->viewheight;
        VectorCopy(self->enemy->s.origin, spot2);
        spot2[2] += self->enemy->viewheight;

        tr = gi.trace(spot1, NULL, NULL, spot2, self, CONTENTS_SOLID | CONTENTS_PLAYER | CONTENTS_MONSTER | CONTENTS_SLIME | CONTENTS_LAVA | CONTENTS_WINDOW);

        // do we have a clear shot?
        if (tr.ent != self->enemy && !(tr.ent->svflags & SVF_PLAYER)) {
            // PGM - we want them to go ahead and shoot at info_notnulls if they can.
            if (self->enemy->solid != SOLID_NOT || tr.fraction < 1.0f) { // PGM
                // PMM - if we can't see our target, and we're not blocked by a monster, go into blind fire if available
                if ((!(tr.ent->svflags & SVF_MONSTER)) && (!visible(self, self->enemy))) {
                    if ((self->monsterinfo.blindfire) && (self->monsterinfo.blind_fire_delay <= SEC(10))) {
                        if (level.time < (self->monsterinfo.trail_time + self->monsterinfo.blind_fire_delay))
                            // wait for our time
                            return false;
                        // make sure we're not going to shoot something we don't want to shoot
                        tr = gi.trace(spot1, NULL, NULL, self->monsterinfo.blind_fire_target, self, CONTENTS_MONSTER | CONTENTS_PLAYER);
                        if (tr.allsolid || tr.startsolid || ((tr.fraction < 1.0f) && (tr.ent != self->enemy && !(tr.ent->svflags & SVF_PLAYER))))
                            return false;
                        self->monsterinfo.attack_state = AS_BLIND;
                        self->monsterinfo.attack_finished = level.time + random_time_sec(0.5f, 2.5f);
                        return true;
                    }
                }
                // pmm
                return false;
            }
        }
    }

    gtime_t nexttime;

    if (self->spawnflags & SPAWNFLAG_TURRET_ROCKET) {
        chance = 0.10f;
        nexttime = SEC(1.8f - (0.2f * skill->integer));
    } else if (self->spawnflags & SPAWNFLAG_TURRET_BLASTER) {
        chance = 0.35f;
        nexttime = SEC(1.2f - (0.2f * skill->integer));
    } else {
        chance = 0.50f;
        nexttime = SEC(0.8f - (0.1f * skill->integer));
    }

    if (skill->integer == 0)
        chance *= 0.5f;
    else if (skill->integer > 1)
        chance *= 2;

    // PGM - go ahead and shoot every time if it's a info_notnull
    // PMM - added visibility check
    if (((frandom() < chance) && (visible(self, self->enemy))) || (self->enemy->solid == SOLID_NOT)) {
        self->monsterinfo.attack_state = AS_MISSILE;
        self->monsterinfo.attack_finished = level.time + nexttime;
        return true;
    }

    self->monsterinfo.attack_state = AS_STRAIGHT;

    return false;
}

// **********************
//  SPAWN
// **********************

static void turret_precache(void)
{
    sound_moved = gi.soundindex("turret/moved.wav");
    sound_moving = gi.soundindex("turret/moving.wav");
}

/*QUAKED monster_turret (1 .5 0) (-16 -16 -16) (16 16 16) Ambush Trigger_Spawn Sight Blaster MachineGun Rocket Heatbeam WallUnit

The automated defense turret that mounts on walls.
Check the weapon you want it to use: blaster, machinegun, rocket, heatbeam.
Default weapon is blaster.
When activated, wall units move 32 units in the direction they're facing.
*/
void SP_monster_turret(edict_t *self)
{
    int angle;

    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    // pre-caches
    G_AddPrecache(turret_precache);

    gi.modelindex("models/objects/debris1/tris.md2");

    self->s.modelindex = gi.modelindex("models/monsters/turret/tris.md2");

    VectorSet(self->mins, -12, -12, -12);
    VectorSet(self->maxs, 12, 12, 12);
    self->movetype = MOVETYPE_NONE;
    self->solid = SOLID_BBOX;

    self->health = 50 * st.health_multiplier;
    self->gib_health = -100;
    self->mass = 250;
    self->yaw_speed = 10 * skill->integer;

    self->monsterinfo.armor_type = IT_ARMOR_COMBAT;
    self->monsterinfo.armor_power = 50;

    self->flags |= FL_MECHANICAL;

    self->pain = turret_pain;
    self->die = turret_die;

    // map designer didn't specify weapon type. set it now.
    if (!(self->spawnflags & SPAWNFLAG_TURRET_WEAPONCHOICE))
        self->spawnflags |= SPAWNFLAG_TURRET_BLASTER;

    if (self->spawnflags & SPAWNFLAG_TURRET_HEATBEAM) {
        self->spawnflags &= ~SPAWNFLAG_TURRET_HEATBEAM;
        self->spawnflags |= SPAWNFLAG_TURRET_BLASTER;
    }

    if (!(self->spawnflags & SPAWNFLAG_TURRET_WALL_UNIT)) {
        self->monsterinfo.stand = turret_stand;
        self->monsterinfo.walk = turret_walk;
        self->monsterinfo.run = turret_run;
        self->monsterinfo.dodge = NULL;
        self->monsterinfo.attack = turret_attack;
        self->monsterinfo.melee = NULL;
        self->monsterinfo.sight = turret_sight;
        self->monsterinfo.search = turret_search;
        M_SetAnimation(self, &turret_move_stand);
    }

    // PMM
    self->monsterinfo.checkattack = turret_checkattack;

    self->monsterinfo.aiflags |= AI_MANUAL_STEERING;
    self->monsterinfo.scale = MODEL_SCALE;
    self->gravity = 0;

    VectorCopy(self->s.angles, self->offset);
    angle = (int)self->s.angles[1];
    switch (angle) {
    case -1: // up
        self->s.angles[0] = 270;
        self->s.angles[1] = 0;
        self->s.origin[2] += 2;
        break;
    case -2: // down
        self->s.angles[0] = 90;
        self->s.angles[1] = 0;
        self->s.origin[2] -= 2;
        break;
    case 0:
        self->s.origin[0] += 2;
        break;
    case 90:
        self->s.origin[1] += 2;
        break;
    case 180:
        self->s.origin[0] -= 2;
        break;
    case 270:
        self->s.origin[1] -= 2;
        break;
    default:
        break;
    }

    gi.linkentity(self);

    if (self->spawnflags & SPAWNFLAG_TURRET_WALL_UNIT) {
        if (!self->targetname) {
            G_FreeEdict(self);
            return;
        }

        self->takedamage = false;
        self->use = turret_activate;
        turret_wall_spawn(self);
        if (!(self->monsterinfo.aiflags & AI_DO_NOT_COUNT)) {
            if (g_debug_monster_kills->integer)
                level.monsters_registered[level.total_monsters] = self;
            level.total_monsters++;
        }
    } else {
        stationarymonster_start(self);
    }

    if (self->spawnflags & SPAWNFLAG_TURRET_MACHINEGUN) {
        gi.soundindex("infantry/infatck1.wav");
        gi.soundindex("weapons/chngnu1a.wav");
        self->s.skinnum = 1;

        self->spawnflags &= ~SPAWNFLAG_TURRET_WEAPONCHOICE;
        self->spawnflags |= SPAWNFLAG_TURRET_MACHINEGUN;
    } else if (self->spawnflags & SPAWNFLAG_TURRET_ROCKET) {
        gi.soundindex("weapons/rockfly.wav");
        gi.modelindex("models/objects/rocket/tris.md2");
        gi.soundindex("chick/chkatck2.wav");
        self->s.skinnum = 2;

        self->spawnflags &= ~SPAWNFLAG_TURRET_WEAPONCHOICE;
        self->spawnflags |= SPAWNFLAG_TURRET_ROCKET;
    } else {
        gi.modelindex("models/objects/laser/tris.md2");
        gi.soundindex("misc/lasfly.wav");
        gi.soundindex("soldier/solatck2.wav");

        self->spawnflags &= ~SPAWNFLAG_TURRET_WEAPONCHOICE;
        self->spawnflags |= SPAWNFLAG_TURRET_BLASTER;
    }

    // PMM  - turrets don't get mad at monsters, and visa versa
    self->monsterinfo.aiflags |= AI_IGNORE_SHOTS;
    // PMM - blindfire
    if (self->spawnflags & (SPAWNFLAG_TURRET_ROCKET | SPAWNFLAG_TURRET_BLASTER))
        self->monsterinfo.blindfire = true;
}
