// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

GLADIATOR

==============================================================================
*/

#include "g_local.h"
#include "m_gladiator.h"

static int sound_pain1;
static int sound_pain2;
static int sound_die;
static int sound_die2;
static int sound_gun;
static int sound_gunb;
static int sound_cleaver_swing;
static int sound_cleaver_hit;
static int sound_cleaver_miss;
static int sound_idle;
static int sound_search;
static int sound_sight;

void MONSTERINFO_IDLE(gladiator_idle)(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

void MONSTERINFO_SIGHT(gladiator_sight)(edict_t *self, edict_t *other)
{
    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void MONSTERINFO_SEARCH(gladiator_search)(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

static void gladiator_cleaver_swing(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_cleaver_swing, 1, ATTN_NORM, 0);
}

static const mframe_t gladiator_frames_stand[] = {
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand }
};
const mmove_t MMOVE_T(gladiator_move_stand) = { FRAME_stand1, FRAME_stand7, gladiator_frames_stand, NULL };

void MONSTERINFO_STAND(gladiator_stand)(edict_t *self)
{
    M_SetAnimation(self, &gladiator_move_stand);
}

static const mframe_t gladiator_frames_walk[] = {
    { ai_walk, 15 },
    { ai_walk, 7 },
    { ai_walk, 6 },
    { ai_walk, 5 },
    { ai_walk, 2, monster_footstep },
    { ai_walk },
    { ai_walk, 2 },
    { ai_walk, 8 },
    { ai_walk, 12 },
    { ai_walk, 8 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 2, monster_footstep },
    { ai_walk, 2 },
    { ai_walk, 1 },
    { ai_walk, 8 }
};
const mmove_t MMOVE_T(gladiator_move_walk) = { FRAME_walk1, FRAME_walk16, gladiator_frames_walk, NULL };

void MONSTERINFO_WALK(gladiator_walk)(edict_t *self)
{
    M_SetAnimation(self, &gladiator_move_walk);
}

static const mframe_t gladiator_frames_run[] = {
    { ai_run, 23 },
    { ai_run, 14 },
    { ai_run, 14, monster_footstep },
    { ai_run, 21 },
    { ai_run, 12 },
    { ai_run, 13, monster_footstep }
};
const mmove_t MMOVE_T(gladiator_move_run) = { FRAME_run1, FRAME_run6, gladiator_frames_run, NULL };

void MONSTERINFO_RUN(gladiator_run)(edict_t *self)
{
    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        M_SetAnimation(self, &gladiator_move_stand);
    else
        M_SetAnimation(self, &gladiator_move_run);
}

static void GladiatorMelee(edict_t *self)
{
    vec3_t aim = { MELEE_DISTANCE, self->mins[0], -4 };
    if (fire_hit(self, aim, irandom2(20, 25), 300))
        gi.sound(self, CHAN_AUTO, sound_cleaver_hit, 1, ATTN_NORM, 0);
    else {
        gi.sound(self, CHAN_AUTO, sound_cleaver_miss, 1, ATTN_NORM, 0);
        self->monsterinfo.melee_debounce_time = level.time + SEC(1.5f);
    }
}

static const mframe_t gladiator_frames_attack_melee[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, gladiator_cleaver_swing },
    { ai_charge },
    { ai_charge, 0, GladiatorMelee },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, gladiator_cleaver_swing },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, GladiatorMelee },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(gladiator_move_attack_melee) = { FRAME_melee3, FRAME_melee16, gladiator_frames_attack_melee, gladiator_run };

void MONSTERINFO_MELEE(gladiator_melee)(edict_t *self)
{
    M_SetAnimation(self, &gladiator_move_attack_melee);
}

static void GladiatorGun(edict_t *self)
{
    vec3_t start;
    vec3_t dir;
    vec3_t forward, right;

    AngleVectors(self->s.angles, forward, right, NULL);
    M_ProjectFlashSource(self, monster_flash_offset[MZ2_GLADIATOR_RAILGUN_1], forward, right, start);

    // calc direction to where we targted
    VectorSubtract(self->pos1, start, dir);
    VectorNormalize(dir);

    monster_fire_railgun(self, start, dir, 50, 100, MZ2_GLADIATOR_RAILGUN_1);
}

static const mframe_t gladiator_frames_attack_gun[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, GladiatorGun },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, monster_footstep },
    { ai_charge }
};
const mmove_t MMOVE_T(gladiator_move_attack_gun) = { FRAME_attack1, FRAME_attack9, gladiator_frames_attack_gun, gladiator_run };

// RAFAEL
static void gladbGun(edict_t *self)
{
    vec3_t start;
    vec3_t dir;
    vec3_t forward, right;

    AngleVectors(self->s.angles, forward, right, NULL);
    M_ProjectFlashSource(self, monster_flash_offset[MZ2_GLADIATOR_RAILGUN_1], forward, right, start);

    // calc direction to where we targeted
    VectorSubtract(self->pos1, start, dir);
    VectorNormalize(dir);

    int damage = 35;
    int radius_damage = 45;

    if (self->s.frame > FRAME_attack3) {
        damage /= 2;
        radius_damage /= 2;
    }

    fire_plasma(self, start, dir, damage, 725, radius_damage, radius_damage);

    // save for aiming the shot
    VectorCopy(self->enemy->s.origin, self->pos1);
    self->pos1[2] += self->enemy->viewheight;
}

static void gladbGun_check(edict_t *self)
{
    if (skill->integer == 3)
        gladbGun(self);
}

static const mframe_t gladb_frames_attack_gun[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, gladbGun },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, gladbGun },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, gladbGun_check }
};
const mmove_t MMOVE_T(gladb_move_attack_gun) = { FRAME_attack1, FRAME_attack9, gladb_frames_attack_gun, gladiator_run };
// RAFAEL

void MONSTERINFO_ATTACK(gladiator_attack)(edict_t *self)
{
    float  range;

    // a small safe zone
    range = Distance(self->s.origin, self->enemy->s.origin);
    if (range <= (MELEE_DISTANCE + 32) && self->monsterinfo.melee_debounce_time <= level.time)
        return;
    if (!M_CheckClearShot(self, monster_flash_offset[MZ2_GLADIATOR_RAILGUN_1]))
        return;

    // charge up the railgun
    VectorCopy(self->enemy->s.origin, self->pos1); // save for aiming the shot
    self->pos1[2] += self->enemy->viewheight;
    // RAFAEL
    if (self->style == 1) {
        gi.sound(self, CHAN_WEAPON, sound_gunb, 1, ATTN_NORM, 0);
        M_SetAnimation(self, &gladb_move_attack_gun);
    } else {
        // RAFAEL
        gi.sound(self, CHAN_WEAPON, sound_gun, 1, ATTN_NORM, 0);
        M_SetAnimation(self, &gladiator_move_attack_gun);
        // RAFAEL
    }
    // RAFAEL
}

static const mframe_t gladiator_frames_pain[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(gladiator_move_pain) = { FRAME_pain2, FRAME_pain5, gladiator_frames_pain, gladiator_run };

static const mframe_t gladiator_frames_pain_air[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(gladiator_move_pain_air) = { FRAME_painup2, FRAME_painup6, gladiator_frames_pain_air, gladiator_run };

void PAIN(gladiator_pain)(edict_t *self, edict_t *other, float kick, int damage, mod_t mod)
{
    if (level.time < self->pain_debounce_time) {
        if ((self->velocity[2] > 100) && (self->monsterinfo.active_move == &gladiator_move_pain))
            M_SetAnimation(self, &gladiator_move_pain_air);
        return;
    }

    self->pain_debounce_time = level.time + SEC(3);

    if (brandom())
        gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
    else
        gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

    if (!M_ShouldReactToPain(self, mod))
        return; // no pain anims in nightmare

    if (self->velocity[2] > 100)
        M_SetAnimation(self, &gladiator_move_pain_air);
    else
        M_SetAnimation(self, &gladiator_move_pain);
}

void MONSTERINFO_SETSKIN(gladiator_setskin)(edict_t *self)
{
    if (self->health < (self->max_health / 2))
        self->s.skinnum |= 1;
    else
        self->s.skinnum &= ~1;
}

static void gladiator_dead(edict_t *self)
{
    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, -8);
    monster_dead(self);
}

static void gladiator_shrink(edict_t *self)
{
    self->maxs[2] = 0;
    self->svflags |= SVF_DEADMONSTER;
    gi.linkentity(self);
}

static const mframe_t gladiator_frames_death[] = {
    { ai_move },
    { ai_move },
    { ai_move, 0, gladiator_shrink },
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
const mmove_t MMOVE_T(gladiator_move_death) = { FRAME_death2, FRAME_death22, gladiator_frames_death, gladiator_dead };

static const gib_def_t gladiator_gibs[] = {
    { "models/objects/gibs/bone/tris.md2", 2 },
    { "models/objects/gibs/sm_meat/tris.md2", 2 },
    { "models/monsters/gladiatr/gibs/thigh.md2", 2, GIB_SKINNED },
    { "models/monsters/gladiatr/gibs/larm.md2", 1, GIB_SKINNED | GIB_UPRIGHT },
    { "models/monsters/gladiatr/gibs/rarm.md2", 1, GIB_SKINNED | GIB_UPRIGHT },
    { "models/monsters/gladiatr/gibs/chest.md2", 1, GIB_SKINNED },
    { "models/monsters/gladiatr/gibs/head.md2", 1, GIB_SKINNED | GIB_HEAD },
    { 0 }
};

void DIE(gladiator_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    // check for gib
    if (M_CheckGib(self, mod)) {
        gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

        self->s.skinnum /= 2;

        ThrowGibs(self, damage, gladiator_gibs);
        self->deadflag = true;
        return;
    }

    if (self->deadflag)
        return;

    // regular death
    gi.sound(self, CHAN_BODY, sound_die, 1, ATTN_NORM, 0);

    if (brandom())
        gi.sound(self, CHAN_VOICE, sound_die2, 1, ATTN_NORM, 0);

    self->deadflag = true;
    self->takedamage = true;

    M_SetAnimation(self, &gladiator_move_death);
}

//===========
// PGM
bool MONSTERINFO_BLOCKED(gladiator_blocked)(edict_t *self, float dist)
{
    if (blocked_checkplat(self, dist))
        return true;

    return false;
}
// PGM
//===========

static void gladiator_precache(void)
{
    sound_pain1 = gi.soundindex("gladiator/pain.wav");
    sound_pain2 = gi.soundindex("gladiator/gldpain2.wav");
    sound_die = gi.soundindex("gladiator/glddeth2.wav");
    sound_die2 = gi.soundindex("gladiator/death.wav");
    sound_cleaver_swing = gi.soundindex("gladiator/melee1.wav");
    sound_cleaver_hit = gi.soundindex("gladiator/melee2.wav");
    sound_cleaver_miss = gi.soundindex("gladiator/melee3.wav");
    sound_idle = gi.soundindex("gladiator/gldidle1.wav");
    sound_search = gi.soundindex("gladiator/gldsrch1.wav");
    sound_sight = gi.soundindex("gladiator/sight.wav");
}

static void gladiator_precache_a(void)
{
    sound_gun = gi.soundindex("gladiator/railgun.wav");
}

static void gladiator_precache_b(void)
{
    sound_gunb = gi.soundindex("weapons/plasshot.wav");
}

/*QUAKED monster_gladiator (1 .5 0) (-32 -32 -24) (32 32 64) Ambush Trigger_Spawn Sight
 */
void SP_monster_gladiator(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    G_AddPrecache(gladiator_precache);

    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;
    self->s.modelindex = gi.modelindex("models/monsters/gladiatr/tris.md2");

    PrecacheGibs(gladiator_gibs);

    // RAFAEL
    if (strcmp(self->classname, "monster_gladb") == 0) {
        G_AddPrecache(gladiator_precache_b);

        self->health = 250 * st.health_multiplier;
        self->mass = 350;

        if (!ED_WasKeySpecified("power_armor_type"))
            self->monsterinfo.power_armor_type = IT_ITEM_POWER_SHIELD;
        if (!ED_WasKeySpecified("power_armor_power"))
            self->monsterinfo.power_armor_power = 250;

        self->s.skinnum = 2;

        self->style = 1;

        self->monsterinfo.weapon_sound = gi.soundindex("weapons/phaloop.wav");
    } else {
        // RAFAEL
        G_AddPrecache(gladiator_precache_a);

        self->health = 400 * st.health_multiplier;
        self->mass = 400;
        // RAFAEL

        self->monsterinfo.weapon_sound = gi.soundindex("weapons/rg_hum.wav");
    }
    // RAFAEL

    self->gib_health = -175;

    VectorSet(self->mins, -32, -32, -24);
    VectorSet(self->maxs, 32, 32, 42);

    self->pain = gladiator_pain;
    self->die = gladiator_die;

    self->monsterinfo.stand = gladiator_stand;
    self->monsterinfo.walk = gladiator_walk;
    self->monsterinfo.run = gladiator_run;
    self->monsterinfo.dodge = NULL;
    self->monsterinfo.attack = gladiator_attack;
    self->monsterinfo.melee = gladiator_melee;
    self->monsterinfo.sight = gladiator_sight;
    self->monsterinfo.idle = gladiator_idle;
    self->monsterinfo.search = gladiator_search;
    self->monsterinfo.blocked = gladiator_blocked; // PGM
    self->monsterinfo.setskin = gladiator_setskin;

    gi.linkentity(self);
    M_SetAnimation(self, &gladiator_move_stand);
    self->monsterinfo.scale = MODEL_SCALE;

    walkmonster_start(self);
}

//
// monster_gladb
// RAFAEL
//
/*QUAKED monster_gladb (1 .5 0) (-32 -32 -24) (32 32 64) Ambush Trigger_Spawn Sight
 */
void SP_monster_gladb(edict_t *self)
{
    SP_monster_gladiator(self);
}
