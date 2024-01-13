// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

INFANTRY

==============================================================================
*/

#include "g_local.h"
#include "m_infantry.h"

static void InfantryMachineGun(edict_t *self);

static int sound_pain1;
static int sound_pain2;
static int sound_die1;
static int sound_die2;

static int sound_gunshot;
static int sound_weapon_cock;
static int sound_punch_swing;
static int sound_punch_hit;
static int sound_sight;
static int sound_search;
static int sound_idle;

// range at which we'll try to initiate a run-attack to close distance
#define RANGE_RUN_ATTACK    (RANGE_NEAR * 0.75f)

static const mframe_t infantry_frames_stand[] = {
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
const mmove_t MMOVE_T(infantry_move_stand) = { FRAME_stand50, FRAME_stand71, infantry_frames_stand, NULL };

void MONSTERINFO_STAND(infantry_stand)(edict_t *self)
{
    M_SetAnimation(self, &infantry_move_stand);
}

static const mframe_t infantry_frames_fidget[] = {
    { ai_stand, 1 },
    { ai_stand },
    { ai_stand, 1 },
    { ai_stand, 3 },
    { ai_stand, 6 },
    { ai_stand, 3, monster_footstep },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand, 1 },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand, 1 },
    { ai_stand },
    { ai_stand, -1 },
    { ai_stand },
    { ai_stand },
    { ai_stand, 1 },
    { ai_stand },
    { ai_stand, -2 },
    { ai_stand, 1 },
    { ai_stand, 1 },
    { ai_stand, 1 },
    { ai_stand, -1 },
    { ai_stand },
    { ai_stand },
    { ai_stand, -1 },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand, -1 },
    { ai_stand },
    { ai_stand },
    { ai_stand, 1 },
    { ai_stand },
    { ai_stand },
    { ai_stand, -1 },
    { ai_stand, -1 },
    { ai_stand },
    { ai_stand, -3 },
    { ai_stand, -2 },
    { ai_stand, -3 },
    { ai_stand, -3, monster_footstep },
    { ai_stand, -2 }
};
const mmove_t MMOVE_T(infantry_move_fidget) = { FRAME_stand01, FRAME_stand49, infantry_frames_fidget, infantry_stand };

void MONSTERINFO_IDLE(infantry_fidget)(edict_t *self)
{
    if (self->enemy)
        return;

    M_SetAnimation(self, &infantry_move_fidget);
    gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

static const mframe_t infantry_frames_walk[] = {
    { ai_walk, 5, monster_footstep },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 5 },
    { ai_walk, 4 },
    { ai_walk, 5 },
    { ai_walk, 6, monster_footstep },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 5 }
};
const mmove_t MMOVE_T(infantry_move_walk) = { FRAME_walk03, FRAME_walk14, infantry_frames_walk, NULL };

void MONSTERINFO_WALK(infantry_walk)(edict_t *self)
{
    M_SetAnimation(self, &infantry_move_walk);
}

static const mframe_t infantry_frames_run[] = {
    { ai_run, 10 },
    { ai_run, 15, monster_footstep },
    { ai_run, 5 },
    { ai_run, 7, monster_done_dodge },
    { ai_run, 18 },
    { ai_run, 20, monster_footstep },
    { ai_run, 2 },
    { ai_run, 6 }
};
const mmove_t MMOVE_T(infantry_move_run) = { FRAME_run01, FRAME_run08, infantry_frames_run, NULL };

void MONSTERINFO_RUN(infantry_run)(edict_t *self)
{
    monster_done_dodge(self);

    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        M_SetAnimation(self, &infantry_move_stand);
    else
        M_SetAnimation(self, &infantry_move_run);
}

static const mframe_t infantry_frames_pain1[] = {
    { ai_move, -3 },
    { ai_move, -2 },
    { ai_move, -1 },
    { ai_move, -2 },
    { ai_move, -1, monster_footstep },
    { ai_move, 1 },
    { ai_move, -1 },
    { ai_move, 1 },
    { ai_move, 6 },
    { ai_move, 2, monster_footstep }
};
const mmove_t MMOVE_T(infantry_move_pain1) = { FRAME_pain101, FRAME_pain110, infantry_frames_pain1, infantry_run };

static const mframe_t infantry_frames_pain2[] = {
    { ai_move, -3 },
    { ai_move, -3 },
    { ai_move },
    { ai_move, -1 },
    { ai_move, -2, monster_footstep },
    { ai_move },
    { ai_move },
    { ai_move, 2 },
    { ai_move, 5 },
    { ai_move, 2, monster_footstep }
};
const mmove_t MMOVE_T(infantry_move_pain2) = { FRAME_pain201, FRAME_pain210, infantry_frames_pain2, infantry_run };

const mmove_t infantry_move_jump;
const mmove_t infantry_move_jump2;

void PAIN(infantry_pain)(edict_t *self, edict_t *other, float kick, int damage, mod_t mod)
{
    int n;

    // allow turret to pain
    if ((self->monsterinfo.active_move == &infantry_move_jump ||
         self->monsterinfo.active_move == &infantry_move_jump2) && self->think == monster_think)
        return;

    monster_done_dodge(self);

    if (level.time < self->pain_debounce_time) {
        if (self->think == monster_think && frandom() < 0.33f)
            self->monsterinfo.dodge(self, other, FRAME_TIME, NULL, false);
        return;
    }

    self->pain_debounce_time = level.time + SEC(3);

    n = brandom();
    if (n == 0)
        gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
    else
        gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

    if (self->think != monster_think)
        return;

    if (!M_ShouldReactToPain(self, mod)) {
        if (self->think == monster_think && frandom() < 0.33f)
            self->monsterinfo.dodge(self, other, FRAME_TIME, NULL, false);
        return; // no pain anims in nightmare
    }

    if (n == 0)
        M_SetAnimation(self, &infantry_move_pain1);
    else
        M_SetAnimation(self, &infantry_move_pain2);

    // PMM - clear duck flag
    if (self->monsterinfo.aiflags & AI_DUCKED)
        monster_duck_up(self);
}

void MONSTERINFO_SETSKIN(infantry_setskin)(edict_t *self)
{
    if (self->health < (self->max_health / 2))
        self->s.skinnum = 1;
    else
        self->s.skinnum = 0;
}

static const vec3_t aimangles[] = {
    { 0.0f, 5.0f, 0.0f },
    { 10.0f, 15.0f, 0.0f },
    { 20.0f, 25.0f, 0.0f },
    { 25.0f, 35.0f, 0.0f },
    { 30.0f, 40.0f, 0.0f },
    { 30.0f, 45.0f, 0.0f },
    { 25.0f, 50.0f, 0.0f },
    { 20.0f, 40.0f, 0.0f },
    { 15.0f, 35.0f, 0.0f },
    { 40.0f, 35.0f, 0.0f },
    { 70.0f, 35.0f, 0.0f },
    { 90.0f, 35.0f, 0.0f }
};

static void InfantryMachineGun(edict_t *self)
{
    vec3_t                   start;
    vec3_t                   forward, right;
    vec3_t                   vec;
    monster_muzzleflash_id_t flash_number;

    if (!self->enemy || !self->enemy->inuse) // PGM
        return;                              // PGM

    bool is_run_attack = (self->s.frame >= FRAME_run201 && self->s.frame <= FRAME_run208);

    if (self->s.frame == FRAME_attak103 || self->s.frame == FRAME_attak311 || is_run_attack || self->s.frame == FRAME_attak416) {
        if (is_run_attack)
            flash_number = MZ2_INFANTRY_MACHINEGUN_14 + (self->s.frame - MZ2_INFANTRY_MACHINEGUN_14);
        else if (self->s.frame == FRAME_attak416)
            flash_number = MZ2_INFANTRY_MACHINEGUN_22;
        else
            flash_number = MZ2_INFANTRY_MACHINEGUN_1;
        AngleVectors(self->s.angles, forward, right, NULL);
        M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right, start);

        if (self->enemy)
            PredictAim(self, self->enemy, start, 0, true, -0.2f, forward, NULL);
        else
            AngleVectors(self->s.angles, forward, right, NULL);
    } else {
        flash_number = MZ2_INFANTRY_MACHINEGUN_2 + (self->s.frame - FRAME_death211);

        AngleVectors(self->s.angles, forward, right, NULL);
        M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right, start);

        VectorSubtract(self->s.angles, aimangles[flash_number - MZ2_INFANTRY_MACHINEGUN_2], vec);
        AngleVectors(vec, forward, NULL, NULL);
    }

    monster_fire_bullet(self, start, forward, 3, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
}

void MONSTERINFO_SIGHT(infantry_sight)(edict_t *self, edict_t *other)
{
    if (brandom())
        gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
    else
        gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

static void infantry_dead(edict_t *self)
{
    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, -8);
    monster_dead(self);
}

static void infantry_shrink(edict_t *self)
{
    self->maxs[2] = 0;
    self->svflags |= SVF_DEADMONSTER;
    gi.linkentity(self);
}

static void infantry_shrink2(edict_t *self)
{
    infantry_shrink(self);
    monster_footstep(self);
}

static const mframe_t infantry_frames_death1[] = {
    { ai_move, -4 },
    { ai_move },
    { ai_move },
    { ai_move, -1 },
    { ai_move, -4, monster_footstep },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, -1, monster_footstep },
    { ai_move, 3 },
    { ai_move, 1 },
    { ai_move, 1 },
    { ai_move, -2 },
    { ai_move, 2 },
    { ai_move, 2 },
    { ai_move, 9, infantry_shrink2 },
    { ai_move, 9 },
    { ai_move, 5, monster_footstep },
    { ai_move, -3 },
    { ai_move, -3 }
};
const mmove_t MMOVE_T(infantry_move_death1) = { FRAME_death101, FRAME_death120, infantry_frames_death1, infantry_dead };

// Off with his head
static const mframe_t infantry_frames_death2[] = {
    { ai_move },
    { ai_move, 1 },
    { ai_move, 5 },
    { ai_move, -1 },
    { ai_move },
    { ai_move, 1, monster_footstep },
    { ai_move, 1, monster_footstep },
    { ai_move, 4 },
    { ai_move, 3 },
    { ai_move },
    { ai_move, -2, InfantryMachineGun },
    { ai_move, -2, InfantryMachineGun },
    { ai_move, -3, InfantryMachineGun },
    { ai_move, -1, InfantryMachineGun },
    { ai_move, -2, InfantryMachineGun },
    { ai_move, 0, InfantryMachineGun },
    { ai_move, 2, InfantryMachineGun },
    { ai_move, 2, InfantryMachineGun },
    { ai_move, 3, InfantryMachineGun },
    { ai_move, -10, InfantryMachineGun },
    { ai_move, -7, InfantryMachineGun },
    { ai_move, -8, InfantryMachineGun },
    { ai_move, -6, infantry_shrink2 },
    { ai_move, 4 },
    { ai_move }
};
const mmove_t MMOVE_T(infantry_move_death2) = { FRAME_death201, FRAME_death225, infantry_frames_death2, infantry_dead };

static void infantry_skipframe(edict_t *self)
{
    self->monsterinfo.nextframe = self->s.frame + 1;
}

static const mframe_t infantry_frames_death3[] = {
    { ai_move, 0 },
    { ai_move },
    { ai_move, 0, infantry_shrink2 },
    { ai_move, -6 },
    { ai_move, -11, infantry_skipframe },
    { ai_move, -3 }, // skipped frame
    { ai_move, -11 },
    { ai_move, 0, monster_footstep },
    { ai_move }
};
const mmove_t MMOVE_T(infantry_move_death3) = { FRAME_death301, FRAME_death309, infantry_frames_death3, infantry_dead };

static const gib_def_t infantry_gibs[] = {
    { "models/objects/gibs/bone/tris.md2", 1 },
    { "models/objects/gibs/sm_meat/tris.md2", 3 },
    { "models/monsters/infantry/gibs/chest.md2", 1, GIB_SKINNED },
    { "models/monsters/infantry/gibs/gun.md2", 1, GIB_SKINNED | GIB_UPRIGHT },
    { "models/monsters/infantry/gibs/foot.md2", 2, GIB_SKINNED },
    { "models/monsters/infantry/gibs/arm.md2", 2, GIB_SKINNED },
    { "models/monsters/infantry/gibs/head.md2" }, // precache only
    { 0 }
};

void DIE(infantry_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    int n;

    // check for gib
    if (M_CheckGib(self, mod)) {
        gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

        self->s.skinnum /= 2;

        ThrowGibs(self, damage, infantry_gibs);

        if (self->monsterinfo.active_move != &infantry_move_death3)
            ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_HEAD | GIB_SKINNED, self->x.scale);
        else
            ThrowGib(self, "models/monsters/infantry/gibs/head.md2", damage, GIB_HEAD | GIB_SKINNED, self->x.scale);

        self->deadflag = true;
        return;
    }

    if (self->deadflag)
        return;

    // regular death
    self->deadflag = true;
    self->takedamage = true;

    n = irandom1(3);

    if (n == 0) {
        M_SetAnimation(self, &infantry_move_death1);
        gi.sound(self, CHAN_VOICE, sound_die2, 1, ATTN_NORM, 0);
    } else if (n == 1) {
        M_SetAnimation(self, &infantry_move_death2);
        gi.sound(self, CHAN_VOICE, sound_die1, 1, ATTN_NORM, 0);
    } else {
        M_SetAnimation(self, &infantry_move_death3);
        gi.sound(self, CHAN_VOICE, sound_die2, 1, ATTN_NORM, 0);
    }

    // don't always pop a head gib, it gets old
    if (n != 2 && frandom() <= 0.25f) {
        edict_t *head = ThrowGib(self, "models/monsters/infantry/gibs/head.md2", damage, GIB_NONE, self->x.scale);

        if (head) {
            VectorCopy(self->s.angles, head->s.angles);
            VectorCopy(self->s.origin, head->s.origin);
            head->s.origin[2] += 32;
            vec3_t dir;
            VectorSubtract(self->s.origin, inflictor->s.origin, dir);
            VectorNormalize(dir);
            VectorScale(dir, 100, head->velocity);
            head->velocity[2] = 200;
            VectorScale(head->avelocity, 0.15f, head->avelocity);
            head->s.skinnum = 0;
            gi.linkentity(head);
        }
    }
}

static const mframe_t infantry_frames_duck[] = {
    { ai_move, -2, monster_duck_down },
    { ai_move, -5, monster_duck_hold },
    { ai_move, 3 },
    { ai_move, 4, monster_duck_up },
    { ai_move }
};
const mmove_t MMOVE_T(infantry_move_duck) = { FRAME_duck01, FRAME_duck05, infantry_frames_duck, infantry_run };

// PMM - dodge code moved below so I can see the attack frames

const mmove_t infantry_move_attack4;

static void infantry_set_firetime(edict_t *self)
{
    self->monsterinfo.fire_wait = level.time + random_time_sec(0.7f, 2);

    if (!(self->monsterinfo.aiflags & AI_STAND_GROUND) && self->enemy && range_to(self, self->enemy) >= RANGE_RUN_ATTACK && ai_check_move(self, 8.0f))
        M_SetAnimationEx(self, &infantry_move_attack4, false);
}

static void infantry_cock_gun(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_weapon_cock, 1, ATTN_NORM, 0);

    // gun cocked
    self->count = 1;
}

static void infantry_fire(edict_t *self);

static void infantry_set_firetime2(edict_t *self)
{
    infantry_set_firetime(self);
    monster_footstep(self);
}

static void infantry_skip114(edict_t *self)
{
    self->monsterinfo.nextframe = FRAME_attak114;
    monster_footstep(self);
}

// cock-less attack, used if he has already cocked his gun
static const mframe_t infantry_frames_attack1[] = {
    { ai_charge },
    { ai_charge, 6, infantry_set_firetime2 },
    { ai_charge, 0, infantry_fire },
    { ai_charge },
    { ai_charge, 1 },
    { ai_charge, -7 },
    { ai_charge, -6, infantry_skip114 },
    // dead frames start
    { ai_charge, -1 },
    { ai_charge, 0, infantry_cock_gun },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    // dead frames end
    { ai_charge, -1 },
    { ai_charge, -1 }
};
const mmove_t MMOVE_T(infantry_move_attack1) = { FRAME_attak101, FRAME_attak115, infantry_frames_attack1, infantry_run };

// old animation, full cock + shoot
static const mframe_t infantry_frames_attack3[] = {
    { ai_charge, 4,  NULL },
    { ai_charge, -1, NULL },
    { ai_charge, -1, NULL },
    { ai_charge, 0,  infantry_cock_gun },
    { ai_charge, -1, NULL },
    { ai_charge, 1,  NULL },
    { ai_charge, 1,  NULL },
    { ai_charge, 2,  NULL },
    { ai_charge, -2, NULL },
    { ai_charge, -3, infantry_set_firetime2 },
    { ai_charge, 1,  infantry_fire },
    { ai_charge, 5,  NULL },
    { ai_charge, -1, NULL },
    { ai_charge, -2, NULL },
    { ai_charge, -3, NULL },
};
const mmove_t MMOVE_T(infantry_move_attack3) = { FRAME_attak301, FRAME_attak315, infantry_frames_attack3, infantry_run };

// even older animation, full cock + shoot
static const mframe_t infantry_frames_attack5[] = {
    // skipped frames
    { ai_charge, 0, NULL },
    { ai_charge, 0, NULL },
    { ai_charge, 0, NULL },
    { ai_charge, 0, NULL },

    { ai_charge, 0, NULL },
    { ai_charge, 0, NULL },
    { ai_charge, 0, monster_footstep },
    { ai_charge, 0, infantry_cock_gun },
    { ai_charge, 0, NULL },
    { ai_charge, 0, NULL },
    { ai_charge, 0, infantry_skipframe },
    { ai_charge, 0, NULL }, // skipped frame
    { ai_charge, 0, NULL },
    { ai_charge, 0, NULL },
    { ai_charge, 0, infantry_set_firetime },
    { ai_charge, 0, infantry_fire },

    // skipped frames
    { ai_charge, 0, NULL },
    { ai_charge, 0, NULL },
    { ai_charge, 0, NULL },

    { ai_charge, 0, NULL },
    { ai_charge, 0, NULL },
    { ai_charge, 0, NULL },
    { ai_charge, 0, monster_footstep }
};
const mmove_t MMOVE_T(infantry_move_attack5) = { FRAME_attak401, FRAME_attak423, infantry_frames_attack5, infantry_run };

static void infantry_fire(edict_t *self)
{
    InfantryMachineGun(self);

    // we fired, so we must cock again before firing
    self->count = 0;

    // check if we ran out of firing time
    if (self->monsterinfo.active_move == &infantry_move_attack4) {
        if (level.time >= self->monsterinfo.fire_wait) {
            monster_done_dodge(self);
            M_SetAnimationEx(self, &infantry_move_attack1, false);
            self->monsterinfo.nextframe = FRAME_attak114;
        // got close to an edge
        } else if (!ai_check_move(self, 8.0f)) {
            M_SetAnimationEx(self, &infantry_move_attack1, false);
            self->monsterinfo.nextframe = FRAME_attak103;
            monster_done_dodge(self);
            self->monsterinfo.attack_state = AS_STRAIGHT;
        }
    } else if ((self->s.frame >= FRAME_attak101 && self->s.frame <= FRAME_attak115) ||
               (self->s.frame >= FRAME_attak301 && self->s.frame <= FRAME_attak315) ||
               (self->s.frame >= FRAME_attak401 && self->s.frame <= FRAME_attak424)) {
        if (level.time >= self->monsterinfo.fire_wait) {
            self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;

            if (self->s.frame == FRAME_attak416)
                self->monsterinfo.nextframe = FRAME_attak420;
        } else
            self->monsterinfo.aiflags |= AI_HOLD_FRAME;
    }
}

static void infantry_swing(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_punch_swing, 1, ATTN_NORM, 0);
}

static void infantry_smack(edict_t *self)
{
    vec3_t aim = { MELEE_DISTANCE, 0, 0 };

    if (fire_hit(self, aim, irandom2(5, 10), 50))
        gi.sound(self, CHAN_WEAPON, sound_punch_hit, 1, ATTN_NORM, 0);
    else
        self->monsterinfo.melee_debounce_time = level.time + SEC(1.5f);
}

static const mframe_t infantry_frames_attack2[] = {
    { ai_charge, 3 },
    { ai_charge, 6 },
    { ai_charge, 0, infantry_swing },
    { ai_charge, 8, monster_footstep },
    { ai_charge, 5 },
    { ai_charge, 8, infantry_smack },
    { ai_charge, 6 },
    { ai_charge, 3 }
};
const mmove_t MMOVE_T(infantry_move_attack2) = { FRAME_attak201, FRAME_attak208, infantry_frames_attack2, infantry_run };

// [Paril-KEX] run-attack, inspired by q2test
static void infantry_attack4_refire(edict_t *self)
{
    // ran out of firing time
    if (level.time >= self->monsterinfo.fire_wait) {
        monster_done_dodge(self);
        M_SetAnimationEx(self, &infantry_move_attack1, false);
        self->monsterinfo.nextframe = FRAME_attak114;
    // we got too close, or we can't move forward, switch us back to regular attack
    } else if ((self->monsterinfo.aiflags & AI_STAND_GROUND) || (self->enemy && (range_to(self, self->enemy) < RANGE_RUN_ATTACK || !ai_check_move(self, 8.0f)))) {
        M_SetAnimationEx(self, &infantry_move_attack1, false);
        self->monsterinfo.nextframe = FRAME_attak103;
        monster_done_dodge(self);
        self->monsterinfo.attack_state = AS_STRAIGHT;
    } else
        self->monsterinfo.nextframe = FRAME_run201;

    infantry_fire(self);
}

static void infantry_fire2(edict_t *self)
{
    monster_footstep(self);
    infantry_fire(self);
}

static const mframe_t infantry_frames_attack4[] = {
    { ai_charge, 16, infantry_fire },
    { ai_charge, 16, infantry_fire2 },
    { ai_charge, 13, infantry_fire },
    { ai_charge, 10, infantry_fire },
    { ai_charge, 16, infantry_fire },
    { ai_charge, 16, infantry_fire2 },
    { ai_charge, 16, infantry_fire },
    { ai_charge, 16, infantry_attack4_refire }
};
const mmove_t MMOVE_T(infantry_move_attack4) = { FRAME_run201, FRAME_run208, infantry_frames_attack4, infantry_run, 0.5f };

void MONSTERINFO_ATTACK(infantry_attack)(edict_t *self)
{
    monster_done_dodge(self);

    float r = range_to(self, self->enemy);

    if (r <= RANGE_MELEE && self->monsterinfo.melee_debounce_time <= level.time)
        M_SetAnimation(self, &infantry_move_attack2);
    else if (M_CheckClearShot(self, monster_flash_offset[MZ2_INFANTRY_MACHINEGUN_1])) {
        if (self->count)
            M_SetAnimation(self, &infantry_move_attack1);
        else {
            M_SetAnimation(self, frandom() <= 0.1f ? &infantry_move_attack5 : &infantry_move_attack3);
            self->monsterinfo.nextframe = FRAME_attak405;
        }
    }
}

//===========
// PGM
static void infantry_jump_now(edict_t *self)
{
    vec3_t forward, up;

    AngleVectors(self->s.angles, forward, NULL, up);
    VectorMA(self->velocity, 100, forward, self->velocity);
    VectorMA(self->velocity, 300, up, self->velocity);
}

static void infantry_jump2_now(edict_t *self)
{
    vec3_t forward, up;

    AngleVectors(self->s.angles, forward, NULL, up);
    VectorMA(self->velocity, 150, forward, self->velocity);
    VectorMA(self->velocity, 400, up, self->velocity);
}

static void infantry_jump_wait_land(edict_t *self)
{
    if (self->groundentity == NULL) {
        self->monsterinfo.nextframe = self->s.frame;

        if (monster_jump_finished(self))
            self->monsterinfo.nextframe = self->s.frame + 1;
    } else
        self->monsterinfo.nextframe = self->s.frame + 1;
}

static const mframe_t infantry_frames_jump[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, infantry_jump_now },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, infantry_jump_wait_land },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(infantry_move_jump) = { FRAME_jump01, FRAME_jump10, infantry_frames_jump, infantry_run };

static const mframe_t infantry_frames_jump2[] = {
    { ai_move, -8 },
    { ai_move, -4 },
    { ai_move, -4 },
    { ai_move, 0, infantry_jump2_now },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, infantry_jump_wait_land },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(infantry_move_jump2) = { FRAME_jump01, FRAME_jump10, infantry_frames_jump2, infantry_run };

static void infantry_jump(edict_t *self, blocked_jump_result_t result)
{
    if (!self->enemy)
        return;

    monster_done_dodge(self);

    if (result == JUMP_JUMP_UP)
        M_SetAnimation(self, &infantry_move_jump2);
    else
        M_SetAnimation(self, &infantry_move_jump);
}

bool MONSTERINFO_BLOCKED(infantry_blocked)(edict_t *self, float dist)
{
    blocked_jump_result_t result = blocked_checkjump(self, dist);
    if (result != NO_JUMP) {
        if (result != JUMP_TURN)
            infantry_jump(self, result);
        return true;
    }

    if (blocked_checkplat(self, dist))
        return true;

    return false;
}

bool MONSTERINFO_DUCK(infantry_duck)(edict_t *self, gtime_t eta)
{
    // if we're jumping, don't dodge
    if ((self->monsterinfo.active_move == &infantry_move_jump) ||
        (self->monsterinfo.active_move == &infantry_move_jump2))
        return false;

    // don't duck during our firing or melee frames
    if (self->s.frame == FRAME_attak103 ||
        self->s.frame == FRAME_attak315 ||
        (self->monsterinfo.active_move == &infantry_move_attack2)) {
        self->monsterinfo.unduck(self);
        return false;
    }

    M_SetAnimation(self, &infantry_move_duck);

    return true;
}

bool MONSTERINFO_SIDESTEP(infantry_sidestep)(edict_t *self)
{
    // if we're jumping, don't dodge
    if ((self->monsterinfo.active_move == &infantry_move_jump) ||
        (self->monsterinfo.active_move == &infantry_move_jump2))
        return false;

    if (self->monsterinfo.active_move == &infantry_move_run)
        return true;

    // Don't sidestep if we're already sidestepping, and def not unless we're actually shooting
    // or if we already cocked
    if (self->monsterinfo.active_move != &infantry_move_attack4 &&
        self->monsterinfo.next_move != &infantry_move_attack4 &&
        ((self->s.frame == FRAME_attak103 ||
          self->s.frame == FRAME_attak311 ||
          self->s.frame == FRAME_attak416) &&
         !self->count)) {
        // give us a fire time boost so we don't end up firing for 1 frame
        self->monsterinfo.fire_wait += random_time_sec(0.3f, 0.6f);

        M_SetAnimationEx(self, &infantry_move_attack4, false);
    }

    return true;
}

void InfantryPrecache(void)
{
    sound_pain1 = gi.soundindex("infantry/infpain1.wav");
    sound_pain2 = gi.soundindex("infantry/infpain2.wav");
    sound_die1 = gi.soundindex("infantry/infdeth1.wav");
    sound_die2 = gi.soundindex("infantry/infdeth2.wav");

    sound_gunshot = gi.soundindex("infantry/infatck1.wav");
    sound_weapon_cock = gi.soundindex("infantry/infatck3.wav");
    sound_punch_swing = gi.soundindex("infantry/infatck2.wav");
    sound_punch_hit = gi.soundindex("infantry/melee2.wav");

    sound_sight = gi.soundindex("infantry/infsght1.wav");
    sound_search = gi.soundindex("infantry/infsrch1.wav");
    sound_idle = gi.soundindex("infantry/infidle1.wav");
}

#define SPAWNFLAG_INFANTRY_NOJUMPING    8

/*QUAKED monster_infantry (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight NoJumping
 */
void SP_monster_infantry(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    G_AddPrecache(InfantryPrecache);

    self->monsterinfo.aiflags |= AI_STINKY;

    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;
    self->s.modelindex = gi.modelindex("models/monsters/infantry/tris.md2");

    PrecacheGibs(infantry_gibs);

    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, 32);

    self->health = 100 * st.health_multiplier;
    self->gib_health = -65;
    self->mass = 200;

    self->pain = infantry_pain;
    self->die = infantry_die;

    self->monsterinfo.combat_style = COMBAT_MIXED;

    self->monsterinfo.stand = infantry_stand;
    self->monsterinfo.walk = infantry_walk;
    self->monsterinfo.run = infantry_run;
    // pmm
    self->monsterinfo.dodge = M_MonsterDodge;
    self->monsterinfo.duck = infantry_duck;
    self->monsterinfo.unduck = monster_duck_up;
    self->monsterinfo.sidestep = infantry_sidestep;
    self->monsterinfo.blocked = infantry_blocked;
    // pmm
    self->monsterinfo.attack = infantry_attack;
    self->monsterinfo.melee = NULL;
    self->monsterinfo.sight = infantry_sight;
    self->monsterinfo.idle = infantry_fidget;
    self->monsterinfo.setskin = infantry_setskin;

    gi.linkentity(self);

    M_SetAnimation(self, &infantry_move_stand);
    self->monsterinfo.scale = MODEL_SCALE;
    self->monsterinfo.can_jump = !(self->spawnflags & SPAWNFLAG_INFANTRY_NOJUMPING);
    self->monsterinfo.drop_height = 192;
    self->monsterinfo.jump_height = 40;

    walkmonster_start(self);
}
