// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include <float.h>

/*
=========================================================

  PLATS

  movement options:

  linear
  smooth start, hard stop
  smooth start, smooth stop

  start
  end
  acceleration
  speed
  deceleration
  begin sound
  end sound
  target fired when reaching end
  wait at end

  object characteristics that use move segments
  ---------------------------------------------
  movetype_push, or movetype_stop
  action when touched
  action when blocked
  action when used
    disabled?
  auto trigger spawning


=========================================================
*/

#define SPAWNFLAG_DOOR_START_OPEN       1
#define SPAWNFLAG_DOOR_CRUSHER          4
#define SPAWNFLAG_DOOR_NOMONSTER        8
#define SPAWNFLAG_DOOR_ANIMATED         16
#define SPAWNFLAG_DOOR_TOGGLE           32
#define SPAWNFLAG_DOOR_ANIMATED_FAST    64

#define SPAWNFLAG_DOOR_ROTATING_X_AXIS      64
#define SPAWNFLAG_DOOR_ROTATING_Y_AXIS      128
#define SPAWNFLAG_DOOR_ROTATING_INACTIVE    0x10000 // Paril: moved to non-reserved
#define SPAWNFLAG_DOOR_ROTATING_SAFE_OPEN   0x20000
#define SPAWNFLAG_DOOR_ROTATING_NO_COLLISION 0x40000

// support routine for setting moveinfo sounds
static int G_GetMoveinfoSoundIndex(edict_t *self, const char *default_value, const char *wanted_value)
{
    if (!wanted_value) {
        if (default_value)
            return gi.soundindex(default_value);
        return 0;
    }

    if (!*wanted_value || *wanted_value == '0' || *wanted_value == ' ')
        return 0;

    return gi.soundindex(wanted_value);
}

void G_SetMoveinfoSounds(edict_t *self, const char *default_start, const char *default_mid, const char *default_end)
{
    self->moveinfo.sound_start = G_GetMoveinfoSoundIndex(self, default_start, st.noise_start);
    self->moveinfo.sound_middle = G_GetMoveinfoSoundIndex(self, default_mid, st.noise_middle);
    self->moveinfo.sound_end = G_GetMoveinfoSoundIndex(self, default_end, st.noise_end);
}

//
// Support routines for movement (changes in origin using velocity)
//

void THINK(Move_Done)(edict_t *ent)
{
    VectorClear(ent->velocity);
    ent->moveinfo.endfunc(ent);
}

void THINK(Move_Final)(edict_t *ent)
{
    if (ent->moveinfo.remaining_distance == 0) {
        Move_Done(ent);
        return;
    }

    // [Paril-KEX] use exact remaining distance
    vec3_t dir;
    VectorSubtract(ent->moveinfo.dest, ent->s.origin, dir);
    VectorScale(dir, 1.0f / FRAME_TIME_SEC, ent->velocity);

    ent->think = Move_Done;
    ent->nextthink = level.time + FRAME_TIME;
}

void THINK(Move_Begin)(edict_t *ent)
{
    float frames;

    if ((ent->moveinfo.speed * FRAME_TIME_SEC) >= ent->moveinfo.remaining_distance) {
        Move_Final(ent);
        return;
    }
    VectorScale(ent->moveinfo.dir, ent->moveinfo.speed, ent->velocity);
    frames = floorf((ent->moveinfo.remaining_distance / ent->moveinfo.speed) / FRAME_TIME_SEC);
    ent->moveinfo.remaining_distance -= frames * ent->moveinfo.speed * FRAME_TIME_SEC;
    ent->nextthink = level.time + (FRAME_TIME * frames);
    ent->think = Move_Final;
}

void Think_AccelMove(edict_t *ent);

void Move_Calc(edict_t *ent, const vec3_t dest, void (*endfunc)(edict_t *self))
{
    VectorClear(ent->velocity);
    VectorCopy(dest, ent->moveinfo.dest);
    VectorSubtract(dest, ent->s.origin, ent->moveinfo.dir);
    ent->moveinfo.remaining_distance = VectorNormalize(ent->moveinfo.dir);
    ent->moveinfo.endfunc = endfunc;

    if (ent->moveinfo.speed == ent->moveinfo.accel && ent->moveinfo.speed == ent->moveinfo.decel) {
        if (level.current_entity == ((ent->flags & FL_TEAMSLAVE) ? ent->teammaster : ent)) {
            Move_Begin(ent);
        } else {
            ent->nextthink = level.time + FRAME_TIME;
            ent->think = Move_Begin;
        }
    } else {
        // accelerative
        ent->moveinfo.current_speed = 0;
        ent->think = Think_AccelMove;
        ent->nextthink = level.time + FRAME_TIME;
    }
}

//
// Support routines for angular movement (changes in angle using avelocity)
//

void THINK(AngleMove_Done)(edict_t *ent)
{
    VectorClear(ent->avelocity);
    ent->moveinfo.endfunc(ent);
}

static void AngleMove_Get(edict_t *ent, vec3_t move)
{
    if (ent->moveinfo.state != STATE_UP)
        VectorSubtract(ent->moveinfo.start_angles, ent->s.angles, move);
    else if (ent->moveinfo.reversing)
        VectorSubtract(ent->moveinfo.end_angles_reversed, ent->s.angles, move);
    else
        VectorSubtract(ent->moveinfo.end_angles, ent->s.angles, move);
}

void THINK(AngleMove_Final)(edict_t *ent)
{
    vec3_t move;

    AngleMove_Get(ent, move);

    if (VectorEmpty(move)) {
        AngleMove_Done(ent);
        return;
    }

    VectorScale(move, 1.0f / FRAME_TIME_SEC, ent->avelocity);

    ent->think = AngleMove_Done;
    ent->nextthink = level.time + FRAME_TIME;
}

void THINK(AngleMove_Begin)(edict_t *ent)
{
    vec3_t destdelta;
    float  len;
    float  traveltime;
    float  frames;

    // PGM
    // accelerate as needed
    if (ent->moveinfo.speed < ent->speed) {
        ent->moveinfo.speed += ent->accel;
        if (ent->moveinfo.speed > ent->speed)
            ent->moveinfo.speed = ent->speed;
    }
    // PGM

    // set destdelta to the vector needed to move
    AngleMove_Get(ent, destdelta);

    // calculate length of vector
    len = VectorLength(destdelta);

    // divide by speed to get time to reach dest
    traveltime = len / ent->moveinfo.speed;

    if (traveltime < FRAME_TIME_SEC) {
        AngleMove_Final(ent);
        return;
    }

    frames = floorf(traveltime / FRAME_TIME_SEC);

    // scale the destdelta vector by the time spent traveling to get velocity
    VectorScale(destdelta, 1.0f / traveltime, ent->avelocity);

    // PGM
    //  if we're done accelerating, act as a normal rotation
    if (ent->moveinfo.speed >= ent->speed) {
        // set nextthink to trigger a think when dest is reached
        ent->nextthink = level.time + (FRAME_TIME * frames);
        ent->think = AngleMove_Final;
    } else {
        ent->nextthink = level.time + FRAME_TIME;
        ent->think = AngleMove_Begin;
    }
    // PGM
}

static void AngleMove_Calc(edict_t *ent, void (*endfunc)(edict_t *self))
{
    VectorClear(ent->avelocity);
    ent->moveinfo.endfunc = endfunc;

    // PGM
    //  if we're supposed to accelerate, this will tell anglemove_begin to do so
    if (ent->accel != ent->speed)
        ent->moveinfo.speed = 0;
    // PGM

    if (level.current_entity == ((ent->flags & FL_TEAMSLAVE) ? ent->teammaster : ent)) {
        AngleMove_Begin(ent);
    } else {
        ent->nextthink = level.time + FRAME_TIME;
        ent->think = AngleMove_Begin;
    }
}

/*
==============
Think_AccelMove

The team has completed a frame of movement, so
change the speed for the next frame
==============
*/
static float AccelerationDistance(float target, float rate)
{
    return (target * ((target / rate) + 1) / 2);
}

static void plat_CalcAcceleratedMove(moveinfo_t *moveinfo)
{
    float accel_dist;
    float decel_dist;

    if (moveinfo->remaining_distance < moveinfo->accel) {
        moveinfo->move_speed = moveinfo->speed;
        moveinfo->current_speed = moveinfo->remaining_distance;
        return;
    }

    accel_dist = AccelerationDistance(moveinfo->speed, moveinfo->accel);
    decel_dist = AccelerationDistance(moveinfo->speed, moveinfo->decel);

    if ((moveinfo->remaining_distance - accel_dist - decel_dist) < 0) {
        float f;

        f = (moveinfo->accel + moveinfo->decel) / (moveinfo->accel * moveinfo->decel);
        f = (-2 + sqrtf(4 - 4 * f * (-2 * moveinfo->remaining_distance))) / (2 * f);

        moveinfo->move_speed = moveinfo->current_speed = f;
        decel_dist = AccelerationDistance(moveinfo->move_speed, moveinfo->decel);
    } else
        moveinfo->move_speed = moveinfo->speed;

    moveinfo->decel_distance = decel_dist;
}

static void plat_Accelerate(moveinfo_t *moveinfo)
{
    // are we decelerating?
    if (moveinfo->remaining_distance <= moveinfo->decel_distance) {
        if (moveinfo->remaining_distance < moveinfo->decel_distance) {
            if (moveinfo->next_speed) {
                moveinfo->current_speed = moveinfo->next_speed;
                moveinfo->next_speed = 0;
                return;
            }
            if (moveinfo->current_speed > moveinfo->decel) {
                moveinfo->current_speed -= moveinfo->decel;

                // [Paril-KEX] fix platforms in xdm6, etc
                if (fabsf(moveinfo->current_speed) < 0.01f)
                    moveinfo->current_speed = moveinfo->remaining_distance + 1;
            }
        }
        return;
    }

    // are we at full speed and need to start decelerating during this move?
    if (moveinfo->current_speed == moveinfo->move_speed)
        if ((moveinfo->remaining_distance - moveinfo->current_speed) < moveinfo->decel_distance) {
            float p1_distance;
            float p2_distance;
            float distance;

            p1_distance = moveinfo->remaining_distance - moveinfo->decel_distance;
            p2_distance = moveinfo->move_speed * (1.0f - (p1_distance / moveinfo->move_speed));
            distance = p1_distance + p2_distance;
            moveinfo->current_speed = moveinfo->move_speed;
            moveinfo->next_speed = moveinfo->move_speed - moveinfo->decel * (p2_distance / distance);
            return;
        }

    // are we accelerating?
    if (moveinfo->current_speed < moveinfo->speed) {
        float old_speed;
        float p1_distance;
        float p1_speed;
        float p2_distance;
        float distance;

        old_speed = moveinfo->current_speed;

        // figure simple acceleration up to move_speed
        moveinfo->current_speed += moveinfo->accel;
        if (moveinfo->current_speed > moveinfo->speed)
            moveinfo->current_speed = moveinfo->speed;

        // are we accelerating throughout this entire move?
        if ((moveinfo->remaining_distance - moveinfo->current_speed) >= moveinfo->decel_distance)
            return;

        // during this move we will accelerate from current_speed to move_speed
        // and cross over the decel_distance; figure the average speed for the
        // entire move
        p1_distance = moveinfo->remaining_distance - moveinfo->decel_distance;
        p1_speed = (old_speed + moveinfo->move_speed) / 2.0f;
        p2_distance = moveinfo->move_speed * (1.0f - (p1_distance / p1_speed));
        distance = p1_distance + p2_distance;
        moveinfo->current_speed = (p1_speed * (p1_distance / distance)) + (moveinfo->move_speed * (p2_distance / distance));
        moveinfo->next_speed = moveinfo->move_speed - moveinfo->decel * (p2_distance / distance);
        return;
    }

    // we are at constant velocity (move_speed)
    return;
}

void THINK(Think_AccelMove)(edict_t *ent)
{
    // [Paril-KEX] calculate distance dynamically
    if (ent->moveinfo.state == STATE_UP)
        ent->moveinfo.remaining_distance = Distance(ent->moveinfo.start_origin, ent->s.origin);
    else
        ent->moveinfo.remaining_distance = Distance(ent->moveinfo.end_origin, ent->s.origin);

    if (ent->moveinfo.current_speed == 0)       // starting or blocked
        plat_CalcAcceleratedMove(&ent->moveinfo);

    plat_Accelerate(&ent->moveinfo);

    // will the entire move complete on next frame?
    if (ent->moveinfo.remaining_distance <= ent->moveinfo.current_speed) {
        Move_Final(ent);
        return;
    }

    VectorScale(ent->moveinfo.dir, ent->moveinfo.current_speed * 10, ent->velocity);
    ent->nextthink = level.time + HZ(10);
    ent->think = Think_AccelMove;
}

void plat_go_down(edict_t *ent);

void MOVEINFO_ENDFUNC(plat_hit_top)(edict_t *ent)
{
    if (!(ent->flags & FL_TEAMSLAVE) && ent->moveinfo.sound_end)
        gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent->moveinfo.sound_end, 1, ATTN_STATIC, 0);

    ent->s.sound = 0;
    ent->moveinfo.state = STATE_TOP;

    ent->think = plat_go_down;
    ent->nextthink = level.time + SEC(3);
}

void MOVEINFO_ENDFUNC(plat_hit_bottom)(edict_t *ent)
{
    if (!(ent->flags & FL_TEAMSLAVE) && ent->moveinfo.sound_end)
        gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent->moveinfo.sound_end, 1, ATTN_STATIC, 0);

    ent->s.sound = 0;
    ent->moveinfo.state = STATE_BOTTOM;

    // ROGUE
    plat2_kill_danger_area(ent);
    // ROGUE
}

void THINK(plat_go_down)(edict_t *ent)
{
    if (!(ent->flags & FL_TEAMSLAVE) && ent->moveinfo.sound_start)
        gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent->moveinfo.sound_start, 1, ATTN_STATIC, 0);

    ent->s.sound = ent->moveinfo.sound_middle;

    ent->moveinfo.state = STATE_DOWN;
    Move_Calc(ent, ent->moveinfo.end_origin, plat_hit_bottom);
}

static void plat_go_up(edict_t *ent)
{
    if (!(ent->flags & FL_TEAMSLAVE) && ent->moveinfo.sound_start)
        gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent->moveinfo.sound_start, 1, ATTN_STATIC, 0);

    ent->s.sound = ent->moveinfo.sound_middle;

    ent->moveinfo.state = STATE_UP;
    Move_Calc(ent, ent->moveinfo.start_origin, plat_hit_top);

    // ROGUE
    plat2_spawn_danger_area(ent);
    // ROGUE
}

void MOVEINFO_BLOCKED(plat_blocked)(edict_t *self, edict_t *other)
{
    if (!(other->svflags & SVF_MONSTER) && (!other->client)) {
        // give it a chance to go away on it's own terms (like gibs)
        T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, DAMAGE_NONE, (mod_t) { MOD_CRUSH });
        // if it's still there, nuke it
        if (other->inuse && other->solid) // PGM
            BecomeExplosion1(other);
        return;
    }

    // PGM
    //  gib dead things
    if (other->health < 1)
        T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 100, 1, DAMAGE_NONE, (mod_t) { MOD_CRUSH });
    // PGM

    T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, DAMAGE_NONE, (mod_t) { MOD_CRUSH });

    // [Paril-KEX] killed the thing, so don't switch directions
    if (!other->inuse || !other->solid)
        return;

    if (self->moveinfo.state == STATE_UP)
        plat_go_down(self);
    else if (self->moveinfo.state == STATE_DOWN)
        plat_go_up(self);
}

#define SPAWNFLAG_PLAT_LOW_TRIGGER  1
#define SPAWNFLAG_PLAT_NO_MONSTER   2

void USE(Use_Plat)(edict_t *ent, edict_t *other, edict_t *activator)
{
    //======
    // ROGUE
    // if a monster is using us, then allow the activity when stopped.
    if ((other->svflags & SVF_MONSTER) && !(ent->spawnflags & SPAWNFLAG_PLAT_NO_MONSTER)) {
        if (ent->moveinfo.state == STATE_TOP)
            plat_go_down(ent);
        else if (ent->moveinfo.state == STATE_BOTTOM)
            plat_go_up(ent);
        return;
    }
    // ROGUE
    //======

    if (ent->think)
        return; // already down
    plat_go_down(ent);
}

void TOUCH(Touch_Plat_Center)(edict_t *ent, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if (!other->client)
        return;

    if (other->health <= 0)
        return;

    ent = ent->enemy; // now point at the plat, not the trigger
    if (ent->moveinfo.state == STATE_BOTTOM)
        plat_go_up(ent);
    else if (ent->moveinfo.state == STATE_TOP)
        ent->nextthink = level.time + SEC(1); // the player is still on the plat, so delay going down
}

// PGM - plat2's change the trigger field
edict_t *plat_spawn_inside_trigger(edict_t *ent)
{
    edict_t *trigger;
    vec3_t   tmin, tmax;

    //
    // middle trigger
    //
    trigger = G_Spawn();
    trigger->touch = Touch_Plat_Center;
    trigger->movetype = MOVETYPE_NONE;
    trigger->solid = SOLID_TRIGGER;
    trigger->enemy = ent;

    tmin[0] = ent->mins[0] + 25;
    tmin[1] = ent->mins[1] + 25;
    tmin[2] = ent->mins[2];

    tmax[0] = ent->maxs[0] - 25;
    tmax[1] = ent->maxs[1] - 25;
    tmax[2] = ent->maxs[2] + 8;

    tmin[2] = tmax[2] - (ent->pos1[2] - ent->pos2[2] + st.lip);

    if (ent->spawnflags & SPAWNFLAG_PLAT_LOW_TRIGGER)
        tmax[2] = tmin[2] + 8;

    if (tmax[0] - tmin[0] <= 0) {
        tmin[0] = (ent->mins[0] + ent->maxs[0]) * 0.5f;
        tmax[0] = tmin[0] + 1;
    }
    if (tmax[1] - tmin[1] <= 0) {
        tmin[1] = (ent->mins[1] + ent->maxs[1]) * 0.5f;
        tmax[1] = tmin[1] + 1;
    }

    VectorCopy(tmin, trigger->mins);
    VectorCopy(tmax, trigger->maxs);

    gi.linkentity(trigger);

    return trigger; // PGM 11/17/97
}

/*QUAKED func_plat (0 .5 .8) ? PLAT_LOW_TRIGGER
speed   default 150

Plats are always drawn in the extended position, so they will light correctly.

If the plat is the target of another trigger or button, it will start out disabled in the extended position until it is triggered, when it will lower and become a normal plat.

"speed" overrides default 200.
"accel" overrides default 500
"lip"   overrides default 8 pixel lip

If the "height" key is set, that will determine the amount the plat moves, instead of being implicitly determined by the model's height.

Set "sounds" to one of the following:
1) base fast
2) chain slow
*/
void SP_func_plat(edict_t *ent)
{
    VectorClear(ent->s.angles);
    ent->solid = SOLID_BSP;
    ent->movetype = MOVETYPE_PUSH;

    gi.setmodel(ent, ent->model);

    ent->moveinfo.blocked = plat_blocked;

    if (!ent->speed)
        ent->speed = 20;
    else
        ent->speed *= 0.1f;

    if (!ent->accel)
        ent->accel = 5;
    else
        ent->accel *= 0.1f;

    if (!ent->decel)
        ent->decel = 5;
    else
        ent->decel *= 0.1f;

    if (!ent->dmg)
        ent->dmg = 2;

    if (!ED_WasKeySpecified("lip"))
        st.lip = 8;

    // pos1 is the top position, pos2 is the bottom
    VectorCopy(ent->s.origin, ent->pos1);
    VectorCopy(ent->s.origin, ent->pos2);
    if (st.height)
        ent->pos2[2] -= st.height;
    else
        ent->pos2[2] -= (ent->maxs[2] - ent->mins[2]) - st.lip;

    ent->use = Use_Plat;

    plat_spawn_inside_trigger(ent); // the "start moving" trigger

    if (ent->targetname) {
        ent->moveinfo.state = STATE_UP;
    } else {
        VectorCopy(ent->pos2, ent->s.origin);
        gi.linkentity(ent);
        ent->moveinfo.state = STATE_BOTTOM;
    }

    ent->moveinfo.speed = ent->speed;
    ent->moveinfo.accel = ent->accel;
    ent->moveinfo.decel = ent->decel;
    ent->moveinfo.wait = ent->wait;
    VectorCopy(ent->pos1, ent->moveinfo.start_origin);
    VectorCopy(ent->pos2, ent->moveinfo.end_origin);
    VectorCopy(ent->s.angles, ent->moveinfo.start_angles);
    VectorCopy(ent->s.angles, ent->moveinfo.end_angles);

    G_SetMoveinfoSounds(ent, "plats/pt1_strt.wav", "plats/pt1_mid.wav", "plats/pt1_end.wav");
}

//====================================================================

// Paril: Rogue added a spawnflag in func_rotating that
// is a reserved editor flag.
#define SPAWNFLAG_ROTATING_START_ON         1
#define SPAWNFLAG_ROTATING_REVERSE          2
#define SPAWNFLAG_ROTATING_X_AXIS           4
#define SPAWNFLAG_ROTATING_Y_AXIS           8
#define SPAWNFLAG_ROTATING_TOUCH_PAIN       16
#define SPAWNFLAG_ROTATING_STOP             32
#define SPAWNFLAG_ROTATING_ANIMATED         64
#define SPAWNFLAG_ROTATING_ANIMATED_FAST    128
#define SPAWNFLAG_ROTATING_ACCEL            0x10000

/*QUAKED func_rotating (0 .5 .8) ? START_ON REVERSE X_AXIS Y_AXIS TOUCH_PAIN STOP ANIMATED ANIMATED_FAST NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP RESERVED1 COOP_ONLY RESERVED2 ACCEL
You need to have an origin brush as part of this entity.
The center of that brush will be the point around which it is rotated. It will rotate around the Z axis by default.
You can check either the X_AXIS or Y_AXIS box to change that.

func_rotating will use it's targets when it stops and starts.

"speed" determines how fast it moves; default value is 100.
"dmg"   damage to inflict when blocked (2 default)
"accel" if specified, is how much the rotation speed will increase per .1sec.

REVERSE will cause the it to rotate in the opposite direction.
STOP mean it will stop moving instead of pushing entities
ACCEL means it will accelerate to it's final speed and decelerate when shutting down.
*/

//============
// PGM
void THINK(rotating_accel)(edict_t *self)
{
    float current_speed = VectorLength(self->avelocity);

    if (current_speed >= (self->speed - self->accel)) { // done
        VectorScale(self->movedir, self->speed, self->avelocity);
        G_UseTargets(self, self);
    } else {
        current_speed += self->accel;
        VectorScale(self->movedir, current_speed, self->avelocity);
        self->think = rotating_accel;
        self->nextthink = level.time + FRAME_TIME;
    }
}

void THINK(rotating_decel)(edict_t *self)
{
    float current_speed = VectorLength(self->avelocity);

    if (current_speed <= self->decel) { // done
        VectorClear(self->avelocity);
        G_UseTargets(self, self);
        self->touch = NULL;
    } else {
        current_speed -= self->decel;
        VectorScale(self->movedir, current_speed, self->avelocity);
        self->think = rotating_decel;
        self->nextthink = level.time + FRAME_TIME;
    }
}
// PGM
//============

void MOVEINFO_BLOCKED(rotating_blocked)(edict_t *self, edict_t *other)
{
    if (!self->dmg)
        return;
    if (level.time < self->touch_debounce_time)
        return;
    self->touch_debounce_time = level.time + HZ(10);
    T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, DAMAGE_NONE, (mod_t) { MOD_CRUSH });
}

void TOUCH(rotating_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if (!VectorEmpty(self->avelocity))
        T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, DAMAGE_NONE, (mod_t) { MOD_CRUSH });
}

void USE(rotating_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (!VectorEmpty(self->avelocity)) {
        self->s.sound = 0;
        // PGM
        if (self->spawnflags & SPAWNFLAG_ROTATING_ACCEL) // Decelerate
            rotating_decel(self);
        else {
            VectorClear(self->avelocity);
            G_UseTargets(self, self);
            self->touch = NULL;
        }
        // PGM
    } else {
        self->s.sound = self->moveinfo.sound_middle;
        // PGM
        if (self->spawnflags & SPAWNFLAG_ROTATING_ACCEL) // accelerate
            rotating_accel(self);
        else {
            VectorScale(self->movedir, self->speed, self->avelocity);
            G_UseTargets(self, self);
        }
        if (self->spawnflags & SPAWNFLAG_ROTATING_TOUCH_PAIN)
            self->touch = rotating_touch;
        // PGM
    }
}

void SP_func_rotating(edict_t *ent)
{
    ent->solid = SOLID_BSP;
    if (ent->spawnflags & SPAWNFLAG_ROTATING_STOP)
        ent->movetype = MOVETYPE_STOP;
    else
        ent->movetype = MOVETYPE_PUSH;

    if (st.noise) {
        ent->moveinfo.sound_middle = gi.soundindex(st.noise);

        // [Paril-KEX] for rhangar1 doors
        if (!ED_WasKeySpecified("attenuation")) {
            ent->attenuation = ATTN_STATIC;
        } else if (ent->attenuation == -1) {
            ent->x.loop_attenuation = ATTN_LOOP_NONE;
            ent->attenuation = ATTN_NONE;
        } else if (ent->attenuation != ATTN_STATIC) {
            ent->x.loop_attenuation = ent->attenuation;
        }
    }

    // set the axis of rotation
    VectorClear(ent->movedir);
    if (ent->spawnflags & SPAWNFLAG_ROTATING_X_AXIS)
        ent->movedir[2] = 1.0f;
    else if (ent->spawnflags & SPAWNFLAG_ROTATING_Y_AXIS)
        ent->movedir[0] = 1.0f;
    else // Z_AXIS
        ent->movedir[1] = 1.0f;

    // check for reverse rotation
    if (ent->spawnflags & SPAWNFLAG_ROTATING_REVERSE)
        VectorInverse(ent->movedir);

    if (!ent->speed)
        ent->speed = 100;
    if (!ED_WasKeySpecified("dmg"))
        ent->dmg = 2;

    ent->use = rotating_use;
    if (ent->dmg)
        ent->moveinfo.blocked = rotating_blocked;

    if (ent->spawnflags & SPAWNFLAG_ROTATING_START_ON)
        ent->use(ent, NULL, NULL);

    if (ent->spawnflags & SPAWNFLAG_ROTATING_ANIMATED)
        ent->s.effects |= EF_ANIM_ALL;
    if (ent->spawnflags & SPAWNFLAG_ROTATING_ANIMATED_FAST)
        ent->s.effects |= EF_ANIM_ALLFAST;

    // PGM
    if (ent->spawnflags & SPAWNFLAG_ROTATING_ACCEL) { // Accelerate / Decelerate
        if (!ent->accel)
            ent->accel = 1;
        else if (ent->accel > ent->speed)
            ent->accel = ent->speed;

        if (!ent->decel)
            ent->decel = 1;
        else if (ent->decel > ent->speed)
            ent->decel = ent->speed;
    }
    // PGM

    gi.setmodel(ent, ent->model);
    gi.linkentity(ent);
}

void THINK(func_spinning_think)(edict_t *ent)
{
    if (ent->timestamp <= level.time) {
        ent->timestamp = level.time + random_time_sec(1, 6);

        for (int i = 0; i < 3; i++) {
            ent->movedir[i] = ent->decel + frandom1(ent->speed - ent->decel);
            if (brandom())
                ent->movedir[i] = -ent->movedir[i];
        }
    }

    for (int i = 0; i < 3; i++) {
        if (ent->avelocity[i] == ent->movedir[i])
            continue;

        if (ent->avelocity[i] < ent->movedir[i])
            ent->avelocity[i] = min(ent->movedir[i], ent->avelocity[i] + ent->accel);
        else
            ent->avelocity[i] = max(ent->movedir[i], ent->avelocity[i] - ent->accel);
    }

    ent->nextthink = level.time + FRAME_TIME;
}

// [Paril-KEX]
void SP_func_spinning(edict_t *ent)
{
    ent->solid = SOLID_BSP;

    if (!ent->speed)
        ent->speed = 100;
    if (!ent->dmg)
        ent->dmg = 2;

    ent->movetype = MOVETYPE_PUSH;

    ent->timestamp = 0;
    ent->nextthink = level.time + FRAME_TIME;
    ent->think = func_spinning_think;

    gi.setmodel(ent, ent->model);
    gi.linkentity(ent);
}

/*
======================================================================

BUTTONS

======================================================================
*/

/*QUAKED func_button (0 .5 .8) ?
When a button is touched, it moves some distance in the direction of it's angle, triggers all of it's targets, waits some time, then returns to it's original position where it can be triggered again.

"angle"     determines the opening direction
"target"    all entities with a matching targetname will be used
"speed"     override the default 40 speed
"wait"      override the default 1 second wait (-1 = never return)
"lip"       override the default 4 pixel lip remaining at end of move
"health"    if set, the button must be killed instead of touched
"sounds"
1) silent
2) steam metal
3) wooden clunk
4) metallic click
5) in-out
*/

void MOVEINFO_ENDFUNC(button_done)(edict_t *self)
{
    self->moveinfo.state = STATE_BOTTOM;
    if (!self->bmodel_anim.enabled) {
        if (level.is_n64)
            self->s.frame = 0;
        else
            self->s.effects &= ~EF_ANIM23;
        self->s.effects |= EF_ANIM01;
    } else
        self->bmodel_anim.alternate = false;
}

void THINK(button_return)(edict_t *self)
{
    self->moveinfo.state = STATE_DOWN;

    Move_Calc(self, self->moveinfo.start_origin, button_done);

    if (self->health)
        self->takedamage = true;
}

void MOVEINFO_ENDFUNC(button_wait)(edict_t *self)
{
    self->moveinfo.state = STATE_TOP;

    if (!self->bmodel_anim.enabled) {
        self->s.effects &= ~EF_ANIM01;
        if (level.is_n64)
            self->s.frame = 2;
        else
            self->s.effects |= EF_ANIM23;
    } else
        self->bmodel_anim.alternate = true;

    G_UseTargets(self, self->activator);

    if (self->moveinfo.wait >= 0) {
        self->nextthink = level.time + SEC(self->moveinfo.wait);
        self->think = button_return;
    }
}

static void button_fire(edict_t *self)
{
    if (self->moveinfo.state == STATE_UP || self->moveinfo.state == STATE_TOP)
        return;

    self->moveinfo.state = STATE_UP;
    if (!(self->flags & FL_TEAMSLAVE) && self->moveinfo.sound_start)
        gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, self->moveinfo.sound_start, 1, ATTN_STATIC, 0);
    Move_Calc(self, self->moveinfo.end_origin, button_wait);
}

void USE(button_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->activator = activator;
    button_fire(self);
}

void TOUCH(button_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if (!other->client)
        return;

    if (other->health <= 0)
        return;

    self->activator = other;
    button_fire(self);
}

void DIE(button_killed)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    self->activator = attacker;
    self->health = self->max_health;
    self->takedamage = false;
    button_fire(self);
}

void SP_func_button(edict_t *ent)
{
    vec3_t abs_movedir;
    float  dist;

    G_SetMovedir(ent->s.angles, ent->movedir);
    ent->movetype = MOVETYPE_STOP;
    ent->solid = SOLID_BSP;
    gi.setmodel(ent, ent->model);

    if (ent->sounds != 1)
        G_SetMoveinfoSounds(ent, "switches/butn2.wav", NULL, NULL);
    else
        G_SetMoveinfoSounds(ent, NULL, NULL, NULL);

    if (!ent->speed)
        ent->speed = 40;
    if (!ent->accel)
        ent->accel = ent->speed;
    if (!ent->decel)
        ent->decel = ent->speed;

    if (!ent->wait)
        ent->wait = 3;
    if (!st.lip)
        st.lip = 4;

    VectorCopy(ent->s.origin, ent->pos1);
    abs_movedir[0] = fabsf(ent->movedir[0]);
    abs_movedir[1] = fabsf(ent->movedir[1]);
    abs_movedir[2] = fabsf(ent->movedir[2]);
    dist = DotProduct(abs_movedir, ent->size) - st.lip;
    VectorMA(ent->pos1, dist, ent->movedir, ent->pos2);

    ent->use = button_use;

    if (!ent->bmodel_anim.enabled)
        ent->s.effects |= EF_ANIM01;

    if (ent->health) {
        ent->max_health = ent->health;
        ent->die = button_killed;
        ent->takedamage = true;
    } else if (!ent->targetname)
        ent->touch = button_touch;

    ent->moveinfo.state = STATE_BOTTOM;

    ent->moveinfo.speed = ent->speed;
    ent->moveinfo.accel = ent->accel;
    ent->moveinfo.decel = ent->decel;
    ent->moveinfo.wait = ent->wait;
    VectorCopy(ent->pos1, ent->moveinfo.start_origin);
    VectorCopy(ent->pos2, ent->moveinfo.end_origin);
    VectorCopy(ent->s.angles, ent->moveinfo.start_angles);
    VectorCopy(ent->s.angles, ent->moveinfo.end_angles);

    gi.linkentity(ent);
}

/*
======================================================================

DOORS

  spawn a trigger surrounding the entire team unless it is
  already targeted by another

======================================================================
*/

/*QUAKED func_door (0 .5 .8) ? START_OPEN x CRUSHER NOMONSTER ANIMATED TOGGLE ANIMATED_FAST
TOGGLE      wait in both the start and end states for a trigger event.
START_OPEN  the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
NOMONSTER   monsters will not trigger this door

"message"   is printed when the door is touched if it is a trigger door and it hasn't been fired yet
"angle"     determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"health"    if set, door must be shot open
"speed"     movement speed (100 default)
"wait"      wait before returning (3 default, -1 = never return)
"lip"       lip remaining at end of move (8 default)
"dmg"       damage to inflict when blocked (2 default)
"sounds"
1)  silent
2)  light
3)  medium
4)  heavy
*/

static void door_use_areaportals(edict_t *self, bool open)
{
    edict_t *t = NULL;

    if (!self->target)
        return;

    while ((t = G_Find(t, FOFS(targetname), self->target))) {
        if (Q_strcasecmp(t->classname, "func_areaportal") == 0) {
            gi.SetAreaPortalState(t->style, open);
        }
    }
}

void door_go_down(edict_t *self);

static void door_play_sound(edict_t *self, int sound)
{
    if (!self->teammaster) {
        gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, sound, 1, self->attenuation, 0);
        return;
    }

    vec3_t p = { 0 };
    int c = 0;

    for (edict_t *t = self->teammaster; t; t = t->teamchain) {
        vec3_t mid;
        VectorAvg(t->absmin, t->absmax, mid);
        VectorAdd(p, mid, p);
        c++;
    }

    if (c == 1) {
        gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, sound, 1, self->attenuation, 0);
        return;
    }

    VectorScale(p, 1.0f / c, p);

    if (gi.pointcontents(p) & CONTENTS_SOLID) {
        gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, sound, 1, self->attenuation, 0);
        return;
    }

    gi.positioned_sound(p, self, CHAN_NO_PHS_ADD | CHAN_VOICE, sound, 1, self->attenuation, 0);
}

void MOVEINFO_ENDFUNC(door_hit_top)(edict_t *self)
{
    if (!(self->flags & FL_TEAMSLAVE) && self->moveinfo.sound_end)
        door_play_sound(self, self->moveinfo.sound_end);

    self->s.sound = 0;
    self->moveinfo.state = STATE_TOP;
    if (self->spawnflags & SPAWNFLAG_DOOR_TOGGLE)
        return;

    if (self->moveinfo.wait >= 0) {
        self->think = door_go_down;
        self->nextthink = level.time + SEC(self->moveinfo.wait);
    }

    if (self->spawnflags & SPAWNFLAG_DOOR_START_OPEN)
        door_use_areaportals(self, false);
}

void MOVEINFO_ENDFUNC(door_hit_bottom)(edict_t *self)
{
    if (!(self->flags & FL_TEAMSLAVE) && self->moveinfo.sound_end)
        door_play_sound(self, self->moveinfo.sound_end);

    self->s.sound = 0;
    self->moveinfo.state = STATE_BOTTOM;

    if (!(self->spawnflags & SPAWNFLAG_DOOR_START_OPEN))
        door_use_areaportals(self, false);
}

void THINK(door_go_down)(edict_t *self)
{
    if (!(self->flags & FL_TEAMSLAVE) && self->moveinfo.sound_start)
        door_play_sound(self, self->moveinfo.sound_start);

    self->s.sound = self->moveinfo.sound_middle;

    if (self->max_health) {
        self->takedamage = true;
        self->health = self->max_health;
    }

    self->moveinfo.state = STATE_DOWN;
    if (strcmp(self->classname, "func_door") == 0 ||
        strcmp(self->classname, "func_water") == 0 ||
        strcmp(self->classname, "func_door_secret") == 0)
        Move_Calc(self, self->moveinfo.start_origin, door_hit_bottom);
    else if (strcmp(self->classname, "func_door_rotating") == 0)
        AngleMove_Calc(self, door_hit_bottom);

    if (self->spawnflags & SPAWNFLAG_DOOR_START_OPEN)
        door_use_areaportals(self, true);
}

static void door_go_up(edict_t *self, edict_t *activator)
{
    if (self->moveinfo.state == STATE_UP)
        return; // already going up

    if (self->moveinfo.state == STATE_TOP) {
        // reset top wait time
        if (self->moveinfo.wait >= 0)
            self->nextthink = level.time + SEC(self->moveinfo.wait);
        return;
    }

    if (!(self->flags & FL_TEAMSLAVE) && self->moveinfo.sound_start)
        door_play_sound(self, self->moveinfo.sound_start);

    self->s.sound = self->moveinfo.sound_middle;

    self->moveinfo.state = STATE_UP;
    if (strcmp(self->classname, "func_door") == 0 ||
        strcmp(self->classname, "func_water") == 0 ||
        strcmp(self->classname, "func_door_secret") == 0)
        Move_Calc(self, self->moveinfo.end_origin, door_hit_top);
    else if (strcmp(self->classname, "func_door_rotating") == 0)
        AngleMove_Calc(self, door_hit_top);

    G_UseTargets(self, activator);

    if (!(self->spawnflags & SPAWNFLAG_DOOR_START_OPEN))
        door_use_areaportals(self, true);
}

//======
// PGM

void THINK(smart_water_go_up)(edict_t *self)
{
    float    distance;
    edict_t *lowestPlayer;
    edict_t *ent;
    float    lowestPlayerPt;

    if (self->moveinfo.state == STATE_TOP) {
        // reset top wait time
        if (self->moveinfo.wait >= 0)
            self->nextthink = level.time + SEC(self->moveinfo.wait);
        return;
    }

    if (self->health && self->absmax[2] >= self->health) {
        VectorClear(self->velocity);
        self->nextthink = 0;
        self->moveinfo.state = STATE_TOP;
        return;
    }

    if (!(self->flags & FL_TEAMSLAVE) && self->moveinfo.sound_start)
        gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, self->moveinfo.sound_start, 1, ATTN_STATIC, 0);

    self->s.sound = self->moveinfo.sound_middle;

    // find the lowest player point.
    lowestPlayerPt = 999999;
    lowestPlayer = NULL;
    for (int i = 0; i < game.maxclients; i++) {
        ent = &g_edicts[1 + i];

        // don't count dead or unused player slots
        if ((ent->inuse) && (ent->health > 0) && (ent->absmin[2] < lowestPlayerPt)) {
            lowestPlayerPt = ent->absmin[2];
            lowestPlayer = ent;
        }
    }

    if (!lowestPlayer)
        return;

    distance = lowestPlayerPt - self->absmax[2];

    // for the calculations, make sure we intend to go up at least a little.
    if (distance < self->accel) {
        distance = 100;
        self->moveinfo.speed = 5;
    } else
        self->moveinfo.speed = distance / self->accel;

    if (self->moveinfo.speed < 5)
        self->moveinfo.speed = 5;
    else if (self->moveinfo.speed > self->speed)
        self->moveinfo.speed = self->speed;

    // FIXME - should this allow any movement other than straight up?
    VectorSet(self->moveinfo.dir, 0, 0, 1);
    VectorScale(self->moveinfo.dir, self->moveinfo.speed, self->velocity);
    self->moveinfo.remaining_distance = distance;

    if (self->moveinfo.state != STATE_UP) {
        G_UseTargets(self, lowestPlayer);
        door_use_areaportals(self, true);
        self->moveinfo.state = STATE_UP;
    }

    self->think = smart_water_go_up;
    self->nextthink = level.time + FRAME_TIME;
}
// PGM
//======

void USE(door_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    edict_t *ent;
    vec3_t   center; // PGM

    if (self->flags & FL_TEAMSLAVE)
        return;

    if ((strcmp(self->classname, "func_door_rotating") == 0) && (self->spawnflags & SPAWNFLAG_DOOR_ROTATING_SAFE_OPEN) &&
        (self->moveinfo.state == STATE_BOTTOM || self->moveinfo.state == STATE_DOWN) && !VectorEmpty(self->moveinfo.dir)) {
        vec3_t forward;
        VectorSubtract(activator->s.origin, self->s.origin, forward);
        VectorNormalize(forward);
        self->moveinfo.reversing = DotProduct(forward, self->moveinfo.dir) > 0;
    }

    if ((self->spawnflags & SPAWNFLAG_DOOR_TOGGLE) && (self->moveinfo.state == STATE_UP || self->moveinfo.state == STATE_TOP)) {
        // trigger all paired doors
        for (ent = self; ent; ent = ent->teamchain) {
            ent->message = NULL;
            ent->touch = NULL;
            door_go_down(ent);
        }
        return;
    }

    // PGM
    //  smart water is different
    VectorAvg(self->mins, self->maxs, center);
    if ((strcmp(self->classname, "func_water") == 0) && (gi.pointcontents(center) & MASK_WATER) && (self->spawnflags & SPAWNFLAG_WATER_SMART)) {
        self->message = NULL;
        self->touch = NULL;
        self->enemy = activator;
        smart_water_go_up(self);
        return;
    }
    // PGM

    // trigger all paired doors
    for (ent = self; ent; ent = ent->teamchain) {
        ent->message = NULL;
        ent->touch = NULL;
        door_go_up(ent, activator);
    }
}

void TOUCH(Touch_DoorTrigger)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if (other->health <= 0)
        return;

    if (!(other->svflags & SVF_MONSTER) && (!other->client))
        return;

    if (other->svflags & SVF_MONSTER) {
        if (self->owner->spawnflags & SPAWNFLAG_DOOR_NOMONSTER)
            return;
        // [Paril-KEX] this is for PSX; the scale is so small that monsters walking
        // around to path_corners often initiate doors unintentionally.
        if (other->spawnflags & SPAWNFLAG_MONSTER_NO_IDLE_DOORS && !other->enemy)
            return;
    }

    if (level.time < self->touch_debounce_time)
        return;
    self->touch_debounce_time = level.time + SEC(1);

    door_use(self->owner, other, other);
}

void THINK(Think_CalcMoveSpeed)(edict_t *self)
{
    edict_t *ent;
    float    min;
    float    time;
    float    newspeed;
    float    ratio;
    float    dist;

    if (self->flags & FL_TEAMSLAVE)
        return; // only the team master does this

    // find the smallest distance any member of the team will be moving
    min = fabsf(self->moveinfo.distance);
    for (ent = self->teamchain; ent; ent = ent->teamchain) {
        dist = fabsf(ent->moveinfo.distance);
        if (dist < min)
            min = dist;
    }

    time = min / self->moveinfo.speed;

    // adjust speeds so they will all complete at the same time
    for (ent = self; ent; ent = ent->teamchain) {
        newspeed = fabsf(ent->moveinfo.distance) / time;
        ratio = newspeed / ent->moveinfo.speed;
        if (ent->moveinfo.accel == ent->moveinfo.speed)
            ent->moveinfo.accel = newspeed;
        else
            ent->moveinfo.accel *= ratio;
        if (ent->moveinfo.decel == ent->moveinfo.speed)
            ent->moveinfo.decel = newspeed;
        else
            ent->moveinfo.decel *= ratio;
        ent->moveinfo.speed = newspeed;
    }
}

void THINK(Think_SpawnDoorTrigger)(edict_t *ent)
{
    edict_t *other;
    vec3_t   mins, maxs;

    if (ent->flags & FL_TEAMSLAVE)
        return; // only the team leader spawns a trigger

    VectorCopy(ent->absmin, mins);
    VectorCopy(ent->absmax, maxs);

    for (other = ent->teamchain; other; other = other->teamchain) {
        AddPointToBounds(other->absmin, mins, maxs);
        AddPointToBounds(other->absmax, mins, maxs);
    }

    // expand
    mins[0] -= 60;
    mins[1] -= 60;
    maxs[0] += 60;
    maxs[1] += 60;

    other = G_Spawn();
    VectorCopy(mins, other->mins);
    VectorCopy(maxs, other->maxs);
    other->owner = ent;
    other->solid = SOLID_TRIGGER;
    other->movetype = MOVETYPE_NONE;
    other->touch = Touch_DoorTrigger;
    gi.linkentity(other);

    Think_CalcMoveSpeed(ent);
}

void MOVEINFO_BLOCKED(door_blocked)(edict_t *self, edict_t *other)
{
    edict_t *ent;

    if (!(other->svflags & SVF_MONSTER) && (!other->client)) {
        // give it a chance to go away on it's own terms (like gibs)
        T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, DAMAGE_NONE, (mod_t) { MOD_CRUSH });
        // if it's still there, nuke it
        if (other->inuse)
            BecomeExplosion1(other);
        return;
    }

    if (self->dmg && !(level.time < self->touch_debounce_time)) {
        self->touch_debounce_time = level.time + HZ(10);
        T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, DAMAGE_NONE, (mod_t) { MOD_CRUSH });
    }

    // [Paril-KEX] don't allow wait -1 doors to return
    if ((self->spawnflags & SPAWNFLAG_DOOR_CRUSHER) || self->wait == -1)
        return;

    // if a door has a negative wait, it would never come back if blocked,
    // so let it just squash the object to death real fast
    if (self->moveinfo.wait >= 0) {
        if (self->moveinfo.state == STATE_DOWN) {
            for (ent = self->teammaster; ent; ent = ent->teamchain)
                door_go_up(ent, ent->activator);
        } else {
            for (ent = self->teammaster; ent; ent = ent->teamchain)
                door_go_down(ent);
        }
    }
}

void DIE(door_killed)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    for (edict_t *ent = self->teammaster; ent; ent = ent->teamchain) {
        ent->health = ent->max_health;
        ent->takedamage = false;
    }
    door_use(self->teammaster, attacker, attacker);
}

void TOUCH(door_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if (!other->client)
        return;

    if (level.time < self->touch_debounce_time)
        return;
    self->touch_debounce_time = level.time + SEC(5);

    gi.centerprintf(other, "%s", self->message);
    gi.sound(other, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
}

void THINK(Think_DoorActivateAreaPortal)(edict_t *ent)
{
    door_use_areaportals(ent, true);

    if (ent->health || ent->targetname)
        Think_CalcMoveSpeed(ent);
    else
        Think_SpawnDoorTrigger(ent);
}

void SP_func_door(edict_t *ent)
{
    vec3_t abs_movedir;

    if (ent->sounds != 1)
        G_SetMoveinfoSounds(ent, "doors/dr1_strt.wav", "doors/dr1_mid.wav", "doors/dr1_end.wav");
    else
        G_SetMoveinfoSounds(ent, NULL, NULL, NULL);

    // [Paril-KEX] for rhangar1 doors
    if (!ED_WasKeySpecified("attenuation")) {
        ent->attenuation = ATTN_STATIC;
    } else if (ent->attenuation == -1) {
        ent->x.loop_attenuation = ATTN_LOOP_NONE;
        ent->attenuation = ATTN_NONE;
    } else if (ent->attenuation != ATTN_STATIC) {
        ent->x.loop_attenuation = ent->attenuation;
    }

    G_SetMovedir(ent->s.angles, ent->movedir);
    ent->movetype = MOVETYPE_PUSH;
    ent->solid = SOLID_BSP;
    ent->svflags |= SVF_DOOR;
    gi.setmodel(ent, ent->model);

    ent->moveinfo.blocked = door_blocked;
    ent->use = door_use;

    if (!ent->speed)
        ent->speed = 100;
    if (deathmatch->integer)
        ent->speed *= 2;

    if (!ent->accel)
        ent->accel = ent->speed;
    if (!ent->decel)
        ent->decel = ent->speed;

    if (!ent->wait)
        ent->wait = 3;
    if (!st.lip)
        st.lip = 8;
    if (!ent->dmg)
        ent->dmg = 2;

    // calculate second position
    VectorCopy(ent->s.origin, ent->pos1);
    abs_movedir[0] = fabsf(ent->movedir[0]);
    abs_movedir[1] = fabsf(ent->movedir[1]);
    abs_movedir[2] = fabsf(ent->movedir[2]);
    ent->moveinfo.distance = DotProduct(abs_movedir, ent->size) - st.lip;
    VectorMA(ent->pos1, ent->moveinfo.distance, ent->movedir, ent->pos2);

    // if it starts open, switch the positions
    if (ent->spawnflags & SPAWNFLAG_DOOR_START_OPEN) {
        VectorCopy(ent->pos2, ent->s.origin);
        VectorCopy(ent->pos1, ent->pos2);
        VectorCopy(ent->s.origin, ent->pos1);
    }

    ent->moveinfo.state = STATE_BOTTOM;

    if (ent->health) {
        ent->takedamage = true;
        ent->die = door_killed;
        ent->max_health = ent->health;
    } else if (ent->targetname) {
        if (ent->message) {
            gi.soundindex("misc/talk.wav");
            ent->touch = door_touch;
        }
        ent->flags |= FL_LOCKED;
    }

    ent->moveinfo.speed = ent->speed;
    ent->moveinfo.accel = ent->accel;
    ent->moveinfo.decel = ent->decel;
    ent->moveinfo.wait = ent->wait;
    VectorCopy(ent->pos1, ent->moveinfo.start_origin);
    VectorCopy(ent->pos2, ent->moveinfo.end_origin);
    VectorCopy(ent->s.angles, ent->moveinfo.start_angles);
    VectorCopy(ent->s.angles, ent->moveinfo.end_angles);

    if (ent->spawnflags & SPAWNFLAG_DOOR_ANIMATED)
        ent->s.effects |= EF_ANIM_ALL;
    if (ent->spawnflags & SPAWNFLAG_DOOR_ANIMATED_FAST)
        ent->s.effects |= EF_ANIM_ALLFAST;

    // to simplify logic elsewhere, make non-teamed doors into a team of one
    if (!ent->team)
        ent->teammaster = ent;

    gi.linkentity(ent);

    ent->nextthink = level.time + FRAME_TIME;

    if (ent->spawnflags & SPAWNFLAG_DOOR_START_OPEN)
        ent->think = Think_DoorActivateAreaPortal;
    else if (ent->health || ent->targetname)
        ent->think = Think_CalcMoveSpeed;
    else
        ent->think = Think_SpawnDoorTrigger;
}

// PGM
void USE(Door_Activate)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->use = NULL;

    if (self->health) {
        self->takedamage = true;
        self->die = door_killed;
        self->max_health = self->health;
    }

    if (self->health)
        self->think = Think_CalcMoveSpeed;
    else
        self->think = Think_SpawnDoorTrigger;
    self->nextthink = level.time + FRAME_TIME;
}
// PGM

/*QUAKED func_door_rotating (0 .5 .8) ? START_OPEN REVERSE CRUSHER NOMONSTER ANIMATED TOGGLE X_AXIS Y_AXIS NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP RESERVED1 COOP_ONLY RESERVED2 INACTIVE SAFE_OPEN
TOGGLE causes the door to wait in both the start and end states for a trigger event.

START_OPEN  the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
NOMONSTER   monsters will not trigger this door

You need to have an origin brush as part of this entity.  The center of that brush will be
the point around which it is rotated. It will rotate around the Z axis by default.  You can
check either the X_AXIS or Y_AXIS box to change that.

"distance" is how many degrees the door will be rotated.
"speed" determines how fast the door moves; default value is 100.
"accel" if specified,is how much the rotation speed will increase each .1 sec. (default: no accel)

REVERSE will cause the door to rotate in the opposite direction.
INACTIVE will cause the door to be inactive until triggered.
SAFE_OPEN will cause the door to open in reverse if you are on the `angles` side of the door.

"message"   is printed when the door is touched if it is a trigger door and it hasn't been fired yet
"angle"     determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"health"    if set, door must be shot open
"speed"     movement speed (100 default)
"wait"      wait before returning (3 default, -1 = never return)
"dmg"       damage to inflict when blocked (2 default)
"sounds"
1)  silent
2)  light
3)  medium
4)  heavy
*/

void SP_func_door_rotating(edict_t *ent)
{
    if (ent->spawnflags & SPAWNFLAG_DOOR_ROTATING_SAFE_OPEN)
        G_SetMovedir(ent->s.angles, ent->moveinfo.dir);

    VectorClear(ent->s.angles);

    // set the axis of rotation
    VectorClear(ent->movedir);
    if (ent->spawnflags & SPAWNFLAG_DOOR_ROTATING_X_AXIS)
        ent->movedir[2] = 1.0f;
    else if (ent->spawnflags & SPAWNFLAG_DOOR_ROTATING_Y_AXIS)
        ent->movedir[0] = 1.0f;
    else // Z_AXIS
        ent->movedir[1] = 1.0f;

    // check for reverse rotation
    if (ent->spawnflags & SPAWNFLAG_DOOR_REVERSE)
        VectorInverse(ent->movedir);

    if (!st.distance) {
        gi.dprintf("%s: no distance set\n", etos(ent));
        st.distance = 90;
    }

    VectorCopy(ent->s.angles, ent->pos1);
    VectorMA(ent->s.angles, st.distance, ent->movedir, ent->pos2);
    VectorMA(ent->s.angles, -st.distance, ent->movedir, ent->pos3);
    ent->moveinfo.distance = st.distance;

    ent->movetype = MOVETYPE_PUSH;
    ent->solid = SOLID_BSP;
    ent->svflags |= SVF_DOOR;
    gi.setmodel(ent, ent->model);

    ent->moveinfo.blocked = door_blocked;
    ent->use = door_use;

    if (!ent->speed)
        ent->speed = 100;
    if (!ent->accel)
        ent->accel = ent->speed;
    if (!ent->decel)
        ent->decel = ent->speed;

    if (!ent->wait)
        ent->wait = 3;
    if (!ent->dmg)
        ent->dmg = 2;

    if (ent->sounds != 1)
        G_SetMoveinfoSounds(ent, "doors/dr1_strt.wav", "doors/dr1_mid.wav", "doors/dr1_end.wav");
    else
        G_SetMoveinfoSounds(ent, NULL, NULL, NULL);

    // [Paril-KEX] for rhangar1 doors
    if (!ED_WasKeySpecified("attenuation")) {
        ent->attenuation = ATTN_STATIC;
    } else if (ent->attenuation == -1) {
        ent->x.loop_attenuation = ATTN_LOOP_NONE;
        ent->attenuation = ATTN_NONE;
    } else if (ent->attenuation != ATTN_STATIC) {
        ent->x.loop_attenuation = ent->attenuation;
    }

    // if it starts open, switch the positions
    if (ent->spawnflags & SPAWNFLAG_DOOR_START_OPEN) {
        if (ent->spawnflags & SPAWNFLAG_DOOR_ROTATING_SAFE_OPEN) {
            ent->spawnflags &= ~SPAWNFLAG_DOOR_ROTATING_SAFE_OPEN;
            gi.dprintf("%s: SAFE_OPEN is not compatible with START_OPEN\n", etos(ent));
        }

        VectorCopy(ent->pos2, ent->s.angles);
        VectorCopy(ent->pos1, ent->pos2);
        VectorCopy(ent->s.angles, ent->pos1);
        VectorInverse(ent->movedir);
    }

    if (ent->spawnflags & SPAWNFLAG_DOOR_ROTATING_NO_COLLISION)
        ent->clipmask = CONTENTS_AREAPORTAL; // just because zero is automatic

    if (ent->health) {
        ent->takedamage = true;
        ent->die = door_killed;
        ent->max_health = ent->health;
    }

    if (ent->targetname && ent->message) {
        gi.soundindex("misc/talk.wav");
        ent->touch = door_touch;
    }

    ent->moveinfo.state = STATE_BOTTOM;
    ent->moveinfo.speed = ent->speed;
    ent->moveinfo.accel = ent->accel;
    ent->moveinfo.decel = ent->decel;
    ent->moveinfo.wait = ent->wait;
    VectorCopy(ent->s.origin, ent->moveinfo.start_origin);
    VectorCopy(ent->s.origin, ent->moveinfo.end_origin);
    VectorCopy(ent->pos1, ent->moveinfo.start_angles);
    VectorCopy(ent->pos2, ent->moveinfo.end_angles);
    VectorCopy(ent->pos3, ent->moveinfo.end_angles_reversed);

    if (ent->spawnflags & SPAWNFLAG_DOOR_ANIMATED)
        ent->s.effects |= EF_ANIM_ALL;

    // to simplify logic elsewhere, make non-teamed doors into a team of one
    if (!ent->team)
        ent->teammaster = ent;

    gi.linkentity(ent);

    ent->nextthink = level.time + FRAME_TIME;
    if (ent->health || ent->targetname)
        ent->think = Think_CalcMoveSpeed;
    else
        ent->think = Think_SpawnDoorTrigger;

    // PGM
    if (ent->spawnflags & SPAWNFLAG_DOOR_ROTATING_INACTIVE) {
        ent->takedamage = false;
        ent->die = NULL;
        ent->think = NULL;
        ent->nextthink = 0;
        ent->use = Door_Activate;
    }
    // PGM
}

void MOVEINFO_BLOCKED(smart_water_blocked)(edict_t *self, edict_t *other)
{
    if (!(other->svflags & SVF_MONSTER) && (!other->client)) {
        // give it a chance to go away on it's own terms (like gibs)
        T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, DAMAGE_NONE, (mod_t) { MOD_LAVA });
        // if it's still there, nuke it
        if (other->inuse && other->solid) // PGM
            BecomeExplosion1(other);
        return;
    }

    T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 100, 1, DAMAGE_NONE, (mod_t) { MOD_LAVA });
}

/*QUAKED func_water (0 .5 .8) ? START_OPEN SMART
func_water is a moveable water brush.  It must be targeted to operate.  Use a non-water texture at your own risk.

START_OPEN causes the water to move to its destination when spawned and operate in reverse.

SMART causes the water to adjust its speed depending on distance to player.
(speed = distance/accel, min 5, max self->speed)
"accel"     for smart water, the divisor to determine water speed. default 20 (smaller = faster)

"health"    maximum height of this water brush
"angle"     determines the opening direction (up or down only)
"speed"     movement speed (25 default)
"wait"      wait before returning (-1 default, -1 = TOGGLE)
"lip"       lip remaining at end of move (0 default)
"sounds"    (yes, these need to be changed)
0)  no sound
1)  water
2)  lava
*/

void SP_func_water(edict_t *self)
{
    vec3_t abs_movedir;

    G_SetMovedir(self->s.angles, self->movedir);
    self->movetype = MOVETYPE_PUSH;
    self->solid = SOLID_BSP;
    gi.setmodel(self, self->model);

    switch (self->sounds) {
    default:
        G_SetMoveinfoSounds(self, NULL, NULL, NULL);
        break;
    case 1: // water
    case 2: // lava
        G_SetMoveinfoSounds(self, "world/mov_watr.wav", NULL, "world/stp_watr.wav");
        break;
    }

    self->attenuation = ATTN_STATIC;

    // calculate second position
    VectorCopy(self->s.origin, self->pos1);
    abs_movedir[0] = fabsf(self->movedir[0]);
    abs_movedir[1] = fabsf(self->movedir[1]);
    abs_movedir[2] = fabsf(self->movedir[2]);
    self->moveinfo.distance = DotProduct(abs_movedir, self->size) - st.lip;
    VectorMA(self->pos1, self->moveinfo.distance, self->movedir, self->pos2);

    // if it starts open, switch the positions
    if (self->spawnflags & SPAWNFLAG_DOOR_START_OPEN) {
        VectorCopy(self->pos2, self->s.origin);
        VectorCopy(self->pos1, self->pos2);
        VectorCopy(self->s.origin, self->pos1);
    }

    VectorCopy(self->pos1, self->moveinfo.start_origin);
    VectorCopy(self->pos2, self->moveinfo.end_origin);
    VectorCopy(self->s.angles, self->moveinfo.start_angles);
    VectorCopy(self->s.angles, self->moveinfo.end_angles);

    self->moveinfo.state = STATE_BOTTOM;

    if (!self->speed)
        self->speed = 25;
    self->moveinfo.accel = self->moveinfo.decel = self->moveinfo.speed = self->speed;

    // ROGUE
    if (self->spawnflags & SPAWNFLAG_WATER_SMART) { // smart water
        // this is actually the divisor of the lowest player's distance to determine speed.
        // self->speed then becomes the cap of the speed.
        if (!self->accel)
            self->accel = 20;
        self->moveinfo.blocked = smart_water_blocked;
    }
    // ROGUE

    if (!self->wait)
        self->wait = -1;
    self->moveinfo.wait = self->wait;

    self->use = door_use;

    if (self->wait == -1)
        self->spawnflags |= SPAWNFLAG_DOOR_TOGGLE;

    gi.linkentity(self);
}

#define SPAWNFLAG_TRAIN_TOGGLE          2
#define SPAWNFLAG_TRAIN_BLOCK_STOPS     4
#define SPAWNFLAG_TRAIN_FIX_OFFSET      16
#define SPAWNFLAG_TRAIN_USE_ORIGIN      32

/*QUAKED func_train (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS MOVE_TEAMCHAIN FIX_OFFSET USE_ORIGIN
Trains are moving platforms that players can ride.
The targets origin specifies the min point of the train at each corner.
The train spawns at the first target it is pointing at.
If the train is the target of a button or trigger, it will not begin moving until activated.
speed   default 100
dmg     default 2
noise   looping sound to play when the train is in motion

To have other entities move with the train, set all the piece's team value to the same thing. They will move in unison.
*/
void train_next(edict_t *self);

void MOVEINFO_BLOCKED(train_blocked)(edict_t *self, edict_t *other)
{
    if (!(other->svflags & SVF_MONSTER) && (!other->client)) {
        // give it a chance to go away on it's own terms (like gibs)
        T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, DAMAGE_NONE, (mod_t) { MOD_CRUSH });
        // if it's still there, nuke it
        if (other->inuse && other->solid)
            BecomeExplosion1(other);
        return;
    }

    if (level.time < self->touch_debounce_time)
        return;

    if (!self->dmg)
        return;
    self->touch_debounce_time = level.time + SEC(0.5f);
    T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, DAMAGE_NONE, (mod_t) { MOD_CRUSH });
}

void MOVEINFO_ENDFUNC(train_wait)(edict_t *self)
{
    if (self->target_ent->pathtarget) {
        const char *savetarget;
        edict_t *ent;

        ent = self->target_ent;
        savetarget = ent->target;
        ent->target = ent->pathtarget;
        G_UseTargets(ent, self->activator);
        ent->target = savetarget;

        // make sure we didn't get killed by a killtarget
        if (!self->inuse)
            return;
    }

    if (self->moveinfo.wait) {
        if (self->moveinfo.wait > 0) {
            self->nextthink = level.time + SEC(self->moveinfo.wait);
            self->think = train_next;
        } else if (self->spawnflags & SPAWNFLAG_TRAIN_TOGGLE) { // && wait < 0
            // PMM - clear target_ent, let train_next get called when we get used
            //          train_next (self);
            self->target_ent = NULL;
            // pmm
            self->spawnflags &= ~SPAWNFLAG_TRAIN_START_ON;
            VectorClear(self->velocity);
            self->nextthink = 0;
        }

        if (!(self->flags & FL_TEAMSLAVE) && self->moveinfo.sound_end)
            gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, self->moveinfo.sound_end, 1, ATTN_STATIC, 0);

        self->s.sound = 0;
    } else {
        train_next(self);
    }
}

// PGM
void MOVEINFO_ENDFUNC(train_piece_wait)(edict_t *self)
{
}
// PGM

void THINK(train_next)(edict_t *self)
{
    edict_t *ent;
    vec3_t   dest;
    bool     first;

    first = true;
again:
    if (!self->target) {
        self->s.sound = 0;
        return;
    }

    ent = G_PickTarget(self->target);
    if (!ent) {
        gi.dprintf("%s: train_next: bad target %s\n", etos(self), self->target);
        return;
    }

    self->target = ent->target;

    // check for a teleport path_corner
    if (ent->spawnflags & SPAWNFLAG_PATH_CORNER_TELEPORT) {
        if (!first) {
            gi.dprintf("%s: connected teleport path_corners\n", etos(ent));
            return;
        }
        first = false;

        if (self->spawnflags & SPAWNFLAG_TRAIN_USE_ORIGIN)
            VectorCopy(ent->s.origin, self->s.origin);
        else {
            VectorSubtract(ent->s.origin, self->mins, self->s.origin);

            if (self->spawnflags & SPAWNFLAG_TRAIN_FIX_OFFSET)
                for (int i = 0; i < 3; i++)
                    self->s.origin[i] -= 1;
        }

        VectorCopy(self->s.origin, self->s.old_origin);
        self->s.event = EV_OTHER_TELEPORT;
        gi.linkentity(self);
        goto again;
    }

    // PGM
    if (ent->speed) {
        self->speed = ent->speed;
        self->moveinfo.speed = ent->speed;
        if (ent->accel)
            self->moveinfo.accel = ent->accel;
        else
            self->moveinfo.accel = ent->speed;
        if (ent->decel)
            self->moveinfo.decel = ent->decel;
        else
            self->moveinfo.decel = ent->speed;
        self->moveinfo.current_speed = 0;
    }
    // PGM

    self->moveinfo.wait = ent->wait;
    self->target_ent = ent;

    if (!(self->flags & FL_TEAMSLAVE) && self->moveinfo.sound_start)
        gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, self->moveinfo.sound_start, 1, ATTN_STATIC, 0);

    self->s.sound = self->moveinfo.sound_middle;

    if (self->spawnflags & SPAWNFLAG_TRAIN_USE_ORIGIN)
        VectorCopy(ent->s.origin, dest);
    else {
        VectorSubtract(ent->s.origin, self->mins, dest);

        if (self->spawnflags & SPAWNFLAG_TRAIN_FIX_OFFSET)
            for (int i = 0; i < 3; i++)
                dest[i] -= 1;
    }

    self->moveinfo.state = STATE_TOP;
    VectorCopy(self->s.origin, self->moveinfo.start_origin);
    VectorCopy(dest, self->moveinfo.end_origin);
    Move_Calc(self, dest, train_wait);
    self->spawnflags |= SPAWNFLAG_TRAIN_START_ON;

    // PGM
    if (self->spawnflags & SPAWNFLAG_TRAIN_MOVE_TEAMCHAIN) {
        edict_t *e;
        vec3_t   dir, dst;

        VectorSubtract(dest, self->s.origin, dir);
        for (e = self->teamchain; e; e = e->teamchain) {
            VectorAdd(e->s.origin, dir, dst);
            VectorCopy(e->s.origin, e->moveinfo.start_origin);
            VectorCopy(dst, e->moveinfo.end_origin);

            e->moveinfo.state = STATE_TOP;
            e->speed = self->speed;
            e->moveinfo.speed = self->moveinfo.speed;
            e->moveinfo.accel = self->moveinfo.accel;
            e->moveinfo.decel = self->moveinfo.decel;
            e->movetype = MOVETYPE_PUSH;
            Move_Calc(e, dst, train_piece_wait);
        }
    }
    // PGM
}

static void train_resume(edict_t *self)
{
    edict_t *ent;
    vec3_t   dest;

    ent = self->target_ent;

    if (self->spawnflags & SPAWNFLAG_TRAIN_USE_ORIGIN)
        VectorCopy(ent->s.origin, dest);
    else {
        VectorSubtract(ent->s.origin, self->mins, dest);

        if (self->spawnflags & SPAWNFLAG_TRAIN_FIX_OFFSET)
            for (int i = 0; i < 3; i++)
                dest[i] -= 1;
    }

    // PGM (Paril)
    if (ent->speed) {
        self->speed = ent->speed;
        self->moveinfo.speed = ent->speed;
        if (ent->accel)
            self->moveinfo.accel = ent->accel;
        else
            self->moveinfo.accel = ent->speed;
        if (ent->decel)
            self->moveinfo.decel = ent->decel;
        else
            self->moveinfo.decel = ent->speed;
        self->moveinfo.current_speed = 0;
    }
    // PGM

    self->s.sound = self->moveinfo.sound_middle;

    self->moveinfo.state = STATE_TOP;
    VectorCopy(self->s.origin, self->moveinfo.start_origin);
    VectorCopy(dest, self->moveinfo.end_origin);
    Move_Calc(self, dest, train_wait);
    self->spawnflags |= SPAWNFLAG_TRAIN_START_ON;
}

void THINK(func_train_find)(edict_t *self)
{
    edict_t *ent;

    if (!self->target) {
        gi.dprintf("%s: train_find: no target\n", etos(self));
        return;
    }
    ent = G_PickTarget(self->target);
    if (!ent) {
        gi.dprintf("%s: train_find: target %s not found\n", etos(self), self->target);
        return;
    }
    self->target = ent->target;

    if (self->spawnflags & SPAWNFLAG_TRAIN_USE_ORIGIN)
        VectorCopy(ent->s.origin, self->s.origin);
    else {
        VectorSubtract(ent->s.origin, self->mins, self->s.origin);

        if (self->spawnflags & SPAWNFLAG_TRAIN_FIX_OFFSET)
            for (int i = 0; i < 3; i++)
                self->s.origin[i] -= 1;
    }

    gi.linkentity(self);

    // if not triggered, start immediately
    if (!self->targetname)
        self->spawnflags |= SPAWNFLAG_TRAIN_START_ON;

    if (self->spawnflags & SPAWNFLAG_TRAIN_START_ON) {
        self->nextthink = level.time + FRAME_TIME;
        self->think = train_next;
        self->activator = self;
    }
}

void USE(train_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->activator = activator;

    if (self->spawnflags & SPAWNFLAG_TRAIN_START_ON) {
        if (!(self->spawnflags & SPAWNFLAG_TRAIN_TOGGLE))
            return;
        self->spawnflags &= ~SPAWNFLAG_TRAIN_START_ON;
        VectorClear(self->velocity);
        self->nextthink = 0;
    } else {
        if (self->target_ent)
            train_resume(self);
        else
            train_next(self);
    }
}

void SP_func_train(edict_t *self)
{
    self->movetype = MOVETYPE_PUSH;

    VectorClear(self->s.angles);
    self->moveinfo.blocked = train_blocked;
    if (self->spawnflags & SPAWNFLAG_TRAIN_BLOCK_STOPS)
        self->dmg = 0;
    else if (!self->dmg)
        self->dmg = 100;
    self->solid = SOLID_BSP;
    gi.setmodel(self, self->model);

    if (st.noise) {
        self->moveinfo.sound_middle = gi.soundindex(st.noise);

        // [Paril-KEX] for rhangar1 doors
        if (!ED_WasKeySpecified("attenuation")) {
            self->attenuation = ATTN_STATIC;
        } else if (self->attenuation == -1) {
            self->x.loop_attenuation = ATTN_LOOP_NONE;
            self->attenuation = ATTN_NONE;
        } else if (self->attenuation != ATTN_STATIC) {
            self->x.loop_attenuation = self->attenuation;
        }
    }

    if (!self->speed)
        self->speed = 100;

    self->moveinfo.speed = self->speed;
    self->moveinfo.accel = self->moveinfo.decel = self->moveinfo.speed;

    self->use = train_use;

    gi.linkentity(self);

    if (self->target) {
        // start trains on the second frame, to make sure their targets have had
        // a chance to spawn
        self->nextthink = level.time + FRAME_TIME;
        self->think = func_train_find;
    } else {
        gi.dprintf("%s: no target\n", etos(self));
    }
}

/*QUAKED trigger_elevator (0.3 0.1 0.6) (-8 -8 -8) (8 8 8)
 */
void USE(trigger_elevator_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    edict_t *target;

    if (self->movetarget->nextthink)
        return;

    if (!other->pathtarget) {
        gi.dprintf("%s: elevator used with no pathtarget\n", etos(self));
        return;
    }

    target = G_PickTarget(other->pathtarget);
    if (!target) {
        gi.dprintf("%s: elevator used with bad pathtarget: %s\n", etos(self), other->pathtarget);
        return;
    }

    self->movetarget->target_ent = target;
    train_resume(self->movetarget);
}

void THINK(trigger_elevator_init)(edict_t *self)
{
    if (!self->target) {
        gi.dprintf("%s: has no target\n", etos(self));
        return;
    }
    self->movetarget = G_PickTarget(self->target);
    if (!self->movetarget) {
        gi.dprintf("%s: unable to find target %s\n", etos(self), self->target);
        return;
    }
    if (strcmp(self->movetarget->classname, "func_train") != 0) {
        gi.dprintf("%s: target %s is not a train\n", etos(self), self->target);
        return;
    }

    self->use = trigger_elevator_use;
    self->svflags = SVF_NOCLIENT;
}

void SP_trigger_elevator(edict_t *self)
{
    self->think = trigger_elevator_init;
    self->nextthink = level.time + FRAME_TIME;
}

/*QUAKED func_timer (0.3 0.1 0.6) (-8 -8 -8) (8 8 8) START_ON
"wait"          base time between triggering all targets, default is 1
"random"        wait variance, default is 0

so, the basic time between firing is a random time between
(wait - random) and (wait + random)

"delay"         delay before first firing when turned on, default is 0

"pausetime"     additional delay used only the very first time
                and only if spawned with START_ON

These can used but not touched.
*/

#define SPAWNFLAG_TIMER_START_ON    1

void THINK(func_timer_think)(edict_t *self)
{
    G_UseTargets(self, self->activator);
    self->nextthink = level.time + SEC(self->wait + crandom() * self->random);
}

void USE(func_timer_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->activator = activator;

    // if on, turn it off
    if (self->nextthink) {
        self->nextthink = 0;
        return;
    }

    // turn it on
    if (self->delay)
        self->nextthink = level.time + SEC(self->delay);
    else
        func_timer_think(self);
}

void SP_func_timer(edict_t *self)
{
    if (!self->wait)
        self->wait = 1.0f;

    self->use = func_timer_use;
    self->think = func_timer_think;

    if (self->random >= self->wait) {
        self->random = self->wait - FRAME_TIME_SEC;
        gi.dprintf("%s: random >= wait\n", etos(self));
    }

    if (self->spawnflags & SPAWNFLAG_TIMER_START_ON) {
        self->nextthink = level.time + SEC(1 + st.pausetime + self->delay + self->wait + crandom() * self->random);
        self->activator = self;
    }

    self->svflags = SVF_NOCLIENT;
}

#define SPAWNFLAG_CONVEYOR_START_ON 1
#define SPAWNFLAG_CONVEYOR_TOGGLE   2

/*QUAKED func_conveyor (0 .5 .8) ? START_ON TOGGLE
Conveyors are stationary brushes that move what's on them.
The brush should be have a surface with at least one current content enabled.
speed   default 100
*/

void USE(func_conveyor_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (self->spawnflags & SPAWNFLAG_CONVEYOR_START_ON) {
        self->speed = 0;
        self->spawnflags &= ~SPAWNFLAG_CONVEYOR_START_ON;
    } else {
        self->speed = self->count;
        self->spawnflags |= SPAWNFLAG_CONVEYOR_START_ON;
    }

    if (!(self->spawnflags & SPAWNFLAG_CONVEYOR_TOGGLE))
        self->count = 0;
}

void SP_func_conveyor(edict_t *self)
{
    if (!self->speed)
        self->speed = 100;

    if (!(self->spawnflags & SPAWNFLAG_CONVEYOR_START_ON)) {
        self->count = self->speed;
        self->speed = 0;
    }

    self->use = func_conveyor_use;

    gi.setmodel(self, self->model);
    self->solid = SOLID_BSP;
    gi.linkentity(self);
}

/*QUAKED func_door_secret (0 .5 .8) ? always_shoot 1st_left 1st_down
A secret door.  Slide back and then to the side.

open_once       doors never closes
1st_left        1st move is left of arrow
1st_down        1st move is down from arrow
always_shoot    door is shootebale even if targeted

"angle"     determines the direction
"dmg"       damage to inflic when blocked (default 2)
"wait"      how long to hold in the open position (default 5, -1 means hold)
*/

#define SPAWNFLAG_SECRET_ALWAYS_SHOOT   1
#define SPAWNFLAG_SECRET_1ST_LEFT       2
#define SPAWNFLAG_SECRET_1ST_DOWN       4

void door_secret_move1(edict_t *self);
void door_secret_move2(edict_t *self);
void door_secret_move3(edict_t *self);
void door_secret_move4(edict_t *self);
void door_secret_move5(edict_t *self);
void door_secret_move6(edict_t *self);
void door_secret_done(edict_t *self);

void USE(door_secret_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    // make sure we're not already moving
    if (!VectorEmpty(self->s.origin))
        return;

    Move_Calc(self, self->pos1, door_secret_move1);
    door_use_areaportals(self, true);
}

void MOVEINFO_ENDFUNC(door_secret_move1)(edict_t *self)
{
    self->nextthink = level.time + SEC(1);
    self->think = door_secret_move2;
}

void THINK(door_secret_move2)(edict_t *self)
{
    Move_Calc(self, self->pos2, door_secret_move3);
}

void MOVEINFO_ENDFUNC(door_secret_move3)(edict_t *self)
{
    if (self->wait == -1)
        return;
    self->nextthink = level.time + SEC(self->wait);
    self->think = door_secret_move4;
}

void THINK(door_secret_move4)(edict_t *self)
{
    Move_Calc(self, self->pos1, door_secret_move5);
}

void MOVEINFO_ENDFUNC(door_secret_move5)(edict_t *self)
{
    self->nextthink = level.time + SEC(1);
    self->think = door_secret_move6;
}

void THINK(door_secret_move6)(edict_t *self)
{
    Move_Calc(self, vec3_origin, door_secret_done);
}

void MOVEINFO_ENDFUNC(door_secret_done)(edict_t *self)
{
    if (!(self->targetname) || (self->spawnflags & SPAWNFLAG_SECRET_ALWAYS_SHOOT)) {
        self->health = 0;
        self->takedamage = true;
    }
    door_use_areaportals(self, false);
}

void MOVEINFO_BLOCKED(door_secret_blocked)(edict_t *self, edict_t *other)
{
    if (!(other->svflags & SVF_MONSTER) && (!other->client)) {
        // give it a chance to go away on it's own terms (like gibs)
        T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, DAMAGE_NONE, (mod_t) { MOD_CRUSH });
        // if it's still there, nuke it
        if (other->inuse && other->solid)
            BecomeExplosion1(other);
        return;
    }

    if (level.time < self->touch_debounce_time)
        return;
    self->touch_debounce_time = level.time + SEC(0.5f);

    T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, DAMAGE_NONE, (mod_t) { MOD_CRUSH });
}

void DIE(door_secret_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    self->takedamage = false;
    door_secret_use(self, attacker, attacker);
}

void SP_func_door_secret(edict_t *ent)
{
    vec3_t forward, right, up;
    float  side;
    float  width;
    float  length;

    G_SetMoveinfoSounds(ent, "doors/dr1_strt.wav", "doors/dr1_mid.wav", "doors/dr1_end.wav");

    ent->attenuation = ATTN_STATIC;

    ent->movetype = MOVETYPE_PUSH;
    ent->solid = SOLID_BSP;
    ent->svflags |= SVF_DOOR;
    gi.setmodel(ent, ent->model);

    ent->moveinfo.blocked = door_secret_blocked;
    ent->use = door_secret_use;

    if (!(ent->targetname) || (ent->spawnflags & SPAWNFLAG_SECRET_ALWAYS_SHOOT)) {
        ent->health = 0;
        ent->takedamage = true;
        ent->die = door_secret_die;
    }

    if (!ent->dmg)
        ent->dmg = 2;

    if (!ent->wait)
        ent->wait = 5;

    if (!ent->speed)
        ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = 50;
    else
        ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = ent->speed * 0.1f;

    // calculate positions
    AngleVectors(ent->s.angles, forward, right, up);
    VectorClear(ent->s.angles);
    side = 1.0f - ((ent->spawnflags & SPAWNFLAG_SECRET_1ST_LEFT) ? 2 : 0);
    if (ent->spawnflags & SPAWNFLAG_SECRET_1ST_DOWN)
        width = fabsf(DotProduct(up, ent->size));
    else
        width = fabsf(DotProduct(right, ent->size));
    length = fabsf(DotProduct(forward, ent->size));
    if (ent->spawnflags & SPAWNFLAG_SECRET_1ST_DOWN)
        VectorMA(ent->s.origin, -1 * width, up, ent->pos1);
    else
        VectorMA(ent->s.origin, side * width, right, ent->pos1);
    VectorMA(ent->pos1, length, forward, ent->pos2);

    if (ent->health) {
        ent->takedamage = true;
        ent->die = door_killed;
        ent->max_health = ent->health;
    } else if (ent->targetname && ent->message) {
        gi.soundindex("misc/talk.wav");
        ent->touch = door_touch;
    }

    gi.linkentity(ent);
}

/*QUAKED func_killbox (1 0 0) ?
Kills everything inside when fired, irrespective of protection.
*/
#define SPAWNFLAG_KILLBOX_DEADLY_COOP       2
#define SPAWNFLAG_KILLBOX_EXACT_COLLISION   4

void USE(use_killbox)(edict_t *self, edict_t *other, edict_t *activator)
{
    if (self->spawnflags & SPAWNFLAG_KILLBOX_DEADLY_COOP)
        level.deadly_kill_box = true;

    self->solid = SOLID_TRIGGER;
    gi.linkentity(self);

    KillBoxEx(self, false, MOD_TELEFRAG, self->spawnflags & SPAWNFLAG_KILLBOX_EXACT_COLLISION, false);

    self->solid = SOLID_NOT;
    gi.linkentity(self);

    level.deadly_kill_box = false;
}

void SP_func_killbox(edict_t *ent)
{
    gi.setmodel(ent, ent->model);
    ent->use = use_killbox;
    ent->svflags = SVF_NOCLIENT;
}

/*QUAKED func_eye (0 1 0) ?
Camera-like eye that can track entities.
"pathtarget" point to an info_notnull (which gets freed after spawn) to automatically set
the eye_position
"target"/"killtarget"/"delay"/"message" target keys to fire when we first spot a player
"eye_position" manually set the eye position; note that this is in "forward right up" format, relative to
the origin of the brush and using the entity's angles
"radius" default 512, detection radius for entities
"speed" default 45, how fast, in degrees per second, we should move on each axis to reach the target
"vision_cone" default 0.5 for half cone; how wide the cone of vision should be (relative to their initial angles)
"wait" default 0, the amount of time to wait before returning to neutral angles
*/
#define SPAWNFLAG_FUNC_EYE_FIRED_TARGETS    BIT(17) // internal use only

void THINK(func_eye_think)(edict_t *self)
{
    // find enemy to track
    float closest_dist = FLT_MAX;
    edict_t *closest_player = NULL;

    for (int i = 1; i <= game.maxclients; i++) {
        edict_t *player = &g_edicts[i];
        if (!player->inuse)
            continue;

        vec3_t dir;
        VectorSubtract(player->s.origin, self->s.origin, dir);
        float dist = VectorNormalize(dir);

        if (DotProduct(dir, self->movedir) < self->yaw_speed)
            continue;

        if (dist >= self->dmg_radius)
            continue;

        if (dist < closest_dist) {
            closest_player = player;
            closest_dist = dist;
        }
    }

    self->enemy = closest_player;

    // tracking player
    vec3_t wanted_angles;

    vec3_t fwd, rgt, up;
    AngleVectors(self->s.angles, fwd, rgt, up);

    vec3_t eye_pos;
    VectorCopy(self->s.origin, eye_pos);
    VectorMA(eye_pos, self->move_origin[0], fwd, eye_pos);
    VectorMA(eye_pos, self->move_origin[1], rgt, eye_pos);
    VectorMA(eye_pos, self->move_origin[2], up, eye_pos);

    if (self->enemy) {
        if (!(self->spawnflags & SPAWNFLAG_FUNC_EYE_FIRED_TARGETS)) {
            G_UseTargets(self, self->enemy);
            self->spawnflags |= SPAWNFLAG_FUNC_EYE_FIRED_TARGETS;
        }

        vec3_t dir;
        VectorSubtract(self->enemy->s.origin, eye_pos, dir);
        VectorNormalize(dir);
        vectoangles(dir, wanted_angles);

        self->s.frame = 2;
        self->timestamp = level.time + SEC(self->wait);
    } else {
        if (self->timestamp <= level.time) {
            // return to neutral
            VectorCopy(self->move_angles, wanted_angles);
            self->s.frame = 0;
        } else
            VectorCopy(self->s.angles, wanted_angles);
    }

    for (int i = 0; i < 2; i++) {
        float current = anglemod(self->s.angles[i]);
        float ideal = wanted_angles[i];

        if (current == ideal)
            continue;

        float move = ideal - current;

        if (ideal > current) {
            if (move >= 180)
                move = move - 360;
        } else {
            if (move <= -180)
                move = move + 360;
        }
        if (move > 0) {
            if (move > self->speed)
                move = self->speed;
        } else {
            if (move < -self->speed)
                move = -self->speed;
        }

        self->s.angles[i] = anglemod(current + move);
    }

    self->nextthink = level.time + FRAME_TIME;
}

void THINK(func_eye_setup)(edict_t *self)
{
    edict_t *eye_pos = G_PickTarget(self->pathtarget);

    if (!eye_pos)
        gi.dprintf("%s: bad target\n", etos(self));
    else
        VectorSubtract(eye_pos->s.origin, self->s.origin, self->move_origin);

    VectorNormalize2(self->move_origin, self->movedir);

    self->think = func_eye_think;
    self->nextthink = level.time + HZ(10);
}

void SP_func_eye(edict_t *ent)
{
    ent->movetype = MOVETYPE_PUSH;
    ent->solid = SOLID_BSP;
    gi.setmodel(ent, ent->model);

    if (!st.radius)
        ent->dmg_radius = 512;
    else
        ent->dmg_radius = st.radius;

    if (!ent->speed)
        ent->speed = 45;

    // set vision cone
    if (ED_WasKeySpecified("vision_cone"))
        ent->yaw_speed = ent->vision_cone;

    if (!ent->yaw_speed)
        ent->yaw_speed = 0.5f;

    ent->speed *= FRAME_TIME_SEC;
    VectorCopy(ent->s.angles, ent->move_angles);

    ent->wait = 1.0f;

    if (ent->pathtarget) {
        ent->think = func_eye_setup;
        ent->nextthink = level.time + HZ(10);
    } else {
        ent->think = func_eye_think;
        ent->nextthink = level.time + HZ(10);

        vec3_t right, up;
        AngleVectors(ent->move_angles, ent->movedir, right, up);

        vec3_t move_origin;
        VectorCopy(ent->move_origin, move_origin);
        VectorScale(ent->movedir, move_origin[0], ent->move_origin);
        VectorMA(ent->move_origin, move_origin[1], right, ent->move_origin);
        VectorMA(ent->move_origin, move_origin[2], up, ent->move_origin);
    }

    gi.linkentity(ent);
}
