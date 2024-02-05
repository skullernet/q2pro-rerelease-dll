// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

GUNNER

==============================================================================
*/

#include "g_local.h"
#include "m_gunner.h"

static int sound_pain;
static int sound_pain2;
static int sound_death;
static int sound_idle;
static int sound_open;
static int sound_search;
static int sound_sight;

static void gunner_idlesound(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

void MONSTERINFO_SIGHT(gunner_sight)(edict_t *self, edict_t *other)
{
    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void MONSTERINFO_SEARCH(gunner_search)(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

static void GunnerGrenade(edict_t *self);
static void GunnerFire(edict_t *self);
static void gunner_fire_chain(edict_t *self);
static void gunner_refire_chain(edict_t *self);

void gunner_stand(edict_t *self);

static const mframe_t gunner_frames_fidget[] = {
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand, 0, gunner_idlesound },
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
const mmove_t MMOVE_T(gunner_move_fidget) = { FRAME_stand31, FRAME_stand70, gunner_frames_fidget, gunner_stand };

static void gunner_fidget(edict_t *self)
{
    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        return;
    if (self->enemy)
        return;
    if (frandom() <= 0.05f)
        M_SetAnimation(self, &gunner_move_fidget);
}

static const mframe_t gunner_frames_stand[] = {
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand, 0, gunner_fidget },

    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand, 0, gunner_fidget },

    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand, 0, gunner_fidget }
};
const mmove_t MMOVE_T(gunner_move_stand) = { FRAME_stand01, FRAME_stand30, gunner_frames_stand, NULL };

void MONSTERINFO_STAND(gunner_stand)(edict_t *self)
{
    M_SetAnimation(self, &gunner_move_stand);
}

static const mframe_t gunner_frames_walk[] = {
    { ai_walk },
    { ai_walk, 3 },
    { ai_walk, 4 },
    { ai_walk, 5 },
    { ai_walk, 7 },
    { ai_walk, 2, monster_footstep },
    { ai_walk, 6 },
    { ai_walk, 4 },
    { ai_walk, 2 },
    { ai_walk, 7 },
    { ai_walk, 5 },
    { ai_walk, 7 },
    { ai_walk, 4, monster_footstep }
};
const mmove_t MMOVE_T(gunner_move_walk) = { FRAME_walk07, FRAME_walk19, gunner_frames_walk, NULL };

void MONSTERINFO_WALK(gunner_walk)(edict_t *self)
{
    M_SetAnimation(self, &gunner_move_walk);
}

static const mframe_t gunner_frames_run[] = {
    { ai_run, 26 },
    { ai_run, 9, monster_footstep },
    { ai_run, 9 },
    { ai_run, 9, monster_done_dodge },
    { ai_run, 15 },
    { ai_run, 10, monster_footstep },
    { ai_run, 13 },
    { ai_run, 6 }
};

const mmove_t MMOVE_T(gunner_move_run) = { FRAME_run01, FRAME_run08, gunner_frames_run, NULL };

void MONSTERINFO_RUN(gunner_run)(edict_t *self)
{
    monster_done_dodge(self);
    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        M_SetAnimation(self, &gunner_move_stand);
    else
        M_SetAnimation(self, &gunner_move_run);
}

#if 0
static const mframe_t gunner_frames_runandshoot[] = {
    { ai_run, 32 },
    { ai_run, 15 },
    { ai_run, 10 },
    { ai_run, 18 },
    { ai_run, 8 },
    { ai_run, 20 }
};

const mmove_t MMOVE_T(gunner_move_runandshoot) = { FRAME_runs01, FRAME_runs06, gunner_frames_runandshoot, NULL };

static void gunner_runandshoot(edict_t *self)
{
    M_SetAnimation(self, &gunner_move_runandshoot);
}
#endif

static const mframe_t gunner_frames_pain3[] = {
    { ai_move, -3 },
    { ai_move, 1 },
    { ai_move, 1 },
    { ai_move },
    { ai_move, 1 }
};
const mmove_t MMOVE_T(gunner_move_pain3) = { FRAME_pain301, FRAME_pain305, gunner_frames_pain3, gunner_run };

static const mframe_t gunner_frames_pain2[] = {
    { ai_move, -2 },
    { ai_move, 11 },
    { ai_move, 6, monster_footstep },
    { ai_move, 2 },
    { ai_move, -1 },
    { ai_move, -7 },
    { ai_move, -2 },
    { ai_move, -7, monster_footstep }
};
const mmove_t MMOVE_T(gunner_move_pain2) = { FRAME_pain201, FRAME_pain208, gunner_frames_pain2, gunner_run };

static const mframe_t gunner_frames_pain1[] = {
    { ai_move, 2 },
    { ai_move },
    { ai_move, -5 },
    { ai_move, 3 },
    { ai_move, -1, monster_footstep },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 1 },
    { ai_move, 1 },
    { ai_move, 2 },
    { ai_move, 1, monster_footstep },
    { ai_move },
    { ai_move, -2 },
    { ai_move, -2 },
    { ai_move },
    { ai_move, 0, monster_footstep }
};
const mmove_t MMOVE_T(gunner_move_pain1) = { FRAME_pain101, FRAME_pain118, gunner_frames_pain1, gunner_run };

const mmove_t gunner_move_jump;
const mmove_t gunner_move_jump2;

void PAIN(gunner_pain)(edict_t *self, edict_t *other, float kick, int damage, const mod_t mod)
{
    monster_done_dodge(self);

    if (self->monsterinfo.active_move == &gunner_move_jump ||
        self->monsterinfo.active_move == &gunner_move_jump2)
        return;

    if (level.time < self->pain_debounce_time)
        return;

    self->pain_debounce_time = level.time + SEC(3);

    if (brandom())
        gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
    else
        gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

    if (!M_ShouldReactToPain(self, mod))
        return; // no pain anims in nightmare

    if (damage <= 10)
        M_SetAnimation(self, &gunner_move_pain3);
    else if (damage <= 25)
        M_SetAnimation(self, &gunner_move_pain2);
    else
        M_SetAnimation(self, &gunner_move_pain1);

    self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;

    // PMM - clear duck flag
    if (self->monsterinfo.aiflags & AI_DUCKED)
        monster_duck_up(self);
}

void MONSTERINFO_SETSKIN(gunner_setskin)(edict_t *self)
{
    if (self->health < (self->max_health / 2))
        self->s.skinnum = 1;
    else
        self->s.skinnum = 0;
}

static void gunner_dead(edict_t *self)
{
    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, -8);
    monster_dead(self);
}

static void gunner_shrink(edict_t *self)
{
    self->maxs[2] = -4;
    self->svflags |= SVF_DEADMONSTER;
    gi.linkentity(self);
}

static const mframe_t gunner_frames_death[] = {
    { ai_move },
    { ai_move },
    { ai_move, 0, monster_footstep },
    { ai_move, -7, gunner_shrink },
    { ai_move, -3 },
    { ai_move, -5 },
    { ai_move, 8 },
    { ai_move, 6 },
    { ai_move, 0, monster_footstep },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(gunner_move_death) = { FRAME_death01, FRAME_death11, gunner_frames_death, gunner_dead };

static const gib_def_t gunner_gibs[] = {
    { "models/objects/gibs/bone/tris.md2", 2 },
    { "models/objects/gibs/sm_meat/tris.md2", 2 },
    { "models/monsters/gunner/gibs/chest.md2", 1, GIB_SKINNED },
    { "models/monsters/gunner/gibs/garm.md2", 1, GIB_SKINNED | GIB_UPRIGHT },
    { "models/monsters/gunner/gibs/gun.md2", 1, GIB_SKINNED | GIB_UPRIGHT },
    { "models/monsters/gunner/gibs/foot.md2", 1, GIB_SKINNED },
    { "models/monsters/gunner/gibs/head.md2", 1, GIB_SKINNED | GIB_HEAD },
    { 0 }
};

void DIE(gunner_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, const mod_t mod)
{
    // check for gib
    if (M_CheckGib(self, mod)) {
        gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
        self->s.skinnum /= 2;
        ThrowGibs(self, damage, gunner_gibs);
        self->deadflag = true;
        return;
    }

    if (self->deadflag)
        return;

    // regular death
    gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
    self->deadflag = true;
    self->takedamage = true;
    M_SetAnimation(self, &gunner_move_death);
}

// PMM - changed to duck code for new dodge

static const mframe_t gunner_frames_duck[] = {
    { ai_move, 1, monster_duck_down },
    { ai_move, 1 },
    { ai_move, 1, monster_duck_hold },
    { ai_move },
    { ai_move, -1 },
    { ai_move, -1 },
    { ai_move, 0, monster_duck_up },
    { ai_move, -1 }
};
const mmove_t MMOVE_T(gunner_move_duck) = { FRAME_duck01, FRAME_duck08, gunner_frames_duck, gunner_run };

// PMM - gunner dodge moved below so I know about attack sequences

static void gunner_opengun(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_open, 1, ATTN_IDLE, 0);
}

static void GunnerFire(edict_t *self)
{
    vec3_t                   start;
    vec3_t                   forward, right;
    vec3_t                   aim;
    monster_muzzleflash_id_t flash_number;

    if (!self->enemy || !self->enemy->inuse) // PGM
        return;                              // PGM

    flash_number = MZ2_GUNNER_MACHINEGUN_1 + (self->s.frame - FRAME_attak216);

    AngleVectors(self->s.angles, forward, right, NULL);
    M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right, start);
    PredictAim(self, self->enemy, start, 0, true, -0.2f, aim, NULL);
    monster_fire_bullet(self, start, aim, 3, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
}

static bool gunner_grenade_check(edict_t *self)
{
    if (!self->enemy)
        return false;

    vec3_t start;

    if (!M_CheckClearShotEx(self, monster_flash_offset[MZ2_GUNNER_GRENADE_1], start))
        return false;

    vec3_t target;

    // check for flag telling us that we're blindfiring
    if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
        VectorCopy(self->monsterinfo.blind_fire_target, target);
    else
        VectorCopy(self->enemy->s.origin, target);

    vec3_t aim;

    // see if we're too close
    VectorSubtract(target, start, aim);

    if (VectorNormalize(aim) < 100)
        return false;

    // check to see that we can trace to the player before we start
    // tossing grenades around.
    return M_CalculatePitchToFire(self, target, start, aim, 600, 2.5f, false, false);
}

static void GunnerGrenade(edict_t *self)
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

    if (self->s.frame == FRAME_attak105 || self->s.frame == FRAME_attak309) {
        spread = -0.10f;
        flash_number = MZ2_GUNNER_GRENADE_1;
    } else if (self->s.frame == FRAME_attak108 || self->s.frame == FRAME_attak312) {
        spread = -0.05f;
        flash_number = MZ2_GUNNER_GRENADE_2;
    } else if (self->s.frame == FRAME_attak111 || self->s.frame == FRAME_attak315) {
        spread = 0.05f;
        flash_number = MZ2_GUNNER_GRENADE_3;
    } else { // (self->s.frame == FRAME_attak114)
        self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
        spread = 0.10f;
        flash_number = MZ2_GUNNER_GRENADE_4;
    }

    // what the fuck?
    if (self->s.frame >= FRAME_attak301 && self->s.frame <= FRAME_attak324)
        flash_number = MZ2_GUNNER_GRENADE2_1 + (MZ2_GUNNER_GRENADE_4 - flash_number);

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
    if (self->enemy) {
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
    }
    // PGM

    VectorMA(forward, spread, right, aim);
    VectorMA(aim, pitch, up, aim);

    // try search for best pitch
    if (M_CalculatePitchToFire(self, target, start, aim, 600, 2.5f, false, false))
        monster_fire_grenade(self, start, aim, 50, 600, flash_number, (crandom_open() * 10.0f), frandom() * 10.0f);
    else // normal shot
        monster_fire_grenade(self, start, aim, 50, 600, flash_number, (crandom_open() * 10.0f), 200.0f + (crandom_open() * 10.0f));
}

static const mframe_t gunner_frames_attack_chain[] = {
    { ai_charge, 0, gunner_opengun },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(gunner_move_attack_chain) = { FRAME_attak209, FRAME_attak215, gunner_frames_attack_chain, gunner_fire_chain };

static const mframe_t gunner_frames_fire_chain[] = {
    { ai_charge, 0, GunnerFire },
    { ai_charge, 0, GunnerFire },
    { ai_charge, 0, GunnerFire },
    { ai_charge, 0, GunnerFire },
    { ai_charge, 0, GunnerFire },
    { ai_charge, 0, GunnerFire },
    { ai_charge, 0, GunnerFire },
    { ai_charge, 0, GunnerFire }
};
const mmove_t MMOVE_T(gunner_move_fire_chain) = { FRAME_attak216, FRAME_attak223, gunner_frames_fire_chain, gunner_refire_chain };

static const mframe_t gunner_frames_endfire_chain[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, monster_footstep }
};
const mmove_t MMOVE_T(gunner_move_endfire_chain) = { FRAME_attak224, FRAME_attak230, gunner_frames_endfire_chain, gunner_run };

static void gunner_blind_check(edict_t *self)
{
    if (self->monsterinfo.aiflags & AI_MANUAL_STEERING) {
        vec3_t aim;
        VectorSubtract(self->monsterinfo.blind_fire_target, self->s.origin, aim);
        self->ideal_yaw = vectoyaw(aim);
    }
}

static const mframe_t gunner_frames_attack_grenade[] = {
    { ai_charge, 0, gunner_blind_check },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, GunnerGrenade },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, GunnerGrenade },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, GunnerGrenade },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, GunnerGrenade },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(gunner_move_attack_grenade) = { FRAME_attak101, FRAME_attak121, gunner_frames_attack_grenade, gunner_run };

static const mframe_t gunner_frames_attack_grenade2[] = {
    { ai_charge, 0, gunner_blind_check },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, GunnerGrenade },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, GunnerGrenade },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, GunnerGrenade },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, GunnerGrenade },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(gunner_move_attack_grenade2) = { FRAME_attak305, FRAME_attak324, gunner_frames_attack_grenade2, gunner_run };

void MONSTERINFO_ATTACK(gunner_attack)(edict_t *self)
{
    monster_done_dodge(self);

    // PMM
    if (self->monsterinfo.attack_state == AS_BLIND) {
        float chance;

        if (self->timestamp > level.time)
            return;

        // setup shot probabilities
        if (self->monsterinfo.blind_fire_delay < SEC(1))
            chance = 1.0f;
        else if (self->monsterinfo.blind_fire_delay < SEC(7.5f))
            chance = 0.4f;
        else
            chance = 0.1f;

        // minimum of 4.1 seconds, plus 0-3, after the shots are done
        self->monsterinfo.blind_fire_delay += random_time_sec(4.1f, 7.1f);

        // don't shoot at the origin
        if (VectorEmpty(self->monsterinfo.blind_fire_target))
            return;

        // don't shoot if the dice say not to
        if (frandom() > chance)
            return;

        // turn on manual steering to signal both manual steering and blindfire
        self->monsterinfo.aiflags |= AI_MANUAL_STEERING;

        if (gunner_grenade_check(self)) {
            // if the check passes, go for the attack
            M_SetAnimation(self, brandom() ? &gunner_move_attack_grenade2 : &gunner_move_attack_grenade);
            self->monsterinfo.attack_finished = level.time + random_time_sec(0, 2);
        } else
            // turn off blindfire flag
            self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;

        self->timestamp = level.time + random_time_sec(2, 3);
        return;
    }
    // pmm

    // PGM - gunner needs to use his chaingun if he's being attacked by a tesla.
    if (self->bad_area || self->timestamp > level.time ||
        (range_to(self, self->enemy) <= RANGE_NEAR * 0.35f && M_CheckClearShot(self, monster_flash_offset[MZ2_GUNNER_MACHINEGUN_1]))) {
        M_SetAnimation(self, &gunner_move_attack_chain);
    } else if (self->timestamp <= level.time && brandom() && gunner_grenade_check(self)) {
        M_SetAnimation(self, brandom() ? &gunner_move_attack_grenade2 : &gunner_move_attack_grenade);
        self->timestamp = level.time + random_time_sec(2, 3);
    } else if (M_CheckClearShot(self, monster_flash_offset[MZ2_GUNNER_MACHINEGUN_1]))
        M_SetAnimation(self, &gunner_move_attack_chain);
}

static void gunner_fire_chain(edict_t *self)
{
    M_SetAnimation(self, &gunner_move_fire_chain);
}

static void gunner_refire_chain(edict_t *self)
{
    if (self->enemy->health > 0 && visible(self, self->enemy) && brandom())
        M_SetAnimationEx(self, &gunner_move_fire_chain, false);
    else
        M_SetAnimationEx(self, &gunner_move_endfire_chain, false);
}

//===========
// PGM
static void gunner_jump_now(edict_t *self)
{
    vec3_t forward, up;

    AngleVectors(self->s.angles, forward, NULL, up);
    VectorMA(self->velocity, 100, forward, self->velocity);
    VectorMA(self->velocity, 300, up, self->velocity);
}

static void gunner_jump2_now(edict_t *self)
{
    vec3_t forward, up;

    AngleVectors(self->s.angles, forward, NULL, up);
    VectorMA(self->velocity, 150, forward, self->velocity);
    VectorMA(self->velocity, 400, up, self->velocity);
}

static void gunner_jump_wait_land(edict_t *self)
{
    if (self->groundentity == NULL) {
        self->monsterinfo.nextframe = self->s.frame;

        if (monster_jump_finished(self))
            self->monsterinfo.nextframe = self->s.frame + 1;
    } else
        self->monsterinfo.nextframe = self->s.frame + 1;
}

static const mframe_t gunner_frames_jump[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, gunner_jump_now },
    { ai_move },
    { ai_move },
    { ai_move, 0, gunner_jump_wait_land },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(gunner_move_jump) = { FRAME_jump01, FRAME_jump10, gunner_frames_jump, gunner_run };

static const mframe_t gunner_frames_jump2[] = {
    { ai_move, -8 },
    { ai_move, -4 },
    { ai_move, -4 },
    { ai_move, 0, gunner_jump2_now },
    { ai_move },
    { ai_move },
    { ai_move, 0, gunner_jump_wait_land },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(gunner_move_jump2) = { FRAME_jump01, FRAME_jump10, gunner_frames_jump2, gunner_run };

static void gunner_jump(edict_t *self, blocked_jump_result_t result)
{
    if (!self->enemy)
        return;

    monster_done_dodge(self);

    if (result == JUMP_JUMP_UP)
        M_SetAnimation(self, &gunner_move_jump2);
    else
        M_SetAnimation(self, &gunner_move_jump);
}

//===========
// PGM
bool MONSTERINFO_BLOCKED(gunner_blocked)(edict_t *self, float dist)
{
    if (blocked_checkplat(self, dist))
        return true;

    blocked_jump_result_t result = blocked_checkjump(self, dist);
    if (result != NO_JUMP) {
        if (result != JUMP_TURN)
            gunner_jump(self, result);
        return true;
    }

    return false;
}
// PGM
//===========

// PMM - new duck code
bool MONSTERINFO_DUCK(gunner_duck)(edict_t *self, gtime_t eta)
{
    if ((self->monsterinfo.active_move == &gunner_move_jump2) ||
        (self->monsterinfo.active_move == &gunner_move_jump))
        return false;

    if ((self->monsterinfo.active_move == &gunner_move_attack_chain) ||
        (self->monsterinfo.active_move == &gunner_move_fire_chain) ||
        (self->monsterinfo.active_move == &gunner_move_attack_grenade) ||
        (self->monsterinfo.active_move == &gunner_move_attack_grenade2)) {
        // if we're shooting don't dodge
        self->monsterinfo.unduck(self);
        return false;
    }

    if (brandom())
        GunnerGrenade(self);

    M_SetAnimation(self, &gunner_move_duck);

    return true;
}

bool MONSTERINFO_SIDESTEP(gunner_sidestep)(edict_t *self)
{
    if ((self->monsterinfo.active_move == &gunner_move_jump2) ||
        (self->monsterinfo.active_move == &gunner_move_jump) ||
        (self->monsterinfo.active_move == &gunner_move_pain1))
        return false;

    if ((self->monsterinfo.active_move == &gunner_move_attack_chain) ||
        (self->monsterinfo.active_move == &gunner_move_fire_chain) ||
        (self->monsterinfo.active_move == &gunner_move_attack_grenade) ||
        (self->monsterinfo.active_move == &gunner_move_attack_grenade2)) {
        // if we're shooting, don't dodge
        return false;
    }

    if (self->monsterinfo.active_move != &gunner_move_run)
        M_SetAnimation(self, &gunner_move_run);

    return true;
}

#define SPAWNFLAG_GUNNER_NOJUMPING  8

static void gunner_precache(void)
{
    sound_death = gi.soundindex("gunner/death1.wav");
    sound_pain = gi.soundindex("gunner/gunpain2.wav");
    sound_pain2 = gi.soundindex("gunner/gunpain1.wav");
    sound_idle = gi.soundindex("gunner/gunidle1.wav");
    sound_open = gi.soundindex("gunner/gunatck1.wav");
    sound_search = gi.soundindex("gunner/gunsrch1.wav");
    sound_sight = gi.soundindex("gunner/sight1.wav");
}

/*QUAKED monster_gunner (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight NoJumping
model="models/monsters/gunner/tris.md2"
*/
void SP_monster_gunner(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    G_AddPrecache(gunner_precache);

    gi.soundindex("gunner/gunatck2.wav");
    gi.soundindex("gunner/gunatck3.wav");

    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;
    self->s.modelindex = gi.modelindex("models/monsters/gunner/tris.md2");

    PrecacheGibs(gunner_gibs);

    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, 36);

    self->health = 175 * st.health_multiplier;
    self->gib_health = -70;
    self->mass = 200;

    self->pain = gunner_pain;
    self->die = gunner_die;

    self->monsterinfo.stand = gunner_stand;
    self->monsterinfo.walk = gunner_walk;
    self->monsterinfo.run = gunner_run;
    // pmm
    self->monsterinfo.dodge = M_MonsterDodge;
    self->monsterinfo.duck = gunner_duck;
    self->monsterinfo.unduck = monster_duck_up;
    self->monsterinfo.sidestep = gunner_sidestep;
    self->monsterinfo.blocked = gunner_blocked; // PGM
    // pmm
    self->monsterinfo.attack = gunner_attack;
    self->monsterinfo.melee = NULL;
    self->monsterinfo.sight = gunner_sight;
    self->monsterinfo.search = gunner_search;
    self->monsterinfo.setskin = gunner_setskin;

    gi.linkentity(self);

    M_SetAnimation(self, &gunner_move_stand);
    self->monsterinfo.scale = MODEL_SCALE;

    // PMM
    self->monsterinfo.blindfire = true;
    self->monsterinfo.can_jump = !(self->spawnflags & SPAWNFLAG_GUNNER_NOJUMPING);
    self->monsterinfo.drop_height = 192;
    self->monsterinfo.jump_height = 40;

    walkmonster_start(self);
}
