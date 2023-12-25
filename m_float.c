// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

floater

==============================================================================
*/

#include "g_local.h"
#include "m_float.h"

static int sound_attack2;
static int sound_attack3;
static int sound_death1;
static int sound_idle;
static int sound_pain1;
static int sound_pain2;
static int sound_sight;

void MONSTERINFO_SIGHT(floater_sight)(edict_t *self, edict_t *other)
{
    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void MONSTERINFO_IDLE(floater_idle)(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

//static void floater_dead(edict_t *self);
void floater_run(edict_t *self);
static void floater_wham(edict_t *self);
static void floater_zap(edict_t *self);

static void floater_fire_blaster(edict_t *self)
{
    vec3_t    start;
    vec3_t    forward, right;
    vec3_t    end;
    vec3_t    dir;

    if (!self->enemy || !self->enemy->inuse) // PGM
        return;                              // PGM

    AngleVectors(self->s.angles, forward, right, NULL);
    M_ProjectFlashSource(self, monster_flash_offset[MZ2_FLOAT_BLASTER_1], forward, right, start);

    VectorCopy(self->enemy->s.origin, end);
    end[2] += self->enemy->viewheight;
    VectorSubtract(end, start, dir);
    VectorNormalize(dir);

    monster_fire_blaster(self, start, dir, 1, 1000, MZ2_FLOAT_BLASTER_1, (self->s.frame % 4) ? EF_NONE : EF_HYPERBLASTER);
}

static const mframe_t floater_frames_stand1[] = {
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
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand }
};
const mmove_t MMOVE_T(floater_move_stand1) = { FRAME_stand101, FRAME_stand152, floater_frames_stand1, NULL };

static const mframe_t floater_frames_stand2[] = {
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
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand }
};
const mmove_t MMOVE_T(floater_move_stand2) = { FRAME_stand201, FRAME_stand252, floater_frames_stand2, NULL };

static const mframe_t floater_frames_pop[FRAME_actvat31 - FRAME_actvat05 + 1] = { 0 };
const mmove_t MMOVE_T(floater_move_pop) = { FRAME_actvat05, FRAME_actvat31, floater_frames_pop, floater_run };

static const mframe_t floater_frames_disguise[] = {
    { ai_stand }
};
const mmove_t MMOVE_T(floater_move_disguise) = { FRAME_actvat01, FRAME_actvat01, floater_frames_disguise, NULL };

void MONSTERINFO_STAND(floater_stand)(edict_t *self)
{
    if (self->monsterinfo.active_move == &floater_move_disguise)
        M_SetAnimation(self, &floater_move_disguise);
    else if (brandom())
        M_SetAnimation(self, &floater_move_stand1);
    else
        M_SetAnimation(self, &floater_move_stand2);
}

static const mframe_t floater_frames_attack1[] = {
    { ai_charge }, // Blaster attack
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, floater_fire_blaster }, // BOOM (0, -25.8, 32.5)    -- LOOP Starts
    { ai_charge, 0, floater_fire_blaster },
    { ai_charge, 0, floater_fire_blaster },
    { ai_charge, 0, floater_fire_blaster },
    { ai_charge, 0, floater_fire_blaster },
    { ai_charge, 0, floater_fire_blaster },
    { ai_charge, 0, floater_fire_blaster },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge } //                            -- LOOP Ends
};
const mmove_t MMOVE_T(floater_move_attack1) = { FRAME_attak101, FRAME_attak114, floater_frames_attack1, floater_run };

// PMM - circle strafe frames
static const mframe_t floater_frames_attack1a[] = {
    { ai_charge, 10 }, // Blaster attack
    { ai_charge, 10 },
    { ai_charge, 10 },
    { ai_charge, 10, floater_fire_blaster }, // BOOM (0, -25.8, 32.5)   -- LOOP Starts
    { ai_charge, 10, floater_fire_blaster },
    { ai_charge, 10, floater_fire_blaster },
    { ai_charge, 10, floater_fire_blaster },
    { ai_charge, 10, floater_fire_blaster },
    { ai_charge, 10, floater_fire_blaster },
    { ai_charge, 10, floater_fire_blaster },
    { ai_charge, 10 },
    { ai_charge, 10 },
    { ai_charge, 10 },
    { ai_charge, 10 } //                            -- LOOP Ends
};
const mmove_t MMOVE_T(floater_move_attack1a) = { FRAME_attak101, FRAME_attak114, floater_frames_attack1a, floater_run };
// pmm

static const mframe_t floater_frames_attack2[] = {
    { ai_charge }, // Claws
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
    { ai_charge, 0, floater_wham }, // WHAM (0, -45, 29.6)      -- LOOP Starts
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge }, //                           -- LOOP Ends
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(floater_move_attack2) = { FRAME_attak201, FRAME_attak225, floater_frames_attack2, floater_run };

static const mframe_t floater_frames_attack3[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, floater_zap }, //                               -- LOOP Starts
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
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge }, //                               -- LOOP Ends
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(floater_move_attack3) = { FRAME_attak301, FRAME_attak334, floater_frames_attack3, floater_run };

#if 0
static const mframe_t floater_frames_death[] = {
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
const mmove_t MMOVE_T(floater_move_death) = { FRAME_death01, FRAME_death13, floater_frames_death, floater_dead };
#endif

static const mframe_t floater_frames_pain1[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(floater_move_pain1) = { FRAME_pain101, FRAME_pain107, floater_frames_pain1, floater_run };

static const mframe_t floater_frames_pain2[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(floater_move_pain2) = { FRAME_pain201, FRAME_pain208, floater_frames_pain2, floater_run };

#if 0
static const mframe_t floater_frames_pain3[] = {
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
const mmove_t MMOVE_T(floater_move_pain3) = { FRAME_pain301, FRAME_pain312, floater_frames_pain3, floater_run };
#endif

static const mframe_t floater_frames_walk[] = {
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
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 5 }
};
const mmove_t MMOVE_T(floater_move_walk) = { FRAME_stand101, FRAME_stand152, floater_frames_walk, NULL };

static const mframe_t floater_frames_run[] = {
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 },
    { ai_run, 13 }
};
const mmove_t MMOVE_T(floater_move_run) = { FRAME_stand101, FRAME_stand152, floater_frames_run, NULL };

void MONSTERINFO_RUN(floater_run)(edict_t *self)
{
    if (self->monsterinfo.active_move == &floater_move_disguise)
        M_SetAnimation(self, &floater_move_pop);
    else if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        M_SetAnimation(self, &floater_move_stand1);
    else
        M_SetAnimation(self, &floater_move_run);
}

void MONSTERINFO_WALK(floater_walk)(edict_t *self)
{
    M_SetAnimation(self, &floater_move_walk);
}

static void floater_wham(edict_t *self)
{
    vec3_t aim = { MELEE_DISTANCE, 0, 0 };
    gi.sound(self, CHAN_WEAPON, sound_attack3, 1, ATTN_NORM, 0);

    if (!fire_hit(self, aim, irandom2(5, 11), -50))
        self->monsterinfo.melee_debounce_time = level.time + SEC(3);
}

static void floater_zap(edict_t *self)
{
    vec3_t forward, right;
    vec3_t origin;
    vec3_t dir;
    vec3_t offset;

    VectorSubtract(self->enemy->s.origin, self->s.origin, dir);

    AngleVectors(self->s.angles, forward, right, NULL);
    // FIXME use a flash and replace these two lines with the commented one
    VectorSet(offset, 18.5f, -0.9f, 10);
    M_ProjectFlashSource(self, offset, forward, right, origin);

    gi.sound(self, CHAN_WEAPON, sound_attack2, 1, ATTN_NORM, 0);

    // FIXME use the flash, Luke
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_SPLASH);
    gi.WriteByte(32);
    gi.WritePosition(origin);
    gi.WriteDir(dir);
    gi.WriteByte(SPLASH_SPARKS);
    gi.multicast(origin, MULTICAST_PVS);

    T_Damage(self->enemy, self, self, dir, self->enemy->s.origin, vec3_origin, irandom2(5, 11), -10, DAMAGE_ENERGY, (mod_t) { MOD_UNKNOWN });
}

void MONSTERINFO_ATTACK(floater_attack)(edict_t *self)
{
    if (brandom()) {
        self->monsterinfo.attack_state = AS_STRAIGHT;
        M_SetAnimation(self, &floater_move_attack1);
    } else { // circle strafe
        if (brandom()) // switch directions
            self->monsterinfo.lefty = !self->monsterinfo.lefty;
        self->monsterinfo.attack_state = AS_SLIDING;
        M_SetAnimation(self, &floater_move_attack1a);
    }
}

void MONSTERINFO_MELEE(floater_melee)(edict_t *self)
{
    if (brandom())
        M_SetAnimation(self, &floater_move_attack3);
    else
        M_SetAnimation(self, &floater_move_attack2);
}

void PAIN(floater_pain)(edict_t *self, edict_t *other, float kick, int damage, const mod_t mod)
{
    int n;

    if (level.time < self->pain_debounce_time)
        return;

    // no pain anims if poppin'
    if (self->monsterinfo.active_move == &floater_move_disguise ||
        self->monsterinfo.active_move == &floater_move_pop)
        return;

    n = irandom1(3);
    if (n == 0)
        gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
    else
        gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

    self->pain_debounce_time = level.time + SEC(3);

    if (!M_ShouldReactToPain(self, mod))
        return; // no pain anims in nightmare

    if (n == 0)
        M_SetAnimation(self, &floater_move_pain1);
    else
        M_SetAnimation(self, &floater_move_pain2);
}

void MONSTERINFO_SETSKIN(floater_setskin)(edict_t *self)
{
    if (self->health < (self->max_health / 2))
        self->s.skinnum = 1;
    else
        self->s.skinnum = 0;
}

#if 0
static void floater_dead(edict_t *self)
{
    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, -8);
    self->movetype = MOVETYPE_TOSS;
    self->svflags |= SVF_DEADMONSTER;
    self->nextthink = 0;
    gi.linkentity(self);
}
#endif

static const gib_def_t floater_gibs[] = {
    { "models/objects/gibs/sm_metal/tris.md2", 2 },
    { "models/objects/gibs/sm_meat/tris.md2", 3 },
    { "models/monsters/float/gibs/piece.md2", 1, GIB_SKINNED },
    { "models/monsters/float/gibs/gun.md2", 1, GIB_SKINNED },
    { "models/monsters/float/gibs/base.md2", 1, GIB_SKINNED },
    { "models/monsters/float/gibs/jar.md2", 1, GIB_SKINNED | GIB_HEAD },
    { 0 }
};

void DIE(floater_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, const mod_t mod)
{
    gi.sound(self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);

    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_EXPLOSION1);
    gi.WritePosition(self->s.origin);
    gi.multicast(self->s.origin, MULTICAST_PHS);

    self->s.skinnum /= 2;

    ThrowGibs(self, 55, floater_gibs);
}

static void float_set_fly_parameters(edict_t *self)
{
    self->monsterinfo.fly_thrusters = false;
    self->monsterinfo.fly_acceleration = 10;
    self->monsterinfo.fly_speed = 100;
    // Technician gets in closer because he has two melee attacks
    self->monsterinfo.fly_min_distance = 20;
    self->monsterinfo.fly_max_distance = 200;
}

static void floater_precache(void)
{
    sound_attack2 = gi.soundindex("floater/fltatck2.wav");
    sound_attack3 = gi.soundindex("floater/fltatck3.wav");
    sound_death1 = gi.soundindex("floater/fltdeth1.wav");
    sound_idle = gi.soundindex("floater/fltidle1.wav");
    sound_pain1 = gi.soundindex("floater/fltpain1.wav");
    sound_pain2 = gi.soundindex("floater/fltpain2.wav");
    sound_sight = gi.soundindex("floater/fltsght1.wav");
}

#define SPAWNFLAG_FLOATER_DISGUISE  8

/*QUAKED monster_floater (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight Disguise
 */
void SP_monster_floater(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    G_AddPrecache(floater_precache);

    gi.soundindex("floater/fltatck1.wav");

    self->monsterinfo.engine_sound = gi.soundindex("floater/fltsrch1.wav");

    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;
    self->s.modelindex = gi.modelindex("models/monsters/float/tris.md2");

    PrecacheGibs(floater_gibs);

    VectorSet(self->mins, -24, -24, -24);
    VectorSet(self->maxs, 24, 24, 48);

    self->health = 200 * st.health_multiplier;
    self->gib_health = -80;
    self->mass = 300;

    self->pain = floater_pain;
    self->die = floater_die;

    self->monsterinfo.stand = floater_stand;
    self->monsterinfo.walk = floater_walk;
    self->monsterinfo.run = floater_run;
    self->monsterinfo.attack = floater_attack;
    self->monsterinfo.melee = floater_melee;
    self->monsterinfo.sight = floater_sight;
    self->monsterinfo.idle = floater_idle;
    self->monsterinfo.setskin = floater_setskin;

    gi.linkentity(self);

    if (self->spawnflags & SPAWNFLAG_FLOATER_DISGUISE)
        M_SetAnimation(self, &floater_move_disguise);
    else if (brandom())
        M_SetAnimation(self, &floater_move_stand1);
    else
        M_SetAnimation(self, &floater_move_stand2);

    self->monsterinfo.scale = MODEL_SCALE;

    self->monsterinfo.aiflags |= AI_ALTERNATE_FLY;
    float_set_fly_parameters(self);

    flymonster_start(self);
}
