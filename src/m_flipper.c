// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

FLIPPER

==============================================================================
*/

#include "g_local.h"
#include "m_flipper.h"

static int sound_chomp;
static int sound_attack;
static int sound_pain1;
static int sound_pain2;
static int sound_death;
static int sound_idle;
static int sound_search;
static int sound_sight;

static const mframe_t flipper_frames_stand[] = {
    { ai_stand }
};

const mmove_t MMOVE_T(flipper_move_stand) = { FRAME_flphor01, FRAME_flphor01, flipper_frames_stand, NULL };

void MONSTERINFO_STAND(flipper_stand)(edict_t *self)
{
    M_SetAnimation(self, &flipper_move_stand);
}

#define FLIPPER_RUN_SPEED   24

static const mframe_t flipper_frames_run[] = {
    { ai_run, FLIPPER_RUN_SPEED }, // 6
    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED }, // 10

    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED }, // 20

    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED },
    { ai_run, FLIPPER_RUN_SPEED } // 29
};
const mmove_t MMOVE_T(flipper_move_run_loop) = { FRAME_flpver06, FRAME_flpver29, flipper_frames_run, NULL };

static void flipper_run_loop(edict_t *self)
{
    M_SetAnimation(self, &flipper_move_run_loop);
}

static const mframe_t flipper_frames_run_start[] = {
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 }
};
const mmove_t MMOVE_T(flipper_move_run_start) = { FRAME_flpver01, FRAME_flpver06, flipper_frames_run_start, flipper_run_loop };

static void flipper_run(edict_t *self)
{
    M_SetAnimation(self, &flipper_move_run_start);
}

/* Standard Swimming */
static const mframe_t flipper_frames_walk[] = {
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 }
};
const mmove_t MMOVE_T(flipper_move_walk) = { FRAME_flphor01, FRAME_flphor24, flipper_frames_walk, NULL };

void MONSTERINFO_WALK(flipper_walk)(edict_t *self)
{
    M_SetAnimation(self, &flipper_move_walk);
}

static const mframe_t flipper_frames_start_run[] = {
    { ai_run },
    { ai_run },
    { ai_run },
    { ai_run },
    { ai_run, 8, flipper_run }
};
const mmove_t MMOVE_T(flipper_move_start_run) = { FRAME_flphor01, FRAME_flphor05, flipper_frames_start_run, NULL };

void MONSTERINFO_RUN(flipper_start_run)(edict_t *self)
{
    M_SetAnimation(self, &flipper_move_start_run);
}

static const mframe_t flipper_frames_pain2[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(flipper_move_pain2) = { FRAME_flppn101, FRAME_flppn105, flipper_frames_pain2, flipper_run };

static const mframe_t flipper_frames_pain1[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(flipper_move_pain1) = { FRAME_flppn201, FRAME_flppn205, flipper_frames_pain1, flipper_run };

static void flipper_bite(edict_t *self)
{
    vec3_t aim = { MELEE_DISTANCE, 0, 0 };
    fire_hit(self, aim, 5, 0);
}

static void flipper_preattack(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_chomp, 1, ATTN_NORM, 0);
}

static const mframe_t flipper_frames_attack[] = {
    { ai_charge, 0, flipper_preattack },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, flipper_bite },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, flipper_bite },
    { ai_charge }
};
const mmove_t MMOVE_T(flipper_move_attack) = { FRAME_flpbit01, FRAME_flpbit20, flipper_frames_attack, flipper_run };

void MONSTERINFO_MELEE(flipper_melee)(edict_t *self)
{
    M_SetAnimation(self, &flipper_move_attack);
}

void PAIN(flipper_pain)(edict_t *self, edict_t *other, float kick, int damage, mod_t mod)
{
    int n;

    if (level.time < self->pain_debounce_time)
        return;

    self->pain_debounce_time = level.time + SEC(3);

    n = brandom();
    if (n == 0)
        gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
    else
        gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

    if (!M_ShouldReactToPain(self, mod))
        return; // no pain anims in nightmare

    if (n == 0)
        M_SetAnimation(self, &flipper_move_pain1);
    else
        M_SetAnimation(self, &flipper_move_pain2);
}

void MONSTERINFO_SETSKIN(flipper_setskin)(edict_t *self)
{
    if (self->health < (self->max_health / 2))
        self->s.skinnum = 1;
    else
        self->s.skinnum = 0;
}

static void flipper_dead(edict_t *self)
{
    VectorSet(self->mins, -16, -16, -8);
    VectorSet(self->maxs, 16, 16, 8);
    monster_dead(self);
}

static const mframe_t flipper_frames_death[] = {
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
    { ai_move }
};
const mmove_t MMOVE_T(flipper_move_death) = { FRAME_flpdth01, FRAME_flpdth56, flipper_frames_death, flipper_dead };

void MONSTERINFO_SIGHT(flipper_sight)(edict_t *self, edict_t *other)
{
    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static const gib_def_t flipper_gibs[] = {
    { "models/objects/gibs/bone/tris.md2", 2 },
    { "models/objects/gibs/sm_meat/tris.md2", 2 },
    { "models/objects/gibs/head2/tris.md2", 1, GIB_HEAD },
    { 0 }
};

void DIE(flipper_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    // check for gib
    if (M_CheckGib(self, mod)) {
        gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
        ThrowGibs(self, damage, flipper_gibs);
        self->deadflag = true;
        return;
    }

    if (self->deadflag)
        return;

    // regular death
    gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
    self->deadflag = true;
    self->takedamage = true;
    self->svflags |= SVF_DEADMONSTER;
    M_SetAnimation(self, &flipper_move_death);
}

static void flipper_set_fly_parameters(edict_t *self)
{
    self->monsterinfo.fly_thrusters = false;
    self->monsterinfo.fly_acceleration = 30;
    self->monsterinfo.fly_speed = 110;
    // only melee, so get in close
    self->monsterinfo.fly_min_distance = 10;
    self->monsterinfo.fly_max_distance = 10;
}

static void flipper_precache(void)
{
    sound_pain1 = gi.soundindex("flipper/flppain1.wav");
    sound_pain2 = gi.soundindex("flipper/flppain2.wav");
    sound_death = gi.soundindex("flipper/flpdeth1.wav");
    sound_chomp = gi.soundindex("flipper/flpatck1.wav");
    sound_attack = gi.soundindex("flipper/flpatck2.wav");
    sound_idle = gi.soundindex("flipper/flpidle1.wav");
    sound_search = gi.soundindex("flipper/flpsrch1.wav");
    sound_sight = gi.soundindex("flipper/flpsght1.wav");
}

/*QUAKED monster_flipper (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
 */
void SP_monster_flipper(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    G_AddPrecache(flipper_precache);

    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;
    self->s.modelindex = gi.modelindex("models/monsters/flipper/tris.md2");
    VectorSet(self->mins, -16, -16, -8);
    VectorSet(self->maxs, 16, 16, 20);

    self->health = 50 * st.health_multiplier;
    self->gib_health = -30;
    self->mass = 100;

    self->pain = flipper_pain;
    self->die = flipper_die;

    self->monsterinfo.stand = flipper_stand;
    self->monsterinfo.walk = flipper_walk;
    self->monsterinfo.run = flipper_start_run;
    self->monsterinfo.melee = flipper_melee;
    self->monsterinfo.sight = flipper_sight;
    self->monsterinfo.setskin = flipper_setskin;

    gi.linkentity(self);

    M_SetAnimation(self, &flipper_move_stand);
    self->monsterinfo.scale = MODEL_SCALE;

    self->monsterinfo.aiflags |= AI_ALTERNATE_FLY;
    flipper_set_fly_parameters(self);

    swimmonster_start(self);
}
