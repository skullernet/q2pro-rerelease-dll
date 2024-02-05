// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

jorg

==============================================================================
*/

#include "g_local.h"
#include "m_boss31.h"

void SP_monster_makron(edict_t *self);

static int sound_pain1;
static int sound_pain2;
static int sound_pain3;
static int sound_idle;
static int sound_death;
static int sound_search1;
static int sound_search2;
static int sound_search3;
static int sound_attack1, sound_attack1_loop, sound_attack1_end;
static int sound_attack2, sound_bfg_fire;
static int sound_firegun;
static int sound_step_left;
static int sound_step_right;
static int sound_death_hit;

void MakronToss(edict_t *self);
void MakronPrecache(void);

static void jorg_attack1_end_sound(edict_t *self)
{
    if (self->monsterinfo.weapon_sound) {
        gi.sound(self, CHAN_WEAPON, sound_attack1_end, 1, ATTN_NORM, 0);
        self->monsterinfo.weapon_sound = 0;
    }
}

void MONSTERINFO_SEARCH(jorg_search)(edict_t *self)
{
    float r = frandom();

    if (r <= 0.3f)
        gi.sound(self, CHAN_VOICE, sound_search1, 1, ATTN_NORM, 0);
    else if (r <= 0.6f)
        gi.sound(self, CHAN_VOICE, sound_search2, 1, ATTN_NORM, 0);
    else
        gi.sound(self, CHAN_VOICE, sound_search3, 1, ATTN_NORM, 0);
}

static void jorg_dead(edict_t *self);
static void jorgBFG(edict_t *self);
static void jorg_firebullet(edict_t *self);
static void jorg_reattack1(edict_t *self);
static void jorg_attack1(edict_t *self);
static void jorg_idle(edict_t *self);
static void jorg_step_left(edict_t *self);
static void jorg_step_right(edict_t *self);
static void jorg_death_hit(edict_t *self);

//
// stand
//

static const mframe_t jorg_frames_stand[] = {
    { ai_stand, 0, jorg_idle },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand }, // 10
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand }, // 20
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand }, // 30
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand, 19 },
    { ai_stand, 11, jorg_step_left },
    { ai_stand },
    { ai_stand },
    { ai_stand, 6 },
    { ai_stand, 9, jorg_step_right },
    { ai_stand }, // 40
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand, -2, NULL },
    { ai_stand, -17, jorg_step_left },
    { ai_stand },
    { ai_stand, -12 },                 // 50
    { ai_stand, -14, jorg_step_right } // 51
};
const mmove_t MMOVE_T(jorg_move_stand) = { FRAME_stand01, FRAME_stand51, jorg_frames_stand, NULL };

static void jorg_idle(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_NORM, 0);
}

static void jorg_death_hit(edict_t *self)
{
    gi.sound(self, CHAN_BODY, sound_death_hit, 1, ATTN_NORM, 0);
}

static void jorg_step_left(edict_t *self)
{
    gi.sound(self, CHAN_BODY, sound_step_left, 1, ATTN_NORM, 0);
}

static void jorg_step_right(edict_t *self)
{
    gi.sound(self, CHAN_BODY, sound_step_right, 1, ATTN_NORM, 0);
}

void MONSTERINFO_STAND(jorg_stand)(edict_t *self)
{
    M_SetAnimation(self, &jorg_move_stand);

    jorg_attack1_end_sound(self);
}

static const mframe_t jorg_frames_run[] = {
    { ai_run, 17, jorg_step_left },
    { ai_run },
    { ai_run },
    { ai_run },
    { ai_run, 12 },
    { ai_run, 8 },
    { ai_run, 10 },
    { ai_run, 33, jorg_step_right },
    { ai_run },
    { ai_run },
    { ai_run },
    { ai_run, 9 },
    { ai_run, 9 },
    { ai_run, 9 }
};
const mmove_t MMOVE_T(jorg_move_run) = { FRAME_walk06, FRAME_walk19, jorg_frames_run, NULL };

//
// walk
//
#if 0
static const mframe_t jorg_frames_start_walk[] = {
    { ai_walk, 5 },
    { ai_walk, 6 },
    { ai_walk, 7 },
    { ai_walk, 9 },
    { ai_walk, 15 }
};
const mmove_t MMOVE_T(jorg_move_start_walk) = { FRAME_walk01, FRAME_walk05, jorg_frames_start_walk, NULL };
#endif

static const mframe_t jorg_frames_walk[] = {
    { ai_walk, 17 },
    { ai_walk },
    { ai_walk },
    { ai_walk },
    { ai_walk, 12 },
    { ai_walk, 8 },
    { ai_walk, 10 },
    { ai_walk, 33 },
    { ai_walk },
    { ai_walk },
    { ai_walk },
    { ai_walk, 9 },
    { ai_walk, 9 },
    { ai_walk, 9 }
};
const mmove_t MMOVE_T(jorg_move_walk) = { FRAME_walk06, FRAME_walk19, jorg_frames_walk, NULL };

#if 0
static const mframe_t jorg_frames_end_walk[] = {
    { ai_walk, 11 },
    { ai_walk },
    { ai_walk },
    { ai_walk },
    { ai_walk, 8 },
    { ai_walk, -8 }
};
const mmove_t MMOVE_T(jorg_move_end_walk) = { FRAME_walk20, FRAME_walk25, jorg_frames_end_walk, NULL };
#endif

void MONSTERINFO_WALK(jorg_walk)(edict_t *self)
{
    M_SetAnimation(self, &jorg_move_walk);
}

void MONSTERINFO_RUN(jorg_run)(edict_t *self)
{
    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        M_SetAnimation(self, &jorg_move_stand);
    else
        M_SetAnimation(self, &jorg_move_run);

    jorg_attack1_end_sound(self);
}

static const mframe_t jorg_frames_pain3[] = {
    { ai_move, -28 },
    { ai_move, -6 },
    { ai_move, -3, jorg_step_left },
    { ai_move, -9 },
    { ai_move, 0, jorg_step_right },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, -7 },
    { ai_move, 1 },
    { ai_move, -11 },
    { ai_move, -4 },
    { ai_move },
    { ai_move },
    { ai_move, 10 },
    { ai_move, 11 },
    { ai_move },
    { ai_move, 10 },
    { ai_move, 3 },
    { ai_move, 10 },
    { ai_move, 7, jorg_step_left },
    { ai_move, 17 },
    { ai_move },
    { ai_move },
    { ai_move, 0, jorg_step_right }
};
const mmove_t MMOVE_T(jorg_move_pain3) = { FRAME_pain301, FRAME_pain325, jorg_frames_pain3, jorg_run };

static const mframe_t jorg_frames_pain2[] = {
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(jorg_move_pain2) = { FRAME_pain201, FRAME_pain203, jorg_frames_pain2, jorg_run };

static const mframe_t jorg_frames_pain1[] = {
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(jorg_move_pain1) = { FRAME_pain101, FRAME_pain103, jorg_frames_pain1, jorg_run };

static const mframe_t jorg_frames_death1[] = {
    { ai_move, 0, BossExplode },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, -2 },
    { ai_move, -5 },
    { ai_move, -8 },
    { ai_move, -15, jorg_step_left },
    { ai_move }, // 10
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, -11 },
    { ai_move, -25 },
    { ai_move, -10, jorg_step_right },
    { ai_move },
    { ai_move }, // 20
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, -21 },
    { ai_move, -10 },
    { ai_move, -16, jorg_step_left },
    { ai_move },
    { ai_move },
    { ai_move }, // 30
    { ai_move },
    { ai_move },
    { ai_move, 22 },
    { ai_move, 33, jorg_step_left },
    { ai_move },
    { ai_move },
    { ai_move, 28 },
    { ai_move, 28, jorg_step_right },
    { ai_move },
    { ai_move }, // 40
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, -19 },
    { ai_move, 0, jorg_death_hit },
    { ai_move },
    { ai_move } // 50
};
const mmove_t MMOVE_T(jorg_move_death) = { FRAME_death01, FRAME_death50, jorg_frames_death1, jorg_dead };

static const mframe_t jorg_frames_attack2[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, jorgBFG },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(jorg_move_attack2) = { FRAME_attak201, FRAME_attak213, jorg_frames_attack2, jorg_run };

static const mframe_t jorg_frames_start_attack1[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(jorg_move_start_attack1) = { FRAME_attak101, FRAME_attak108, jorg_frames_start_attack1, jorg_attack1 };

static const mframe_t jorg_frames_attack1[] = {
    { ai_charge, 0, jorg_firebullet },
    { ai_charge, 0, jorg_firebullet },
    { ai_charge, 0, jorg_firebullet },
    { ai_charge, 0, jorg_firebullet },
    { ai_charge, 0, jorg_firebullet },
    { ai_charge, 0, jorg_firebullet }
};
const mmove_t MMOVE_T(jorg_move_attack1) = { FRAME_attak109, FRAME_attak114, jorg_frames_attack1, jorg_reattack1 };

static const mframe_t jorg_frames_end_attack1[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(jorg_move_end_attack1) = { FRAME_attak115, FRAME_attak118, jorg_frames_end_attack1, jorg_run };

static void jorg_reattack1(edict_t *self)
{
    if (visible(self, self->enemy)) {
        if (frandom() < 0.9f)
            M_SetAnimation(self, &jorg_move_attack1);
        else {
            M_SetAnimation(self, &jorg_move_end_attack1);
            jorg_attack1_end_sound(self);
        }
    } else {
        M_SetAnimation(self, &jorg_move_end_attack1);
        jorg_attack1_end_sound(self);
    }
}

static void jorg_attack1(edict_t *self)
{
    M_SetAnimation(self, &jorg_move_attack1);
}

void PAIN(jorg_pain)(edict_t *self, edict_t *other, float kick, int damage, const mod_t mod)
{
    if (level.time < self->pain_debounce_time)
        return;

    // Lessen the chance of him going into his pain frames if he takes little damage
    if (mod.id != MOD_CHAINFIST) {
        if (damage <= 40)
            if (frandom() <= 0.6f)
                return;

        /*
        If he's entering his attack1 or using attack1, lessen the chance of him
        going into pain
        */

        if ((self->s.frame >= FRAME_attak101) && (self->s.frame <= FRAME_attak108))
            if (frandom() <= 0.005f)
                return;

        if ((self->s.frame >= FRAME_attak109) && (self->s.frame <= FRAME_attak114))
            if (frandom() <= 0.00005f)
                return;

        if ((self->s.frame >= FRAME_attak201) && (self->s.frame <= FRAME_attak208))
            if (frandom() <= 0.005f)
                return;
    }

    self->pain_debounce_time = level.time + SEC(3);

    bool do_pain3 = false;

    if (damage > 50) {
        if (damage <= 100) {
            gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
        } else {
            if (frandom() <= 0.3f) {
                do_pain3 = true;
                gi.sound(self, CHAN_VOICE, sound_pain3, 1, ATTN_NORM, 0);
            }
        }
    }

    if (!M_ShouldReactToPain(self, mod))
        return; // no pain anims in nightmare

    jorg_attack1_end_sound(self);

    if (damage <= 50)
        M_SetAnimation(self, &jorg_move_pain1);
    else if (damage <= 100)
        M_SetAnimation(self, &jorg_move_pain2);
    else if (do_pain3)
        M_SetAnimation(self, &jorg_move_pain3);
}

void MONSTERINFO_SETSKIN(jorg_setskin)(edict_t *self)
{
    if (self->health < (self->max_health / 2))
        self->s.skinnum = 1;
    else
        self->s.skinnum = 0;
}

static void jorgBFG(edict_t *self)
{
    vec3_t forward, right;
    vec3_t start;
    vec3_t dir;
    vec3_t vec;

    AngleVectors(self->s.angles, forward, right, NULL);
    M_ProjectFlashSource(self, monster_flash_offset[MZ2_JORG_BFG_1], forward, right, start);

    VectorCopy(self->enemy->s.origin, vec);
    vec[2] += self->enemy->viewheight;
    VectorSubtract(vec, start, dir);
    VectorNormalize(dir);
    gi.sound(self, CHAN_WEAPON, sound_bfg_fire, 1, ATTN_NORM, 0);
    monster_fire_bfg(self, start, dir, 50, 300, 100, 200, MZ2_JORG_BFG_1);
}

static void jorg_firebullet_right(edict_t *self)
{
    vec3_t forward, right, start;
    AngleVectors(self->s.angles, forward, right, NULL);
    M_ProjectFlashSource(self, monster_flash_offset[MZ2_JORG_MACHINEGUN_R1], forward, right, start);
    PredictAim(self, self->enemy, start, 0, false, -0.2f, forward, NULL);
    monster_fire_bullet(self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_JORG_MACHINEGUN_R1);
}

static void jorg_firebullet_left(edict_t *self)
{
    vec3_t forward, right, start;
    AngleVectors(self->s.angles, forward, right, NULL);
    M_ProjectFlashSource(self, monster_flash_offset[MZ2_JORG_MACHINEGUN_L1], forward, right, start);
    PredictAim(self, self->enemy, start, 0, false, 0.2f, forward, NULL);
    monster_fire_bullet(self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_JORG_MACHINEGUN_L1);
}

static void jorg_firebullet(edict_t *self)
{
    jorg_firebullet_left(self);
    jorg_firebullet_right(self);
}

void MONSTERINFO_ATTACK(jorg_attack)(edict_t *self)
{
    if (frandom() <= 0.75f) {
        gi.sound(self, CHAN_WEAPON, sound_attack1, 1, ATTN_NORM, 0);
        self->monsterinfo.weapon_sound = gi.soundindex("boss3/w_loop.wav");
        M_SetAnimation(self, &jorg_move_start_attack1);
    } else {
        gi.sound(self, CHAN_VOICE, sound_attack2, 1, ATTN_NORM, 0);
        M_SetAnimation(self, &jorg_move_attack2);
    }
}

static const gib_def_t jorg_gibs[] = {
    { "models/objects/gibs/sm_meat/tris.md2", 2 },
    { "models/objects/gibs/sm_metal/tris.md2", 2, GIB_METALLIC },
    { "models/monsters/boss3/jorg/gibs/chest.md2", 1, GIB_SKINNED },
    { "models/monsters/boss3/jorg/gibs/foot.md2", 2, GIB_SKINNED },
    { "models/monsters/boss3/jorg/gibs/gun.md2", 2, GIB_SKINNED | GIB_UPRIGHT },
    { "models/monsters/boss3/jorg/gibs/thigh.md2", 2, GIB_SKINNED | GIB_UPRIGHT },
    { "models/monsters/boss3/jorg/gibs/spine.md2", 1, GIB_SKINNED | GIB_UPRIGHT },
    { "models/monsters/boss3/jorg/gibs/tube.md2", 4, GIB_SKINNED },
    { "models/monsters/boss3/jorg/gibs/spike.md2", 6, GIB_SKINNED },
    { "models/monsters/boss3/jorg/gibs/head.md2", 1, GIB_SKINNED | GIB_METALLIC | GIB_HEAD },
    { 0 }
};

static void jorg_dead(edict_t *self)
{
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_EXPLOSION1_BIG);
    gi.WritePosition(self->s.origin);
    gi.multicast(self->s.origin, MULTICAST_PHS);

    self->s.sound = 0;
    self->s.skinnum /= 2;

    ThrowGibs(self, 500, jorg_gibs);

    MakronToss(self);
}

void DIE(jorg_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, const mod_t mod)
{
    gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
    jorg_attack1_end_sound(self);
    self->deadflag = true;
    self->takedamage = false;
    self->count = 0;
    M_SetAnimation(self, &jorg_move_death);
}

// [Paril-KEX] use generic function
bool MONSTERINFO_CHECKATTACK(Jorg_CheckAttack)(edict_t *self)
{
    return M_CheckAttack_Base(self, 0.4f, 0.8f, 0.4f, 0.2f, 0, 0);
}

static void jorg_precache(void)
{
    sound_pain1 = gi.soundindex("boss3/bs3pain1.wav");
    sound_pain2 = gi.soundindex("boss3/bs3pain2.wav");
    sound_pain3 = gi.soundindex("boss3/bs3pain3.wav");
    sound_death = gi.soundindex("boss3/bs3deth1.wav");
    sound_attack1 = gi.soundindex("boss3/bs3atck1.wav");
    sound_attack1_loop = gi.soundindex("boss3/bs3atck1_loop.wav");
    sound_attack1_end = gi.soundindex("boss3/bs3atck1_end.wav");
    sound_attack2 = gi.soundindex("boss3/bs3atck2.wav");
    sound_search1 = gi.soundindex("boss3/bs3srch1.wav");
    sound_search2 = gi.soundindex("boss3/bs3srch2.wav");
    sound_search3 = gi.soundindex("boss3/bs3srch3.wav");
    sound_idle = gi.soundindex("boss3/bs3idle1.wav");
    sound_step_left = gi.soundindex("boss3/step1.wav");
    sound_step_right = gi.soundindex("boss3/step2.wav");
    sound_firegun = gi.soundindex("boss3/xfire.wav");
    sound_death_hit = gi.soundindex("boss3/d_hit.wav");
    sound_bfg_fire = gi.soundindex("makron/bfg_fire.wav");

    MakronPrecache();
}

/*QUAKED monster_jorg (1 .5 0) (-80 -80 0) (90 90 140) Ambush Trigger_Spawn Sight
 */
void SP_monster_jorg(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    G_AddPrecache(jorg_precache);

    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;
    self->s.modelindex = gi.modelindex("models/monsters/boss3/jorg/tris.md2");
    self->s.modelindex2 = gi.modelindex("models/monsters/boss3/rider/tris.md2");

    PrecacheGibs(jorg_gibs);

    VectorSet(self->mins, -80, -80, 0);
    VectorSet(self->maxs, 80, 80, 140);

    self->health = 8000 * st.health_multiplier;
    self->gib_health = -2000;
    self->mass = 1000;

    self->pain = jorg_pain;
    self->die = jorg_die;
    self->monsterinfo.stand = jorg_stand;
    self->monsterinfo.walk = jorg_walk;
    self->monsterinfo.run = jorg_run;
    self->monsterinfo.dodge = NULL;
    self->monsterinfo.attack = jorg_attack;
    self->monsterinfo.search = jorg_search;
    self->monsterinfo.melee = NULL;
    self->monsterinfo.sight = NULL;
    self->monsterinfo.checkattack = Jorg_CheckAttack;
    self->monsterinfo.setskin = jorg_setskin;
    gi.linkentity(self);

    M_SetAnimation(self, &jorg_move_stand);
    self->monsterinfo.scale = MODEL_SCALE;

    walkmonster_start(self);
    // PMM
    self->monsterinfo.aiflags |= AI_IGNORE_SHOTS;
    // pmm
    self->monsterinfo.aiflags |= AI_DOUBLE_TROUBLE;
}
