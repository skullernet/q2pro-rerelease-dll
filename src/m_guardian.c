// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

GUARDIAN

==============================================================================
*/

#include "g_local.h"
#include "m_guardian.h"

//
// stand
//

static const mframe_t guardian_frames_stand[] = {
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
const mmove_t MMOVE_T(guardian_move_stand) = { FRAME_idle1, FRAME_idle52, guardian_frames_stand, NULL };

void MONSTERINFO_STAND(guardian_stand)(edict_t *self)
{
    M_SetAnimation(self, &guardian_move_stand);
}

//
// walk
//

static int sound_step;

static void guardian_footstep(edict_t *self)
{
    gi.sound(self, CHAN_BODY, sound_step, 1, ATTN_NORM, 0);
}

static const mframe_t guardian_frames_walk[] = {
    { ai_walk, 8 },
    { ai_walk, 8 },
    { ai_walk, 8 },
    { ai_walk, 8 },
    { ai_walk, 8 },
    { ai_walk, 8 },
    { ai_walk, 8 },
    { ai_walk, 8, guardian_footstep },
    { ai_walk, 8 },
    { ai_walk, 8 },
    { ai_walk, 8 },
    { ai_walk, 8 },
    { ai_walk, 8 },
    { ai_walk, 8 },
    { ai_walk, 8 },
    { ai_walk, 8 },
    { ai_walk, 8 },
    { ai_walk, 8, guardian_footstep },
    { ai_walk, 8 }
};
const mmove_t MMOVE_T(guardian_move_walk) = { FRAME_walk1, FRAME_walk19, guardian_frames_walk, NULL };

void MONSTERINFO_WALK(guardian_walk)(edict_t *self)
{
    M_SetAnimation(self, &guardian_move_walk);
}

//
// run
//

static const mframe_t guardian_frames_run[] = {
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8, guardian_footstep },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8 },
    { ai_run, 8, guardian_footstep },
    { ai_run, 8 }
};
const mmove_t MMOVE_T(guardian_move_run) = { FRAME_walk1, FRAME_walk19, guardian_frames_run, NULL };

void MONSTERINFO_RUN(guardian_run)(edict_t *self)
{
    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        M_SetAnimation(self, &guardian_move_stand);
    else
        M_SetAnimation(self, &guardian_move_run);
}

//
// pain
//

static const mframe_t guardian_frames_pain1[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(guardian_move_pain1) = { FRAME_pain1_1, FRAME_pain1_8, guardian_frames_pain1, guardian_run };

void PAIN(guardian_pain)(edict_t *self, edict_t *other, float kick, int damage, mod_t mod)
{
    if (mod.id != MOD_CHAINFIST && damage <= 10)
        return;

    if (level.time < self->pain_debounce_time)
        return;

    if (mod.id != MOD_CHAINFIST && damage <= 75 && frandom() > 0.2f)
        return;

    // don't go into pain while attacking
    if ((self->s.frame >= FRAME_atk1_spin1) && (self->s.frame <= FRAME_atk1_spin15))
        return;
    if ((self->s.frame >= FRAME_atk2_fire1) && (self->s.frame <= FRAME_atk2_fire4))
        return;
    if ((self->s.frame >= FRAME_kick_in1) && (self->s.frame <= FRAME_kick_in13))
        return;

    self->pain_debounce_time = level.time + SEC(3);
    //gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

    if (!M_ShouldReactToPain(self, mod))
        return; // no pain anims in nightmare

    M_SetAnimation(self, &guardian_move_pain1);
    self->monsterinfo.weapon_sound = 0;
}

static const mframe_t guardian_frames_atk1_out[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(guardian_atk1_out) = { FRAME_atk1_out1, FRAME_atk1_out3, guardian_frames_atk1_out, guardian_run };

static void guardian_atk1_finish(edict_t *self)
{
    M_SetAnimation(self, &guardian_atk1_out);
    self->monsterinfo.weapon_sound = 0;
}

static int sound_charge;
static int sound_spin_loop;

static void guardian_atk1_charge(edict_t *self)
{
    self->monsterinfo.weapon_sound = sound_spin_loop;
    gi.sound(self, CHAN_WEAPON, sound_charge, 1, ATTN_NORM, 0);
}

static void guardian_fire_blaster(edict_t *self)
{
    vec3_t forward, right, target;
    vec3_t start;
    monster_muzzleflash_id_t id = MZ2_GUARDIAN_BLASTER;

    AngleVectors(self->s.angles, forward, right, NULL);
    M_ProjectFlashSource(self, monster_flash_offset[id], forward, right, start);
    VectorCopy(self->enemy->s.origin, target);
    target[2] += self->enemy->viewheight;
    for (int i = 0; i < 3; i++)
        target[i] += crandom_open() * 5;
    VectorSubtract(target, start, forward);
    VectorNormalize(forward);

    monster_fire_blaster(self, start, forward, 2, 1000, id, (self->s.frame % 4) ? EF_NONE : EF_HYPERBLASTER);

    if (self->enemy && self->enemy->health > 0 && self->s.frame == FRAME_atk1_spin12 && self->timestamp > level.time && visible(self, self->enemy))
        self->monsterinfo.nextframe = FRAME_atk1_spin5;
}

static const mframe_t guardian_frames_atk1_spin[] = {
    { ai_charge, 0, guardian_atk1_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, guardian_fire_blaster },
    { ai_charge, 0, guardian_fire_blaster },
    { ai_charge, 0, guardian_fire_blaster },
    { ai_charge, 0, guardian_fire_blaster },
    { ai_charge, 0, guardian_fire_blaster },
    { ai_charge, 0, guardian_fire_blaster },
    { ai_charge, 0, guardian_fire_blaster },
    { ai_charge, 0, guardian_fire_blaster },
    { ai_charge, 0 },
    { ai_charge, 0 },
    { ai_charge, 0 }
};
const mmove_t MMOVE_T(guardian_move_atk1_spin) = { FRAME_atk1_spin1, FRAME_atk1_spin15, guardian_frames_atk1_spin, guardian_atk1_finish };

static void guardian_atk1(edict_t *self)
{
    M_SetAnimation(self, &guardian_move_atk1_spin);
    self->timestamp = level.time + random_time_sec(0.65f, 2.15f);
}

static const mframe_t guardian_frames_atk1_in[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(guardian_move_atk1_in) = { FRAME_atk1_in1, FRAME_atk1_in3, guardian_frames_atk1_in, guardian_atk1 };

static const mframe_t guardian_frames_atk2_out[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, guardian_footstep },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(guardian_move_atk2_out) = { FRAME_atk2_out1, FRAME_atk2_out7, guardian_frames_atk2_out, guardian_run };

static void guardian_atk2_out(edict_t *self)
{
    M_SetAnimation(self, &guardian_move_atk2_out);
}

static int sound_laser;

static const vec3_t laser_positions[] = {
    { 125, -70, 60 },
    { 112, -62, 60 }
};

void PRETHINK(guardian_fire_update)(edict_t *laser)
{
    edict_t *self = laser->owner;

    vec3_t forward, right, target;
    vec3_t start;

    AngleVectors(self->s.angles, forward, right, NULL);
    M_ProjectFlashSource(self, laser_positions[1 - (self->s.frame & 1)], forward, right, start);
    VectorAdd(self->enemy->s.origin, self->enemy->mins, target);
    VectorMA(target, frandom(), self->enemy->size, target);
    VectorSubtract(target, start, forward);
    VectorNormalize(forward);

    VectorCopy(start, laser->s.origin);
    VectorCopy(forward, laser->movedir);
    gi.linkentity(laser);
    dabeam_update(laser, false);
}

static void guardian_laser_fire(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_laser, 1, ATTN_NORM, 0);
    monster_fire_dabeam(self, 25, self->s.frame & 1, guardian_fire_update);
}

static const mframe_t guardian_frames_atk2_fire[] = {
    { ai_charge, 0, guardian_laser_fire },
    { ai_charge, 0, guardian_laser_fire },
    { ai_charge, 0, guardian_laser_fire },
    { ai_charge, 0, guardian_laser_fire }
};
const mmove_t MMOVE_T(guardian_move_atk2_fire) = { FRAME_atk2_fire1, FRAME_atk2_fire4, guardian_frames_atk2_fire, guardian_atk2_out };

static void guardian_atk2(edict_t *self)
{
    M_SetAnimation(self, &guardian_move_atk2_fire);
}

static const mframe_t guardian_frames_atk2_in[] = {
    { ai_charge, 0, guardian_footstep },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, guardian_footstep },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, guardian_footstep },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(guardian_move_atk2_in) = { FRAME_atk2_in1, FRAME_atk2_in12, guardian_frames_atk2_in, guardian_atk2 };

static void guardian_kick(edict_t *self)
{
    vec3_t aim = { MELEE_DISTANCE, 0, -80 };
    if (!fire_hit(self, aim, 85, 700))
        self->monsterinfo.melee_debounce_time = level.time + SEC(1);
}

static const mframe_t guardian_frames_kick[] = {
    { ai_charge },
    { ai_charge, 0, guardian_footstep },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, guardian_kick },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, guardian_footstep },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(guardian_move_kick) = { FRAME_kick_in1, FRAME_kick_in13, guardian_frames_kick, guardian_run };

void MONSTERINFO_ATTACK(guardian_attack)(edict_t *self)
{
    if (!self->enemy || !self->enemy->inuse)
        return;

    float r = range_to(self, self->enemy);

    if (r > RANGE_NEAR)
        M_SetAnimation(self, &guardian_move_atk2_in);
    else if (self->monsterinfo.melee_debounce_time < level.time && r < 120)
        M_SetAnimation(self, &guardian_move_kick);
    else
        M_SetAnimation(self, &guardian_move_atk1_in);
}

//
// death
//

static void guardian_explode(edict_t *self)
{
    vec3_t pos;

    VectorAdd(self->s.origin, self->mins, pos);
    VectorMA(pos, frandom(), self->size, pos);

    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_EXPLOSION1_BIG);
    gi.WritePosition(pos);
    gi.multicast(self->s.origin, MULTICAST_ALL);
}

static const gib_def_t guardian_gibs[] = {
    { "models/objects/gibs/sm_meat/tris.md2", 2 },
    { "models/objects/gibs/sm_metal/tris.md2", 4, GIB_METALLIC },
    { "models/monsters/guardian/gib1.md2", 2, GIB_METALLIC },
    { "models/monsters/guardian/gib2.md2", 2, GIB_METALLIC },
    { "models/monsters/guardian/gib3.md2", 2, GIB_METALLIC },
    { "models/monsters/guardian/gib4.md2", 2, GIB_METALLIC },
    { "models/monsters/guardian/gib5.md2", 2, GIB_METALLIC },
    { "models/monsters/guardian/gib6.md2", 2, GIB_METALLIC },
    { "models/monsters/guardian/gib7.md2", 1, GIB_METALLIC | GIB_HEAD },
    { 0 }
};

static void guardian_dead(edict_t *self)
{
    for (int i = 0; i < 3; i++)
        guardian_explode(self);

    ThrowGibs(self, 125, guardian_gibs);
}

static const mframe_t guardian_frames_death1[] = {
    { ai_move, 0, BossExplode },
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
const mmove_t MMOVE_T(guardian_move_death) = { FRAME_death1, FRAME_death26, guardian_frames_death1, guardian_dead };

void DIE(guardian_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    // regular death
    //gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
    self->monsterinfo.weapon_sound = 0;
    self->deadflag = true;
    self->takedamage = true;

    M_SetAnimation(self, &guardian_move_death);
}

//
// monster_tank
//

static void guadian_precache(void)
{
    sound_step = gi.soundindex("zortemp/step.wav");
    sound_charge = gi.soundindex("weapons/hyprbu1a.wav");
    sound_spin_loop = gi.soundindex("weapons/hyprbl1a.wav");
    sound_laser = gi.soundindex("weapons/laser2.wav");
}

/*QUAKED monster_guardian (1 .5 0) (-96 -96 -66) (96 96 62) Ambush Trigger_Spawn Sight
 */
void SP_monster_guardian(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    G_AddPrecache(guadian_precache);

    PrecacheGibs(guardian_gibs);

    self->s.modelindex = gi.modelindex("models/monsters/guardian/tris.md2");
    VectorSet(self->mins, -96, -96, -66);
    VectorSet(self->maxs, 96, 96, 62);
    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;

    self->health = 2500 * st.health_multiplier;
    self->gib_health = -200;

    self->monsterinfo.scale = MODEL_SCALE;

    self->mass = 850;

    self->pain = guardian_pain;
    self->die = guardian_die;
    self->monsterinfo.stand = guardian_stand;
    self->monsterinfo.walk = guardian_walk;
    self->monsterinfo.run = guardian_run;
    self->monsterinfo.attack = guardian_attack;

    gi.linkentity(self);

    M_SetAnimation(self, &guardian_move_stand);

    walkmonster_start(self);
}
