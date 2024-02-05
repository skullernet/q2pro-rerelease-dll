// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

mutant

==============================================================================
*/

#include "g_local.h"
#include "m_mutant.h"

#define SPAWNFLAG_MUTANT_NOJUMPING  8

static int sound_swing;
static int sound_hit;
static int sound_hit2;
static int sound_death;
static int sound_idle;
static int sound_pain1;
static int sound_pain2;
static int sound_sight;
static int sound_search;
static int sound_step1;
static int sound_step2;
static int sound_step3;
static int sound_thud;

//
// SOUNDS
//

static void mutant_step(edict_t *self)
{
    int n = irandom1(3);
    if (n == 0)
        gi.sound(self, CHAN_BODY, sound_step1, 1, ATTN_NORM, 0);
    else if (n == 1)
        gi.sound(self, CHAN_BODY, sound_step2, 1, ATTN_NORM, 0);
    else
        gi.sound(self, CHAN_BODY, sound_step3, 1, ATTN_NORM, 0);
}

void MONSTERINFO_SIGHT(mutant_sight)(edict_t *self, edict_t *other)
{
    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void MONSTERINFO_SEARCH(mutant_search)(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

//
// STAND
//

static const mframe_t mutant_frames_stand[] = {
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand }, // 10

    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand }, // 20

    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand }, // 30

    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand }, // 40

    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand }, // 50

    { ai_stand }
};
const mmove_t MMOVE_T(mutant_move_stand) = { FRAME_stand101, FRAME_stand151, mutant_frames_stand, NULL };

void MONSTERINFO_STAND(mutant_stand)(edict_t *self)
{
    M_SetAnimation(self, &mutant_move_stand);
}

//
// IDLE
//

static void mutant_idle_loop(edict_t *self)
{
    if (frandom() < 0.75f)
        self->monsterinfo.nextframe = FRAME_stand155;
}

static const mframe_t mutant_frames_idle[] = {
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand }, // scratch loop start
    { ai_stand },
    { ai_stand },
    { ai_stand, 0, mutant_idle_loop }, // scratch loop end
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand }
};
const mmove_t MMOVE_T(mutant_move_idle) = { FRAME_stand152, FRAME_stand164, mutant_frames_idle, mutant_stand };

void MONSTERINFO_IDLE(mutant_idle)(edict_t *self)
{
    M_SetAnimation(self, &mutant_move_idle);
    gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

//
// WALK
//

static const mframe_t mutant_frames_walk[] = {
    { ai_walk, 3 },
    { ai_walk, 1 },
    { ai_walk, 5 },
    { ai_walk, 10 },
    { ai_walk, 13 },
    { ai_walk, 10 },
    { ai_walk },
    { ai_walk, 5 },
    { ai_walk, 6 },
    { ai_walk, 16 },
    { ai_walk, 15 },
    { ai_walk, 6 }
};
const mmove_t MMOVE_T(mutant_move_walk) = { FRAME_walk05, FRAME_walk16, mutant_frames_walk, NULL };

static void mutant_walk_loop(edict_t *self)
{
    M_SetAnimation(self, &mutant_move_walk);
}

static const mframe_t mutant_frames_start_walk[] = {
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, -2 },
    { ai_walk, 1 }
};
const mmove_t MMOVE_T(mutant_move_start_walk) = { FRAME_walk01, FRAME_walk04, mutant_frames_start_walk, mutant_walk_loop };

void MONSTERINFO_WALK(mutant_walk)(edict_t *self)
{
    M_SetAnimation(self, &mutant_move_start_walk);
}

//
// RUN
//

static const mframe_t mutant_frames_run[] = {
    { ai_run, 40 },
    { ai_run, 40, mutant_step },
    { ai_run, 24 },
    { ai_run, 5, mutant_step },
    { ai_run, 17 },
    { ai_run, 10 }
};
const mmove_t MMOVE_T(mutant_move_run) = { FRAME_run03, FRAME_run08, mutant_frames_run, NULL };

void MONSTERINFO_RUN(mutant_run)(edict_t *self)
{
    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        M_SetAnimation(self, &mutant_move_stand);
    else
        M_SetAnimation(self, &mutant_move_run);
}

//
// MELEE
//

static void mutant_hit_left(edict_t *self)
{
    vec3_t aim = { MELEE_DISTANCE, self->mins[0], 8 };
    if (fire_hit(self, aim, irandom2(5, 15), 100))
        gi.sound(self, CHAN_WEAPON, sound_hit, 1, ATTN_NORM, 0);
    else {
        gi.sound(self, CHAN_WEAPON, sound_swing, 1, ATTN_NORM, 0);
        self->monsterinfo.melee_debounce_time = level.time + SEC(1.5f);
    }
}

static void mutant_hit_right(edict_t *self)
{
    vec3_t aim = { MELEE_DISTANCE, self->maxs[0], 8 };
    if (fire_hit(self, aim, irandom2(5, 15), 100))
        gi.sound(self, CHAN_WEAPON, sound_hit2, 1, ATTN_NORM, 0);
    else {
        gi.sound(self, CHAN_WEAPON, sound_swing, 1, ATTN_NORM, 0);
        self->monsterinfo.melee_debounce_time = level.time + SEC(1.5f);
    }
}

static void mutant_check_refire(edict_t *self)
{
    if (!self->enemy || !self->enemy->inuse || self->enemy->health <= 0)
        return;

    if ((self->monsterinfo.melee_debounce_time <= level.time) && ((brandom()) || (range_to(self, self->enemy) <= RANGE_MELEE)))
        self->monsterinfo.nextframe = FRAME_attack09;
}

static const mframe_t mutant_frames_attack[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, mutant_hit_left },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, mutant_hit_right },
    { ai_charge, 0, mutant_check_refire }
};
const mmove_t MMOVE_T(mutant_move_attack) = { FRAME_attack09, FRAME_attack15, mutant_frames_attack, mutant_run };

void MONSTERINFO_MELEE(mutant_melee)(edict_t *self)
{
    M_SetAnimation(self, &mutant_move_attack);
}

//
// ATTACK
//

void TOUCH(mutant_jump_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    vec3_t point;
    vec3_t normal;
    int    damage;

    if (self->health <= 0) {
        self->touch = NULL;
        return;
    }

    // [Paril-KEX] only if we're actually moving fast enough to hurt
    if (self->style == 1 && other->takedamage && VectorNormalize2(self->velocity, normal) > 30) {
        VectorMA(self->s.origin, self->maxs[0], normal, point);
        damage = irandom2(40, 50);
        T_Damage(other, self, self, self->velocity, point, normal, damage, damage, DAMAGE_NONE, (mod_t) { MOD_UNKNOWN });
        self->style = 0;
    }

    if (!M_CheckBottom(self)) {
        if (self->groundentity) {
            self->monsterinfo.nextframe = FRAME_attack02;
            self->touch = NULL;
        }
        return;
    }

    self->touch = NULL;
}

static void mutant_jump_takeoff(edict_t *self)
{
    vec3_t forward;

    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
    AngleVectors(self->s.angles, forward, NULL, NULL);
    self->s.origin[2] += 1;
    VectorScale(forward, 425, self->velocity);
    self->velocity[2] = 160;
    self->groundentity = NULL;
    self->monsterinfo.aiflags |= AI_DUCKED;
    self->monsterinfo.attack_finished = level.time + SEC(3);
    self->style = 1;
    self->touch = mutant_jump_touch;
}

static void mutant_check_landing(edict_t *self)
{
    monster_jump_finished(self);

    if (self->groundentity) {
        gi.sound(self, CHAN_WEAPON, sound_thud, 1, ATTN_NORM, 0);
        self->monsterinfo.attack_finished = level.time + random_time_sec(0.5f, 1.5f);

        if (self->monsterinfo.unduck)
            self->monsterinfo.unduck(self);

        if (range_to(self, self->enemy) <= RANGE_MELEE * 2)
            self->monsterinfo.melee(self);

        return;
    }

    if (level.time > self->monsterinfo.attack_finished)
        self->monsterinfo.nextframe = FRAME_attack02;
    else
        self->monsterinfo.nextframe = FRAME_attack05;
}

static const mframe_t mutant_frames_jump[] = {
    { ai_charge },
    { ai_charge, 17 },
    { ai_charge, 15, mutant_jump_takeoff },
    { ai_charge, 15 },
    { ai_charge, 15, mutant_check_landing },
    { ai_charge },
    { ai_charge, 3 },
    { ai_charge }
};
const mmove_t MMOVE_T(mutant_move_jump) = { FRAME_attack01, FRAME_attack08, mutant_frames_jump, mutant_run };

void MONSTERINFO_ATTACK(mutant_jump)(edict_t *self)
{
    M_SetAnimation(self, &mutant_move_jump);
}

//
// CHECKATTACK
//

static bool mutant_check_melee(edict_t *self)
{
    return range_to(self, self->enemy) <= RANGE_MELEE && self->monsterinfo.melee_debounce_time <= level.time;
}

static bool mutant_check_jump(edict_t *self)
{
    vec3_t v;
    float  distance;

    // Paril: no harm in letting them jump down if you're below them
    // if (self->absmin[2] > (self->enemy->absmin[2] + 0.75 * self->enemy->size[2]))
    //  return false;

    // don't jump if there's no way we can reach standing height
    if (self->absmin[2] + 125 < self->enemy->absmin[2])
        return false;

    VectorSubtract(self->s.origin, self->enemy->s.origin, v);
    v[2] = 0;
    distance = VectorLength(v);

    // if we're not trying to avoid a melee, then don't jump
    if (distance < 100 && self->monsterinfo.melee_debounce_time <= level.time)
        return false;
    // only use it to close distance gaps
    if (distance > 265)
        return false;

    return self->monsterinfo.attack_finished < level.time && brandom();
}

bool MONSTERINFO_CHECKATTACK(mutant_checkattack)(edict_t *self)
{
    if (!self->enemy || self->enemy->health <= 0)
        return false;

    if (mutant_check_melee(self)) {
        self->monsterinfo.attack_state = AS_MELEE;
        return true;
    }

    if (!(self->spawnflags & SPAWNFLAG_MUTANT_NOJUMPING) && mutant_check_jump(self)) {
        self->monsterinfo.attack_state = AS_MISSILE;
        return true;
    }

    return false;
}

//
// PAIN
//

static const mframe_t mutant_frames_pain1[] = {
    { ai_move, 4 },
    { ai_move, -3 },
    { ai_move, -8 },
    { ai_move, 2 },
    { ai_move, 5 }
};
const mmove_t MMOVE_T(mutant_move_pain1) = { FRAME_pain101, FRAME_pain105, mutant_frames_pain1, mutant_run };

static const mframe_t mutant_frames_pain2[] = {
    { ai_move, -24 },
    { ai_move, 11 },
    { ai_move, 5 },
    { ai_move, -2 },
    { ai_move, 6 },
    { ai_move, 4 }
};
const mmove_t MMOVE_T(mutant_move_pain2) = { FRAME_pain201, FRAME_pain206, mutant_frames_pain2, mutant_run };

static const mframe_t mutant_frames_pain3[] = {
    { ai_move, -22 },
    { ai_move, 3 },
    { ai_move, 3 },
    { ai_move, 2 },
    { ai_move, 1 },
    { ai_move, 1 },
    { ai_move, 6 },
    { ai_move, 3 },
    { ai_move, 2 },
    { ai_move },
    { ai_move, 1 }
};
const mmove_t MMOVE_T(mutant_move_pain3) = { FRAME_pain301, FRAME_pain311, mutant_frames_pain3, mutant_run };

void PAIN(mutant_pain)(edict_t *self, edict_t *other, float kick, int damage, const mod_t mod)
{
    float r;

    if (level.time < self->pain_debounce_time)
        return;

    self->pain_debounce_time = level.time + SEC(3);

    r = frandom();
    if (r < 0.33f)
        gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
    else if (r < 0.66f)
        gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
    else
        gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);

    if (!M_ShouldReactToPain(self, mod))
        return; // no pain anims in nightmare

    if (r < 0.33f)
        M_SetAnimation(self, &mutant_move_pain1);
    else if (r < 0.66f)
        M_SetAnimation(self, &mutant_move_pain2);
    else
        M_SetAnimation(self, &mutant_move_pain3);
}

void MONSTERINFO_SETSKIN(mutant_setskin)(edict_t *self)
{
    if (self->health < (self->max_health / 2))
        self->s.skinnum = 1;
    else
        self->s.skinnum = 0;
}

//
// DEATH
//

static void mutant_shrink(edict_t *self)
{
    self->maxs[2] = 0;
    self->svflags |= SVF_DEADMONSTER;
    gi.linkentity(self);
}

// [Paril-KEX]
static void ai_move_slide_right(edict_t *self, float dist)
{
    M_walkmove(self, self->s.angles[YAW] + 90, dist);
}

static void ai_move_slide_left(edict_t *self, float dist)
{
    M_walkmove(self, self->s.angles[YAW] - 90, dist);
}

static const mframe_t mutant_frames_death1[] = {
    { ai_move_slide_right },
    { ai_move_slide_right },
    { ai_move_slide_right },
    { ai_move_slide_right, 2 },
    { ai_move_slide_right, 5 },
    { ai_move_slide_right, 7, mutant_shrink },
    { ai_move_slide_right, 6 },
    { ai_move_slide_right, 2 },
    { ai_move_slide_right }
};
const mmove_t MMOVE_T(mutant_move_death1) = { FRAME_death101, FRAME_death109, mutant_frames_death1, monster_dead };

static const mframe_t mutant_frames_death2[] = {
    { ai_move_slide_left },
    { ai_move_slide_left },
    { ai_move_slide_left },
    { ai_move_slide_left, 1 },
    { ai_move_slide_left, 3, mutant_shrink },
    { ai_move_slide_left, 6 },
    { ai_move_slide_left, 8 },
    { ai_move_slide_left, 5 },
    { ai_move_slide_left, 2 },
    { ai_move_slide_left }
};
const mmove_t MMOVE_T(mutant_move_death2) = { FRAME_death201, FRAME_death210, mutant_frames_death2, monster_dead };

static const gib_def_t mutant_gibs[] = {
    { "models/objects/gibs/bone/tris.md2", 2 },
    { "models/objects/gibs/sm_meat/tris.md2", 4 },
    { "models/monsters/mutant/gibs/hand.md2", 2, GIB_SKINNED | GIB_UPRIGHT },
    { "models/monsters/mutant/gibs/foot.md2", 2, GIB_SKINNED },
    { "models/monsters/mutant/gibs/chest.md2", 1, GIB_SKINNED },
    { "models/monsters/mutant/gibs/head.md2", 1, GIB_SKINNED | GIB_HEAD },
    { 0 }
};

void DIE(mutant_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, const mod_t mod)
{
    if (M_CheckGib(self, mod)) {
        gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
        self->s.skinnum /= 2;
        ThrowGibs(self, damage, mutant_gibs);
        self->deadflag = true;
        return;
    }

    if (self->deadflag)
        return;

    gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
    self->deadflag = true;
    self->takedamage = true;

    if (brandom())
        M_SetAnimation(self, &mutant_move_death1);
    else
        M_SetAnimation(self, &mutant_move_death2);
}

//================
// ROGUE
static void mutant_jump_down(edict_t *self)
{
    vec3_t forward, up;

    AngleVectors(self->s.angles, forward, NULL, up);
    VectorMA(self->velocity, 100, forward, self->velocity);
    VectorMA(self->velocity, 300, up, self->velocity);
}

static void mutant_jump_up(edict_t *self)
{
    vec3_t forward, up;

    AngleVectors(self->s.angles, forward, NULL, up);
    VectorMA(self->velocity, 200, forward, self->velocity);
    VectorMA(self->velocity, 450, up, self->velocity);
}

static void mutant_jump_wait_land(edict_t *self)
{
    if (!monster_jump_finished(self) && self->groundentity == NULL)
        self->monsterinfo.nextframe = self->s.frame;
    else
        self->monsterinfo.nextframe = self->s.frame + 1;
}

static const mframe_t mutant_frames_jump_up[] = {
    { ai_move, -8 },
    { ai_move, -8, mutant_jump_up },
    { ai_move, 0, mutant_jump_wait_land },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(mutant_move_jump_up) = { FRAME_jump01, FRAME_jump05, mutant_frames_jump_up, mutant_run };

static const mframe_t mutant_frames_jump_down[] = {
    { ai_move },
    { ai_move, 0, mutant_jump_down },
    { ai_move, 0, mutant_jump_wait_land },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(mutant_move_jump_down) = { FRAME_jump01, FRAME_jump05, mutant_frames_jump_down, mutant_run };

static void mutant_jump_updown(edict_t *self, blocked_jump_result_t result)
{
    if (!self->enemy)
        return;

    if (result == JUMP_JUMP_UP)
        M_SetAnimation(self, &mutant_move_jump_up);
    else
        M_SetAnimation(self, &mutant_move_jump_down);
}

/*
===
Blocked
===
*/
bool MONSTERINFO_BLOCKED(mutant_blocked)(edict_t *self, float dist)
{
    blocked_jump_result_t result = blocked_checkjump(self, dist);
    if (result != NO_JUMP) {
        if (result != JUMP_TURN)
            mutant_jump_updown(self, result);
        return true;
    }

    if (blocked_checkplat(self, dist))
        return true;

    return false;
}
// ROGUE
//================

//
// SPAWN
//

static void mutant_precache(void)
{
    sound_swing = gi.soundindex("mutant/mutatck1.wav");
    sound_hit = gi.soundindex("mutant/mutatck2.wav");
    sound_hit2 = gi.soundindex("mutant/mutatck3.wav");
    sound_death = gi.soundindex("mutant/mutdeth1.wav");
    sound_idle = gi.soundindex("mutant/mutidle1.wav");
    sound_pain1 = gi.soundindex("mutant/mutpain1.wav");
    sound_pain2 = gi.soundindex("mutant/mutpain2.wav");
    sound_sight = gi.soundindex("mutant/mutsght1.wav");
    sound_search = gi.soundindex("mutant/mutsrch1.wav");
    sound_step1 = gi.soundindex("mutant/step1.wav");
    sound_step2 = gi.soundindex("mutant/step2.wav");
    sound_step3 = gi.soundindex("mutant/step3.wav");
    sound_thud = gi.soundindex("mutant/thud1.wav");

}

/*QUAKED monster_mutant (1 .5 0) (-32 -32 -24) (32 32 32) Ambush Trigger_Spawn Sight NoJumping
model="models/monsters/mutant/tris.md2"
*/
void SP_monster_mutant(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    G_AddPrecache(mutant_precache);

    self->monsterinfo.aiflags |= AI_STINKY;

    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;
    self->s.modelindex = gi.modelindex("models/monsters/mutant/tris.md2");

    PrecacheGibs(mutant_gibs);

    VectorSet(self->mins, -18, -18, -24);
    VectorSet(self->maxs, 18, 18, 30);

    self->health = 300 * st.health_multiplier;
    self->gib_health = -120;
    self->mass = 300;

    self->pain = mutant_pain;
    self->die = mutant_die;

    self->monsterinfo.stand = mutant_stand;
    self->monsterinfo.walk = mutant_walk;
    self->monsterinfo.run = mutant_run;
    self->monsterinfo.dodge = NULL;
    self->monsterinfo.attack = mutant_jump;
    self->monsterinfo.melee = mutant_melee;
    self->monsterinfo.sight = mutant_sight;
    self->monsterinfo.search = mutant_search;
    self->monsterinfo.idle = mutant_idle;
    self->monsterinfo.checkattack = mutant_checkattack;
    self->monsterinfo.blocked = mutant_blocked; // PGM
    self->monsterinfo.setskin = mutant_setskin;

    gi.linkentity(self);

    M_SetAnimation(self, &mutant_move_stand);

    self->monsterinfo.combat_style = COMBAT_MELEE;

    self->monsterinfo.scale = MODEL_SCALE;
    self->monsterinfo.can_jump = !(self->spawnflags & SPAWNFLAG_MUTANT_NOJUMPING);
    self->monsterinfo.drop_height = 256;
    self->monsterinfo.jump_height = 68;

    walkmonster_start(self);
}
