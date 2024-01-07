// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

brain

==============================================================================
*/

#include "g_local.h"
#include "m_brain.h"

static int sound_chest_open;
static int sound_tentacles_extend;
static int sound_tentacles_retract;
static int sound_death;
static int sound_idle1;
static int sound_idle2;
static int sound_idle3;
static int sound_pain1;
static int sound_pain2;
static int sound_sight;
static int sound_search;
static int sound_melee1;
static int sound_melee2;
static int sound_melee3;

void MONSTERINFO_SIGHT(brain_sight)(edict_t *self, edict_t *other)
{
    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void MONSTERINFO_SEARCH(brain_search)(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void brain_run(edict_t *self);
static void brain_dead(edict_t *self);

#define SPAWNFLAG_BRAIN_NO_LASERS   8

//
// STAND
//

static const mframe_t brain_frames_stand[] = {
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
const mmove_t MMOVE_T(brain_move_stand) = { FRAME_stand01, FRAME_stand30, brain_frames_stand, NULL };

void MONSTERINFO_STAND(brain_stand)(edict_t *self)
{
    M_SetAnimation(self, &brain_move_stand);
}

//
// IDLE
//

static const mframe_t brain_frames_idle[] = {
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
const mmove_t MMOVE_T(brain_move_idle) = { FRAME_stand31, FRAME_stand60, brain_frames_idle, brain_stand };

void MONSTERINFO_IDLE(brain_idle)(edict_t *self)
{
    gi.sound(self, CHAN_AUTO, sound_idle3, 1, ATTN_IDLE, 0);
    M_SetAnimation(self, &brain_move_idle);
}

//
// WALK
//
static const mframe_t brain_frames_walk1[] = {
    { ai_walk, 7 },
    { ai_walk, 2 },
    { ai_walk, 3 },
    { ai_walk, 3, monster_footstep },
    { ai_walk, 1 },
    { ai_walk },
    { ai_walk },
    { ai_walk, 9 },
    { ai_walk, -4 },
    { ai_walk, -1, monster_footstep },
    { ai_walk, 2 }
};
const mmove_t MMOVE_T(brain_move_walk1) = { FRAME_walk101, FRAME_walk111, brain_frames_walk1, NULL };

void MONSTERINFO_WALK(brain_walk)(edict_t *self)
{
    M_SetAnimation(self, &brain_move_walk1);
}

#if 0
static const mframe_t brain_frames_defense[] = {
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
const mmove_t MMOVE_T(brain_move_defense) = { FRAME_defens01, FRAME_defens08, brain_frames_defense, NULL };
#endif

static const mframe_t brain_frames_pain3[] = {
    { ai_move, -2 },
    { ai_move, 2 },
    { ai_move, 1 },
    { ai_move, 3 },
    { ai_move },
    { ai_move, -4 }
};
const mmove_t MMOVE_T(brain_move_pain3) = { FRAME_pain301, FRAME_pain306, brain_frames_pain3, brain_run };

static const mframe_t brain_frames_pain2[] = {
    { ai_move, -2 },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 3 },
    { ai_move, 1 },
    { ai_move, -2 }
};
const mmove_t MMOVE_T(brain_move_pain2) = { FRAME_pain201, FRAME_pain208, brain_frames_pain2, brain_run };

static const mframe_t brain_frames_pain1[] = {
    { ai_move, -6 },
    { ai_move, -2 },
    { ai_move, -6, monster_footstep },
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
    { ai_move, 2 },
    { ai_move },
    { ai_move, 2 },
    { ai_move, 1 },
    { ai_move, 7 },
    { ai_move },
    { ai_move, 3, monster_footstep },
    { ai_move, -1 }
};
const mmove_t MMOVE_T(brain_move_pain1) = { FRAME_pain101, FRAME_pain121, brain_frames_pain1, brain_run };

static void brain_duck_down(edict_t *self)
{
    monster_duck_down(self);
    monster_footstep(self);
}

static const mframe_t brain_frames_duck[] = {
    { ai_move },
    { ai_move, -2, brain_duck_down },
    { ai_move, 17, monster_duck_hold },
    { ai_move, -3 },
    { ai_move, -1, monster_duck_up },
    { ai_move, -5 },
    { ai_move, -6 },
    { ai_move, -6, monster_footstep }
};
const mmove_t MMOVE_T(brain_move_duck) = { FRAME_duck01, FRAME_duck08, brain_frames_duck, brain_run };

static void brain_shrink(edict_t *self)
{
    self->maxs[2] = 0;
    self->svflags |= SVF_DEADMONSTER;
    gi.linkentity(self);
}

static const mframe_t brain_frames_death2[] = {
    { ai_move },
    { ai_move, 0, monster_footstep },
    { ai_move, 0, brain_shrink },
    { ai_move, 9 },
    { ai_move }
};
const mmove_t MMOVE_T(brain_move_death2) = { FRAME_death201, FRAME_death205, brain_frames_death2, brain_dead };

static void brain_shrink2(edict_t *self)
{
    brain_shrink(self);
    monster_footstep(self);
}

static const mframe_t brain_frames_death1[] = {
    { ai_move },
    { ai_move, 0, monster_footstep },
    { ai_move, -2 },
    { ai_move, 9, brain_shrink2 },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, monster_footstep },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, monster_footstep },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(brain_move_death1) = { FRAME_death101, FRAME_death118, brain_frames_death1, brain_dead };

//
// MELEE
//

static void brain_swing_right(edict_t *self)
{
    gi.sound(self, CHAN_BODY, sound_melee1, 1, ATTN_NORM, 0);
}

static void brain_hit_right(edict_t *self)
{
    vec3_t aim = { MELEE_DISTANCE, self->maxs[0], 8 };
    if (fire_hit(self, aim, irandom2(15, 20), 40))
        gi.sound(self, CHAN_WEAPON, sound_melee3, 1, ATTN_NORM, 0);
    else
        self->monsterinfo.melee_debounce_time = level.time + SEC(3);
}

static void brain_swing_left(edict_t *self)
{
    gi.sound(self, CHAN_BODY, sound_melee2, 1, ATTN_NORM, 0);
}

static void brain_hit_left(edict_t *self)
{
    vec3_t aim = { MELEE_DISTANCE, self->mins[0], 8 };
    if (fire_hit(self, aim, irandom2(15, 20), 40))
        gi.sound(self, CHAN_WEAPON, sound_melee3, 1, ATTN_NORM, 0);
    else
        self->monsterinfo.melee_debounce_time = level.time + SEC(3);
}

static const mframe_t brain_frames_attack1[] = {
    { ai_charge, 8 },
    { ai_charge, 3 },
    { ai_charge, 5 },
    { ai_charge, 0, monster_footstep },
    { ai_charge, -3, brain_swing_right },
    { ai_charge },
    { ai_charge, -5 },
    { ai_charge, -7, brain_hit_right },
    { ai_charge },
    { ai_charge, 6, brain_swing_left },
    { ai_charge, 1 },
    { ai_charge, 2, brain_hit_left },
    { ai_charge, -3 },
    { ai_charge, 6 },
    { ai_charge, -1 },
    { ai_charge, -3 },
    { ai_charge, 2 },
    { ai_charge, -11, monster_footstep }
};
const mmove_t MMOVE_T(brain_move_attack1) = { FRAME_attak101, FRAME_attak118, brain_frames_attack1, brain_run };

static void brain_chest_open(edict_t *self)
{
    self->count = 0;
    self->monsterinfo.power_armor_type = IT_NULL;
    gi.sound(self, CHAN_BODY, sound_chest_open, 1, ATTN_NORM, 0);
}

static void brain_tentacle_attack(edict_t *self)
{
    vec3_t aim = { MELEE_DISTANCE, 0, 8 };
    if (fire_hit(self, aim, irandom2(10, 15), -600))
        self->count = 1;
    else
        self->monsterinfo.melee_debounce_time = level.time + SEC(3);
    gi.sound(self, CHAN_WEAPON, sound_tentacles_retract, 1, ATTN_NORM, 0);
}

static void brain_chest_closed(edict_t *self)
{
    self->monsterinfo.power_armor_type = IT_ITEM_POWER_SCREEN;
    if (self->count) {
        self->count = 0;
        M_SetAnimation(self, &brain_move_attack1);
    }
}

static const mframe_t brain_frames_attack2[] = {
    { ai_charge, 5 },
    { ai_charge, -4 },
    { ai_charge, -4 },
    { ai_charge, -3 },
    { ai_charge, 0, brain_chest_open },
    { ai_charge },
    { ai_charge, 13, brain_tentacle_attack },
    { ai_charge },
    { ai_charge, 2 },
    { ai_charge },
    { ai_charge, -9, brain_chest_closed },
    { ai_charge },
    { ai_charge, 4 },
    { ai_charge, 3 },
    { ai_charge, 2 },
    { ai_charge, -3 },
    { ai_charge, -6 }
};
const mmove_t MMOVE_T(brain_move_attack2) = { FRAME_attak201, FRAME_attak217, brain_frames_attack2, brain_run };

void MONSTERINFO_MELEE(brain_melee)(edict_t *self)
{
    if (brandom())
        M_SetAnimation(self, &brain_move_attack1);
    else
        M_SetAnimation(self, &brain_move_attack2);
}

// RAFAEL
static bool brain_tounge_attack_ok(const vec3_t start, const vec3_t end)
{
    vec3_t dir, angles;

    // check for max distance
    VectorSubtract(start, end, dir);
    if (VectorLength(dir) > 512)
        return false;

    // check for min/max pitch
    vectoangles(dir, angles);
    if (angles[0] < -180)
        angles[0] += 360;
    if (fabsf(angles[0]) > 30)
        return false;

    return true;
}

static void brain_tounge_attack(edict_t *self)
{
    vec3_t  offset, start, f, r, end, dir;
    trace_t tr;
    int     damage;

    AngleVectors(self->s.angles, f, r, NULL);
    VectorSet(offset, 24, 0, 16);
    M_ProjectFlashSource(self, offset, f, r, start);

    VectorCopy(self->enemy->s.origin, end);
    if (!brain_tounge_attack_ok(start, end)) {
        end[2] = self->enemy->s.origin[2] + self->enemy->maxs[2] - 8;
        if (!brain_tounge_attack_ok(start, end)) {
            end[2] = self->enemy->s.origin[2] + self->enemy->mins[2] + 8;
            if (!brain_tounge_attack_ok(start, end))
                return;
        }
    }
    VectorCopy(self->enemy->s.origin, end);

    tr = gi.trace(start, NULL, NULL, end, self, MASK_PROJECTILE);
    if (tr.ent != self->enemy)
        return;

    damage = 5;
    gi.sound(self, CHAN_WEAPON, sound_tentacles_retract, 1, ATTN_NORM, 0);

    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_PARASITE_ATTACK);
    gi.WriteShort(self - g_edicts);
    gi.WritePosition(start);
    gi.WritePosition(end);
    gi.multicast(self->s.origin, MULTICAST_PVS);

    VectorSubtract(start, end, dir);
    T_Damage(self->enemy, self, self, dir, self->enemy->s.origin, vec3_origin, damage, 0, DAMAGE_NO_KNOCKBACK, (mod_t) { MOD_BRAINTENTACLE });

    // pull the enemy in
    vec3_t forward;
    self->s.origin[2] += 1;
    AngleVectors(self->s.angles, forward, NULL, NULL);
    VectorScale(forward, -1200, self->enemy->velocity);
}

// Brain right eye center
static const vec3_t brain_reye[] = {
    { 0.238370f, -0.746700f, 34.167690f },
    { 0.238370f, 1.076390f, 33.386372f },
    { 5.334300f, 1.335500f, 32.177170f },
    { 8.846370f, 0.175360f, 30.635479f },
    { 7.804610f, 2.757590f, 30.150860f },
    { 5.152840f, 5.575090f, 30.056160f },
    { 3.262470f, 7.017550f, 30.552521f },
    { 0.638800f, 7.915740f, 33.176189f },
    { 8.285730f, 3.915390f, 33.976349f },
    { 10.933030f, 0.913540f, 34.141811f },
    { 8.923900f, 0.369900f, 34.189079f }
};

// Brain left eye center
static const vec3_t brain_leye[] = {
    { 0.327750f, 3.364710f, 33.938381f },
    { 0.493480f, 5.140450f, 32.659851f },
    { 5.646980f, 5.341980f, 31.277901f },
    { 9.277440f, 4.134480f, 29.925621f },
    { 6.815090f, 6.598340f, 29.322620f },
    { 2.529650f, 8.610840f, 29.251591f },
    { 0.093280f, 9.231360f, 29.747959f },
    { 1.936930f, 11.004110f, 32.395260f },
    { 7.648190f, 7.878310f, 33.148151f },
    { 11.430050f, 4.947370f, 33.313610f },
    { 9.444570f, 4.332820f, 33.526340f }
};

void PRETHINK(brain_right_eye_laser_update)(edict_t *laser)
{
    edict_t *self = laser->owner;

    vec3_t start, axis[3], dir;

    // rotate into worlds frame of reference
    AnglesToAxis(self->s.angles, axis);
    TransposeAxis(axis);

    // dis is my right eye
    VectorCopy(brain_reye[self->s.frame - FRAME_walk101], start);
    RotatePoint(start, axis);
    VectorAdd(start, self->s.origin, start);

    PredictAim(self, self->enemy, start, 0, false, frandom2(0.1f, 0.2f), dir, NULL);

    VectorCopy(start, laser->s.origin);
    VectorCopy(dir, laser->movedir);
    gi.linkentity(laser);
}

void PRETHINK(brain_left_eye_laser_update)(edict_t *laser)
{
    edict_t *self = laser->owner;

    vec3_t start, axis[3], dir;

    // rotate into worlds frame of reference
    AnglesToAxis(self->s.angles, axis);
    TransposeAxis(axis);

    // dis is my left eye
    VectorCopy(brain_leye[self->s.frame - FRAME_walk101], start);
    RotatePoint(start, axis);
    VectorAdd(start, self->s.origin, start);

    PredictAim(self, self->enemy, start, 0, false, frandom2(0.1f, 0.2f), dir, NULL);

    VectorCopy(start, laser->s.origin);
    VectorCopy(dir, laser->movedir);
    gi.linkentity(laser);
    dabeam_update(laser, false);
}

static void brain_laserbeam(edict_t *self)
{
    // dis is my right eye
    monster_fire_dabeam(self, 1, false, brain_right_eye_laser_update);

    // dis is me left eye
    monster_fire_dabeam(self, 1, true, brain_left_eye_laser_update);
}

static void brain_laserbeam_reattack(edict_t *self)
{
    if (brandom() && visible(self, self->enemy) && self->enemy->health > 0)
        self->s.frame = FRAME_walk101;
}

static const mframe_t brain_frames_attack3[] = {
    { ai_charge, 5 },
    { ai_charge, -4 },
    { ai_charge, -4 },
    { ai_charge, -3 },
    { ai_charge, 0, brain_chest_open },
    { ai_charge, 0, brain_tounge_attack },
    { ai_charge, 13 },
    { ai_charge, 0, brain_tentacle_attack },
    { ai_charge, 2 },
    { ai_charge, 0, brain_tounge_attack },
    { ai_charge, -9, brain_chest_closed },
    { ai_charge },
    { ai_charge, 4 },
    { ai_charge, 3 },
    { ai_charge, 2 },
    { ai_charge, -3 },
    { ai_charge, -6 }
};
const mmove_t MMOVE_T(brain_move_attack3) = { FRAME_attak201, FRAME_attak217, brain_frames_attack3, brain_run };

static const mframe_t brain_frames_attack4[] = {
    { ai_charge, 9, brain_laserbeam },
    { ai_charge, 2, brain_laserbeam },
    { ai_charge, 3, brain_laserbeam },
    { ai_charge, 3, brain_laserbeam },
    { ai_charge, 1, brain_laserbeam },
    { ai_charge, 0, brain_laserbeam },
    { ai_charge, 0, brain_laserbeam },
    { ai_charge, 10, brain_laserbeam },
    { ai_charge, -4, brain_laserbeam },
    { ai_charge, -1, brain_laserbeam },
    { ai_charge, 2, brain_laserbeam_reattack }
};
const mmove_t MMOVE_T(brain_move_attack4) = { FRAME_walk101, FRAME_walk111, brain_frames_attack4, brain_run };

// RAFAEL
void MONSTERINFO_ATTACK(brain_attack)(edict_t *self)
{
    if (range_to(self, self->enemy) <= RANGE_NEAR && brandom())
        M_SetAnimation(self, &brain_move_attack3);
    else if (!(self->spawnflags & SPAWNFLAG_BRAIN_NO_LASERS))
        M_SetAnimation(self, &brain_move_attack4);
}
// RAFAEL

//
// RUN
//

static const mframe_t brain_frames_run[] = {
    { ai_run, 9 },
    { ai_run, 2 },
    { ai_run, 3 },
    { ai_run, 3 },
    { ai_run, 1 },
    { ai_run },
    { ai_run },
    { ai_run, 10 },
    { ai_run, -4 },
    { ai_run, -1 },
    { ai_run, 2 }
};
const mmove_t MMOVE_T(brain_move_run) = { FRAME_walk101, FRAME_walk111, brain_frames_run, NULL };

void MONSTERINFO_RUN(brain_run)(edict_t *self)
{
    self->monsterinfo.power_armor_type = IT_ITEM_POWER_SCREEN;
    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        M_SetAnimation(self, &brain_move_stand);
    else
        M_SetAnimation(self, &brain_move_run);
}

void PAIN(brain_pain)(edict_t *self, edict_t *other, float kick, int damage, const mod_t mod)
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
        M_SetAnimation(self, &brain_move_pain1);
    else if (r < 0.66f)
        M_SetAnimation(self, &brain_move_pain2);
    else
        M_SetAnimation(self, &brain_move_pain3);

    // PMM - clear duck flag
    if (self->monsterinfo.aiflags & AI_DUCKED)
        monster_duck_up(self);
}

void MONSTERINFO_SETSKIN(brain_setskin)(edict_t *self)
{
    if (self->health < (self->max_health / 2))
        self->s.skinnum = 1;
    else
        self->s.skinnum = 0;
}

void brain_dead(edict_t *self)
{
    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, -8);
    monster_dead(self);
}

static const gib_def_t brain_gibs[] = {
    { "models/objects/gibs/bone/tris.md2", 1 },
    { "models/objects/gibs/sm_meat/tris.md2", 2 },
    { "models/monsters/brain/gibs/arm.md2", 2, GIB_SKINNED | GIB_UPRIGHT },
    { "models/monsters/brain/gibs/boot.md2", 2, GIB_SKINNED | GIB_UPRIGHT },
    { "models/monsters/brain/gibs/pelvis.md2", 1, GIB_SKINNED },
    { "models/monsters/brain/gibs/chest.md2", 1, GIB_SKINNED },
    { "models/monsters/brain/gibs/door.md2", 2, GIB_SKINNED | GIB_UPRIGHT },
    { "models/monsters/brain/gibs/head.md2", 1, GIB_SKINNED | GIB_HEAD },
    { 0 }
};

void DIE(brain_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, const mod_t mod)
{
    self->s.effects = EF_NONE;
    self->monsterinfo.power_armor_type = IT_NULL;

    // check for gib
    if (M_CheckGib(self, mod)) {
        gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

        self->s.skinnum /= 2;

        if (self->beam) {
            G_FreeEdict(self->beam);
            self->beam = NULL;
        }
        if (self->beam2) {
            G_FreeEdict(self->beam2);
            self->beam2 = NULL;
        }

        ThrowGibs(self, damage, brain_gibs);
        self->deadflag = true;
        return;
    }

    if (self->deadflag)
        return;

    // regular death
    gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
    self->deadflag = true;
    self->takedamage = true;
    if (brandom())
        M_SetAnimation(self, &brain_move_death1);
    else
        M_SetAnimation(self, &brain_move_death2);
}

bool MONSTERINFO_DUCK(brain_duck)(edict_t *self, gtime_t eta)
{
    M_SetAnimation(self, &brain_move_duck);
    return true;
}

static void brain_precache(void)
{
    sound_chest_open = gi.soundindex("brain/brnatck1.wav");
    sound_tentacles_extend = gi.soundindex("brain/brnatck2.wav");
    sound_tentacles_retract = gi.soundindex("brain/brnatck3.wav");
    sound_death = gi.soundindex("brain/brndeth1.wav");
    sound_idle1 = gi.soundindex("brain/brnidle1.wav");
    sound_idle2 = gi.soundindex("brain/brnidle2.wav");
    sound_idle3 = gi.soundindex("brain/brnlens1.wav");
    sound_pain1 = gi.soundindex("brain/brnpain1.wav");
    sound_pain2 = gi.soundindex("brain/brnpain2.wav");
    sound_sight = gi.soundindex("brain/brnsght1.wav");
    sound_search = gi.soundindex("brain/brnsrch1.wav");
    sound_melee1 = gi.soundindex("brain/melee1.wav");
    sound_melee2 = gi.soundindex("brain/melee2.wav");
    sound_melee3 = gi.soundindex("brain/melee3.wav");
}

/*QUAKED monster_brain (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
 */
void SP_monster_brain(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    G_AddPrecache(brain_precache);

    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;
    self->s.modelindex = gi.modelindex("models/monsters/brain/tris.md2");

    PrecacheGibs(brain_gibs);

    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, 32);

    self->health = 300 * st.health_multiplier;
    self->gib_health = -150;
    self->mass = 400;

    self->pain = brain_pain;
    self->die = brain_die;

    self->monsterinfo.stand = brain_stand;
    self->monsterinfo.walk = brain_walk;
    self->monsterinfo.run = brain_run;
    // PMM
    self->monsterinfo.dodge = M_MonsterDodge;
    self->monsterinfo.duck = brain_duck;
    self->monsterinfo.unduck = monster_duck_up;
    // pmm
    // RAFAEL
    self->monsterinfo.attack = brain_attack;
    // RAFAEL
    self->monsterinfo.melee = brain_melee;
    self->monsterinfo.sight = brain_sight;
    self->monsterinfo.search = brain_search;
    self->monsterinfo.idle = brain_idle;
    self->monsterinfo.setskin = brain_setskin;

    if (!ED_WasKeySpecified("power_armor_type"))
        self->monsterinfo.power_armor_type = IT_ITEM_POWER_SCREEN;
    if (!ED_WasKeySpecified("power_armor_power"))
        self->monsterinfo.power_armor_power = 100;

    gi.linkentity(self);

    M_SetAnimation(self, &brain_move_stand);
    self->monsterinfo.scale = MODEL_SCALE;

    walkmonster_start(self);
}
