// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

TANK

==============================================================================
*/

#include "g_local.h"
#include "m_tank.h"

static void tank_refire_rocket(edict_t *self);
static void tank_doattack_rocket(edict_t *self);
static void tank_reattack_blaster(edict_t *self);

static int sound_thud;
static int sound_pain, sound_pain2;
static int sound_idle;
static int sound_die;
static int sound_step;
static int sound_sight;
static int sound_windup;
static int sound_strike;

#define SPAWNFLAG_TANK_COMMANDER_GUARDIAN       8
#define SPAWNFLAG_TANK_COMMANDER_HEAT_SEEKING   16

//
// misc
//

void MONSTERINFO_SIGHT(tank_sight)(edict_t *self, edict_t *other)
{
    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void tank_footstep(edict_t *self)
{
    gi.sound(self, CHAN_BODY, sound_step, 1, ATTN_NORM, 0);
}

static void tank_thud(edict_t *self)
{
    gi.sound(self, CHAN_BODY, sound_thud, 1, ATTN_NORM, 0);
}

static void tank_windup(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_windup, 1, ATTN_NORM, 0);
}

void MONSTERINFO_IDLE(tank_idle)(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

//
// stand
//

static const mframe_t tank_frames_stand[] = {
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
const mmove_t MMOVE_T(tank_move_stand) = { FRAME_stand01, FRAME_stand30, tank_frames_stand, NULL };

void MONSTERINFO_STAND(tank_stand)(edict_t *self)
{
    M_SetAnimation(self, &tank_move_stand);
}

//
// walk
//

#if 0
void tank_walk(edict_t *self);

static const mframe_t tank_frames_start_walk[] = {
    { ai_walk },
    { ai_walk, 6 },
    { ai_walk, 6 },
    { ai_walk, 11, tank_footstep }
};
const mmove_t MMOVE_T(tank_move_start_walk) = { FRAME_walk01, FRAME_walk04, tank_frames_start_walk, tank_walk };
#endif

static const mframe_t tank_frames_walk[] = {
    { ai_walk, 4 },
    { ai_walk, 5 },
    { ai_walk, 3 },
    { ai_walk, 2 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 4 },
    { ai_walk, 4, tank_footstep },
    { ai_walk, 3 },
    { ai_walk, 5 },
    { ai_walk, 4 },
    { ai_walk, 5 },
    { ai_walk, 7 },
    { ai_walk, 7 },
    { ai_walk, 6 },
    { ai_walk, 6, tank_footstep }
};
const mmove_t MMOVE_T(tank_move_walk) = { FRAME_walk05, FRAME_walk20, tank_frames_walk, NULL };

#if 0
static const mframe_t tank_frames_stop_walk[] = {
    { ai_walk, 3 },
    { ai_walk, 3 },
    { ai_walk, 2 },
    { ai_walk, 2 },
    { ai_walk, 4, tank_footstep }
};
const mmove_t MMOVE_T(tank_move_stop_walk) = { FRAME_walk21, FRAME_walk25, tank_frames_stop_walk, tank_stand };
#endif

void MONSTERINFO_WALK(tank_walk)(edict_t *self)
{
    M_SetAnimation(self, &tank_move_walk);
}

//
// run
//

void tank_run(edict_t *self);

static const mframe_t tank_frames_start_run[] = {
    { ai_run },
    { ai_run, 6 },
    { ai_run, 6 },
    { ai_run, 11, tank_footstep }
};
const mmove_t MMOVE_T(tank_move_start_run) = { FRAME_walk01, FRAME_walk04, tank_frames_start_run, tank_run };

static const mframe_t tank_frames_run[] = {
    { ai_run, 4 },
    { ai_run, 5 },
    { ai_run, 3 },
    { ai_run, 2 },
    { ai_run, 5 },
    { ai_run, 5 },
    { ai_run, 4 },
    { ai_run, 4, tank_footstep },
    { ai_run, 3 },
    { ai_run, 5 },
    { ai_run, 4 },
    { ai_run, 5 },
    { ai_run, 7 },
    { ai_run, 7 },
    { ai_run, 6 },
    { ai_run, 6, tank_footstep }
};
const mmove_t MMOVE_T(tank_move_run) = { FRAME_walk05, FRAME_walk20, tank_frames_run, NULL };

#if 0
static const mframe_t tank_frames_stop_run[] = {
    { ai_run, 3 },
    { ai_run, 3 },
    { ai_run, 2 },
    { ai_run, 2 },
    { ai_run, 4, tank_footstep }
};
const mmove_t MMOVE_T(tank_move_stop_run) = { FRAME_walk21, FRAME_walk25, tank_frames_stop_run, tank_walk };
#endif

void MONSTERINFO_RUN(tank_run)(edict_t *self)
{
    if (self->enemy && self->enemy->client)
        self->monsterinfo.aiflags |= AI_BRUTAL;
    else
        self->monsterinfo.aiflags &= ~AI_BRUTAL;

    if (self->monsterinfo.aiflags & AI_STAND_GROUND) {
        M_SetAnimation(self, &tank_move_stand);
        return;
    }

    if (self->monsterinfo.active_move == &tank_move_walk ||
        self->monsterinfo.active_move == &tank_move_start_run)
        M_SetAnimation(self, &tank_move_run);
    else
        M_SetAnimation(self, &tank_move_start_run);
}

//
// pain
//

static const mframe_t tank_frames_pain1[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(tank_move_pain1) = { FRAME_pain101, FRAME_pain104, tank_frames_pain1, tank_run };

static const mframe_t tank_frames_pain2[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(tank_move_pain2) = { FRAME_pain201, FRAME_pain205, tank_frames_pain2, tank_run };

static const mframe_t tank_frames_pain3[] = {
    { ai_move, -7 },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 2 },
    { ai_move },
    { ai_move },
    { ai_move, 3 },
    { ai_move },
    { ai_move, 2 },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, tank_footstep }
};
const mmove_t MMOVE_T(tank_move_pain3) = { FRAME_pain301, FRAME_pain316, tank_frames_pain3, tank_run };

void PAIN(tank_pain)(edict_t *self, edict_t *other, float kick, int damage, const mod_t mod)
{
    if (mod.id != MOD_CHAINFIST && damage <= 10)
        return;

    if (level.time < self->pain_debounce_time)
        return;

    if (mod.id != MOD_CHAINFIST) {
        if (damage <= 30)
            if (frandom() > 0.2f)
                return;

        // don't go into pain while attacking
        if ((self->s.frame >= FRAME_attak301) && (self->s.frame <= FRAME_attak330))
            return;
        if ((self->s.frame >= FRAME_attak101) && (self->s.frame <= FRAME_attak116))
            return;
    }

    self->pain_debounce_time = level.time + SEC(3);

    if (self->count)
        gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
    else
        gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

    if (!M_ShouldReactToPain(self, mod))
        return; // no pain anims in nightmare

    // PMM - blindfire cleanup
    self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
    // pmm

    if (damage <= 30)
        M_SetAnimation(self, &tank_move_pain1);
    else if (damage <= 60)
        M_SetAnimation(self, &tank_move_pain2);
    else
        M_SetAnimation(self, &tank_move_pain3);
}

void MONSTERINFO_SETSKIN(tank_setskin)(edict_t *self)
{
    if (self->health < (self->max_health / 2))
        self->s.skinnum |= 1;
    else
        self->s.skinnum &= ~1;
}

// [Paril-KEX]
static bool M_AdjustBlindfireTarget(edict_t *self, const vec3_t start, const vec3_t target, const vec3_t right, vec3_t out_dir)
{
    for (int i = 0; i < 3; i++) {
        vec3_t end;

        if (i == 0)
            VectorCopy(target, end);
        else if (i == 1) // try shifting the target to the left a little (to help counter large offset)
            VectorMA(target, -20, right, end);
        else // ok, that failed. try to the right
            VectorMA(target, 20, right, end);

        trace_t trace = gi.trace(start, NULL, NULL, end, self, MASK_PROJECTILE);
        // blindfire has different fail criteria for the trace
        if (!(trace.startsolid || trace.allsolid || (trace.fraction < 0.5f))) {
            VectorSubtract(end, start, out_dir);
            VectorNormalize(out_dir);
            return true;
        }
    }

    return false;
}

//
// attacks
//

static void TankBlaster(edict_t *self)
{
    vec3_t                   forward, right;
    vec3_t                   start;
    vec3_t                   dir;
    monster_muzzleflash_id_t flash_number;

    if (!self->enemy || !self->enemy->inuse) // PGM
        return;                              // PGM

    bool blindfire = self->monsterinfo.aiflags & AI_MANUAL_STEERING;

    if (self->s.frame == FRAME_attak110)
        flash_number = MZ2_TANK_BLASTER_1;
    else if (self->s.frame == FRAME_attak113)
        flash_number = MZ2_TANK_BLASTER_2;
    else // (self->s.frame == FRAME_attak116)
        flash_number = MZ2_TANK_BLASTER_3;

    AngleVectors(self->s.angles, forward, right, NULL);
    M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right, start);

    // pmm - blindfire support
    // PMM
    if (blindfire) {
        if (!M_AdjustBlindfireTarget(self, start, self->monsterinfo.blind_fire_target, right, dir))
            return;
    } else
        PredictAim(self, self->enemy, start, 0, false, 0, dir, NULL);
    // pmm

    monster_fire_blaster(self, start, dir, 30, 800, flash_number, EF_BLASTER);
}

static void TankStrike(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_strike, 1, ATTN_NORM, 0);
}

static void TankRocket(edict_t *self)
{
    vec3_t                   forward, right;
    vec3_t                   start;
    vec3_t                   dir;
    vec3_t                   vec;
    monster_muzzleflash_id_t flash_number;
    int                      rocketSpeed; // PGM

    if (!self->enemy || !self->enemy->inuse) // PGM
        return;                              // PGM

    bool blindfire = self->monsterinfo.aiflags & AI_MANUAL_STEERING;

    if (self->s.frame == FRAME_attak324)
        flash_number = MZ2_TANK_ROCKET_1;
    else if (self->s.frame == FRAME_attak327)
        flash_number = MZ2_TANK_ROCKET_2;
    else // (self->s.frame == FRAME_attak330)
        flash_number = MZ2_TANK_ROCKET_3;

    AngleVectors(self->s.angles, forward, right, NULL);

    // [Paril-KEX] scale
    M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right, start);

    if (self->speed)
        rocketSpeed = self->speed;
    else if (self->spawnflags & SPAWNFLAG_TANK_COMMANDER_HEAT_SEEKING)
        rocketSpeed = 500;
    else
        rocketSpeed = 650;

    // PGM
    //  PMM - blindfire shooting
    if (blindfire) {
        VectorCopy(self->monsterinfo.blind_fire_target, vec);
    // pmm
    // don't shoot at feet if they're above me.
    } else if (frandom() < 0.66f || (start[2] < self->enemy->absmin[2])) {
        VectorCopy(self->enemy->s.origin, vec);
        vec[2] += self->enemy->viewheight;
    } else {
        VectorCopy(self->enemy->s.origin, vec);
        vec[2] = self->enemy->absmin[2] + 1;
    }
    VectorSubtract(vec, start, dir);
    VectorNormalize(dir);
    // PGM

    //======
    // PMM - lead target  (not when blindfiring)
    // 20, 35, 50, 65 chance of leading
    if ((!blindfire) && ((frandom() < (0.2f + ((3 - skill->integer) * 0.15f)))))
        PredictAim(self, self->enemy, start, rocketSpeed, false, 0, dir, vec);
    // PMM - lead target
    //======

    // pmm blindfire doesn't check target (done in checkattack)
    // paranoia, make sure we're not shooting a target right next to us
    if (blindfire) {
        // blindfire has different fail criteria for the trace
        if (M_AdjustBlindfireTarget(self, start, vec, right, dir)) {
            if (self->spawnflags & SPAWNFLAG_TANK_COMMANDER_HEAT_SEEKING)
                monster_fire_heat(self, start, dir, 50, rocketSpeed, flash_number, self->accel);
            else
                monster_fire_rocket(self, start, dir, 50, rocketSpeed, flash_number);
        }
    } else {
        trace_t trace = gi.trace(start, NULL, NULL, vec, self, MASK_PROJECTILE);

        if (trace.fraction > 0.5f || trace.ent->solid != SOLID_BSP) {
            if (self->spawnflags & SPAWNFLAG_TANK_COMMANDER_HEAT_SEEKING)
                monster_fire_heat(self, start, dir, 50, rocketSpeed, flash_number, self->accel);
            else
                monster_fire_rocket(self, start, dir, 50, rocketSpeed, flash_number);
        }
    }
}

static void TankMachineGun(edict_t *self)
{
    vec3_t                   dir;
    vec3_t                   vec;
    vec3_t                   start;
    vec3_t                   forward, right;
    monster_muzzleflash_id_t flash_number;

    if (!self->enemy || !self->enemy->inuse) // PGM
        return;                              // PGM

    flash_number = MZ2_TANK_MACHINEGUN_1 + (self->s.frame - FRAME_attak406);

    AngleVectors(self->s.angles, forward, right, NULL);

    M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right, start);

    if (self->enemy) {
        VectorCopy(self->enemy->s.origin, vec);
        vec[2] += self->enemy->viewheight;
        VectorSubtract(vec, start, vec);
        vectoangles(vec, vec);
        dir[0] = vec[0];
    } else {
        dir[0] = 0;
    }
    if (self->s.frame <= FRAME_attak415)
        dir[1] = self->s.angles[1] - 8 * (self->s.frame - FRAME_attak411);
    else
        dir[1] = self->s.angles[1] + 8 * (self->s.frame - FRAME_attak419);
    dir[2] = 0;

    AngleVectors(dir, forward, NULL, NULL);

    monster_fire_bullet(self, start, forward, 20, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
}

static void tank_blind_check(edict_t *self)
{
    if (self->monsterinfo.aiflags & AI_MANUAL_STEERING) {
        vec3_t aim;
        VectorSubtract(self->monsterinfo.blind_fire_target, self->s.origin, aim);
        self->ideal_yaw = vectoyaw(aim);
    }
}

static const mframe_t tank_frames_attack_blast[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, -1 },
    { ai_charge, -2 },
    { ai_charge, -1 },
    { ai_charge, -1, tank_blind_check },
    { ai_charge },
    { ai_charge, 0, TankBlaster }, // 10
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, TankBlaster },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, TankBlaster } // 16
};
const mmove_t MMOVE_T(tank_move_attack_blast) = { FRAME_attak101, FRAME_attak116, tank_frames_attack_blast, tank_reattack_blaster };

static const mframe_t tank_frames_reattack_blast[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, TankBlaster },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, TankBlaster } // 16
};
const mmove_t MMOVE_T(tank_move_reattack_blast) = { FRAME_attak111, FRAME_attak116, tank_frames_reattack_blast, tank_reattack_blaster };

static const mframe_t tank_frames_attack_post_blast[] = {
    { ai_move }, // 17
    { ai_move },
    { ai_move, 2 },
    { ai_move, 3 },
    { ai_move, 2 },
    { ai_move, -2, tank_footstep } // 22
};
const mmove_t MMOVE_T(tank_move_attack_post_blast) = { FRAME_attak117, FRAME_attak122, tank_frames_attack_post_blast, tank_run };

static void tank_reattack_blaster(edict_t *self)
{
    if (self->monsterinfo.aiflags & AI_MANUAL_STEERING) {
        self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
        M_SetAnimation(self, &tank_move_attack_post_blast);
        return;
    }

    if (visible(self, self->enemy) && self->enemy->health > 0 && frandom() <= 0.6f)
        M_SetAnimation(self, &tank_move_reattack_blast);
    else
        M_SetAnimation(self, &tank_move_attack_post_blast);
}

static void tank_poststrike(edict_t *self)
{
    self->enemy = NULL;
    // [Paril-KEX]
    self->monsterinfo.pausetime = HOLD_FOREVER;
    self->monsterinfo.stand(self);
}

static const mframe_t tank_frames_attack_strike[] = {
    { ai_move, 3 },
    { ai_move, 2 },
    { ai_move, 2 },
    { ai_move, 1 },
    { ai_move, 6 },
    { ai_move, 7 },
    { ai_move, 9, tank_footstep },
    { ai_move, 2 },
    { ai_move, 1 },
    { ai_move, 2 },
    { ai_move, 2, tank_footstep },
    { ai_move, 2 },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, -2 },
    { ai_move, -2 },
    { ai_move, 0, tank_windup },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, TankStrike },
    { ai_move },
    { ai_move, -1 },
    { ai_move, -1 },
    { ai_move, -1 },
    { ai_move, -1 },
    { ai_move, -1 },
    { ai_move, -3 },
    { ai_move, -10 },
    { ai_move, -10 },
    { ai_move, -2 },
    { ai_move, -3 },
    { ai_move, -2, tank_footstep }
};
const mmove_t MMOVE_T(tank_move_attack_strike) = { FRAME_attak201, FRAME_attak238, tank_frames_attack_strike, tank_poststrike };

static const mframe_t tank_frames_attack_pre_rocket[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge }, // 10

    { ai_charge },
    { ai_charge, 1 },
    { ai_charge, 2 },
    { ai_charge, 7 },
    { ai_charge, 7 },
    { ai_charge, 7, tank_footstep },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge }, // 20

    { ai_charge, -3 }
};
const mmove_t MMOVE_T(tank_move_attack_pre_rocket) = { FRAME_attak301, FRAME_attak321, tank_frames_attack_pre_rocket, tank_doattack_rocket };

static const mframe_t tank_frames_attack_fire_rocket[] = {
    { ai_charge, -3, tank_blind_check }, // Loop Start  22
    { ai_charge },
    { ai_charge, 0, TankRocket }, // 24
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, TankRocket },
    { ai_charge },
    { ai_charge },
    { ai_charge, -1, TankRocket } // 30 Loop End
};
const mmove_t MMOVE_T(tank_move_attack_fire_rocket) = { FRAME_attak322, FRAME_attak330, tank_frames_attack_fire_rocket, tank_refire_rocket };

static const mframe_t tank_frames_attack_post_rocket[] = {
    { ai_charge }, // 31
    { ai_charge, -1 },
    { ai_charge, -1 },
    { ai_charge },
    { ai_charge, 2 },
    { ai_charge, 3 },
    { ai_charge, 4 },
    { ai_charge, 2 },
    { ai_charge },
    { ai_charge }, // 40

    { ai_charge },
    { ai_charge, -9 },
    { ai_charge, -8 },
    { ai_charge, -7 },
    { ai_charge, -1 },
    { ai_charge, -1, tank_footstep },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge }, // 50

    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(tank_move_attack_post_rocket) = { FRAME_attak331, FRAME_attak353, tank_frames_attack_post_rocket, tank_run };

static const mframe_t tank_frames_attack_chain[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { NULL, 0, TankMachineGun },
    { NULL, 0, TankMachineGun },
    { NULL, 0, TankMachineGun },
    { NULL, 0, TankMachineGun },
    { NULL, 0, TankMachineGun },
    { NULL, 0, TankMachineGun },
    { NULL, 0, TankMachineGun },
    { NULL, 0, TankMachineGun },
    { NULL, 0, TankMachineGun },
    { NULL, 0, TankMachineGun },
    { NULL, 0, TankMachineGun },
    { NULL, 0, TankMachineGun },
    { NULL, 0, TankMachineGun },
    { NULL, 0, TankMachineGun },
    { NULL, 0, TankMachineGun },
    { NULL, 0, TankMachineGun },
    { NULL, 0, TankMachineGun },
    { NULL, 0, TankMachineGun },
    { NULL, 0, TankMachineGun },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(tank_move_attack_chain) = { FRAME_attak401, FRAME_attak429, tank_frames_attack_chain, tank_run };

static void tank_refire_rocket(edict_t *self)
{
    // PMM - blindfire cleanup
    if (self->monsterinfo.aiflags & AI_MANUAL_STEERING) {
        self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
        M_SetAnimation(self, &tank_move_attack_post_rocket);
        return;
    }
    // pmm

    if (self->enemy->health > 0 && visible(self, self->enemy) && frandom() <= 0.4f)
        M_SetAnimation(self, &tank_move_attack_fire_rocket);
    else
        M_SetAnimation(self, &tank_move_attack_post_rocket);
}

static void tank_doattack_rocket(edict_t *self)
{
    M_SetAnimation(self, &tank_move_attack_fire_rocket);
}

void MONSTERINFO_ATTACK(tank_attack)(edict_t *self)
{
    float  range;
    float  r;

    // PMM
    if (!self->enemy || !self->enemy->inuse)
        return;

    if (self->enemy->health <= 0) {
        M_SetAnimation(self, &tank_move_attack_strike);
        self->monsterinfo.aiflags &= ~AI_BRUTAL;
        return;
    }

    // PMM
    if (self->monsterinfo.attack_state == AS_BLIND) {
        float chance;

        // setup shot probabilities
        if (self->monsterinfo.blind_fire_delay < SEC(1))
            chance = 1.0f;
        else if (self->monsterinfo.blind_fire_delay < SEC(7.5f))
            chance = 0.4f;
        else
            chance = 0.1f;

        self->monsterinfo.blind_fire_delay += random_time_sec(5.2f, 8.2f);

        // don't shoot at the origin
        if (VectorEmpty(self->monsterinfo.blind_fire_target))
            return;

        // don't shoot if the dice say not to
        if (frandom() > chance)
            return;

        bool rocket_visible = M_CheckClearShot(self, monster_flash_offset[MZ2_TANK_ROCKET_1]);
        bool blaster_visible = M_CheckClearShot(self, monster_flash_offset[MZ2_TANK_BLASTER_1]);

        if (!rocket_visible && !blaster_visible)
            return;

        bool use_rocket = (rocket_visible && blaster_visible) ? brandom() : rocket_visible;

        // turn on manual steering to signal both manual steering and blindfire
        self->monsterinfo.aiflags |= AI_MANUAL_STEERING;

        if (use_rocket)
            M_SetAnimation(self, &tank_move_attack_fire_rocket);
        else {
            M_SetAnimation(self, &tank_move_attack_blast);
            self->monsterinfo.nextframe = FRAME_attak108;
        }

        self->monsterinfo.attack_finished = level.time + random_time_sec(3, 5);
        self->pain_debounce_time = level.time + SEC(5); // no pain for a while
        return;
    }
    // pmm

    range = Distance(self->enemy->s.origin, self->s.origin);

    r = frandom();

    if (range <= 125) {
        bool can_machinegun = (!self->enemy->classname || strcmp(self->enemy->classname, "tesla_mine")) && M_CheckClearShot(self, monster_flash_offset[MZ2_TANK_MACHINEGUN_5]);

        if (can_machinegun && r < 0.5f)
            M_SetAnimation(self, &tank_move_attack_chain);
        else if (M_CheckClearShot(self, monster_flash_offset[MZ2_TANK_BLASTER_1]))
            M_SetAnimation(self, &tank_move_attack_blast);
    } else if (range <= 250) {
        bool can_machinegun = (!self->enemy->classname || strcmp(self->enemy->classname, "tesla_mine")) && M_CheckClearShot(self, monster_flash_offset[MZ2_TANK_MACHINEGUN_5]);

        if (can_machinegun && r < 0.25f)
            M_SetAnimation(self, &tank_move_attack_chain);
        else if (M_CheckClearShot(self, monster_flash_offset[MZ2_TANK_BLASTER_1]))
            M_SetAnimation(self, &tank_move_attack_blast);
    } else {
        bool can_machinegun = M_CheckClearShot(self, monster_flash_offset[MZ2_TANK_MACHINEGUN_5]);
        bool can_rocket = M_CheckClearShot(self, monster_flash_offset[MZ2_TANK_ROCKET_1]);

        if (can_machinegun && r < 0.33f)
            M_SetAnimation(self, &tank_move_attack_chain);
        else if (can_rocket && r < 0.66f) {
            M_SetAnimation(self, &tank_move_attack_pre_rocket);
            self->pain_debounce_time = level.time + SEC(5); // no pain for a while
        } else if (M_CheckClearShot(self, monster_flash_offset[MZ2_TANK_BLASTER_1]))
            M_SetAnimation(self, &tank_move_attack_blast);
    }
}

//
// death
//

static void tank_dead(edict_t *self)
{
    VectorSet(self->mins, -16, -16, -16);
    VectorSet(self->maxs, 16, 16, -0);
    monster_dead(self);
}

static void tank_shrink(edict_t *self)
{
    self->maxs[2] = 0;
    self->svflags |= SVF_DEADMONSTER;
    gi.linkentity(self);
}

static const mframe_t tank_frames_death1[] = {
    { ai_move, -7 },
    { ai_move, -2 },
    { ai_move, -2 },
    { ai_move, 1 },
    { ai_move, 3 },
    { ai_move, 6 },
    { ai_move, 1 },
    { ai_move, 1 },
    { ai_move, 2 },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, -2 },
    { ai_move },
    { ai_move },
    { ai_move, -3 },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, -4 },
    { ai_move, -6 },
    { ai_move, -4 },
    { ai_move, -5 },
    { ai_move, -7, tank_shrink },
    { ai_move, -15, tank_thud },
    { ai_move, -5 },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(tank_move_death) = { FRAME_death101, FRAME_death132, tank_frames_death1, tank_dead };

static const gib_def_t tank_gibs[] = {
    { "models/objects/gibs/sm_meat/tris.md2", 1 },
    { "models/objects/gibs/sm_metal/tris.md2", 3, GIB_METALLIC },
    { "models/objects/gibs/gear/tris.md2", 1, GIB_METALLIC },
    { "models/monsters/tank/gibs/foot.md2", 2, GIB_SKINNED | GIB_METALLIC },
    { "models/monsters/tank/gibs/thigh.md2", 2, GIB_SKINNED | GIB_METALLIC },
    { "models/monsters/tank/gibs/chest.md2", 1, GIB_SKINNED },
    { "models/monsters/tank/gibs/head.md2", 1, GIB_HEAD | GIB_SKINNED },
    { 0 }
};

void DIE(tank_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, const mod_t mod)
{
    // check for gib
    if (M_CheckGib(self, mod)) {
        gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

        self->s.skinnum /= 2;

        if (!self->style)
            ThrowGib(self, "models/monsters/tank/gibs/barm.md2", damage, GIB_SKINNED | GIB_UPRIGHT, self->x.scale);

        ThrowGibs(self, damage, tank_gibs);

        self->deadflag = true;
        return;
    }

    if (self->deadflag)
        return;

    // [Paril-KEX] dropped arm
    if (!self->style) {
        vec3_t f, r, u;

        self->style = 1;
        AngleVectors(self->s.angles, f, r, u);

        edict_t *arm = ThrowGib(self, "models/monsters/tank/gibs/barm.md2", damage, GIB_SKINNED | GIB_UPRIGHT, self->x.scale);
        VectorMA(self->s.origin, -16, r, arm->s.origin);
        VectorMA(arm->s.origin, 23, u, arm->s.origin);
        VectorCopy(arm->s.origin, arm->s.old_origin);
        arm->avelocity[0] = crandom() * 15;
        arm->avelocity[1] = crandom() * 15;
        arm->avelocity[2] = 180;
        VectorScale(r, -120, arm->velocity);
        VectorMA(arm->velocity, 100, u, arm->velocity);
        VectorCopy(self->s.angles, arm->s.angles);
        arm->s.angles[2] = -90;
        arm->s.skinnum /= 2;
        gi.linkentity(arm);
    }

    // regular death
    gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
    self->deadflag = true;
    self->takedamage = true;

    M_SetAnimation(self, &tank_move_death);
}

//===========
// PGM
bool MONSTERINFO_BLOCKED(tank_blocked)(edict_t *self, float dist)
{
    if (blocked_checkplat(self, dist))
        return true;

    return false;
}
// PGM
//===========

//
// monster_tank
//

static void tank_precache(void)
{
    sound_thud = gi.soundindex("tank/tnkdeth2.wav");
    sound_idle = gi.soundindex("tank/tnkidle1.wav");
    sound_die = gi.soundindex("tank/death.wav");
    sound_step = gi.soundindex("tank/step.wav");
    sound_windup = gi.soundindex("tank/tnkatck4.wav");
    sound_strike = gi.soundindex("tank/tnkatck5.wav");
    sound_sight = gi.soundindex("tank/sight1.wav");
}

/*QUAKED monster_tank (1 .5 0) (-32 -32 -16) (32 32 72) Ambush Trigger_Spawn Sight
model="models/monsters/tank/tris.md2"
*/
/*QUAKED monster_tank_commander (1 .5 0) (-32 -32 -16) (32 32 72) Ambush Trigger_Spawn Sight Guardian HeatSeeking
 */
void SP_monster_tank(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    self->s.modelindex = gi.modelindex("models/monsters/tank/tris.md2");
    VectorSet(self->mins, -32, -32, -16);
    VectorSet(self->maxs, 32, 32, 64);
    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;

    G_AddPrecache(tank_precache);

    gi.soundindex("tank/tnkatck1.wav");
    gi.soundindex("tank/tnkatk2a.wav");
    gi.soundindex("tank/tnkatk2b.wav");
    gi.soundindex("tank/tnkatk2c.wav");
    gi.soundindex("tank/tnkatk2d.wav");
    gi.soundindex("tank/tnkatk2e.wav");
    gi.soundindex("tank/tnkatck3.wav");

    PrecacheGibs(tank_gibs);

    if (strcmp(self->classname, "monster_tank_commander") == 0) {
        self->health = 1000 * st.health_multiplier;
        self->gib_health = -225;
        self->count = 1;
        sound_pain2 = gi.soundindex("tank/pain.wav");
    } else {
        self->health = 750 * st.health_multiplier;
        self->gib_health = -200;
        sound_pain = gi.soundindex("tank/tnkpain2.wav");
    }

    self->monsterinfo.scale = MODEL_SCALE;

    // [Paril-KEX] N64 tank commander is a chonky boy
    if (self->spawnflags & SPAWNFLAG_TANK_COMMANDER_GUARDIAN) {
        if (!self->x.scale)
            self->x.scale = 1.5f;
        self->health = 1500 * st.health_multiplier;
    }

    // heat seekingness
    if (!self->accel)
        self->accel = 0.075f;

    self->mass = 500;

    self->pain = tank_pain;
    self->die = tank_die;
    self->monsterinfo.stand = tank_stand;
    self->monsterinfo.walk = tank_walk;
    self->monsterinfo.run = tank_run;
    self->monsterinfo.dodge = NULL;
    self->monsterinfo.attack = tank_attack;
    self->monsterinfo.melee = NULL;
    self->monsterinfo.sight = tank_sight;
    self->monsterinfo.idle = tank_idle;
    self->monsterinfo.blocked = tank_blocked; // PGM
    self->monsterinfo.setskin = tank_setskin;

    gi.linkentity(self);

    M_SetAnimation(self, &tank_move_stand);

    walkmonster_start(self);

    // PMM
    self->monsterinfo.aiflags |= AI_IGNORE_SHOTS;
    self->monsterinfo.blindfire = true;
    // pmm
    if (strcmp(self->classname, "monster_tank_commander") == 0)
        self->s.skinnum = 2;
}

void Use_Boss3(edict_t *ent, edict_t *other, edict_t *activator);

void THINK(Think_TankStand)(edict_t *ent)
{
    if (ent->s.frame == FRAME_stand30)
        ent->s.frame = FRAME_stand01;
    else
        ent->s.frame++;
    ent->nextthink = level.time + HZ(10);
}

/*QUAKED monster_tank_stand (1 .5 0) (-32 -32 0) (32 32 90)

Just stands and cycles in one place until targeted, then teleports away.
N64 edition!
*/
void SP_monster_tank_stand(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;
    self->model = "models/monsters/tank/tris.md2";
    self->s.modelindex = gi.modelindex(self->model);
    self->s.frame = FRAME_stand01;
    self->s.skinnum = 2;

    gi.soundindex("misc/bigtele.wav");

    VectorSet(self->mins, -32, -32, -16);
    VectorSet(self->maxs, 32, 32, 64);

    if (!self->x.scale)
        self->x.scale = 1.5f;

    VectorScale(self->mins, self->x.scale, self->mins);
    VectorScale(self->maxs, self->x.scale, self->maxs);

    self->use = Use_Boss3;
    self->think = Think_TankStand;
    self->nextthink = level.time + HZ(10);
    gi.linkentity(self);
}
