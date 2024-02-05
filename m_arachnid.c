// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

TANK

==============================================================================
*/

#include "g_local.h"
#include "m_arachnid.h"

static int sound_step;
static int sound_charge;
static int sound_melee;
static int sound_melee_hit;
static int sound_pain;
static int sound_death;
static int sound_sight;

void MONSTERINFO_SIGHT(arachnid_sight)(edict_t *self, edict_t *other)
{
    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

//
// stand
//

static const mframe_t arachnid_frames_stand[] = {
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand }
};
const mmove_t MMOVE_T(arachnid_move_stand) = { FRAME_idle1, FRAME_idle13, arachnid_frames_stand, NULL };

void MONSTERINFO_STAND(arachnid_stand)(edict_t *self)
{
    M_SetAnimation(self, &arachnid_move_stand);
}

//
// walk
//

static void arachnid_footstep(edict_t *self)
{
    gi.sound(self, CHAN_BODY, sound_step, 0.5f, ATTN_IDLE, 0.0f);
}

static const mframe_t arachnid_frames_walk[] = {
    { ai_walk, 8, arachnid_footstep },
    { ai_walk, 8 },
    { ai_walk, 8 },
    { ai_walk, 8 },
    { ai_walk, 8 },
    { ai_walk, 8, arachnid_footstep },
    { ai_walk, 8 },
    { ai_walk, 8 },
    { ai_walk, 8 },
    { ai_walk, 8 }
};
const mmove_t MMOVE_T(arachnid_move_walk) = { FRAME_walk1, FRAME_walk10, arachnid_frames_walk, NULL };

void MONSTERINFO_WALK(arachnid_walk)(edict_t *self)
{
    M_SetAnimation(self, &arachnid_move_walk);
}

//
// run
//

static const mframe_t arachnid_frames_run[] = {
    { ai_run, 8, arachnid_footstep },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8, arachnid_footstep },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 }
};
const mmove_t MMOVE_T(arachnid_move_run) = { FRAME_walk1, FRAME_walk10, arachnid_frames_run, NULL };

void MONSTERINFO_RUN(arachnid_run)(edict_t *self)
{
    if (self->monsterinfo.aiflags & AI_STAND_GROUND) {
        M_SetAnimation(self, &arachnid_move_stand);
        return;
    }

    M_SetAnimation(self, &arachnid_move_run);
}

//
// pain
//

static const mframe_t arachnid_frames_pain1[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(arachnid_move_pain1) = { FRAME_pain11, FRAME_pain15, arachnid_frames_pain1, arachnid_run };

static const mframe_t arachnid_frames_pain2[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(arachnid_move_pain2) = { FRAME_pain21, FRAME_pain26, arachnid_frames_pain2, arachnid_run };

void PAIN(arachnid_pain)(edict_t *self, edict_t *other, float kick, int damage, mod_t mod)
{
    if (level.time < self->pain_debounce_time)
        return;

    self->pain_debounce_time = level.time + SEC(3);
    gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

    if (!M_ShouldReactToPain(self, mod))
        return; // no pain anims in nightmare

    if (brandom())
        M_SetAnimation(self, &arachnid_move_pain1);
    else
        M_SetAnimation(self, &arachnid_move_pain2);
}

static void arachnid_charge_rail(edict_t *self)
{
    if (!self->enemy || !self->enemy->inuse)
        return;

    gi.sound(self, CHAN_WEAPON, sound_charge, 1, ATTN_NORM, 0);
    VectorCopy(self->enemy->s.origin, self->pos1);
    self->pos1[2] += self->enemy->viewheight;
}

static void arachnid_rail(edict_t *self)
{
    vec3_t start;
    vec3_t dir;
    vec3_t forward, right;
    monster_muzzleflash_id_t id;

    switch (self->s.frame) {
    case FRAME_rails4:
    default:
        id = MZ2_ARACHNID_RAIL1;
        break;
    case FRAME_rails8:
        id = MZ2_ARACHNID_RAIL2;
        break;
    case FRAME_rails_up7:
        id = MZ2_ARACHNID_RAIL_UP1;
        break;
    case FRAME_rails_up11:
        id = MZ2_ARACHNID_RAIL_UP2;
        break;
    }

    AngleVectors(self->s.angles, forward, right, NULL);
    M_ProjectFlashSource(self, monster_flash_offset[id], forward, right, start);

    // calc direction to where we targeted
    VectorSubtract(self->pos1, start, dir);
    VectorNormalize(dir);

    monster_fire_railgun(self, start, dir, 35, 100, id);
}

static const mframe_t arachnid_frames_attack1[] = {
    { ai_charge, 0, arachnid_charge_rail },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, arachnid_rail },
    { ai_charge, 0, arachnid_charge_rail },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, arachnid_rail },
    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(arachnid_attack1) = { FRAME_rails1, FRAME_rails11, arachnid_frames_attack1, arachnid_run };

static const mframe_t arachnid_frames_attack_up1[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, arachnid_charge_rail },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, arachnid_rail },
    { ai_charge, 0, arachnid_charge_rail },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, arachnid_rail },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
};
const mmove_t MMOVE_T(arachnid_attack_up1) = { FRAME_rails_up1, FRAME_rails_up16, arachnid_frames_attack_up1, arachnid_run };

static void arachnid_melee_charge(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_melee, 1, ATTN_NORM, 0);
}

static void arachnid_melee_hit(edict_t *self)
{
    if (!fire_hit(self, (vec3_t){ MELEE_DISTANCE, 0, 0 }, 15, 50))
        self->monsterinfo.melee_debounce_time = level.time + SEC(1);
}

static const mframe_t arachnid_frames_melee[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, arachnid_melee_charge },
    { ai_charge },
    { ai_charge, 0, arachnid_melee_hit },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, arachnid_melee_charge },
    { ai_charge },
    { ai_charge, 0, arachnid_melee_hit },
    { ai_charge }
};
const mmove_t MMOVE_T(arachnid_melee) = { FRAME_melee_atk1, FRAME_melee_atk12, arachnid_frames_melee, arachnid_run };

void MONSTERINFO_ATTACK(arachnid_attack)(edict_t *self)
{
    if (!self->enemy || !self->enemy->inuse)
        return;

    if (self->monsterinfo.melee_debounce_time < level.time && range_to(self, self->enemy) < MELEE_DISTANCE)
        M_SetAnimation(self, &arachnid_melee);
    else if ((self->enemy->s.origin[2] - self->s.origin[2]) > 150)
        M_SetAnimation(self, &arachnid_attack_up1);
    else
        M_SetAnimation(self, &arachnid_attack1);
}

//
// death
//

static void arachnid_dead(edict_t *self)
{
    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, -8);
    self->movetype = MOVETYPE_TOSS;
    self->svflags |= SVF_DEADMONSTER;
    self->nextthink = 0;
    gi.linkentity(self);
}

static const mframe_t arachnid_frames_death1[] = {
    { ai_move, 0 },
    { ai_move, -1.23f },
    { ai_move, -1.23f },
    { ai_move, -1.23f },
    { ai_move, -1.23f },
    { ai_move, -1.64f },
    { ai_move, -1.64f },
    { ai_move, -2.45f },
    { ai_move, -8.63f },
    { ai_move, -4.0f },
    { ai_move, -4.5f },
    { ai_move, -6.8f },
    { ai_move, -8.0f },
    { ai_move, -5.4f },
    { ai_move, -3.4f },
    { ai_move, -1.9f },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(arachnid_move_death) = { FRAME_death1, FRAME_death20, arachnid_frames_death1, arachnid_dead };

static const gib_def_t arachnid_gibs[] = {
    { "models/objects/gibs/bone/tris.md2", 2 },
    { "models/objects/gibs/sm_meat/tris.md2", 4 },
    { "models/objects/gibs/head2/tris.md2", 1, GIB_HEAD },
    { 0 }
};

void DIE(arachnid_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    // check for gib
    if (M_CheckGib(self, mod)) {
        gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
        ThrowGibs(self, damage, arachnid_gibs);
        self->deadflag = true;
        return;
    }

    if (self->deadflag)
        return;

    // regular death
    gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
    self->deadflag = true;
    self->takedamage = true;

    M_SetAnimation(self, &arachnid_move_death);
}

//
// monster_arachnid
//

static void arachnid_precache(void)
{
    sound_step      = gi.soundindex("insane/insane11.wav");
    sound_charge    = gi.soundindex("gladiator/railgun.wav");
    sound_melee     = gi.soundindex("gladiator/melee3.wav");
    sound_melee_hit = gi.soundindex("gladiator/melee2.wav");
    sound_pain      = gi.soundindex("arachnid/pain.wav");
    sound_death     = gi.soundindex("arachnid/death.wav");
    sound_sight     = gi.soundindex("arachnid/sight.wav");
}

/*QUAKED monster_arachnid (1 .5 0) (-48 -48 -20) (48 48 48) Ambush Trigger_Spawn Sight
 */
void SP_monster_arachnid(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    G_AddPrecache(arachnid_precache);

    self->s.modelindex = gi.modelindex("models/monsters/arachnid/tris.md2");
    VectorSet(self->mins, -48, -48, -20);
    VectorSet(self->maxs, 48, 48, 48);
    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;

    self->health = 1000 * st.health_multiplier;
    self->gib_health = -200;

    self->monsterinfo.scale = MODEL_SCALE;

    self->mass = 450;

    self->pain = arachnid_pain;
    self->die = arachnid_die;
    self->monsterinfo.stand = arachnid_stand;
    self->monsterinfo.walk = arachnid_walk;
    self->monsterinfo.run = arachnid_run;
    self->monsterinfo.attack = arachnid_attack;
    self->monsterinfo.sight = arachnid_sight;

    gi.linkentity(self);

    M_SetAnimation(self, &arachnid_move_stand);

    walkmonster_start(self);
}
