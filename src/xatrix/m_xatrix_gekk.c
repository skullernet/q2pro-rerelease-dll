// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
    xatrix
    gekk.c
*/

#include "g_local.h"
#include "m_xatrix_gekk.h"

#define SPAWNFLAG_GEKK_CHANT        8
#define SPAWNFLAG_GEKK_NOJUMPING    16
#define SPAWNFLAG_GEKK_NOSWIM       32

static int sound_swing;
static int sound_hit;
static int sound_hit2;
static int sound_speet;
static int loogie_hit;
static int sound_death;
static int sound_pain1;
static int sound_sight;
static int sound_search;
static int sound_step1;
static int sound_step2;
static int sound_step3;
static int sound_thud;
static int sound_chantlow;
static int sound_chantmid;
static int sound_chanthigh;

//static void gekk_swim(edict_t *self);

static void gekk_jump_takeoff(edict_t *self);
//static void gekk_jump_takeoff2(edict_t *self);
static void gekk_check_landing(edict_t *self);
static void gekk_stop_skid(edict_t *self);

//static void water_to_land(edict_t *self);
static void land_to_water(edict_t *self);

static void gekk_check_underwater(edict_t *self);
static void gekk_bite(edict_t *self);

static void gekk_hit_left(edict_t *self);
static void gekk_hit_right(edict_t *self);

const mmove_t gekk_move_attack1;
const mmove_t gekk_move_attack2;
const mmove_t gekk_move_chant;
const mmove_t gekk_move_swim_start;
const mmove_t gekk_move_swim_loop;
const mmove_t gekk_move_spit;
const mmove_t gekk_move_run_start;
const mmove_t gekk_move_run;

//
// CHECKATTACK
//

static bool gekk_check_melee(edict_t *self)
{
    if (!self->enemy || self->enemy->health <= 0 || self->monsterinfo.melee_debounce_time > level.time)
        return false;

    return range_to(self, self->enemy) <= RANGE_MELEE;
}

static bool gekk_check_jump(edict_t *self)
{
    vec3_t v;

    // don't jump if there's no way we can reach standing height
    if (self->absmin[2] + 125 < self->enemy->absmin[2])
        return false;

    VectorSubtract(self->s.origin, self->enemy->s.origin, v);
    v[2] = 0;

    if (VectorLength(v) < 100)
        return false;

    if (frandom() < (self->waterlevel >= WATER_WAIST ? 0.2f : 0.9f))
        return false;

    return true;
}

static bool gekk_check_jump_close(edict_t *self)
{
    vec3_t v;

    VectorSubtract(self->s.origin, self->enemy->s.origin, v);
    v[2] = 0;

    // don't do this if our head is below their feet
    if (VectorLength(v) < 100 && self->absmax[2] <= self->enemy->absmin[2])
        return false;

    return true;
}

bool MONSTERINFO_CHECKATTACK(gekk_checkattack)(edict_t *self)
{
    if (!self->enemy || self->enemy->health <= 0)
        return false;

    if (gekk_check_melee(self)) {
        self->monsterinfo.attack_state = AS_MELEE;
        return true;
    }

    if (self->monsterinfo.attack_state == AS_STRAIGHT && self->monsterinfo.attack_finished > level.time) {
        // keep running fool
        return false;
    }

    if (visible_ex(self, self->enemy, false)) {
        if (gekk_check_jump(self)) {
            self->monsterinfo.attack_state = AS_MISSILE;
            return true;
        }

        if (gekk_check_jump_close(self) && !(self->flags & FL_SWIM)) {
            self->monsterinfo.attack_state = AS_MISSILE;
            return true;
        }
    }

    return false;
}

//
// SOUNDS
//

static void gekk_step(edict_t *self)
{
    int n = irandom1(3);
    if (n == 0)
        gi.sound(self, CHAN_VOICE, sound_step1, 1, ATTN_NORM, 0);
    else if (n == 1)
        gi.sound(self, CHAN_VOICE, sound_step2, 1, ATTN_NORM, 0);
    else
        gi.sound(self, CHAN_VOICE, sound_step3, 1, ATTN_NORM, 0);
}

void MONSTERINFO_SIGHT(gekk_sight)(edict_t *self, edict_t *other)
{
    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void MONSTERINFO_SEARCH(gekk_search)(edict_t *self)
{
    float r;

    if (self->spawnflags & SPAWNFLAG_GEKK_CHANT) {
        r = frandom();
        if (r < 0.33f)
            gi.sound(self, CHAN_VOICE, sound_chantlow, 1, ATTN_NORM, 0);
        else if (r < 0.66f)
            gi.sound(self, CHAN_VOICE, sound_chantmid, 1, ATTN_NORM, 0);
        else
            gi.sound(self, CHAN_VOICE, sound_chanthigh, 1, ATTN_NORM, 0);
    } else
        gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);

    self->health += irandom2(10, 20);
    if (self->health > self->max_health)
        self->health = self->max_health;

    self->monsterinfo.setskin(self);
}

void MONSTERINFO_SETSKIN(gekk_setskin)(edict_t *self)
{
    if (self->health < (self->max_health / 4))
        self->s.skinnum = 2;
    else if (self->health < (self->max_health / 2))
        self->s.skinnum = 1;
    else
        self->s.skinnum = 0;
}

static void gekk_face(edict_t *self)
{
    M_SetAnimation(self, &gekk_move_run);
}

//
// STAND
//

static void ai_stand_gekk(edict_t *self, float dist)
{
    if (self->spawnflags & SPAWNFLAG_GEKK_CHANT) {
        ai_move(self, dist);
        if (!(self->spawnflags & SPAWNFLAG_MONSTER_AMBUSH) && (self->monsterinfo.idle) && (level.time > self->monsterinfo.idle_time)) {
            if (self->monsterinfo.idle_time) {
                self->monsterinfo.idle(self);
                self->monsterinfo.idle_time = level.time + random_time_sec(15, 30);
            } else {
                self->monsterinfo.idle_time = level.time + random_time_sec(0, 15);
            }
        }
    } else
        ai_stand(self, dist);
}

static const mframe_t gekk_frames_stand[] = {
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk }, // 10

    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk }, // 20

    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk }, // 30

    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },

    { ai_stand_gekk, 0, gekk_check_underwater },
};
const mmove_t MMOVE_T(gekk_move_stand) = { FRAME_stand_01, FRAME_stand_39, gekk_frames_stand, NULL };

static const mframe_t gekk_frames_standunderwater[] = {
    { ai_stand_gekk, 14 },
    { ai_stand_gekk, 14 },
    { ai_stand_gekk, 14 },
    { ai_stand_gekk, 14 },
    { ai_stand_gekk, 16 },
    { ai_stand_gekk, 16 },
    { ai_stand_gekk, 16 },
    { ai_stand_gekk, 18 },
    { ai_stand_gekk, 18 },
    { ai_stand_gekk, 18 },

    { ai_stand_gekk, 20 },
    { ai_stand_gekk, 20 },
    { ai_stand_gekk, 22 },
    { ai_stand_gekk, 22 },
    { ai_stand_gekk, 24 },
    { ai_stand_gekk, 24 },
    { ai_stand_gekk, 26 },
    { ai_stand_gekk, 26 },
    { ai_stand_gekk, 24 },
    { ai_stand_gekk, 24 },

    { ai_stand_gekk, 22 },
    { ai_stand_gekk, 22 },
    { ai_stand_gekk, 22 },
    { ai_stand_gekk, 22 },
    { ai_stand_gekk, 22 },
    { ai_stand_gekk, 22 },
    { ai_stand_gekk, 22 },
    { ai_stand_gekk, 22 },
    { ai_stand_gekk, 18 },
    { ai_stand_gekk, 18 },

    { ai_stand_gekk, 18 },
    { ai_stand_gekk, 18 }
};

const mmove_t MMOVE_T(gekk_move_standunderwater) = { FRAME_swim_01, FRAME_swim_32, gekk_frames_standunderwater, NULL };

static void gekk_swim_loop(edict_t *self)
{
    self->monsterinfo.aiflags |= AI_ALTERNATE_FLY;
    self->flags |= FL_SWIM;
    M_SetAnimation(self, &gekk_move_swim_loop);
}

static const mframe_t gekk_frames_swim[] = {
    { ai_run, 14 },
    { ai_run, 14 },
    { ai_run, 14 },
    { ai_run, 14 },
    { ai_run, 16 },
    { ai_run, 16 },
    { ai_run, 16 },
    { ai_run, 18 },
    { ai_run, 18 },
    { ai_run, 18 },

    { ai_run, 20 },
    { ai_run, 20 },
    { ai_run, 22 },
    { ai_run, 22 },
    { ai_run, 24 },
    { ai_run, 24 },
    { ai_run, 26 },
    { ai_run, 26 },
    { ai_run, 24 },
    { ai_run, 24 },

    { ai_run, 22 },
    { ai_run, 22 },
    { ai_run, 22 },
    { ai_run, 22 },
    { ai_run, 22 },
    { ai_run, 22 },
    { ai_run, 22 },
    { ai_run, 22 },
    { ai_run, 18 },
    { ai_run, 18 },

    { ai_run, 18 },
    { ai_run, 18 }
};
const mmove_t MMOVE_T(gekk_move_swim_loop) = { FRAME_swim_01, FRAME_swim_32, gekk_frames_swim, gekk_swim_loop };

static const mframe_t gekk_frames_swim_start[] = {
    { ai_run, 14 },
    { ai_run, 14 },
    { ai_run, 14 },
    { ai_run, 14 },
    { ai_run, 16 },
    { ai_run, 16 },
    { ai_run, 16 },
    { ai_run, 18 },
    { ai_run, 18, gekk_hit_left },
    { ai_run, 18 },

    { ai_run, 20 },
    { ai_run, 20 },
    { ai_run, 22 },
    { ai_run, 22 },
    { ai_run, 24, gekk_hit_right },
    { ai_run, 24 },
    { ai_run, 26 },
    { ai_run, 26 },
    { ai_run, 24 },
    { ai_run, 24 },

    { ai_run, 22, gekk_bite },
    { ai_run, 22 },
    { ai_run, 22 },
    { ai_run, 22 },
    { ai_run, 22 },
    { ai_run, 22 },
    { ai_run, 22 },
    { ai_run, 22 },
    { ai_run, 18 },
    { ai_run, 18 },

    { ai_run, 18 },
    { ai_run, 18 }
};
const mmove_t MMOVE_T(gekk_move_swim_start) = { FRAME_swim_01, FRAME_swim_32, gekk_frames_swim_start, gekk_swim_loop };

#if 0
static void gekk_swim(edict_t *self)
{
    if (gekk_checkattack(self)) {
        if (self->enemy->waterlevel < WATER_WAIST && frandom() > 0.7f)
            water_to_land(self);
        else
            M_SetAnimation(self, &gekk_move_swim_start);
    } else
        M_SetAnimation(self, &gekk_move_swim_start);
}
#endif

void MONSTERINFO_STAND(gekk_stand)(edict_t *self)
{
    if (self->waterlevel >= WATER_WAIST) {
        self->flags |= FL_SWIM;
        self->monsterinfo.aiflags |= AI_ALTERNATE_FLY;
        M_SetAnimation(self, &gekk_move_standunderwater);
    // Don't break out of the chant loop, which is initiated in the spawn function
    } else if (self->monsterinfo.active_move != &gekk_move_chant)
        M_SetAnimation(self, &gekk_move_stand);
}

static void gekk_chant(edict_t *self)
{
    M_SetAnimation(self, &gekk_move_chant);
}

//
// IDLE
//

static void gekk_idle_loop(edict_t *self)
{
    if (frandom() > 0.75f && self->health < self->max_health)
        self->monsterinfo.nextframe = FRAME_idle_01;
}

static const mframe_t gekk_frames_idle[] = {
    { ai_stand_gekk, 0, gekk_search },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },

    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },

    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },
    { ai_stand_gekk },

    { ai_stand_gekk },
    { ai_stand_gekk, 0, gekk_idle_loop }
};
const mmove_t MMOVE_T(gekk_move_idle) = { FRAME_idle_01, FRAME_idle_32, gekk_frames_idle, gekk_stand };
const mmove_t MMOVE_T(gekk_move_idle2) = { FRAME_idle_01, FRAME_idle_32, gekk_frames_idle, gekk_face };

static const mframe_t gekk_frames_idle2[] = {
    { ai_move, 0, gekk_search },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },

    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },

    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },

    { ai_move },
    { ai_move, 0, gekk_idle_loop }
};
const mmove_t MMOVE_T(gekk_move_chant) = { FRAME_idle_01, FRAME_idle_32, gekk_frames_idle2, gekk_chant };

void MONSTERINFO_IDLE(gekk_idle)(edict_t *self)
{
    if ((self->spawnflags & SPAWNFLAG_GEKK_NOSWIM) || self->waterlevel < WATER_WAIST)
        M_SetAnimation(self, &gekk_move_idle);
    else
        M_SetAnimation(self, &gekk_move_swim_start);
    // gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

//
// WALK
//

static const mframe_t gekk_frames_walk[] = {
    { ai_walk, 3.849f, gekk_check_underwater }, // frame 0
    { ai_walk, 19.606f },                       // frame 1
    { ai_walk, 25.583f },                       // frame 2
    { ai_walk, 34.625f, gekk_step },            // frame 3
    { ai_walk, 27.365f },                       // frame 4
    { ai_walk, 28.480f },                       // frame 5
};

const mmove_t MMOVE_T(gekk_move_walk) = { FRAME_run_01, FRAME_run_06, gekk_frames_walk, NULL };

void MONSTERINFO_WALK(gekk_walk)(edict_t *self)
{
    M_SetAnimation(self, &gekk_move_walk);
}

//
// RUN
//

void MONSTERINFO_RUN(gekk_run_start)(edict_t *self)
{
    if (!(self->spawnflags & SPAWNFLAG_GEKK_NOSWIM) && self->waterlevel >= WATER_WAIST)
        M_SetAnimation(self, &gekk_move_swim_start);
    else
        M_SetAnimation(self, &gekk_move_run_start);
}

static void gekk_run(edict_t *self)
{
    if (!(self->spawnflags & SPAWNFLAG_GEKK_NOSWIM) && self->waterlevel >= WATER_WAIST)
        M_SetAnimation(self, &gekk_move_swim_start);
    else if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        M_SetAnimation(self, &gekk_move_stand);
    else
        M_SetAnimation(self, &gekk_move_run);
}

static const mframe_t gekk_frames_run[] = {
    { ai_run, 3.849f, gekk_check_underwater }, // frame 0
    { ai_run, 19.606f },                       // frame 1
    { ai_run, 25.583f },                       // frame 2
    { ai_run, 34.625f, gekk_step },            // frame 3
    { ai_run, 27.365f },                       // frame 4
    { ai_run, 28.480f },                       // frame 5
};
const mmove_t MMOVE_T(gekk_move_run) = { FRAME_run_01, FRAME_run_06, gekk_frames_run, NULL };

static const mframe_t gekk_frames_run_st[] = {
    { ai_run, 0.212f },  // frame 0
    { ai_run, 19.753f }, // frame 1
};
const mmove_t MMOVE_T(gekk_move_run_start) = { FRAME_stand_01, FRAME_stand_02, gekk_frames_run_st, gekk_run };

//
// MELEE
//

static void gekk_hit_left(edict_t *self)
{
    if (!self->enemy)
        return;

    vec3_t aim = { MELEE_DISTANCE, self->mins[0], 8 };
    if (fire_hit(self, aim, irandom2(5, 10), 100))
        gi.sound(self, CHAN_WEAPON, sound_hit, 1, ATTN_NORM, 0);
    else {
        gi.sound(self, CHAN_WEAPON, sound_swing, 1, ATTN_NORM, 0);
        self->monsterinfo.melee_debounce_time = level.time + SEC(1.5f);
    }
}

static void gekk_hit_right(edict_t *self)
{
    if (!self->enemy)
        return;

    vec3_t aim = { MELEE_DISTANCE, self->maxs[0], 8 };
    if (fire_hit(self, aim, irandom2(5, 10), 100))
        gi.sound(self, CHAN_WEAPON, sound_hit2, 1, ATTN_NORM, 0);
    else {
        gi.sound(self, CHAN_WEAPON, sound_swing, 1, ATTN_NORM, 0);
        self->monsterinfo.melee_debounce_time = level.time + SEC(1.5f);
    }
}

static void gekk_check_refire(edict_t *self)
{
    if (!self->enemy || !self->enemy->inuse || self->enemy->health <= 0)
        return;
    if (range_to(self, self->enemy) > RANGE_MELEE)
        return;
    if (self->monsterinfo.melee_debounce_time > level.time)
        return;

    if (self->s.frame == FRAME_clawatk3_09)
        M_SetAnimation(self, &gekk_move_attack2);
    else if (self->s.frame == FRAME_clawatk5_09)
        M_SetAnimation(self, &gekk_move_attack1);
}

void TOUCH(loogie_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if (other == self->owner)
        return;

    if (tr->surface && (tr->surface->flags & SURF_SKY)) {
        G_FreeEdict(self);
        return;
    }

    if (self->owner->client)
        PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

    if (other->takedamage)
        T_Damage(other, self, self->owner, self->velocity, self->s.origin, tr->plane.normal, self->dmg, 1, DAMAGE_ENERGY, (mod_t) { MOD_GEKK });

    gi.sound(self, CHAN_AUTO, loogie_hit, 1.0f, ATTN_NORM, 0);

    G_FreeEdict(self);
};

static void fire_loogie(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed)
{
    edict_t *loogie;
    trace_t  tr;

    loogie = G_Spawn();
    VectorCopy(start, loogie->s.origin);
    VectorCopy(start, loogie->s.old_origin);
    vectoangles(dir, loogie->s.angles);
    VectorScale(dir, speed, loogie->velocity);
    loogie->movetype = MOVETYPE_FLYMISSILE;
    loogie->clipmask = MASK_PROJECTILE;
    loogie->solid = SOLID_BBOX;
    // Paril: this was originally the wrong effect,
    // but it makes it look more acid-y.
    loogie->s.effects |= EF_BLASTER;
    loogie->s.renderfx |= RF_FULLBRIGHT;
    loogie->s.modelindex = gi.modelindex("models/objects/loogy/tris.md2");
    loogie->owner = self;
    loogie->touch = loogie_touch;
    loogie->nextthink = level.time + SEC(2);
    loogie->think = G_FreeEdict;
    loogie->dmg = damage;
    loogie->svflags |= SVF_PROJECTILE;
    gi.linkentity(loogie);

    tr = gi.trace(self->s.origin, NULL, NULL, loogie->s.origin, loogie, MASK_PROJECTILE);
    if (tr.fraction < 1.0f) {
        VectorAdd(tr.endpos, tr.plane.normal, loogie->s.origin);
        loogie->touch(loogie, tr.ent, &tr, false);
    }
}

static void loogie(edict_t *self)
{
    vec3_t start;
    vec3_t forward, right, up;
    vec3_t end;
    vec3_t dir;
    vec3_t gekkoffset = { -18, -0.8f, 24 };

    if (!self->enemy || self->enemy->health <= 0)
        return;

    AngleVectors(self->s.angles, forward, right, up);
    M_ProjectFlashSource(self, gekkoffset, forward, right, start);
    VectorMA(start, 2, up, start);

    VectorCopy(self->enemy->s.origin, end);
    end[2] += self->enemy->viewheight;
    VectorSubtract(end, start, dir);
    VectorNormalize(dir);

    fire_loogie(self, start, dir, 5, 550);

    gi.sound(self, CHAN_BODY, sound_speet, 1.0f, ATTN_NORM, 0);
}

static void reloogie(edict_t *self)
{
    if (frandom() > 0.8f && self->health < self->max_health)
        M_SetAnimation(self, &gekk_move_idle2);
    else if (self->enemy->health > 0 && frandom() > 0.7f && (range_to(self, self->enemy) <= RANGE_NEAR))
        M_SetAnimation(self, &gekk_move_spit);
}

static const mframe_t gekk_frames_spit[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },

    { ai_charge, 0, loogie },
    { ai_charge, 0, reloogie }
};
const mmove_t MMOVE_T(gekk_move_spit) = { FRAME_spit_01, FRAME_spit_07, gekk_frames_spit, gekk_run_start };

static const mframe_t gekk_frames_attack1[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },

    { ai_charge, 0, gekk_hit_left },
    { ai_charge },
    { ai_charge },

    { ai_charge },
    { ai_charge },
    { ai_charge, 0, gekk_check_refire }
};
const mmove_t MMOVE_T(gekk_move_attack1) = { FRAME_clawatk3_01, FRAME_clawatk3_09, gekk_frames_attack1, gekk_run_start };

static const mframe_t gekk_frames_attack2[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, gekk_hit_left },

    { ai_charge },
    { ai_charge },
    { ai_charge, 0, gekk_hit_right },

    { ai_charge },
    { ai_charge },
    { ai_charge, 0, gekk_check_refire }
};
const mmove_t MMOVE_T(gekk_move_attack2) = { FRAME_clawatk5_01, FRAME_clawatk5_09, gekk_frames_attack2, gekk_run_start };

static void gekk_check_underwater(edict_t *self)
{
    if (!(self->spawnflags & SPAWNFLAG_GEKK_NOSWIM) && self->waterlevel >= WATER_WAIST)
        land_to_water(self);
}

static const mframe_t gekk_frames_leapatk[] = {
    { ai_charge },                              // frame 0
    { ai_charge, -0.387f },                     // frame 1
    { ai_charge, -1.113f },                     // frame 2
    { ai_charge, -0.237f },                     // frame 3
    { ai_charge, 6.720f, gekk_jump_takeoff },   // frame 4  last frame on ground
    { ai_charge, 6.414f },                      // frame 5  leaves ground
    { ai_charge, 0.163f },                      // frame 6
    { ai_charge, 28.316f },                     // frame 7
    { ai_charge, 24.198f },                     // frame 8
    { ai_charge, 31.742f },                     // frame 9
    { ai_charge, 35.977f, gekk_check_landing }, // frame 10  last frame in air
    { ai_charge, 12.303f, gekk_stop_skid },     // frame 11  feet back on ground
    { ai_charge, 20.122f, gekk_stop_skid },     // frame 12
    { ai_charge, -1.042f, gekk_stop_skid },     // frame 13
    { ai_charge, 2.556f, gekk_stop_skid },      // frame 14
    { ai_charge, 0.544f, gekk_stop_skid },      // frame 15
    { ai_charge, 1.862f, gekk_stop_skid },      // frame 16
    { ai_charge, 1.224f, gekk_stop_skid },      // frame 17

    { ai_charge, -0.457f, gekk_check_underwater }, // frame 18
};
const mmove_t MMOVE_T(gekk_move_leapatk) = { FRAME_leapatk_01, FRAME_leapatk_19, gekk_frames_leapatk, gekk_run_start };

#if 0
static const mframe_t gekk_frames_leapatk2[] = {
    { ai_charge },                              // frame 0
    { ai_charge, -0.387f },                     // frame 1
    { ai_charge, -1.113f },                     // frame 2
    { ai_charge, -0.237f },                     // frame 3
    { ai_charge, 6.720f, gekk_jump_takeoff2 },  // frame 4  last frame on ground
    { ai_charge, 6.414f },                      // frame 5  leaves ground
    { ai_charge, 0.163f },                      // frame 6
    { ai_charge, 28.316f },                     // frame 7
    { ai_charge, 24.198f },                     // frame 8
    { ai_charge, 31.742f },                     // frame 9
    { ai_charge, 35.977f, gekk_check_landing }, // frame 10  last frame in air
    { ai_charge, 12.303f, gekk_stop_skid },     // frame 11  feet back on ground
    { ai_charge, 20.122f, gekk_stop_skid },     // frame 12
    { ai_charge, -1.042f, gekk_stop_skid },     // frame 13
    { ai_charge, 2.556f, gekk_stop_skid },      // frame 14
    { ai_charge, 0.544f, gekk_stop_skid },      // frame 15
    { ai_charge, 1.862f, gekk_stop_skid },      // frame 16
    { ai_charge, 1.224f, gekk_stop_skid },      // frame 17

    { ai_charge, -0.457f, gekk_check_underwater }, // frame 18
};
const mmove_t MMOVE_T(gekk_move_leapatk2) = { FRAME_leapatk_01, FRAME_leapatk_19, gekk_frames_leapatk2, gekk_run_start };
#endif

static void gekk_bite(edict_t *self)
{
    if (!self->enemy)
        return;

    vec3_t aim = { MELEE_DISTANCE, 0, 0 };
    fire_hit(self, aim, 5, 0);
}

static void gekk_preattack(edict_t *self)
{
    // underwater attack sound
    // gi.sound (self, CHAN_WEAPON, something something underwater sound, 1, ATTN_NORM, 0);
    return;
}

static const mframe_t gekk_frames_attack[] = {
    { ai_charge, 16, gekk_preattack },
    { ai_charge, 16 },
    { ai_charge, 16 },
    { ai_charge, 16 },
    { ai_charge, 16, gekk_bite },
    { ai_charge, 16 },
    { ai_charge, 16 },
    { ai_charge, 16 },
    { ai_charge, 16 },
    { ai_charge, 16, gekk_bite },

    { ai_charge, 16 },
    { ai_charge, 16 },
    { ai_charge, 16 },
    { ai_charge, 16, gekk_hit_left },
    { ai_charge, 16 },
    { ai_charge, 16 },
    { ai_charge, 16 },
    { ai_charge, 16 },
    { ai_charge, 16, gekk_hit_right },
    { ai_charge, 16 },

    { ai_charge, 16 }
};
const mmove_t MMOVE_T(gekk_move_attack) = { FRAME_attack_01, FRAME_attack_21, gekk_frames_attack, gekk_run_start };

void MONSTERINFO_MELEE(gekk_melee)(edict_t *self)
{
    if (self->waterlevel >= WATER_WAIST)
        M_SetAnimation(self, &gekk_move_attack);
    else if (frandom() > 0.66f)
        M_SetAnimation(self, &gekk_move_attack1);
    else
        M_SetAnimation(self, &gekk_move_attack2);
}

//
// ATTACK
//

void TOUCH(gekk_jump_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    vec3_t point;
    vec3_t normal;
    int    damage;

    if (self->health <= 0) {
        self->touch = NULL;
        return;
    }

    if (self->style == 1 && other->takedamage && VectorNormalize2(self->velocity, normal) > 200) {
        VectorMA(self->s.origin, self->maxs[0], normal, point);
        damage = irandom2(10, 20);
        T_Damage(other, self, self, self->velocity, point, normal, damage, damage, DAMAGE_NONE, (mod_t) { MOD_GEKK });
        self->style = 0;
    }

    if (!M_CheckBottom(self)) {
        if (self->groundentity) {
            self->monsterinfo.nextframe = FRAME_leapatk_11;
            self->touch = NULL;
        }
        return;
    }

    self->touch = NULL;
}

static void gekk_jump_takeoff(edict_t *self)
{
    vec3_t forward;

    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
    AngleVectors(self->s.angles, forward, NULL, NULL);
    self->s.origin[2] += 1;

    // high jump
    if (gekk_check_jump(self)) {
        VectorScale(forward, 700, self->velocity);
        self->velocity[2] = 250;
    } else {
        VectorScale(forward, 250, self->velocity);
        self->velocity[2] = 400;
    }

    self->groundentity = NULL;
    self->monsterinfo.aiflags |= AI_DUCKED;
    self->monsterinfo.attack_finished = level.time + SEC(3);
    self->touch = gekk_jump_touch;
    self->style = 1;
}

#if 0
static void gekk_jump_takeoff2(edict_t *self)
{
    vec3_t forward;

    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
    AngleVectors(self->s.angles, forward, NULL, NULL);
    self->s.origin[2] = self->enemy->s.origin[2];

    if (gekk_check_jump(self)) {
        VectorScale(forward, 300, self->velocity);
        self->velocity[2] = 250;
    } else {
        VectorScale(forward, 150, self->velocity);
        self->velocity[2] = 300;
    }

    self->groundentity = NULL;
    self->monsterinfo.aiflags |= AI_DUCKED;
    self->monsterinfo.attack_finished = level.time + SEC(3);
    self->touch = gekk_jump_touch;
    self->style = 1;
}
#endif

static void gekk_stop_skid(edict_t *self)
{
    if (self->groundentity)
        VectorClear(self->velocity);
}

static void gekk_check_landing(edict_t *self)
{
    if (self->groundentity) {
        gi.sound(self, CHAN_WEAPON, sound_thud, 1, ATTN_NORM, 0);
        self->monsterinfo.attack_finished = 0;

        if (self->monsterinfo.unduck)
            self->monsterinfo.unduck(self);

        VectorClear(self->velocity);
        return;
    }

    // Paril: allow them to "pull" up ledges
    vec3_t fwd;
    AngleVectors(self->s.angles, fwd, NULL, NULL);

    if (DotProduct(fwd, self->velocity) < 200)
        VectorMA(self->velocity, 200, fwd, self->velocity);

    // note to self
    // causing skid
    if (level.time > self->monsterinfo.attack_finished)
        self->monsterinfo.nextframe = FRAME_leapatk_11;
    else
        self->monsterinfo.nextframe = FRAME_leapatk_12;
}

void MONSTERINFO_ATTACK(gekk_attack)(edict_t *self)
{
    float r = range_to(self, self->enemy);

    if (self->flags & FL_SWIM) {
        if (self->enemy && self->enemy->waterlevel >= WATER_WAIST && r <= RANGE_NEAR)
            return;

        self->flags &= ~FL_SWIM;
        self->monsterinfo.aiflags &= ~AI_ALTERNATE_FLY;
        M_SetAnimation(self, &gekk_move_leapatk);
        self->monsterinfo.nextframe = FRAME_leapatk_05;
    } else if (r >= RANGE_MID) {
        if (brandom()) {
            M_SetAnimation(self, &gekk_move_spit);
        } else {
            M_SetAnimation(self, &gekk_move_run_start);
            self->monsterinfo.attack_finished = level.time + SEC(2);
        }
    } else if (frandom() > 0.7f) {
        M_SetAnimation(self, &gekk_move_spit);
    } else if ((self->spawnflags & SPAWNFLAG_GEKK_NOJUMPING) || frandom() > 0.7f) {
        M_SetAnimation(self, &gekk_move_run_start);
        self->monsterinfo.attack_finished = level.time + SEC(1.4f);
    } else {
        M_SetAnimation(self, &gekk_move_leapatk);
    }
}

//
// PAIN
//

static const mframe_t gekk_frames_pain[] = {
    { ai_move }, // frame 0
    { ai_move }, // frame 1
    { ai_move }, // frame 2
    { ai_move }, // frame 3
    { ai_move }, // frame 4
    { ai_move }, // frame 5
};
const mmove_t MMOVE_T(gekk_move_pain) = { FRAME_pain_01, FRAME_pain_06, gekk_frames_pain, gekk_run_start };

static const mframe_t gekk_frames_pain1[] = {
    { ai_move }, // frame 0
    { ai_move }, // frame 1
    { ai_move }, // frame 2
    { ai_move }, // frame 3
    { ai_move }, // frame 4
    { ai_move }, // frame 5
    { ai_move }, // frame 6
    { ai_move }, // frame 7
    { ai_move }, // frame 8
    { ai_move }, // frame 9

    { ai_move, 0, gekk_check_underwater }
};
const mmove_t MMOVE_T(gekk_move_pain1) = { FRAME_pain3_01, FRAME_pain3_11, gekk_frames_pain1, gekk_run_start };

static const mframe_t gekk_frames_pain2[] = {
    { ai_move }, // frame 0
    { ai_move }, // frame 1
    { ai_move }, // frame 2
    { ai_move }, // frame 3
    { ai_move }, // frame 4
    { ai_move }, // frame 5
    { ai_move }, // frame 6
    { ai_move }, // frame 7
    { ai_move }, // frame 8
    { ai_move }, // frame 9

    { ai_move }, // frame 10
    { ai_move }, // frame 11
    { ai_move, 0, gekk_check_underwater },
};
const mmove_t MMOVE_T(gekk_move_pain2) = { FRAME_pain4_01, FRAME_pain4_13, gekk_frames_pain2, gekk_run_start };

void PAIN(gekk_pain)(edict_t *self, edict_t *other, float kick, int damage, mod_t mod)
{
    if (self->spawnflags & SPAWNFLAG_GEKK_CHANT) {
        self->spawnflags &= ~SPAWNFLAG_GEKK_CHANT;
        return;
    }

    if (level.time < self->pain_debounce_time)
        return;

    self->pain_debounce_time = level.time + SEC(3);

    gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);

    if (self->waterlevel >= WATER_WAIST) {
        if (!(self->flags & FL_SWIM)) {
            self->monsterinfo.aiflags |= AI_ALTERNATE_FLY;
            self->flags |= FL_SWIM;
        }

        if (M_ShouldReactToPain(self, mod)) // no pain anims in nightmare
            M_SetAnimation(self, &gekk_move_pain);
    } else if (M_ShouldReactToPain(self, mod)) { // no pain anims in nightmare
        if (brandom())
            M_SetAnimation(self, &gekk_move_pain1);
        else
            M_SetAnimation(self, &gekk_move_pain2);
    }
}

//
// DEATH
//

static void gekk_dead(edict_t *self)
{
    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, -8);
    monster_dead(self);
}

static const gib_def_t gekk_gibs[] = {
    { "models/objects/gekkgib/pelvis/tris.md2", 1, GIB_ACID },
    { "models/objects/gekkgib/arm/tris.md2", 2, GIB_ACID },
    { "models/objects/gekkgib/torso/tris.md2", 1, GIB_ACID },
    { "models/objects/gekkgib/claw/tris.md2", 1, GIB_ACID },
    { "models/objects/gekkgib/leg/tris.md2", 2, GIB_ACID },
    { "models/objects/gekkgib/head/tris.md2", 1, GIB_ACID | GIB_HEAD },
    { 0 }
};

static void gekk_gib(edict_t *self, int damage)
{
    gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
    ThrowGibs(self, damage, gekk_gibs);
}

static void gekk_gibfest(edict_t *self)
{
    gekk_gib(self, 20);
    self->deadflag = true;
}

static void isgibfest(edict_t *self)
{
    if (frandom() > 0.9f)
        gekk_gibfest(self);
}

static void gekk_shrink(edict_t *self)
{
    self->maxs[2] = 0;
    self->svflags |= SVF_DEADMONSTER;
    gi.linkentity(self);
}

static const mframe_t gekk_frames_death1[] = {
    { ai_move, -5.151f },              // frame 0
    { ai_move, -12.223f },             // frame 1
    { ai_move, -11.484f },             // frame 2
    { ai_move, -17.952f },             // frame 3
    { ai_move, -6.953f },              // frame 4
    { ai_move, -7.393f, gekk_shrink }, // frame 5
    { ai_move, -10.713f },             // frame 6
    { ai_move, -17.464f },             // frame 7
    { ai_move, -11.678f },             // frame 8
    { ai_move, -11.678f }              // frame 9
};
const mmove_t MMOVE_T(gekk_move_death1) = { FRAME_death1_01, FRAME_death1_10, gekk_frames_death1, gekk_dead };

static const mframe_t gekk_frames_death3[] = {
    { ai_move },                     // frame 0
    { ai_move, 0.022f },             // frame 1
    { ai_move, 0.169f },             // frame 2
    { ai_move, -0.710f },            // frame 3
    { ai_move, -13.446f },           // frame 4
    { ai_move, -7.654f, isgibfest }, // frame 5
    { ai_move, -31.951f },           // frame 6
};
const mmove_t MMOVE_T(gekk_move_death3) = { FRAME_death3_01, FRAME_death3_07, gekk_frames_death3, gekk_dead };

static const mframe_t gekk_frames_death4[] = {
    { ai_move, 5.103f },               // frame 0
    { ai_move, -4.808f },              // frame 1
    { ai_move, -10.509f },             // frame 2
    { ai_move, -9.899f },              // frame 3
    { ai_move, 4.033f, isgibfest },    // frame 4
    { ai_move, -5.197f },              // frame 5
    { ai_move, -0.919f },              // frame 6
    { ai_move, -8.821f },              // frame 7
    { ai_move, -5.626f },              // frame 8
    { ai_move, -8.865f, isgibfest },   // frame 9
    { ai_move, -0.845f },              // frame 10
    { ai_move, 1.986f },               // frame 11
    { ai_move, 0.170f },               // frame 12
    { ai_move, 1.339f, isgibfest },    // frame 13
    { ai_move, -0.922f },              // frame 14
    { ai_move, 0.818f },               // frame 15
    { ai_move, -1.288f },              // frame 16
    { ai_move, -1.408f, isgibfest },   // frame 17
    { ai_move, -7.787f },              // frame 18
    { ai_move, -3.995f },              // frame 19
    { ai_move, -4.604f },              // frame 20
    { ai_move, -1.715f, isgibfest },   // frame 21
    { ai_move, -0.564f },              // frame 22
    { ai_move, -0.597f },              // frame 23
    { ai_move, 0.074f },               // frame 24
    { ai_move, -0.309f, isgibfest },   // frame 25
    { ai_move, -0.395f },              // frame 26
    { ai_move, -0.501f },              // frame 27
    { ai_move, -0.325f },              // frame 28
    { ai_move, -0.931f, isgibfest },   // frame 29
    { ai_move, -1.433f },              // frame 30
    { ai_move, -1.626f },              // frame 31
    { ai_move, 4.680f },               // frame 32
    { ai_move, 0.560f },               // frame 33
    { ai_move, -0.549f, gekk_gibfest } // frame 34
};
const mmove_t MMOVE_T(gekk_move_death4) = { FRAME_death4_01, FRAME_death4_35, gekk_frames_death4, gekk_dead };

static const mframe_t gekk_frames_wdeath[] = {
    { ai_move }, // frame 0
    { ai_move }, // frame 1
    { ai_move }, // frame 2
    { ai_move }, // frame 3
    { ai_move }, // frame 4
    { ai_move }, // frame 5
    { ai_move }, // frame 6
    { ai_move }, // frame 7
    { ai_move }, // frame 8
    { ai_move }, // frame 9
    { ai_move }, // frame 10
    { ai_move }, // frame 11
    { ai_move }, // frame 12
    { ai_move }, // frame 13
    { ai_move }, // frame 14
    { ai_move }, // frame 15
    { ai_move }, // frame 16
    { ai_move }, // frame 17
    { ai_move }, // frame 18
    { ai_move }, // frame 19
    { ai_move }, // frame 20
    { ai_move }, // frame 21
    { ai_move }, // frame 22
    { ai_move }, // frame 23
    { ai_move }, // frame 24
    { ai_move }, // frame 25
    { ai_move }, // frame 26
    { ai_move }, // frame 27
    { ai_move }, // frame 28
    { ai_move }, // frame 29
    { ai_move }, // frame 30
    { ai_move }, // frame 31
    { ai_move }, // frame 32
    { ai_move }, // frame 33
    { ai_move }, // frame 34
    { ai_move }, // frame 35
    { ai_move }, // frame 36
    { ai_move }, // frame 37
    { ai_move }, // frame 38
    { ai_move }, // frame 39
    { ai_move }, // frame 40
    { ai_move }, // frame 41
    { ai_move }, // frame 42
    { ai_move }, // frame 43
    { ai_move }  // frame 44
};
const mmove_t MMOVE_T(gekk_move_wdeath) = { FRAME_wdeath_01, FRAME_wdeath_45, gekk_frames_wdeath, gekk_dead };

void DIE(gekk_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    float r;

    if (M_CheckGib(self, mod)) {
        gekk_gib(self, damage);
        self->deadflag = true;
        return;
    }

    if (self->deadflag)
        return;

    gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
    self->deadflag = true;
    self->takedamage = true;

    if (self->waterlevel >= WATER_WAIST) {
        gekk_shrink(self);
        M_SetAnimation(self, &gekk_move_wdeath);
    } else {
        r = frandom();
        if (r > 0.66f)
            M_SetAnimation(self, &gekk_move_death1);
        else if (r > 0.33f)
            M_SetAnimation(self, &gekk_move_death3);
        else
            M_SetAnimation(self, &gekk_move_death4);
    }
}

/*
    duck
*/
static const mframe_t gekk_frames_lduck[] = {
    { ai_move }, // frame 0
    { ai_move }, // frame 1
    { ai_move }, // frame 2
    { ai_move }, // frame 3
    { ai_move }, // frame 4
    { ai_move }, // frame 5
    { ai_move }, // frame 6
    { ai_move }, // frame 7
    { ai_move }, // frame 8
    { ai_move }, // frame 9

    { ai_move }, // frame 10
    { ai_move }, // frame 11
    { ai_move }  // frame 12
};
const mmove_t MMOVE_T(gekk_move_lduck) = { FRAME_lduck_01, FRAME_lduck_13, gekk_frames_lduck, gekk_run_start };

static const mframe_t gekk_frames_rduck[] = {
    { ai_move }, // frame 0
    { ai_move }, // frame 1
    { ai_move }, // frame 2
    { ai_move }, // frame 3
    { ai_move }, // frame 4
    { ai_move }, // frame 5
    { ai_move }, // frame 6
    { ai_move }, // frame 7
    { ai_move }, // frame 8
    { ai_move }, // frame 9
    { ai_move }, // frame 10
    { ai_move }, // frame 11
    { ai_move }  // frame 12
};
const mmove_t MMOVE_T(gekk_move_rduck) = { FRAME_rduck_01, FRAME_rduck_13, gekk_frames_rduck, gekk_run_start };

void MONSTERINFO_DODGE(gekk_dodge)(edict_t *self, edict_t *attacker, gtime_t eta, trace_t *tr, bool gravity)
{
    // [Paril-KEX] this dodge is bad
#if 0
    float r;

    r = frandom();
    if (r > 0.25f)
        return;

    if (!self->enemy)
        self->enemy = attacker;

    if (self->waterlevel) {
        M_SetAnimation(self, &gekk_move_attack);
        return;
    }

    if (skill->integer == 0) {
        r = frandom();
        if (r > 0.5f)
            M_SetAnimation(self, &gekk_move_lduck);
        else
            M_SetAnimation(self, &gekk_move_rduck);
        return;
    }

    self->monsterinfo.pausetime = level.time + eta + SEC(0.3f);
    r = frandom();

    if (skill->integer == 1) {
        if (r > 0.33f) {
            r = frandom();
            if (r > 0.5f)
                M_SetAnimation(self, &gekk_move_lduck);
            else
                M_SetAnimation(self, &gekk_move_rduck);
        } else {
            r = frandom();
            if (r > 0.66f)
                M_SetAnimation(self, &gekk_move_attack1);
            else
                M_SetAnimation(self, &gekk_move_attack2);
        }
        return;
    }

    if (skill->integer == 2) {
        if (r > 0.66f) {
            r = frandom();
            if (r > 0.5f)
                M_SetAnimation(self, &gekk_move_lduck);
            else
                M_SetAnimation(self, &gekk_move_rduck);
        } else {
            r = frandom();
            if (r > 0.66f)
                M_SetAnimation(self, &gekk_move_attack1);
            else
                M_SetAnimation(self, &gekk_move_attack2);
        }
        return;
    }

    r = frandom();
    if (r > 0.66f)
        M_SetAnimation(self, &gekk_move_attack1);
    else
        M_SetAnimation(self, &gekk_move_attack2);
#endif
}

//
// SPAWN
//

static void gekk_set_fly_parameters(edict_t *self)
{
    self->monsterinfo.fly_thrusters = false;
    self->monsterinfo.fly_acceleration = 25;
    self->monsterinfo.fly_speed = 150;
    // only melee, so get in close
    self->monsterinfo.fly_min_distance = 10;
    self->monsterinfo.fly_max_distance = 10;
}


//================
// ROGUE
static void gekk_jump_down(edict_t *self)
{
    vec3_t forward, up;

    AngleVectors(self->s.angles, forward, NULL, up);
    VectorMA(self->velocity, 100, forward, self->velocity);
    VectorMA(self->velocity, 300, up, self->velocity);
}

static void gekk_jump_up(edict_t *self)
{
    vec3_t forward, up;

    AngleVectors(self->s.angles, forward, NULL, up);
    VectorMA(self->velocity, 200, forward, self->velocity);
    VectorMA(self->velocity, 450, up, self->velocity);
}

static void gekk_jump_wait_land(edict_t *self)
{
    if (!monster_jump_finished(self) && self->groundentity == NULL)
        self->monsterinfo.nextframe = self->s.frame;
    else
        self->monsterinfo.nextframe = self->s.frame + 1;
}

static const mframe_t gekk_frames_jump_up[] = {
    { ai_move, -8, gekk_jump_up },
    { ai_move, -8 },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, gekk_jump_wait_land },
    { ai_move }
};
const mmove_t MMOVE_T(gekk_move_jump_up) = { FRAME_leapatk_04, FRAME_leapatk_11, gekk_frames_jump_up, gekk_run };

static const mframe_t gekk_frames_jump_down[] = {
    { ai_move, 0, gekk_jump_down },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, gekk_jump_wait_land },
    { ai_move }
};
const mmove_t MMOVE_T(gekk_move_jump_down) = { FRAME_leapatk_04, FRAME_leapatk_11, gekk_frames_jump_down, gekk_run };

static void gekk_jump_updown(edict_t *self, blocked_jump_result_t result)
{
    if (!self->enemy)
        return;

    if (result == JUMP_JUMP_UP)
        M_SetAnimation(self, &gekk_move_jump_up);
    else
        M_SetAnimation(self, &gekk_move_jump_down);
}

/*
===
Blocked
===
*/
bool MONSTERINFO_BLOCKED(gekk_blocked)(edict_t *self, float dist)
{
    blocked_jump_result_t result = blocked_checkjump(self, dist);
    if (result != NO_JUMP) {
        if (result != JUMP_TURN)
            gekk_jump_updown(self, result);
        return true;
    }

    if (blocked_checkplat(self, dist))
        return true;

    return false;
}
// ROGUE
//================

static void gekk_precache(void)
{
    sound_swing = gi.soundindex("gek/gk_atck1.wav");
    sound_hit = gi.soundindex("gek/gk_atck2.wav");
    sound_hit2 = gi.soundindex("gek/gk_atck3.wav");
    sound_speet = gi.soundindex("gek/gk_atck4.wav");
    loogie_hit = gi.soundindex("gek/loogie_hit.wav");
    sound_death = gi.soundindex("gek/gk_deth1.wav");
    sound_pain1 = gi.soundindex("gek/gk_pain1.wav");
    sound_sight = gi.soundindex("gek/gk_sght1.wav");
    sound_search = gi.soundindex("gek/gk_idle1.wav");
    sound_step1 = gi.soundindex("gek/gk_step1.wav");
    sound_step2 = gi.soundindex("gek/gk_step2.wav");
    sound_step3 = gi.soundindex("gek/gk_step3.wav");
    sound_thud = gi.soundindex("mutant/thud1.wav");

    sound_chantlow = gi.soundindex("gek/gek_low.wav");
    sound_chantmid = gi.soundindex("gek/gek_mid.wav");
    sound_chanthigh = gi.soundindex("gek/gek_high.wav");
}

/*QUAKED monster_gekk (1 .5 0) (-16 -16 -24) (16 16 24) Ambush Trigger_Spawn Sight Chant NoJumping
 */
void SP_monster_gekk(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    G_AddPrecache(gekk_precache);

    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;
    self->s.modelindex = gi.modelindex("models/monsters/gekk/tris.md2");
    VectorSet(self->mins, -18, -18, -24);
    VectorSet(self->maxs, 18, 18, 24);

    PrecacheGibs(gekk_gibs);

    self->health = 125 * st.health_multiplier;
    self->gib_health = -30;
    self->mass = 300;

    self->pain = gekk_pain;
    self->die = gekk_die;

    self->monsterinfo.stand = gekk_stand;

    self->monsterinfo.walk = gekk_walk;
    self->monsterinfo.run = gekk_run_start;
    self->monsterinfo.dodge = gekk_dodge;
    self->monsterinfo.attack = gekk_attack;
    self->monsterinfo.melee = gekk_melee;
    self->monsterinfo.sight = gekk_sight;
    self->monsterinfo.search = gekk_search;
    self->monsterinfo.idle = gekk_idle;
    self->monsterinfo.checkattack = gekk_checkattack;
    self->monsterinfo.setskin = gekk_setskin;

    gi.linkentity(self);

    M_SetAnimation(self, &gekk_move_stand);

    self->monsterinfo.scale = MODEL_SCALE;

    walkmonster_start(self);

    if (self->spawnflags & SPAWNFLAG_GEKK_CHANT)
        M_SetAnimation(self, &gekk_move_chant);

    self->monsterinfo.can_jump = !(self->spawnflags & SPAWNFLAG_GEKK_NOJUMPING);
    self->monsterinfo.drop_height = 256;
    self->monsterinfo.jump_height = 68;
    self->monsterinfo.blocked = gekk_blocked;

    gekk_set_fly_parameters(self);
}

#if 0
static void water_to_land(edict_t *self)
{
    self->monsterinfo.aiflags &= ~AI_ALTERNATE_FLY;
    self->flags &= ~FL_SWIM;
    self->yaw_speed = 20;
    self->viewheight = 25;

    M_SetAnimation(self, &gekk_move_leapatk2);

    VectorSet(self->mins, -18, -18, -24);
    VectorSet(self->maxs, 18, 18, 24);
}
#endif

static void land_to_water(edict_t *self)
{
    self->monsterinfo.aiflags |= AI_ALTERNATE_FLY;
    self->flags |= FL_SWIM;
    self->yaw_speed = 10;
    self->viewheight = 10;

    M_SetAnimation(self, &gekk_move_swim_start);

    VectorSet(self->mins, -18, -18, -24);
    VectorSet(self->maxs, 18, 18, 16);
}
