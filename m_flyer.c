// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

flyer

==============================================================================
*/

#include "g_local.h"
#include "m_flyer.h"

static int sound_sight;
static int sound_idle;
static int sound_pain1;
static int sound_pain2;
static int sound_slash;
static int sound_sproing;
static int sound_die;

static void flyer_check_melee(edict_t *self);
static void flyer_loop_melee(edict_t *self);

// ROGUE - kamikaze stuff
static void flyer_kamikaze(edict_t *self);
static void flyer_kamikaze_check(edict_t *self);
void flyer_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod);

void MONSTERINFO_SIGHT(flyer_sight)(edict_t *self, edict_t *other)
{
    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void MONSTERINFO_IDLE(flyer_idle)(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

static void flyer_pop_blades(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_sproing, 1, ATTN_NORM, 0);
}

static const mframe_t flyer_frames_stand[] = {
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
    { ai_stand },
    { ai_stand },
    { ai_stand }
};
const mmove_t MMOVE_T(flyer_move_stand) = { FRAME_stand01, FRAME_stand45, flyer_frames_stand, NULL };

static const mframe_t flyer_frames_walk[] = {
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 }
};
const mmove_t MMOVE_T(flyer_move_walk) = { FRAME_stand01, FRAME_stand45, flyer_frames_walk, NULL };

static const mframe_t flyer_frames_run[] = {
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 },
    { ai_run, 10 }
};
const mmove_t MMOVE_T(flyer_move_run) = { FRAME_stand01, FRAME_stand45, flyer_frames_run, NULL };

static const mframe_t flyer_frames_kamizake[] = {
    { ai_charge, 40, flyer_kamikaze_check },
    { ai_charge, 40, flyer_kamikaze_check },
    { ai_charge, 40, flyer_kamikaze_check },
    { ai_charge, 40, flyer_kamikaze_check },
    { ai_charge, 40, flyer_kamikaze_check }
};
const mmove_t MMOVE_T(flyer_move_kamikaze) = { FRAME_rollr02, FRAME_rollr06, flyer_frames_kamizake, flyer_kamikaze };

void MONSTERINFO_RUN(flyer_run)(edict_t *self)
{
    if (self->mass > 50)
        M_SetAnimation(self, &flyer_move_kamikaze);
    else if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        M_SetAnimation(self, &flyer_move_stand);
    else
        M_SetAnimation(self, &flyer_move_run);
}

void MONSTERINFO_WALK(flyer_walk)(edict_t *self)
{
    if (self->mass > 50)
        flyer_run(self);
    else
        M_SetAnimation(self, &flyer_move_walk);
}

void MONSTERINFO_STAND(flyer_stand)(edict_t *self)
{
    if (self->mass > 50)
        flyer_run(self);
    else
        M_SetAnimation(self, &flyer_move_stand);
}

// ROGUE - kamikaze stuff

static void flyer_kamikaze_explode(edict_t *self)
{
    vec3_t dir;

    if (self->monsterinfo.commander && self->monsterinfo.commander->inuse &&
        !strcmp(self->monsterinfo.commander->classname, "monster_carrier"))
        self->monsterinfo.commander->monsterinfo.monster_slots++;

    if (self->enemy) {
        VectorSubtract(self->enemy->s.origin, self->s.origin, dir);
        T_Damage(self->enemy, self, self, dir, self->s.origin, vec3_origin, 50, 50, DAMAGE_RADIUS, (mod_t) { MOD_UNKNOWN });
    }

    flyer_die(self, NULL, NULL, 0, dir, (mod_t) { MOD_EXPLOSIVE });
}

static void flyer_kamikaze(edict_t *self)
{
    M_SetAnimation(self, &flyer_move_kamikaze);
}

static void flyer_kamikaze_check(edict_t *self)
{
    vec3_t dir;
    float dist;

    // PMM - this needed because we could have gone away before we get here (blocked code)
    if (!self->inuse)
        return;

    if ((!self->enemy) || (!self->enemy->inuse)) {
        flyer_kamikaze_explode(self);
        return;
    }

    VectorSubtract(self->enemy->s.origin, self->s.origin, dir);
    vectoangles(dir, dir);
    self->s.angles[0] = dir[0];

    self->goalentity = self->enemy;

    dist = realrange(self, self->enemy);

    if (dist < 90)
        flyer_kamikaze_explode(self);
}

#if 0
static const mframe_t flyer_frames_rollright[] = {
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
const mmove_t MMOVE_T(flyer_move_rollright) = { FRAME_rollr01, FRAME_rollr09, flyer_frames_rollright, NULL };

static const mframe_t flyer_frames_rollleft[] = {
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
const mmove_t MMOVE_T(flyer_move_rollleft) = { FRAME_rollf01, FRAME_rollf09, flyer_frames_rollleft, NULL };
#endif

static const mframe_t flyer_frames_pain3[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(flyer_move_pain3) = { FRAME_pain301, FRAME_pain304, flyer_frames_pain3, flyer_run };

static const mframe_t flyer_frames_pain2[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(flyer_move_pain2) = { FRAME_pain201, FRAME_pain204, flyer_frames_pain2, flyer_run };

static const mframe_t flyer_frames_pain1[] = {
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
const mmove_t MMOVE_T(flyer_move_pain1) = { FRAME_pain101, FRAME_pain109, flyer_frames_pain1, flyer_run };

#if 0
static const mframe_t flyer_frames_defense[] = {
    { ai_move },
    { ai_move },
    { ai_move }, // Hold this frame
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(flyer_move_defense) = { FRAME_defens01, FRAME_defens06, flyer_frames_defense, NULL };

static const mframe_t flyer_frames_bankright[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(flyer_move_bankright) = { FRAME_bankr01, FRAME_bankr07, flyer_frames_bankright, NULL };

static const mframe_t flyer_frames_bankleft[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(flyer_move_bankleft) = { FRAME_bankl01, FRAME_bankl07, flyer_frames_bankleft, NULL };
#endif

static void flyer_fire(edict_t *self, monster_muzzleflash_id_t flash_number)
{
    vec3_t    start;
    vec3_t    forward, right;
    vec3_t    end;
    vec3_t    dir;

    if (!self->enemy || !self->enemy->inuse) // PGM
        return;                              // PGM

    AngleVectors(self->s.angles, forward, right, NULL);
    M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right, start);

    VectorCopy(self->enemy->s.origin, end);
    end[2] += self->enemy->viewheight;
    VectorSubtract(end, start, dir);
    VectorNormalize(dir);

    monster_fire_blaster(self, start, dir, 1, 1000, flash_number, (self->s.frame % 4) ? EF_NONE : EF_HYPERBLASTER);
}

static void flyer_fireleft(edict_t *self)
{
    flyer_fire(self, MZ2_FLYER_BLASTER_1);
}

static void flyer_fireright(edict_t *self)
{
    flyer_fire(self, MZ2_FLYER_BLASTER_2);
}

static const mframe_t flyer_frames_attack2[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, -10, flyer_fireleft },  // left gun
    { ai_charge, -10, flyer_fireright }, // right gun
    { ai_charge, -10, flyer_fireleft },  // left gun
    { ai_charge, -10, flyer_fireright }, // right gun
    { ai_charge, -10, flyer_fireleft },  // left gun
    { ai_charge, -10, flyer_fireright }, // right gun
    { ai_charge, -10, flyer_fireleft },  // left gun
    { ai_charge, -10, flyer_fireright }, // right gun
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(flyer_move_attack2) = { FRAME_attak201, FRAME_attak217, flyer_frames_attack2, flyer_run };

// PMM
// circle strafe frames

static const mframe_t flyer_frames_attack3[] = {
    { ai_charge, 10 },
    { ai_charge, 10 },
    { ai_charge, 10 },
    { ai_charge, 10, flyer_fireleft },  // left gun
    { ai_charge, 10, flyer_fireright }, // right gun
    { ai_charge, 10, flyer_fireleft },  // left gun
    { ai_charge, 10, flyer_fireright }, // right gun
    { ai_charge, 10, flyer_fireleft },  // left gun
    { ai_charge, 10, flyer_fireright }, // right gun
    { ai_charge, 10, flyer_fireleft },  // left gun
    { ai_charge, 10, flyer_fireright }, // right gun
    { ai_charge, 10 },
    { ai_charge, 10 },
    { ai_charge, 10 },
    { ai_charge, 10 },
    { ai_charge, 10 },
    { ai_charge, 10 }
};
const mmove_t MMOVE_T(flyer_move_attack3) = { FRAME_attak201, FRAME_attak217, flyer_frames_attack3, flyer_run };
// pmm

static void flyer_slash_left(edict_t *self)
{
    vec3_t aim = { MELEE_DISTANCE, self->mins[0], 0 };
    if (!fire_hit(self, aim, 5, 0))
        self->monsterinfo.melee_debounce_time = level.time + SEC(1.5f);
    gi.sound(self, CHAN_WEAPON, sound_slash, 1, ATTN_NORM, 0);
}

static void flyer_slash_right(edict_t *self)
{
    vec3_t aim = { MELEE_DISTANCE, self->maxs[0], 0 };
    if (!fire_hit(self, aim, 5, 0))
        self->monsterinfo.melee_debounce_time = level.time + SEC(1.5f);
    gi.sound(self, CHAN_WEAPON, sound_slash, 1, ATTN_NORM, 0);
}

static const mframe_t flyer_frames_start_melee[] = {
    { ai_charge, 0, flyer_pop_blades },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(flyer_move_start_melee) = { FRAME_attak101, FRAME_attak106, flyer_frames_start_melee, flyer_loop_melee };

static const mframe_t flyer_frames_end_melee[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(flyer_move_end_melee) = { FRAME_attak119, FRAME_attak121, flyer_frames_end_melee, flyer_run };

static const mframe_t flyer_frames_loop_melee[] = {
    { ai_charge }, // Loop Start
    { ai_charge },
    { ai_charge, 0, flyer_slash_left }, // Left Wing Strike
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, flyer_slash_right }, // Right Wing Strike
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge } // Loop Ends

};
const mmove_t MMOVE_T(flyer_move_loop_melee) = { FRAME_attak107, FRAME_attak118, flyer_frames_loop_melee, flyer_check_melee };

static void flyer_loop_melee(edict_t *self)
{
    M_SetAnimation(self, &flyer_move_loop_melee);
}

static void flyer_set_fly_parameters(edict_t *self, bool melee)
{
    if (melee) {
        // engage thrusters for a slice
        self->monsterinfo.fly_pinned = false;
        self->monsterinfo.fly_thrusters = true;
        self->monsterinfo.fly_position_time = 0;
        self->monsterinfo.fly_acceleration = 20;
        self->monsterinfo.fly_speed = 210;
        self->monsterinfo.fly_min_distance = 0;
        self->monsterinfo.fly_max_distance = 10;
    } else {
        self->monsterinfo.fly_thrusters = false;
        self->monsterinfo.fly_acceleration = 15;
        self->monsterinfo.fly_speed = 165;
        self->monsterinfo.fly_min_distance = 45;
        self->monsterinfo.fly_max_distance = 200;
    }
}

void MONSTERINFO_ATTACK(flyer_attack)(edict_t *self)
{
    if (self->mass > 50) {
        flyer_run(self);
        return;
    }

    float range = range_to(self, self->enemy);

    if (self->enemy && visible(self, self->enemy) && range <= 225 && frandom() > (range / 225) * 0.35f) {
        // fly-by slicing!
        self->monsterinfo.attack_state = AS_STRAIGHT;
        M_SetAnimation(self, &flyer_move_start_melee);
        flyer_set_fly_parameters(self, true);
    } else {
        self->monsterinfo.attack_state = AS_STRAIGHT;
        M_SetAnimation(self, &flyer_move_attack2);
    }

    // [Paril-KEX] for alternate fly mode, sometimes we'll pin us
    // down, kind of like a pseudo-stand ground
    if (!self->monsterinfo.fly_pinned && brandom() && self->enemy && visible(self, self->enemy)) {
        self->monsterinfo.fly_pinned = true;
        self->monsterinfo.fly_position_time = self->monsterinfo.fly_position_time + SEC(1.7f); // make sure there's enough time for attack2/3

        if (brandom()) {
            float dist = frandom();
            VectorMA(self->s.origin, dist, self->velocity, self->monsterinfo.fly_ideal_position); // pin to our current position
        } else {
            VectorAdd(self->monsterinfo.fly_ideal_position, self->enemy->s.origin, self->monsterinfo.fly_ideal_position); // make un-relative
        }
    }

    // if we're currently pinned, fly_position_time will unpin us eventually
}

void MONSTERINFO_MELEE(flyer_melee)(edict_t *self)
{
    if (self->mass > 50)
        flyer_run(self);
    else {
        M_SetAnimation(self, &flyer_move_start_melee);
        flyer_set_fly_parameters(self, true);
    }
}

static void flyer_check_melee(edict_t *self)
{
    if (range_to(self, self->enemy) <= RANGE_MELEE && self->monsterinfo.melee_debounce_time <= level.time) {
        M_SetAnimation(self, &flyer_move_loop_melee);
        return;
    }

    M_SetAnimation(self, &flyer_move_end_melee);
    flyer_set_fly_parameters(self, false);
}

void PAIN(flyer_pain)(edict_t *self, edict_t *other, float kick, int damage, mod_t mod)
{
    int n;

    //  pmm  - kamikaze's don't feel pain
    if (self->mass != 50)
        return;
    // pmm

    if (level.time < self->pain_debounce_time)
        return;

    self->pain_debounce_time = level.time + SEC(3);

    n = irandom1(3);
    if (n == 0)
        gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
    else if (n == 1)
        gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
    else
        gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);

    if (!M_ShouldReactToPain(self, mod))
        return; // no pain anims in nightmare

    flyer_set_fly_parameters(self, false);

    if (n == 0)
        M_SetAnimation(self, &flyer_move_pain1);
    else if (n == 1)
        M_SetAnimation(self, &flyer_move_pain2);
    else
        M_SetAnimation(self, &flyer_move_pain3);
}

void MONSTERINFO_SETSKIN(flyer_setskin)(edict_t *self)
{
    if (self->health < (self->max_health / 2))
        self->s.skinnum = 1;
    else
        self->s.skinnum = 0;
}

static const gib_def_t flyer_gibs[] = {
    { "models/objects/gibs/sm_metal/tris.md2", 2 },
    { "models/objects/gibs/sm_meat/tris.md2", 2 },
    { "models/monsters/flyer/gibs/base.md2", 1, GIB_SKINNED },
    { "models/monsters/flyer/gibs/gun.md2", 2, GIB_SKINNED },
    { "models/monsters/flyer/gibs/wing.md2", 2, GIB_SKINNED },
    { "models/monsters/flyer/gibs/head.md2", 1, GIB_SKINNED | GIB_HEAD },
    { 0 }
};

void DIE(flyer_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);

    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_EXPLOSION1);
    gi.WritePosition(self->s.origin);
    gi.multicast(self->s.origin, MULTICAST_PHS);

    self->s.skinnum /= 2;

    ThrowGibs(self, 55, flyer_gibs);

    self->touch = NULL;
}

// PMM - kamikaze code .. blow up if blocked
bool MONSTERINFO_BLOCKED(flyer_blocked)(edict_t *self, float dist)
{
    // kamikaze = 100, normal = 50
    if (self->mass == 100) {
        flyer_kamikaze_check(self);

        // if the above didn't blow us up (i.e. I got blocked by the player)
        if (self->inuse)
            T_Damage(self, self, self, vec3_origin, self->s.origin, vec3_origin, 9999, 100, DAMAGE_NONE, (mod_t) { MOD_UNKNOWN });

        return true;
    }

    return false;
}

void TOUCH(kamikaze_touch)(edict_t *ent, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    vec3_t dir;
    VectorNormalize2(ent->velocity, dir);
    T_Damage(ent, ent, ent, dir, ent->s.origin, dir, 9999, 100, DAMAGE_NONE, (mod_t) { MOD_UNKNOWN });
}

void TOUCH(flyer_touch)(edict_t *ent, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if ((other->monsterinfo.aiflags & AI_ALTERNATE_FLY) && (other->flags & FL_FLY) &&
        (ent->monsterinfo.duck_wait_time < level.time))
    {
        ent->monsterinfo.duck_wait_time = level.time + SEC(1);
        ent->monsterinfo.fly_thrusters = false;

        vec3_t dir;
        VectorSubtract(ent->s.origin, other->s.origin, dir);
        VectorNormalize(dir);
        VectorScale(dir, 500, ent->velocity);

        gi.WriteByte(svc_temp_entity);
        gi.WriteByte(TE_SPLASH);
        gi.WriteByte(32);
        gi.WritePosition(tr->endpos);
        gi.WriteDir(dir);
        gi.WriteByte(SPLASH_SPARKS);
        gi.multicast(tr->endpos, MULTICAST_PVS);
    }
}

static void flyer_precache(void)
{
    sound_sight = gi.soundindex("flyer/flysght1.wav");
    sound_idle = gi.soundindex("flyer/flysrch1.wav");
    sound_pain1 = gi.soundindex("flyer/flypain1.wav");
    sound_pain2 = gi.soundindex("flyer/flypain2.wav");
    sound_slash = gi.soundindex("flyer/flyatck2.wav");
    sound_sproing = gi.soundindex("flyer/flyatck1.wav");
    sound_die = gi.soundindex("flyer/flydeth1.wav");
}

/*QUAKED monster_flyer (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
 */
void SP_monster_flyer(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    G_AddPrecache(flyer_precache);

    gi.soundindex("flyer/flyatck3.wav");

    self->s.modelindex = gi.modelindex("models/monsters/flyer/tris.md2");

    PrecacheGibs(flyer_gibs);

    VectorSet(self->mins, -16, -16, -24);
    // PMM - shortened to 16 from 32
    VectorSet(self->maxs, 16, 16, 16);
    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;

    self->viewheight = 12;

    self->monsterinfo.engine_sound = gi.soundindex("flyer/flyidle1.wav");

    self->health = 50 * st.health_multiplier;
    self->mass = 50;

    self->pain = flyer_pain;
    self->die = flyer_die;

    self->monsterinfo.stand = flyer_stand;
    self->monsterinfo.walk = flyer_walk;
    self->monsterinfo.run = flyer_run;
    self->monsterinfo.attack = flyer_attack;
    self->monsterinfo.melee = flyer_melee;
    self->monsterinfo.sight = flyer_sight;
    self->monsterinfo.idle = flyer_idle;
    self->monsterinfo.blocked = flyer_blocked;
    self->monsterinfo.setskin = flyer_setskin;

    gi.linkentity(self);

    M_SetAnimation(self, &flyer_move_stand);
    self->monsterinfo.scale = MODEL_SCALE;

    if (self->s.effects & EF_ROCKET) {
        // PMM - normal flyer has mass of 50
        self->mass = 100;
        self->yaw_speed = 5;
        self->touch = kamikaze_touch;
    } else {
        self->monsterinfo.aiflags |= AI_ALTERNATE_FLY;
        self->monsterinfo.fly_buzzard = true;
        flyer_set_fly_parameters(self, false);
        self->touch = flyer_touch;
    }

    flymonster_start(self);
}

// PMM - suicide fliers
void SP_monster_kamikaze(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    self->s.effects |= EF_ROCKET;

    SP_monster_flyer(self);
}
