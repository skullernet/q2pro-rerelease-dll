// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

GUNNER

==============================================================================
*/

#include "g_local.h"
#include "m_gunner.h"

#define SPAWNFLAG_GUNCMDR_NOJUMPING 8

static int sound_pain;
static int sound_pain2;
static int sound_death;
static int sound_idle;
static int sound_open;
static int sound_search;
static int sound_sight;

static void guncmdr_idlesound(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

void MONSTERINFO_SIGHT(guncmdr_sight)(edict_t *self, edict_t *other)
{
    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void MONSTERINFO_SEARCH(guncmdr_search)(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

static void guncmdr_fire_chain(edict_t *self);
static void guncmdr_refire_chain(edict_t *self);

void guncmdr_stand(edict_t *self);

static const mframe_t guncmdr_frames_fidget[] = {
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand, 0, guncmdr_idlesound },
    { ai_stand },
    { ai_stand },

    { ai_stand },
    { ai_stand, 0, guncmdr_idlesound },
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
    { ai_stand },
    { ai_stand },

    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand }
};
const mmove_t MMOVE_T(guncmdr_move_fidget) = { FRAME_c_stand201, FRAME_c_stand254, guncmdr_frames_fidget, guncmdr_stand };

static void guncmdr_fidget(edict_t *self)
{
    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        return;
    if (self->enemy)
        return;
    if (frandom() <= 0.05f)
        M_SetAnimation(self, &guncmdr_move_fidget);
}

static const mframe_t guncmdr_frames_stand[] = {
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand, 0, guncmdr_fidget },

    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand, 0, guncmdr_fidget },

    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand, 0, guncmdr_fidget },

    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand, 0, guncmdr_fidget }
};
const mmove_t MMOVE_T(guncmdr_move_stand) = { FRAME_c_stand101, FRAME_c_stand140, guncmdr_frames_stand, NULL };

void MONSTERINFO_STAND(guncmdr_stand)(edict_t *self)
{
    M_SetAnimation(self, &guncmdr_move_stand);
}

static const mframe_t guncmdr_frames_walk[] = {
    { ai_walk, 1.5f, monster_footstep },
    { ai_walk, 2.5f },
    { ai_walk, 3.0f },
    { ai_walk, 2.5f },
    { ai_walk, 2.3f },
    { ai_walk, 3.0f },
    { ai_walk, 2.8f, monster_footstep },
    { ai_walk, 3.6f },
    { ai_walk, 2.8f },
    { ai_walk, 2.5f },

    { ai_walk, 2.3f },
    { ai_walk, 4.3f },
    { ai_walk, 3.0f, monster_footstep },
    { ai_walk, 1.5f },
    { ai_walk, 2.5f },
    { ai_walk, 3.3f },
    { ai_walk, 2.8f },
    { ai_walk, 3.0f },
    { ai_walk, 2.0f, monster_footstep },
    { ai_walk, 2.0f },

    { ai_walk, 3.3f },
    { ai_walk, 3.6f },
    { ai_walk, 3.4f },
    { ai_walk, 2.8f },
};
const mmove_t MMOVE_T(guncmdr_move_walk) = { FRAME_c_walk101, FRAME_c_walk124, guncmdr_frames_walk, NULL };

void MONSTERINFO_WALK(guncmdr_walk)(edict_t *self)
{
    M_SetAnimation(self, &guncmdr_move_walk);
}

static const mframe_t guncmdr_frames_run[] = {
    { ai_run, 15, monster_done_dodge },
    { ai_run, 16, monster_footstep },
    { ai_run, 20 },
    { ai_run, 18 },
    { ai_run, 24, monster_footstep },
    { ai_run, 13.5f }
};

const mmove_t MMOVE_T(guncmdr_move_run) = { FRAME_c_run101, FRAME_c_run106, guncmdr_frames_run, NULL };

void MONSTERINFO_RUN(guncmdr_run)(edict_t *self)
{
    monster_done_dodge(self);
    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        M_SetAnimation(self, &guncmdr_move_stand);
    else
        M_SetAnimation(self, &guncmdr_move_run);
}

// standing pains

static const mframe_t guncmdr_frames_pain1[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
};
const mmove_t MMOVE_T(guncmdr_move_pain1) = { FRAME_c_pain101, FRAME_c_pain104, guncmdr_frames_pain1, guncmdr_run };

static const mframe_t guncmdr_frames_pain2[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(guncmdr_move_pain2) = { FRAME_c_pain201, FRAME_c_pain204, guncmdr_frames_pain2, guncmdr_run };

static const mframe_t guncmdr_frames_pain3[] = {
    { ai_move, -3.0f },
    { ai_move },
    { ai_move },
    { ai_move },
};
const mmove_t MMOVE_T(guncmdr_move_pain3) = { FRAME_c_pain301, FRAME_c_pain304, guncmdr_frames_pain3, guncmdr_run };

static const mframe_t guncmdr_frames_pain4[] = {
    { ai_move, -17.1f },
    { ai_move, -3.2f },
    { ai_move, 0.9f },
    { ai_move, 3.6f },
    { ai_move, -2.6f },
    { ai_move, 1.0f },
    { ai_move, -5.1f },
    { ai_move, -6.7f },
    { ai_move, -8.8f },
    { ai_move },

    { ai_move },
    { ai_move, -2.1f },
    { ai_move, -2.3f },
    { ai_move, -2.5f },
    { ai_move }
};
const mmove_t MMOVE_T(guncmdr_move_pain4) = { FRAME_c_pain401, FRAME_c_pain415, guncmdr_frames_pain4, guncmdr_run };

static void guncmdr_dead(edict_t *self);

static const mframe_t guncmdr_frames_death1[] = {
    { ai_move },
    { ai_move },
    { ai_move, 4.0f }, // scoot
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
const mmove_t MMOVE_T(guncmdr_move_death1) = { FRAME_c_death101, FRAME_c_death118, guncmdr_frames_death1, guncmdr_dead };

static void guncmdr_pain5_to_death1(edict_t *self)
{
    if (self->health < 0)
        M_SetAnimationEx(self, &guncmdr_move_death1, false);
}

static const mframe_t guncmdr_frames_death2[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(guncmdr_move_death2) = { FRAME_c_death201, FRAME_c_death204, guncmdr_frames_death2, guncmdr_dead };

static void guncmdr_pain5_to_death2(edict_t *self)
{
    if (self->health < 0 && brandom())
        M_SetAnimationEx(self, &guncmdr_move_death2, false);
}

static const mframe_t guncmdr_frames_pain5[] = {
    { ai_move, -29 },
    { ai_move, -5 },
    { ai_move, -5 },
    { ai_move, -3 },
    { ai_move },
    { ai_move, 0, guncmdr_pain5_to_death2 },
    { ai_move, 9 },
    { ai_move, 3 },
    { ai_move, 0, guncmdr_pain5_to_death1 },
    { ai_move },

    { ai_move },
    { ai_move, -4.6f },
    { ai_move, -4.8f },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 9.5f },
    { ai_move, 3.4f },
    { ai_move },
    { ai_move },

    { ai_move, -2.4f },
    { ai_move, -9.0f },
    { ai_move, -5.0f },
    { ai_move, -3.6f },
};
const mmove_t MMOVE_T(guncmdr_move_pain5) = { FRAME_c_pain501, FRAME_c_pain524, guncmdr_frames_pain5, guncmdr_run };

static void guncmdr_dead(edict_t *self)
{
    static const vec3_t mins = { -16, -16, -24 };
    static const vec3_t maxs = { 16, 16, -8 };

    VectorScale(mins, self->x.scale, self->mins);
    VectorScale(maxs, self->x.scale, self->maxs);

    monster_dead(self);
}

static void guncmdr_shrink(edict_t *self)
{
    self->maxs[2] = -4 * self->x.scale;
    self->svflags |= SVF_DEADMONSTER;
    gi.linkentity(self);
}

static const mframe_t guncmdr_frames_death6[] = {
    { ai_move, 0, guncmdr_shrink },
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
const mmove_t MMOVE_T(guncmdr_move_death6) = { FRAME_c_death601, FRAME_c_death614, guncmdr_frames_death6, guncmdr_dead };

static void guncmdr_pain6_to_death6(edict_t *self)
{
    if (self->health < 0)
        M_SetAnimationEx(self, &guncmdr_move_death6, false);
}

static const mframe_t guncmdr_frames_pain6[] = {
    { ai_move, 16 },
    { ai_move, 16 },
    { ai_move, 12 },
    { ai_move, 5.5f, monster_duck_down },
    { ai_move, 3.0f },
    { ai_move, -4.7f },
    { ai_move, -6.0f, guncmdr_pain6_to_death6 },
    { ai_move },
    { ai_move, 1.8f },
    { ai_move, 0.7f },

    { ai_move },
    { ai_move, -2.1f },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },

    { ai_move },
    { ai_move, -6.1f },
    { ai_move, 10.5f },
    { ai_move, 4.3f },
    { ai_move, 4.7f, monster_duck_up },
    { ai_move, 1.4f },
    { ai_move },
    { ai_move, -3.2f },
    { ai_move, 2.3f },
    { ai_move, -4.4f },

    { ai_move, -4.4f },
    { ai_move, -2.4f }
};
const mmove_t MMOVE_T(guncmdr_move_pain6) = { FRAME_c_pain601, FRAME_c_pain632, guncmdr_frames_pain6, guncmdr_run };

static const mframe_t guncmdr_frames_pain7[] = {
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
const mmove_t MMOVE_T(guncmdr_move_pain7) = { FRAME_c_pain701, FRAME_c_pain714, guncmdr_frames_pain7, guncmdr_run };

const mmove_t guncmdr_move_jump;
const mmove_t guncmdr_move_jump2;
const mmove_t guncmdr_move_duck_attack;

void PAIN(guncmdr_pain)(edict_t *self, edict_t *other, float kick, int damage, const mod_t mod)
{
    monster_done_dodge(self);

    if (self->monsterinfo.active_move == &guncmdr_move_jump ||
        self->monsterinfo.active_move == &guncmdr_move_jump2 ||
        self->monsterinfo.active_move == &guncmdr_move_duck_attack)
        return;

    if (level.time < self->pain_debounce_time) {
        if (frandom() < 0.3f)
            self->monsterinfo.dodge(self, other, FRAME_TIME, NULL, false);
        return;
    }

    self->pain_debounce_time = level.time + SEC(3);

    if (brandom())
        gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
    else
        gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

    if (!M_ShouldReactToPain(self, mod)) {
        if (frandom() < 0.3f)
            self->monsterinfo.dodge(self, other, FRAME_TIME, NULL, false);
        return; // no pain anims in nightmare
    }

    vec3_t forward;
    AngleVectors(self->s.angles, forward, NULL, NULL);

    vec3_t dif;
    VectorSubtract(other->s.origin, self->s.origin, dif);
    dif[2] = 0;
    VectorNormalize(dif);

    // small pain
    if (damage < 35) {
        int r = irandom1(4);

        if (r == 0)
            M_SetAnimation(self, &guncmdr_move_pain3);
        else if (r == 1)
            M_SetAnimation(self, &guncmdr_move_pain2);
        else if (r == 2)
            M_SetAnimation(self, &guncmdr_move_pain1);
        else
            M_SetAnimation(self, &guncmdr_move_pain7);
    // large pain from behind (aka Paril)
    } else if (DotProduct(dif, forward) < -0.40f) {
        M_SetAnimation(self, &guncmdr_move_pain6);

        self->pain_debounce_time += SEC(1.5f);
    } else {
        if (brandom())
            M_SetAnimation(self, &guncmdr_move_pain4);
        else
            M_SetAnimation(self, &guncmdr_move_pain5);

        self->pain_debounce_time += SEC(1.5f);
    }

    self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;

    // PMM - clear duck flag
    if (self->monsterinfo.aiflags & AI_DUCKED)
        monster_duck_up(self);
}

void MONSTERINFO_SETSKIN(guncmdr_setskin)(edict_t *self)
{
    if (self->health < (self->max_health / 2))
        self->s.skinnum |= 1;
    else
        self->s.skinnum &= ~1;
}

static void guncmdr_shrink2(edict_t *self)
{
    monster_footstep(self);
    guncmdr_shrink(self);
}

static const mframe_t guncmdr_frames_death3[] = {
    { ai_move, 20 },
    { ai_move, 10 },
    { ai_move, 10, guncmdr_shrink2 },
    { ai_move, 0, monster_footstep },
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
const mmove_t MMOVE_T(guncmdr_move_death3) = { FRAME_c_death301, FRAME_c_death321, guncmdr_frames_death3, guncmdr_dead };

static const mframe_t guncmdr_frames_death7[] = {
    { ai_move, 30 },
    { ai_move, 20 },
    { ai_move, 16, guncmdr_shrink2 },
    { ai_move, 5, monster_footstep },
    { ai_move, -6 },
    { ai_move, -7, monster_footstep },
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
    { ai_move, 0, monster_footstep },
    { ai_move },
    { ai_move },
    { ai_move, 0, monster_footstep },
    { ai_move },
    { ai_move },
};
const mmove_t MMOVE_T(guncmdr_move_death7) = { FRAME_c_death701, FRAME_c_death730, guncmdr_frames_death7, guncmdr_dead };

static const mframe_t guncmdr_frames_death4[] = {
    { ai_move, -20 },
    { ai_move, -16 },
    { ai_move, -26, guncmdr_shrink2 },
    { ai_move, 0, monster_footstep },
    { ai_move, -12 },
    { ai_move, 16 },
    { ai_move, 9.2f },
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
const mmove_t MMOVE_T(guncmdr_move_death4) = { FRAME_c_death401, FRAME_c_death436, guncmdr_frames_death4, guncmdr_dead };

static const mframe_t guncmdr_frames_death5[] = {
    { ai_move, -14.0f },
    { ai_move, -2.7f },
    { ai_move, -2.5f },
    { ai_move, -4.6f, monster_footstep },
    { ai_move, -4.0f, monster_footstep },
    { ai_move, -1.5f },
    { ai_move, 2.3f },
    { ai_move, 2.5f },
    { ai_move },
    { ai_move },

    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 3.5f },
    { ai_move, 12.9f, monster_footstep },
    { ai_move, 3.8f },
    { ai_move },
    { ai_move },
    { ai_move },

    { ai_move, -2.1f },
    { ai_move, -1.3f },
    { ai_move },
    { ai_move },
    { ai_move, 3.4f },
    { ai_move, 5.7f },
    { ai_move, 11.2f },
    { ai_move, 0, monster_footstep }
};
const mmove_t MMOVE_T(guncmdr_move_death5) = { FRAME_c_death501, FRAME_c_death528, guncmdr_frames_death5, guncmdr_dead };

static const gib_def_t guncmdr_gibs[] = {
    { "models/objects/gibs/bone/tris.md2", 2 },
    { "models/objects/gibs/sm_meat/tris.md2", 2 },
    { "models/objects/gibs/gear/tris.md2", 1 },
    { "models/monsters/gunner/gibs/chest.md2", 1, GIB_SKINNED },
    { "models/monsters/gunner/gibs/garm.md2", 1, GIB_SKINNED | GIB_UPRIGHT },
    { "models/monsters/gunner/gibs/gun.md2", 1, GIB_SKINNED | GIB_UPRIGHT },
    { "models/monsters/gunner/gibs/foot.md2", 1, GIB_SKINNED },
    { "models/monsters/gunner/gibs/head.md2" }, // precache only
    { 0 }
};

void DIE(guncmdr_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, const mod_t mod)
{
    // check for gib
    if (M_CheckGib(self, mod)) {
        gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

        self->s.skinnum /= 2;

        ThrowGibs(self, damage, guncmdr_gibs);

        if (self->monsterinfo.active_move != &guncmdr_move_death5)
            ThrowGib(self, "models/monsters/gunner/gibs/head.md2", damage, GIB_SKINNED | GIB_HEAD, self->x.scale);
        else
            ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_SKINNED | GIB_HEAD, self->x.scale);

        self->deadflag = true;
        return;
    }

    if (self->deadflag)
        return;

    // regular death
    gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
    self->deadflag = true;
    self->takedamage = true;

    // these animations cleanly transitions to death, so just keep going
    if (self->monsterinfo.active_move == &guncmdr_move_pain5 && self->s.frame < FRAME_c_pain508)
        return;
    if (self->monsterinfo.active_move == &guncmdr_move_pain6 && self->s.frame < FRAME_c_pain607)
        return;

    vec3_t forward;
    AngleVectors(self->s.angles, forward, NULL, NULL);

    vec3_t dif;
    VectorSubtract(inflictor->s.origin, self->s.origin, dif);
    dif[2] = 0;
    VectorNormalize(dif);

    // off with da head
    if (fabsf((self->s.origin[2] + self->viewheight) - point[2]) <= 4 && self->velocity[2] < 65) {
        M_SetAnimation(self, &guncmdr_move_death5);

        edict_t *head = ThrowGib(self, "models/monsters/gunner/gibs/head.md2", damage, GIB_NONE, self->x.scale);
        if (head) {
            VectorCopy(self->s.angles, head->s.angles);
            VectorCopy(self->s.origin, head->s.origin);
            head->s.origin[2] += 24;
            vec3_t dir;
            VectorSubtract(self->s.origin, inflictor->s.origin, dir);
            VectorNormalize(dir);
            VectorScale(dir, 100, head->velocity);
            head->velocity[2] = 200;
            VectorScale(head->avelocity, 0.15f, head->avelocity);
            gi.linkentity(head);
        }
    // damage came from behind; use backwards death
    } else if (DotProduct(dif, forward) < -0.4f) {
        int r = irandom1(self->monsterinfo.active_move == &guncmdr_move_pain6 ? 2 : 3);

        if (r == 0)
            M_SetAnimation(self, &guncmdr_move_death3);
        else if (r == 1)
            M_SetAnimation(self, &guncmdr_move_death7);
        else if (r == 2)
            M_SetAnimation(self, &guncmdr_move_pain6);
    } else {
        int r = irandom1(self->monsterinfo.active_move == &guncmdr_move_pain5 ? 1 : 2);

        if (r == 0)
            M_SetAnimation(self, &guncmdr_move_death4);
        else
            M_SetAnimation(self, &guncmdr_move_pain5);
    }
}

static void guncmdr_opengun(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_open, 1, ATTN_IDLE, 0);
}

static void GunnerCmdrFire(edict_t *self)
{
    vec3_t                   start;
    vec3_t                   forward, right;
    vec3_t                   aim;
    monster_muzzleflash_id_t flash_number;

    if (!self->enemy || !self->enemy->inuse) // PGM
        return;                              // PGM

    if (self->s.frame >= FRAME_c_attack401 && self->s.frame <= FRAME_c_attack505)
        flash_number = MZ2_GUNCMDR_CHAINGUN_2;
    else
        flash_number = MZ2_GUNCMDR_CHAINGUN_1;

    AngleVectors(self->s.angles, forward, right, NULL);
    M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right, start);
    PredictAim(self, self->enemy, start, 800, false, frandom() * 0.3f, aim, NULL);
    for (int i = 0; i < 3; i++)
        aim[i] += crandom_open() * 0.025f;
    monster_fire_flechette(self, start, aim, 4, 800, flash_number);
}

static const mframe_t guncmdr_frames_attack_chain[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, guncmdr_opengun },
    { ai_charge }
};
const mmove_t MMOVE_T(guncmdr_move_attack_chain) = { FRAME_c_attack101, FRAME_c_attack106, guncmdr_frames_attack_chain, guncmdr_fire_chain };

static const mframe_t guncmdr_frames_fire_chain[] = {
    { ai_charge, 0, GunnerCmdrFire },
    { ai_charge, 0, GunnerCmdrFire },
    { ai_charge, 0, GunnerCmdrFire },
    { ai_charge, 0, GunnerCmdrFire },
    { ai_charge, 0, GunnerCmdrFire },
    { ai_charge, 0, GunnerCmdrFire }
};
const mmove_t MMOVE_T(guncmdr_move_fire_chain) = { FRAME_c_attack107, FRAME_c_attack112, guncmdr_frames_fire_chain, guncmdr_refire_chain };

static const mframe_t guncmdr_frames_fire_chain_run[] = {
    { ai_charge, 15, GunnerCmdrFire },
    { ai_charge, 16, GunnerCmdrFire },
    { ai_charge, 20, GunnerCmdrFire },
    { ai_charge, 18, GunnerCmdrFire },
    { ai_charge, 24, GunnerCmdrFire },
    { ai_charge, 13.5f, GunnerCmdrFire }
};
const mmove_t MMOVE_T(guncmdr_move_fire_chain_run) = { FRAME_c_run201, FRAME_c_run206, guncmdr_frames_fire_chain_run, guncmdr_refire_chain };

static const mframe_t guncmdr_frames_fire_chain_dodge_right[] = {
    { ai_charge, 5.1f * 2.0f, GunnerCmdrFire },
    { ai_charge, 9.0f * 2.0f, GunnerCmdrFire },
    { ai_charge, 3.5f * 2.0f, GunnerCmdrFire },
    { ai_charge, 3.6f * 2.0f, GunnerCmdrFire },
    { ai_charge, -1.0f * 2.0f, GunnerCmdrFire }
};
const mmove_t MMOVE_T(guncmdr_move_fire_chain_dodge_right) = { FRAME_c_attack401, FRAME_c_attack405, guncmdr_frames_fire_chain_dodge_right, guncmdr_refire_chain };

static const mframe_t guncmdr_frames_fire_chain_dodge_left[] = {
    { ai_charge, 5.1f * 2.0f, GunnerCmdrFire },
    { ai_charge, 9.0f * 2.0f, GunnerCmdrFire },
    { ai_charge, 3.5f * 2.0f, GunnerCmdrFire },
    { ai_charge, 3.6f * 2.0f, GunnerCmdrFire },
    { ai_charge, -1.0f * 2.0f, GunnerCmdrFire }
};
const mmove_t MMOVE_T(guncmdr_move_fire_chain_dodge_left) = { FRAME_c_attack501, FRAME_c_attack505, guncmdr_frames_fire_chain_dodge_left, guncmdr_refire_chain };

static const mframe_t guncmdr_frames_endfire_chain[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, guncmdr_opengun },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(guncmdr_move_endfire_chain) = { FRAME_c_attack118, FRAME_c_attack124, guncmdr_frames_endfire_chain, guncmdr_run };

#define MORTAR_SPEED    850
#define GRENADE_SPEED   600

static void GunnerCmdrGrenade(edict_t *self)
{
    vec3_t                   start;
    vec3_t                   forward, right, up;
    vec3_t                   aim;
    monster_muzzleflash_id_t flash_number;
    float                    spread;
    float                    pitch = 0;
    // PMM
    vec3_t                  target;
    bool                    blindfire;

    if (!self->enemy || !self->enemy->inuse) // PGM
        return;                              // PGM

    // pmm
    blindfire = self->monsterinfo.aiflags & AI_MANUAL_STEERING;

    switch (self->s.frame) {
    case FRAME_c_attack205:
        flash_number = MZ2_GUNCMDR_GRENADE_MORTAR_1;
        spread = -0.1f;
        break;
    case FRAME_c_attack208:
        flash_number = MZ2_GUNCMDR_GRENADE_MORTAR_2;
        spread = 0;
        break;
    case FRAME_c_attack211:
        flash_number = MZ2_GUNCMDR_GRENADE_MORTAR_3;
        spread = 0.1f;
        break;
    case FRAME_c_attack304:
        flash_number = MZ2_GUNCMDR_GRENADE_FRONT_1;
        spread = -0.1f;
        break;
    case FRAME_c_attack307:
        flash_number = MZ2_GUNCMDR_GRENADE_FRONT_2;
        spread = 0;
        break;
    case FRAME_c_attack310:
        flash_number = MZ2_GUNCMDR_GRENADE_FRONT_3;
        spread = 0.1f;
        break;
    case FRAME_c_attack911:
        flash_number = MZ2_GUNCMDR_GRENADE_CROUCH_1;
        spread = 0.25f;
        break;
    case FRAME_c_attack912:
        flash_number = MZ2_GUNCMDR_GRENADE_CROUCH_2;
        spread = 0;
        break;
    case FRAME_c_attack913:
        flash_number = MZ2_GUNCMDR_GRENADE_CROUCH_3;
        spread = -0.25f;
        break;
    default:
        return;
    }

    //  pmm
    // if we're shooting blind and we still can't see our enemy
    if ((blindfire) && (!visible(self, self->enemy))) {
        // and we have a valid blind_fire_target
        if (VectorEmpty(self->monsterinfo.blind_fire_target))
            return;

        VectorCopy(self->monsterinfo.blind_fire_target, target);
    } else
        VectorCopy(self->enemy->s.origin, target);
    // pmm

    AngleVectors(self->s.angles, forward, right, up); // PGM
    M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right, start);

    // PGM
    if (self->enemy && !(flash_number >= MZ2_GUNCMDR_GRENADE_CROUCH_1 && flash_number <= MZ2_GUNCMDR_GRENADE_CROUCH_3)) {
        float dist;

        VectorSubtract(target, self->s.origin, aim);
        dist = VectorLength(aim);

        // aim up if they're on the same level as me and far away.
        if ((dist > 512) && (aim[2] < 64) && (aim[2] > -64))
            aim[2] += (dist - 512);

        VectorNormalize(aim);
        pitch = aim[2];
        if (pitch > 0.4f)
            pitch = 0.4f;
        else if (pitch < -0.5f)
            pitch = -0.5f;

        if ((self->enemy->absmin[2] - self->absmax[2]) > 16 && flash_number >= MZ2_GUNCMDR_GRENADE_MORTAR_1 && flash_number <= MZ2_GUNCMDR_GRENADE_MORTAR_3)
            pitch += 0.5f;
    }
    // PGM

    if (flash_number >= MZ2_GUNCMDR_GRENADE_FRONT_1 && flash_number <= MZ2_GUNCMDR_GRENADE_FRONT_3)
        pitch -= 0.05f;

    if (!(flash_number >= MZ2_GUNCMDR_GRENADE_CROUCH_1 && flash_number <= MZ2_GUNCMDR_GRENADE_CROUCH_3)) {
        VectorMA(forward, spread, right, aim);
        VectorMA(aim, pitch, up, aim);
    } else {
        PredictAim(self, self->enemy, start, 800, false, 0, aim, NULL);
        VectorMA(aim, spread, right, aim);
    }
    VectorNormalize(aim);

    if (flash_number >= MZ2_GUNCMDR_GRENADE_CROUCH_1 && flash_number <= MZ2_GUNCMDR_GRENADE_CROUCH_3) {
        const float inner_spread = 0.125f;

        for (int i = 0; i < 3; i++) {
            float spread = -(inner_spread * 2) + (inner_spread * (i + 1));
            vec3_t tmp;
            VectorMA(aim, spread, right, tmp);
            fire_ionripper(self, start, tmp, 15, 800, EF_IONRIPPER);
        }

        monster_muzzleflash(self, start, flash_number);
    } else {
        // mortar fires farther
        float speed = GRENADE_SPEED;
        bool mortar = false;

        if (flash_number >= MZ2_GUNCMDR_GRENADE_MORTAR_1 && flash_number <= MZ2_GUNCMDR_GRENADE_MORTAR_3) {
            speed = MORTAR_SPEED;
            mortar = true;
        }

        // try search for best pitch
        if (M_CalculatePitchToFire(self, target, start, aim, speed, 2.5f, mortar, false))
            monster_fire_grenade(self, start, aim, 50, speed, flash_number, (crandom_open() * 10.0f), frandom() * 10);
        else // normal shot
            monster_fire_grenade(self, start, aim, 50, speed, flash_number, (crandom_open() * 10.0f), 200.0f + (crandom_open() * 10.0f));
    }
}

static const mframe_t guncmdr_frames_attack_mortar[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, GunnerCmdrGrenade },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, GunnerCmdrGrenade },
    { ai_charge },
    { ai_charge },

    { ai_charge, 0, GunnerCmdrGrenade },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, monster_duck_up },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(guncmdr_move_attack_mortar) = { FRAME_c_attack201, FRAME_c_attack221, guncmdr_frames_attack_mortar, guncmdr_run };

static void guncmdr_grenade_mortar_resume(edict_t *self)
{
    M_SetAnimation(self, &guncmdr_move_attack_mortar);
    self->monsterinfo.attack_state = AS_STRAIGHT;
    self->s.frame = self->count;
}

static const mframe_t guncmdr_frames_attack_mortar_dodge[] = {
    { ai_charge, 11 },
    { ai_charge, 12 },
    { ai_charge, 16 },
    { ai_charge, 16 },
    { ai_charge, 12 },
    { ai_charge, 11 }
};
const mmove_t MMOVE_T(guncmdr_move_attack_mortar_dodge) = { FRAME_c_duckstep01, FRAME_c_duckstep06, guncmdr_frames_attack_mortar_dodge, guncmdr_grenade_mortar_resume };

static const mframe_t guncmdr_frames_attack_back[] = {
    //{ ai_charge },
    { ai_charge, -2.0f },
    { ai_charge, -1.5f },
    { ai_charge, -0.5f, GunnerCmdrGrenade },
    { ai_charge, -6.0f },
    { ai_charge, -4.0f },
    { ai_charge, -2.5f, GunnerCmdrGrenade },
    { ai_charge, -7.0f },
    { ai_charge, -3.5f },
    { ai_charge, -1.1f, GunnerCmdrGrenade },

    { ai_charge, -4.6f },
    { ai_charge, 1.9f },
    { ai_charge, 1.0f },
    { ai_charge, -4.5f },
    { ai_charge, 3.2f },
    { ai_charge, 4.4f },
    { ai_charge, -6.5f },
    { ai_charge, -6.1f },
    { ai_charge, 3.0f },
    { ai_charge, -0.7f },
    { ai_charge, -1.0f }
};
const mmove_t MMOVE_T(guncmdr_move_attack_grenade_back) = { FRAME_c_attack302, FRAME_c_attack321, guncmdr_frames_attack_back, guncmdr_run };

static void guncmdr_grenade_back_dodge_resume(edict_t *self)
{
    M_SetAnimation(self, &guncmdr_move_attack_grenade_back);
    self->monsterinfo.attack_state = AS_STRAIGHT;
    self->s.frame = self->count;
}

static const mframe_t guncmdr_frames_attack_grenade_back_dodge_right[] = {
    { ai_charge, 5.1f * 2.0f },
    { ai_charge, 9.0f * 2.0f },
    { ai_charge, 3.5f * 2.0f },
    { ai_charge, 3.6f * 2.0f },
    { ai_charge, -1.0f * 2.0f }
};
const mmove_t MMOVE_T(guncmdr_move_attack_grenade_back_dodge_right) = { FRAME_c_attack601, FRAME_c_attack605, guncmdr_frames_attack_grenade_back_dodge_right, guncmdr_grenade_back_dodge_resume };

static const mframe_t guncmdr_frames_attack_grenade_back_dodge_left[] = {
    { ai_charge, 5.1f * 2.0f },
    { ai_charge, 9.0f * 2.0f },
    { ai_charge, 3.5f * 2.0f },
    { ai_charge, 3.6f * 2.0f },
    { ai_charge, -1.0f * 2.0f }
};
const mmove_t MMOVE_T(guncmdr_move_attack_grenade_back_dodge_left) = { FRAME_c_attack701, FRAME_c_attack705, guncmdr_frames_attack_grenade_back_dodge_left, guncmdr_grenade_back_dodge_resume };

static void guncmdr_kick_finished(edict_t *self)
{
    self->monsterinfo.melee_debounce_time = level.time + SEC(3);
    self->monsterinfo.attack(self);
}

static void guncmdr_kick(edict_t *self)
{
    vec3_t aim = { MELEE_DISTANCE, 0, -32 };

    if (fire_hit(self, aim, 15, 400) && self->enemy && self->enemy->client && self->enemy->velocity[2] < 270)
        self->enemy->velocity[2] = 270;
}

static const mframe_t guncmdr_frames_attack_kick[] = {
    { ai_charge, -7.7f },
    { ai_charge, -4.9f },
    { ai_charge, 12.6f, guncmdr_kick },
    { ai_charge },
    { ai_charge, -3.0f },
    { ai_charge },
    { ai_charge, -4.1f },
    { ai_charge, 8.6f },
    //{ ai_charge, -3.5f }
};
const mmove_t MMOVE_T(guncmdr_move_attack_kick) = { FRAME_c_attack801, FRAME_c_attack808, guncmdr_frames_attack_kick, guncmdr_kick_finished };

// don't ever try grenades if we get this close
#define RANGE_GRENADE   100

// always use mortar at this range
#define RANGE_GRENADE_MORTAR    525

// at this range, run towards the enemy
#define RANGE_CHAINGUN_RUN      400

void MONSTERINFO_ATTACK(guncmdr_attack)(edict_t *self)
{
    monster_done_dodge(self);

    float d = range_to(self, self->enemy);

    vec3_t forward, right, start, aim;
    AngleVectors(self->s.angles, forward, right, NULL); // PGM

    // always use chaingun on tesla
    // kick close enemies
    if (!self->bad_area && d < RANGE_MELEE && self->monsterinfo.melee_debounce_time < level.time) {
        M_SetAnimation(self, &guncmdr_move_attack_kick);
        return;
    }

    if (self->bad_area || ((d <= RANGE_GRENADE || brandom()) && M_CheckClearShot(self, monster_flash_offset[MZ2_GUNCMDR_CHAINGUN_1]))) {
        M_SetAnimation(self, &guncmdr_move_attack_chain);
        return;
    }

    VectorSubtract(self->enemy->s.origin, self->s.origin, aim);
    VectorNormalize(aim);

    if ((d >= RANGE_GRENADE_MORTAR || fabs(self->absmin[2] - self->enemy->absmax[2]) > 64) // enemy is far below or above us, always try mortar
        && M_CheckClearShot(self, monster_flash_offset[MZ2_GUNCMDR_GRENADE_MORTAR_1])) {
        M_ProjectFlashSource(self, monster_flash_offset[MZ2_GUNCMDR_GRENADE_MORTAR_1], forward, right, start);
        if (M_CalculatePitchToFire(self, self->enemy->s.origin, start, aim, MORTAR_SPEED, 2.5f, true, false)) {
            M_SetAnimation(self, &guncmdr_move_attack_mortar);
            monster_duck_down(self);
            return;
        }
    }

    if (self->monsterinfo.aiflags & AI_STAND_GROUND) {
        M_SetAnimation(self, &guncmdr_move_attack_chain);
        return;
    }

    if (M_CheckClearShot(self, monster_flash_offset[MZ2_GUNCMDR_GRENADE_FRONT_1])) {
        M_ProjectFlashSource(self, monster_flash_offset[MZ2_GUNCMDR_GRENADE_FRONT_1], forward, right, start);
        if (M_CalculatePitchToFire(self, self->enemy->s.origin, start, aim, GRENADE_SPEED, 2.5f, false, false)) {
            M_SetAnimation(self, &guncmdr_move_attack_grenade_back);
            return;
        }
    }
}

static void guncmdr_fire_chain(edict_t *self)
{
    if (!(self->monsterinfo.aiflags & AI_STAND_GROUND) && self->enemy && range_to(self, self->enemy) > RANGE_CHAINGUN_RUN && ai_check_move(self, 8.0f))
        M_SetAnimation(self, &guncmdr_move_fire_chain_run);
    else
        M_SetAnimation(self, &guncmdr_move_fire_chain);
}

static void guncmdr_refire_chain(edict_t *self)
{
    monster_done_dodge(self);
    self->monsterinfo.attack_state = AS_STRAIGHT;

    if (self->enemy->health > 0 && visible(self, self->enemy) && brandom()) {
        if (!(self->monsterinfo.aiflags & AI_STAND_GROUND) && self->enemy && range_to(self, self->enemy) > RANGE_CHAINGUN_RUN && ai_check_move(self, 8.0f))
            M_SetAnimationEx(self, &guncmdr_move_fire_chain_run, false);
        else
            M_SetAnimationEx(self, &guncmdr_move_fire_chain, false);
    } else
        M_SetAnimationEx(self, &guncmdr_move_endfire_chain, false);
}

//===========
// PGM
static void guncmdr_jump_now(edict_t *self)
{
    vec3_t forward, up;

    AngleVectors(self->s.angles, forward, NULL, up);
    VectorMA(self->velocity, 100, forward, self->velocity);
    VectorMA(self->velocity, 300, up, self->velocity);
}

static void guncmdr_jump2_now(edict_t *self)
{
    vec3_t forward, up;

    AngleVectors(self->s.angles, forward, NULL, up);
    VectorMA(self->velocity, 150, forward, self->velocity);
    VectorMA(self->velocity, 400, up, self->velocity);
}

static void guncmdr_jump_wait_land(edict_t *self)
{
    if (self->groundentity == NULL) {
        self->monsterinfo.nextframe = self->s.frame;

        if (monster_jump_finished(self))
            self->monsterinfo.nextframe = self->s.frame + 1;
    } else
        self->monsterinfo.nextframe = self->s.frame + 1;
}

static const mframe_t guncmdr_frames_jump[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, guncmdr_jump_now },
    { ai_move },
    { ai_move },
    { ai_move, 0, guncmdr_jump_wait_land },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(guncmdr_move_jump) = { FRAME_c_jump01, FRAME_c_jump10, guncmdr_frames_jump, guncmdr_run };

static const mframe_t guncmdr_frames_jump2[] = {
    { ai_move, -8 },
    { ai_move, -4 },
    { ai_move, -4 },
    { ai_move, 0, guncmdr_jump2_now },
    { ai_move },
    { ai_move },
    { ai_move, 0, guncmdr_jump_wait_land },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(guncmdr_move_jump2) = { FRAME_c_jump01, FRAME_c_jump10, guncmdr_frames_jump2, guncmdr_run };

static void guncmdr_jump(edict_t *self, blocked_jump_result_t result)
{
    if (!self->enemy)
        return;

    monster_done_dodge(self);

    if (result == JUMP_JUMP_UP)
        M_SetAnimation(self, &guncmdr_move_jump2);
    else
        M_SetAnimation(self, &guncmdr_move_jump);
}

void T_SlamRadiusDamage(vec3_t point, edict_t *inflictor, edict_t *attacker, float damage, float kick, edict_t *ignore, float radius, mod_t mod);

static void GunnerCmdrCounter(edict_t *self)
{
    vec3_t f, r, start;
    AngleVectors(self->s.angles, f, r, NULL);
    M_ProjectFlashSource(self, (const vec3_t) { 20, 0, 14 }, f, r, start);
    trace_t tr = gi.trace(self->s.origin, NULL, NULL, start, self, MASK_SOLID);

    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_BERSERK_SLAM);
    gi.WritePosition(tr.endpos);
    gi.WriteDir(f);
    gi.multicast(tr.endpos, MULTICAST_PHS);

    T_SlamRadiusDamage(tr.endpos, self, self, 15, 250, self, 200, (mod_t) { MOD_UNKNOWN });
}

//===========
// PGM
static const mframe_t guncmdr_frames_duck_attack[] = {
    { ai_move, 3.6f },
    { ai_move, 5.6f, monster_duck_down },
    { ai_move, 8.4f },
    { ai_move, 2.0f, monster_duck_hold },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },

    //{ ai_charge, 0, GunnerCmdrGrenade },
    //{ ai_charge, 9.5f, GunnerCmdrGrenade },
    //{ ai_charge, -1.5f, GunnerCmdrGrenade },

    { ai_charge, 0 },
    { ai_charge, 9.5f, GunnerCmdrCounter },
    { ai_charge, -1.5f },
    { ai_charge },
    { ai_charge, 0, monster_duck_up },
    { ai_charge },
    { ai_charge, 11.0f },
    { ai_charge, 2.0f },
    { ai_charge, 5.6f }
};
const mmove_t MMOVE_T(guncmdr_move_duck_attack) = { FRAME_c_attack901, FRAME_c_attack919, guncmdr_frames_duck_attack, guncmdr_run };

bool MONSTERINFO_DUCK(guncmdr_duck)(edict_t *self, gtime_t eta)
{
    if ((self->monsterinfo.active_move == &guncmdr_move_jump2) ||
        (self->monsterinfo.active_move == &guncmdr_move_jump))
        return false;

    if ((self->monsterinfo.active_move == &guncmdr_move_fire_chain_dodge_left) ||
        (self->monsterinfo.active_move == &guncmdr_move_fire_chain_dodge_right) ||
        (self->monsterinfo.active_move == &guncmdr_move_attack_grenade_back_dodge_left) ||
        (self->monsterinfo.active_move == &guncmdr_move_attack_grenade_back_dodge_right) ||
        (self->monsterinfo.active_move == &guncmdr_move_attack_mortar_dodge)) {
        // if we're dodging, don't duck
        self->monsterinfo.unduck(self);
        return false;
    }

    M_SetAnimation(self, &guncmdr_move_duck_attack);

    return true;
}

bool MONSTERINFO_SIDESTEP(guncmdr_sidestep)(edict_t *self)
{
    // use special dodge during the main firing anim
    if (self->monsterinfo.active_move == &guncmdr_move_fire_chain ||
        self->monsterinfo.active_move == &guncmdr_move_fire_chain_run) {
        M_SetAnimationEx(self, !self->monsterinfo.lefty ? &guncmdr_move_fire_chain_dodge_right : &guncmdr_move_fire_chain_dodge_left, false);
        return true;
    }

    // for backwards mortar, back up where we are in the animation and do a quick dodge
    if (self->monsterinfo.active_move == &guncmdr_move_attack_grenade_back) {
        self->count = self->s.frame;
        M_SetAnimationEx(self, !self->monsterinfo.lefty ? &guncmdr_move_attack_grenade_back_dodge_right : &guncmdr_move_attack_grenade_back_dodge_left, false);
        return true;
    }

    // use crouch-move for mortar dodge
    if (self->monsterinfo.active_move == &guncmdr_move_attack_mortar) {
        self->count = self->s.frame;
        M_SetAnimationEx(self, &guncmdr_move_attack_mortar_dodge, false);
        return true;
    }

    // regular sidestep during run
    if (self->monsterinfo.active_move == &guncmdr_move_run) {
        M_SetAnimation(self, &guncmdr_move_run);
        return true;
    }

    return false;
}

bool MONSTERINFO_BLOCKED(guncmdr_blocked)(edict_t *self, float dist)
{
    if (blocked_checkplat(self, dist))
        return true;

    blocked_jump_result_t result = blocked_checkjump(self, dist);
    if (result != NO_JUMP) {
        if (result != JUMP_TURN)
            guncmdr_jump(self, result);
        return true;
    }

    return false;
}
// PGM
//===========

static void guncmdr_precache(void)
{
    sound_death = gi.soundindex("guncmdr/gcdrdeath1.wav");
    sound_pain = gi.soundindex("guncmdr/gcdrpain2.wav");
    sound_pain2 = gi.soundindex("guncmdr/gcdrpain1.wav");
    sound_idle = gi.soundindex("guncmdr/gcdridle1.wav");
    sound_open = gi.soundindex("guncmdr/gcdratck1.wav");
    sound_search = gi.soundindex("guncmdr/gcdrsrch1.wav");
    sound_sight = gi.soundindex("guncmdr/sight1.wav");
}

/*QUAKED monster_guncmdr (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight NoJumping
model="models/monsters/guncmdr/tris.md2"
*/
void SP_monster_guncmdr(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    G_AddPrecache(guncmdr_precache);

    gi.soundindex("guncmdr/gcdratck2.wav");
    gi.soundindex("guncmdr/gcdratck3.wav");

    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;
    self->s.modelindex = gi.modelindex("models/monsters/gunner/tris.md2");

    PrecacheGibs(guncmdr_gibs);

    self->x.scale = 1.25f;
    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, 36);
    self->s.skinnum = 2;

    self->health = 325 * st.health_multiplier;
    self->gib_health = -175;
    self->mass = 255;

    self->pain = guncmdr_pain;
    self->die = guncmdr_die;

    self->monsterinfo.stand = guncmdr_stand;
    self->monsterinfo.walk = guncmdr_walk;
    self->monsterinfo.run = guncmdr_run;
    // pmm
    self->monsterinfo.dodge = M_MonsterDodge;
    self->monsterinfo.duck = guncmdr_duck;
    self->monsterinfo.unduck = monster_duck_up;
    self->monsterinfo.sidestep = guncmdr_sidestep;
    self->monsterinfo.blocked = guncmdr_blocked; // PGM
    // pmm
    self->monsterinfo.attack = guncmdr_attack;
    self->monsterinfo.melee = NULL;
    self->monsterinfo.sight = guncmdr_sight;
    self->monsterinfo.search = guncmdr_search;
    self->monsterinfo.setskin = guncmdr_setskin;

    gi.linkentity(self);

    M_SetAnimation(self, &guncmdr_move_stand);
    self->monsterinfo.scale = MODEL_SCALE;

    if (!ED_WasKeySpecified("power_armor_power"))
        self->monsterinfo.power_armor_power = 200;
    if (!ED_WasKeySpecified("power_armor_type"))
        self->monsterinfo.power_armor_type = IT_ITEM_POWER_SHIELD;

    // PMM
    //self->monsterinfo.blindfire = true;
    self->monsterinfo.can_jump = !(self->spawnflags & SPAWNFLAG_GUNCMDR_NOJUMPING);
    self->monsterinfo.drop_height = 192;
    self->monsterinfo.jump_height = 40;

    walkmonster_start(self);
}
