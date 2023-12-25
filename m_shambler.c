// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

SHAMBLER

==============================================================================
*/

#include "g_local.h"
#include "m_shambler.h"

static int sound_pain;
static int sound_idle;
static int sound_die;
static int sound_sight;
static int sound_windup;
static int sound_melee1;
static int sound_melee2;
static int sound_smack;
static int sound_boom;

//
// misc
//

void MONSTERINFO_SIGHT(shambler_sight)(edict_t *self, edict_t *other)
{
    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static const vec3_t lightning_left_hand[] = {
    { 44, 36, 25 },
    { 10, 44, 57 },
    { -1, 40, 70 },
    { -10, 34, 75 },
    { 7.4f, 24, 89 }
};

static const vec3_t lightning_right_hand[] = {
    { 28, -38, 25 },
    { 31, -7, 70 },
    { 20, 0, 80 },
    { 16, 1.2f, 81 },
    { 27, -11, 83 }
};

static void shambler_lightning_update(edict_t *self)
{
    edict_t *lightning = self->beam;

    if (!lightning)
        return;

    if (self->s.frame >= FRAME_magic01 + q_countof(lightning_left_hand)) {
        G_FreeEdict(lightning);
        self->beam = NULL;
        return;
    }

    vec3_t f, r;
    AngleVectors(self->s.angles, f, r, NULL);
    M_ProjectFlashSource(self, lightning_left_hand[self->s.frame - FRAME_magic01], f, r, lightning->s.origin);
    M_ProjectFlashSource(self, lightning_right_hand[self->s.frame - FRAME_magic01], f, r, lightning->s.old_origin);
    gi.linkentity(lightning);
}

static void shambler_windup(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_windup, 1, ATTN_NORM, 0);

    edict_t *lightning = self->beam = G_Spawn();
    lightning->s.modelindex = gi.modelindex("models/proj/lightning/tris.md2");
    lightning->s.renderfx |= RF_BEAM;
    lightning->owner = self;
    shambler_lightning_update(self);
}

void MONSTERINFO_IDLE(shambler_idle)(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

static void shambler_maybe_idle(edict_t *self)
{
    if (frandom() > 0.8f)
        gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

//
// stand
//

static const mframe_t shambler_frames_stand[] = {
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
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
const mmove_t MMOVE_T(shambler_move_stand) = { FRAME_stand01, FRAME_stand17, shambler_frames_stand, NULL };

void MONSTERINFO_STAND(shambler_stand)(edict_t *self)
{
    M_SetAnimation(self, &shambler_move_stand);
}

//
// walk
//

static const mframe_t shambler_frames_walk[] = {
    { ai_walk, 10 }, // FIXME: add footsteps?
    { ai_walk, 9 },
    { ai_walk, 9 },
    { ai_walk, 5 },
    { ai_walk, 6 },
    { ai_walk, 12 },
    { ai_walk, 8 },
    { ai_walk, 3 },
    { ai_walk, 13 },
    { ai_walk, 9 },
    { ai_walk, 7, shambler_maybe_idle },
    { ai_walk, 5 },
};
const mmove_t MMOVE_T(shambler_move_walk) = { FRAME_walk01, FRAME_walk12, shambler_frames_walk, NULL };

void MONSTERINFO_WALK(shambler_walk)(edict_t *self)
{
    M_SetAnimation(self, &shambler_move_walk);
}

//
// run
//

static const mframe_t shambler_frames_run[] = {
    { ai_run, 20 }, // FIXME: add footsteps?
    { ai_run, 24 },
    { ai_run, 20 },
    { ai_run, 20 },
    { ai_run, 24 },
    { ai_run, 20, shambler_maybe_idle },
};
const mmove_t MMOVE_T(shambler_move_run) = { FRAME_run01, FRAME_run06, shambler_frames_run, NULL };

void MONSTERINFO_RUN(shambler_run)(edict_t *self)
{
    if (self->enemy && self->enemy->client)
        self->monsterinfo.aiflags |= AI_BRUTAL;
    else
        self->monsterinfo.aiflags &= ~AI_BRUTAL;

    if (self->monsterinfo.aiflags & AI_STAND_GROUND) {
        M_SetAnimation(self, &shambler_move_stand);
        return;
    }

    M_SetAnimation(self, &shambler_move_run);
}

//
// pain
//

// FIXME: needs halved explosion damage

static const mframe_t shambler_frames_pain[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
};
const mmove_t MMOVE_T(shambler_move_pain) = { FRAME_pain01, FRAME_pain06, shambler_frames_pain, shambler_run };

void PAIN(shambler_pain)(edict_t *self, edict_t *other, float kick, int damage, const mod_t mod)
{
    if (level.time < self->timestamp)
        return;

    self->timestamp = level.time + FRAME_TIME;
    gi.sound(self, CHAN_AUTO, sound_pain, 1, ATTN_NORM, 0);

    if (mod.id != MOD_CHAINFIST && damage <= 30 && frandom() > 0.2f)
        return;

    // If hard or nightmare, don't go into pain while attacking
    if (skill->integer >= 2) {
        if ((self->s.frame >= FRAME_smash01) && (self->s.frame <= FRAME_smash12))
            return;
        if ((self->s.frame >= FRAME_swingl01) && (self->s.frame <= FRAME_swingl09))
            return;
        if ((self->s.frame >= FRAME_swingr01) && (self->s.frame <= FRAME_swingr09))
            return;
    }

    if (!M_ShouldReactToPain(self, mod))
        return; // no pain anims in nightmare

    if (level.time < self->pain_debounce_time)
        return;

    self->pain_debounce_time = level.time + SEC(2);
    M_SetAnimation(self, &shambler_move_pain);
}

void MONSTERINFO_SETSKIN(shambler_setskin)(edict_t *self)
{
    // FIXME: create pain skin?
    //if (self->health < (self->max_health / 2))
    //  self->s.skinnum |= 1;
    //else
    //  self->s.skinnum &= ~1;
}

//
// attacks
//

static void ShamblerSaveLoc(edict_t *self)
{
    VectorCopy(self->enemy->s.origin, self->pos1); // save for aiming the shot
    self->pos1[2] += self->enemy->viewheight;
    self->monsterinfo.nextframe = FRAME_magic09;

    gi.sound(self, CHAN_WEAPON, sound_boom, 1, ATTN_NORM, 0);
    shambler_lightning_update(self);
}

#define SPAWNFLAG_SHAMBLER_PRECISE  1

static void FindShamblerOffset(edict_t *self, vec3_t offset)
{
    VectorSet(offset, 0, 0, 48);

    for (int i = 0; i < 8; i++) {
        if (M_CheckClearShot(self, offset))
            return;
        offset[2] -= 4;
    }

    VectorSet(offset, 0, 0, 48);
}

static void ShamblerCastLightning(edict_t *self)
{
    if (!self->enemy)
        return;

    vec3_t start, end;
    vec3_t dir;
    vec3_t forward, right;
    vec3_t offset;

    FindShamblerOffset(self, offset);

    AngleVectors(self->s.angles, forward, right, NULL);
    M_ProjectFlashSource(self, offset, forward, right, start);

    // calc direction to where we targted
    PredictAim(self, self->enemy, start, 0, false, (self->spawnflags & SPAWNFLAG_SHAMBLER_PRECISE) ? 0.0f : 0.1f, dir, NULL);

    VectorMA(start, 8192, dir, end);
    trace_t tr = gi.trace(start, NULL, NULL, end, self, MASK_PROJECTILE | CONTENTS_SLIME | CONTENTS_LAVA);

    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_LIGHTNING);
    gi.WriteShort(self - g_edicts); // source entity
    gi.WriteShort(world - g_edicts); // destination entity
    gi.WritePosition(start);
    gi.WritePosition(tr.endpos);
    gi.multicast(start, MULTICAST_PVS);

    fire_bullet(self, start, dir, irandom2(8, 12), 15, 0, 0, (mod_t) { MOD_TESLA });
}

static const mframe_t shambler_frames_magic[] = {
    { ai_charge, 0, shambler_windup },
    { ai_charge, 0, shambler_lightning_update },
    { ai_charge, 0, shambler_lightning_update },
    { ai_move, 0, shambler_lightning_update },
    { ai_move, 0, shambler_lightning_update },
    { ai_move, 0, ShamblerSaveLoc},
    { ai_move },
    { ai_charge },
    { ai_move, 0, ShamblerCastLightning },
    { ai_move, 0, ShamblerCastLightning },
    { ai_move, 0, ShamblerCastLightning },
    { ai_move },
};

const mmove_t MMOVE_T(shambler_attack_magic) = { FRAME_magic01, FRAME_magic12, shambler_frames_magic, shambler_run };

void MONSTERINFO_ATTACK(shambler_attack)(edict_t *self)
{
    M_SetAnimation(self, &shambler_attack_magic);
}

//
// melee
//

static void shambler_melee1(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_melee1, 1, ATTN_NORM, 0);
}

static void shambler_melee2(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_melee2, 1, ATTN_NORM, 0);
}

static void sham_swingl9(edict_t *self);
static void sham_swingr9(edict_t *self);

static void sham_smash10(edict_t *self)
{
    if (!self->enemy)
        return;

    ai_charge(self, 0);

    if (!CanDamage(self->enemy, self))
        return;

    vec3_t aim = { MELEE_DISTANCE, self->mins[0], -4 };
    bool hit = fire_hit(self, aim, irandom2(110, 120), 120); // Slower attack

    if (hit)
        gi.sound(self, CHAN_WEAPON, sound_smack, 1, ATTN_NORM, 0);
}

static void ShamClaw(edict_t *self)
{
    if (!self->enemy)
        return;

    ai_charge(self, 10);

    if (!CanDamage(self->enemy, self))
        return;

    vec3_t aim = { MELEE_DISTANCE, self->mins[0], -4 };
    bool hit = fire_hit(self, aim, irandom2(70, 80), 80); // Slower attack

    if (hit)
        gi.sound(self, CHAN_WEAPON, sound_smack, 1, ATTN_NORM, 0);
}

static const mframe_t shambler_frames_smash[] = {
    { ai_charge, 2, shambler_melee1 },
    { ai_charge, 6 },
    { ai_charge, 6 },
    { ai_charge, 5 },
    { ai_charge, 4 },
    { ai_charge, 1 },
    { ai_charge, 0 },
    { ai_charge, 0 },
    { ai_charge, 0 },
    { ai_charge, 0, sham_smash10 },
    { ai_charge, 5 },
    { ai_charge, 4 },
};

const mmove_t MMOVE_T(shambler_attack_smash) = { FRAME_smash01, FRAME_smash12, shambler_frames_smash, shambler_run };

static const mframe_t shambler_frames_swingl[] = {
    { ai_charge, 5, shambler_melee1 },
    { ai_charge, 3 },
    { ai_charge, 7 },
    { ai_charge, 3 },
    { ai_charge, 7 },
    { ai_charge, 9 },
    { ai_charge, 5, ShamClaw },
    { ai_charge, 4 },
    { ai_charge, 8, sham_swingl9 },
};

const mmove_t MMOVE_T(shambler_attack_swingl) = { FRAME_swingl01, FRAME_swingl09, shambler_frames_swingl, shambler_run };

static const mframe_t shambler_frames_swingr[] = {
    { ai_charge, 1, shambler_melee2 },
    { ai_charge, 8 },
    { ai_charge, 14 },
    { ai_charge, 7 },
    { ai_charge, 3 },
    { ai_charge, 6 },
    { ai_charge, 6, ShamClaw },
    { ai_charge, 3 },
    { ai_charge, 8, sham_swingr9 },
};

const mmove_t MMOVE_T(shambler_attack_swingr) = { FRAME_swingr01, FRAME_swingr09, shambler_frames_swingr, shambler_run };

static void sham_swingl9(edict_t *self)
{
    ai_charge(self, 8);

    if (brandom() && self->enemy && range_to(self, self->enemy) < MELEE_DISTANCE)
        M_SetAnimation(self, &shambler_attack_swingr);
}

static void sham_swingr9(edict_t *self)
{
    ai_charge(self, 1);
    ai_charge(self, 10);

    if (brandom() && self->enemy && range_to(self, self->enemy) < MELEE_DISTANCE)
        M_SetAnimation(self, &shambler_attack_swingl);
}

void MONSTERINFO_MELEE(shambler_melee)(edict_t *self)
{
    float chance = frandom();

    if (chance > 0.6f || self->health == 600)
        M_SetAnimation(self, &shambler_attack_smash);
    else if (chance > 0.3f)
        M_SetAnimation(self, &shambler_attack_swingl);
    else
        M_SetAnimation(self, &shambler_attack_swingr);
}

//
// death
//

static void shambler_dead(edict_t *self)
{
    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, -0);
    monster_dead(self);
}

static void shambler_shrink(edict_t *self)
{
    self->maxs[2] = 0;
    self->svflags |= SVF_DEADMONSTER;
    gi.linkentity(self);
}

static const mframe_t shambler_frames_death[] = {
    { ai_move, 0 },
    { ai_move, 0 },
    { ai_move, 0, shambler_shrink },
    { ai_move, 0 },
    { ai_move, 0 },
    { ai_move, 0 },
    { ai_move, 0 },
    { ai_move, 0 },
    { ai_move, 0 },
    { ai_move, 0 },
    { ai_move, 0 }, // FIXME: thud?
};
const mmove_t MMOVE_T(shambler_move_death) = { FRAME_death01, FRAME_death11, shambler_frames_death, shambler_dead };

// FIXME: better gibs for shambler, shambler head
static const gib_def_t shambler_gibs[] = {
    { "models/objects/gibs/sm_meat/tris.md2", 1 },
    { "models/objects/gibs/chest/tris.md2", 1 },
    { "models/objects/gibs/head2/tris.md2", 1, GIB_HEAD },
    { 0 }
};

void DIE(shambler_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, const mod_t mod)
{
    if (self->beam) {
        G_FreeEdict(self->beam);
        self->beam = NULL;
    }

    if (self->beam2) {
        G_FreeEdict(self->beam2);
        self->beam2 = NULL;
    }

    // check for gib
    if (M_CheckGib(self, mod)) {
        gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
        ThrowGibs(self, damage, shambler_gibs);
        self->deadflag = true;
        return;
    }

    if (self->deadflag)
        return;

    // regular death
    gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
    self->deadflag = true;
    self->takedamage = true;

    M_SetAnimation(self, &shambler_move_death);
}

static void shambler_precache(void)
{
    sound_pain = gi.soundindex("shambler/shurt2.wav");
    sound_idle = gi.soundindex("shambler/sidle.wav");
    sound_die = gi.soundindex("shambler/sdeath.wav");
    sound_windup = gi.soundindex("shambler/sattck1.wav");
    sound_melee1 = gi.soundindex("shambler/melee1.wav");
    sound_melee2 = gi.soundindex("shambler/melee2.wav");
    sound_sight = gi.soundindex("shambler/ssight.wav");
    sound_smack = gi.soundindex("shambler/smack.wav");
    sound_boom = gi.soundindex("shambler/sboom.wav");
}

void SP_monster_shambler(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    self->s.modelindex = gi.modelindex("models/monsters/shambler/tris.md2");
    VectorSet(self->mins, -32, -32, -24);
    VectorSet(self->maxs, 32, 32, 64);
    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;

    gi.modelindex("models/proj/lightning/tris.md2");

    G_AddPrecache(shambler_precache);

    self->health = 600 * st.health_multiplier;
    self->gib_health = -60;

    self->mass = 500;

    self->pain = shambler_pain;
    self->die = shambler_die;
    self->monsterinfo.stand = shambler_stand;
    self->monsterinfo.walk = shambler_walk;
    self->monsterinfo.run = shambler_run;
    self->monsterinfo.dodge = NULL;
    self->monsterinfo.attack = shambler_attack;
    self->monsterinfo.melee = shambler_melee;
    self->monsterinfo.sight = shambler_sight;
    self->monsterinfo.idle = shambler_idle;
    self->monsterinfo.blocked = NULL;
    self->monsterinfo.setskin = shambler_setskin;

    gi.linkentity(self);

    if (self->spawnflags & SPAWNFLAG_SHAMBLER_PRECISE)
        self->monsterinfo.aiflags |= AI_IGNORE_SHOTS;

    M_SetAnimation(self, &shambler_move_stand);
    self->monsterinfo.scale = MODEL_SCALE;

    walkmonster_start(self);
}
