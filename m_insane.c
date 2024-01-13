// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

insane

==============================================================================
*/

#include "g_local.h"
#include "m_insane.h"

#define SPAWNFLAG_INSANE_CRAWL          4
#define SPAWNFLAG_INSANE_CRUCIFIED      8
#define SPAWNFLAG_INSANE_STAND_GROUND   16
#define SPAWNFLAG_INSANE_ALWAYS_STAND   32
#define SPAWNFLAG_INSANE_QUIET          64

static int sound_fist;
static int sound_shake;
static int sound_moan;
static int sound_scream[8];

static void insane_fist(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_fist, 1, ATTN_IDLE, 0);
}

static void insane_shake(edict_t *self)
{
    if (!(self->spawnflags & SPAWNFLAG_INSANE_QUIET))
        gi.sound(self, CHAN_VOICE, sound_shake, 1, ATTN_IDLE, 0);
}

const mmove_t insane_move_cross;
const mmove_t insane_move_struggle_cross;

static void insane_moan(edict_t *self)
{
    if (self->spawnflags & SPAWNFLAG_INSANE_QUIET)
        return;

    // Paril: don't moan every second
    if (self->monsterinfo.attack_finished < level.time) {
        gi.sound(self, CHAN_VOICE, sound_moan, 1, ATTN_IDLE, 0);
        self->monsterinfo.attack_finished = level.time + random_time_sec(1, 3);
    }
}

static void insane_scream(edict_t *self)
{
    if (self->spawnflags & SPAWNFLAG_INSANE_QUIET)
        return;

    // Paril: don't moan every second
    if (self->monsterinfo.attack_finished < level.time) {
        gi.sound(self, CHAN_VOICE, random_element(sound_scream), 1, ATTN_IDLE, 0);
        self->monsterinfo.attack_finished = level.time + random_time_sec(1, 3);
    }
}

void insane_stand(edict_t *self);
static void insane_dead(edict_t *self);
static void insane_cross(edict_t *self);
void insane_walk(edict_t *self);
void insane_run(edict_t *self);
static void insane_checkdown(edict_t *self);
static void insane_checkup(edict_t *self);
static void insane_onground(edict_t *self);

#if 0
// Paril: unused atm because it breaks N64.
// may fix later
static void insane_shrink(edict_t *self)
{
    self->maxs[2] = 0;
    self->svflags |= SVF_DEADMONSTER;
    gi.linkentity(self);
}
#endif

static const mframe_t insane_frames_stand_normal[] = {
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand, 0, insane_checkdown }
};
const mmove_t MMOVE_T(insane_move_stand_normal) = { FRAME_stand60, FRAME_stand65, insane_frames_stand_normal, insane_stand };

static const mframe_t insane_frames_stand_insane[] = {
    { ai_stand, 0, insane_shake },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand, 0, insane_checkdown }
};
const mmove_t MMOVE_T(insane_move_stand_insane) = { FRAME_stand65, FRAME_stand94, insane_frames_stand_insane, insane_stand };

static const mframe_t insane_frames_uptodown[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, insane_moan },
    { ai_move },//, 0, monster_duck_down },
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

    { ai_move, 2.7f },
    { ai_move, 4.1f },
    { ai_move, 6 },
    { ai_move, 7.6f },
    { ai_move, 3.6f },
    { ai_move },
    { ai_move },
    { ai_move, 0, insane_fist },
    { ai_move },
    { ai_move },

    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, insane_fist },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(insane_move_uptodown) = { FRAME_stand1, FRAME_stand40, insane_frames_uptodown, insane_onground };

static const mframe_t insane_frames_downtoup[] = {
    { ai_move, -0.7f }, // 41
    { ai_move, -1.2f }, // 42
    { ai_move, -1.5f }, // 43
    { ai_move, -4.5f }, // 44
    { ai_move, -3.5f }, // 45
    { ai_move, -0.2f }, // 46
    { ai_move },        // 47
    { ai_move, -1.3f }, // 48
    { ai_move, -3 },    // 49
    { ai_move, -2 },    // 50
    { ai_move  },//, 0, monster_duck_up },      // 51
    { ai_move },        // 52
    { ai_move },        // 53
    { ai_move, -3.3f }, // 54
    { ai_move, -1.6f }, // 55
    { ai_move, -0.3f }, // 56
    { ai_move },        // 57
    { ai_move },        // 58
    { ai_move }         // 59
};
const mmove_t MMOVE_T(insane_move_downtoup) = { FRAME_stand41, FRAME_stand59, insane_frames_downtoup, insane_stand };

static const mframe_t insane_frames_jumpdown[] = {
    { ai_move, 0.2f },
    { ai_move, 11.5f },
    { ai_move, 5.1f },
    { ai_move, 7.1f },
    { ai_move }
};
const mmove_t MMOVE_T(insane_move_jumpdown) = { FRAME_stand96, FRAME_stand100, insane_frames_jumpdown, insane_onground };

static const mframe_t insane_frames_down[] = {
    { ai_move }, // 100
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }, // 110
    { ai_move, -1.7f },
    { ai_move, -1.6f },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, insane_fist },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }, // 120
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }, // 130
    { ai_move },
    { ai_move },
    { ai_move, 0, insane_moan },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }, // 140
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }, // 150
    { ai_move, 0.5f },
    { ai_move },
    { ai_move, -0.2f, insane_scream },
    { ai_move },
    { ai_move, 0.2f },
    { ai_move, 0.4f },
    { ai_move, 0.6f },
    { ai_move, 0.8f },
    { ai_move, 0.7f },
    { ai_move, 0, insane_checkup } // 160
};
const mmove_t MMOVE_T(insane_move_down) = { FRAME_stand100, FRAME_stand160, insane_frames_down, insane_onground };

static const mframe_t insane_frames_walk_normal[] = {
    { ai_walk, 0, insane_scream },
    { ai_walk, 2.5f },
    { ai_walk, 3.5f },
    { ai_walk, 1.7f },
    { ai_walk, 2.3f },
    { ai_walk, 2.4f },
    { ai_walk, 2.2f, monster_footstep },
    { ai_walk, 4.2f },
    { ai_walk, 5.6f },
    { ai_walk, 3.3f },
    { ai_walk, 2.4f },
    { ai_walk, 0.9f },
    { ai_walk, 0, monster_footstep }
};
const mmove_t MMOVE_T(insane_move_walk_normal) = { FRAME_walk27, FRAME_walk39, insane_frames_walk_normal, insane_walk };
const mmove_t MMOVE_T(insane_move_run_normal) = { FRAME_walk27, FRAME_walk39, insane_frames_walk_normal, insane_run };

static const mframe_t insane_frames_walk_insane[] = {
    { ai_walk, 0, insane_scream }, // walk 1
    { ai_walk, 3.4f },             // walk 2
    { ai_walk, 3.6f },             // 3
    { ai_walk, 2.9f },             // 4
    { ai_walk, 2.2f },             // 5
    { ai_walk, 2.6f, monster_footstep },               // 6
    { ai_walk },                   // 7
    { ai_walk, 0.7f },             // 8
    { ai_walk, 4.8f },             // 9
    { ai_walk, 5.3f },             // 10
    { ai_walk, 1.1f },             // 11
    { ai_walk, 2, monster_footstep },                  // 12
    { ai_walk, 0.5f },             // 13
    { ai_walk },                   // 14
    { ai_walk },                   // 15
    { ai_walk, 4.9f },             // 16
    { ai_walk, 6.7f },             // 17
    { ai_walk, 3.8f },             // 18
    { ai_walk, 2, monster_footstep },                  // 19
    { ai_walk, 0.2f },             // 20
    { ai_walk },                   // 21
    { ai_walk, 3.4f },             // 22
    { ai_walk, 6.4f },             // 23
    { ai_walk, 5 },                // 24
    { ai_walk, 1.8f, monster_footstep },               // 25
    { ai_walk }                    // 26
};
const mmove_t MMOVE_T(insane_move_walk_insane) = { FRAME_walk1, FRAME_walk26, insane_frames_walk_insane, insane_walk };
const mmove_t MMOVE_T(insane_move_run_insane) = { FRAME_walk1, FRAME_walk26, insane_frames_walk_insane, insane_run };

static const mframe_t insane_frames_stand_pain[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, monster_footstep },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, monster_footstep }
};
const mmove_t MMOVE_T(insane_move_stand_pain) = { FRAME_st_pain2, FRAME_st_pain12, insane_frames_stand_pain, insane_run };

static const mframe_t insane_frames_stand_death[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, monster_footstep },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, monster_footstep },
    { ai_move },
    { ai_move, 0, monster_footstep },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(insane_move_stand_death) = { FRAME_st_death2, FRAME_st_death18, insane_frames_stand_death, insane_dead };

static const mframe_t insane_frames_crawl[] = {
    { ai_walk, 0, insane_scream },
    { ai_walk, 1.5f },
    { ai_walk, 2.1f },
    { ai_walk, 3.6f },
    { ai_walk, 2, monster_footstep },
    { ai_walk, 0.9f },
    { ai_walk, 3 },
    { ai_walk, 3.4f },
    { ai_walk, 2.4f, monster_footstep }
};
const mmove_t MMOVE_T(insane_move_crawl) = { FRAME_crawl1, FRAME_crawl9, insane_frames_crawl, NULL };
const mmove_t MMOVE_T(insane_move_runcrawl) = { FRAME_crawl1, FRAME_crawl9, insane_frames_crawl, NULL };

static const mframe_t insane_frames_crawl_pain[] = {
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
const mmove_t MMOVE_T(insane_move_crawl_pain) = { FRAME_cr_pain2, FRAME_cr_pain10, insane_frames_crawl_pain, insane_run };

static const mframe_t insane_frames_crawl_death[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(insane_move_crawl_death) = { FRAME_cr_death10, FRAME_cr_death16, insane_frames_crawl_death, insane_dead };

static const mframe_t insane_frames_cross[] = {
    { ai_move, 0, insane_moan },
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
const mmove_t MMOVE_T(insane_move_cross) = { FRAME_cross1, FRAME_cross15, insane_frames_cross, insane_cross };

static const mframe_t insane_frames_struggle_cross[] = {
    { ai_move, 0, insane_scream },
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
const mmove_t MMOVE_T(insane_move_struggle_cross) = { FRAME_cross16, FRAME_cross30, insane_frames_struggle_cross, insane_cross };

static void insane_cross(edict_t *self)
{
    if (frandom() < 0.8f)
        M_SetAnimation(self, &insane_move_cross);
    else
        M_SetAnimation(self, &insane_move_struggle_cross);
}

void MONSTERINFO_WALK(insane_walk)(edict_t *self)
{
    if (self->spawnflags & SPAWNFLAG_INSANE_STAND_GROUND && self->s.frame == FRAME_cr_pain10) { // Hold Ground?
        M_SetAnimation(self, &insane_move_down);
        //monster_duck_down(self);
        return;
    }
    if (self->spawnflags & SPAWNFLAG_INSANE_CRAWL)
        M_SetAnimation(self, &insane_move_crawl);
    else if (brandom())
        M_SetAnimation(self, &insane_move_walk_normal);
    else
        M_SetAnimation(self, &insane_move_walk_insane);
}

void MONSTERINFO_RUN(insane_run)(edict_t *self)
{
    if (self->spawnflags & SPAWNFLAG_INSANE_STAND_GROUND && self->s.frame == FRAME_cr_pain10) { // Hold Ground?
        M_SetAnimation(self, &insane_move_down);
        //monster_duck_down(self);
        return;
    }

    if ((self->spawnflags & SPAWNFLAG_INSANE_CRAWL) ||
        (self->s.frame >= FRAME_cr_pain2 && self->s.frame <= FRAME_cr_pain10) ||
        (self->s.frame >= FRAME_crawl1 && self->s.frame <= FRAME_crawl9) ||
        (self->s.frame >= FRAME_stand99 && self->s.frame <= FRAME_stand160)) { // Crawling?
        M_SetAnimation(self, &insane_move_runcrawl);
        return;
    }

    if (brandom()) { // Else, mix it up
        M_SetAnimation(self, &insane_move_run_normal);
        //monster_duck_up(self);
        return;
    }

    M_SetAnimation(self, &insane_move_run_insane);
    //monster_duck_up(self);
}

void PAIN(insane_pain)(edict_t *self, edict_t *other, float kick, int damage, mod_t mod)
{
    int l, r;

    if (level.time < self->pain_debounce_time)
        return;

    self->pain_debounce_time = level.time + SEC(3);

    r = 1 + brandom();
    if (self->health < 25)
        l = 25;
    else if (self->health < 50)
        l = 50;
    else if (self->health < 75)
        l = 75;
    else
        l = 100;
    gi.sound(self, CHAN_VOICE, gi.soundindex(va("player/male/pain%d_%d.wav", l, r)), 1, ATTN_IDLE, 0);

    // Don't go into pain frames if crucified.
    if (self->spawnflags & SPAWNFLAG_INSANE_CRUCIFIED) {
        M_SetAnimation(self, &insane_move_struggle_cross);
        return;
    }

    if (((self->s.frame >= FRAME_crawl1) && (self->s.frame <= FRAME_crawl9)) ||
        ((self->s.frame >= FRAME_stand99) && (self->s.frame <= FRAME_stand160)) ||
        ((self->s.frame >= FRAME_stand1 && self->s.frame <= FRAME_stand40))) {
        M_SetAnimation(self, &insane_move_crawl_pain);
    } else {
        M_SetAnimation(self, &insane_move_stand_pain);
        //monster_duck_up(self);
    }
}

static void insane_onground(edict_t *self)
{
    M_SetAnimation(self, &insane_move_down);
    //monster_duck_down(self);
}

static void insane_checkdown(edict_t *self)
{
    //  if ( (self->s.frame == FRAME_stand94) || (self->s.frame == FRAME_stand65) )
    if (self->spawnflags & SPAWNFLAG_INSANE_ALWAYS_STAND) // Always stand
        return;
    if (frandom() < 0.3f) {
        if (brandom())
            M_SetAnimation(self, &insane_move_uptodown);
        else
            M_SetAnimation(self, &insane_move_jumpdown);
    }
}

static void insane_checkup(edict_t *self)
{
    // If Hold_Ground and Crawl are set
    if ((self->spawnflags & (SPAWNFLAG_INSANE_CRAWL | SPAWNFLAG_INSANE_STAND_GROUND)) == (SPAWNFLAG_INSANE_CRAWL | SPAWNFLAG_INSANE_STAND_GROUND))
        return;
    if (brandom()) {
        M_SetAnimation(self, &insane_move_downtoup);
        //monster_duck_up(self);
    }
}

void MONSTERINFO_STAND(insane_stand)(edict_t *self)
{
    if (self->spawnflags & SPAWNFLAG_INSANE_CRUCIFIED) { // If crucified
        M_SetAnimation(self, &insane_move_cross);
        self->monsterinfo.aiflags |= AI_STAND_GROUND;
        return;
    }

    // If Hold_Ground and Crawl are set
    if ((self->spawnflags & (SPAWNFLAG_INSANE_CRAWL | SPAWNFLAG_INSANE_STAND_GROUND)) == (SPAWNFLAG_INSANE_CRAWL | SPAWNFLAG_INSANE_STAND_GROUND)) {
        M_SetAnimation(self, &insane_move_down);
        //monster_duck_down(self);
        return;
    }

    if (brandom())
        M_SetAnimation(self, &insane_move_stand_normal);
    else
        M_SetAnimation(self, &insane_move_stand_insane);
}

static void insane_dead(edict_t *self)
{
    if (self->spawnflags & SPAWNFLAG_INSANE_CRUCIFIED) {
        self->flags |= FL_FLY;
    } else {
        VectorSet(self->mins, -16, -16, -24);
        VectorSet(self->maxs, 16, 16, -8);
        self->movetype = MOVETYPE_TOSS;
    }
    monster_dead(self);
}

static const gib_def_t insane_gibs[] = {
    { "models/objects/gibs/bone/tris.md2", 2 },
    { "models/objects/gibs/sm_meat/tris.md2", 4 },
    { "models/objects/gibs/head2/tris.md2", 1, GIB_HEAD },
    { 0 }
};

void DIE(insane_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    if (M_CheckGib(self, mod)) {
        gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_IDLE, 0);
        ThrowGibs(self, damage, insane_gibs);
        self->deadflag = true;
        return;
    }

    if (self->deadflag)
        return;

    gi.sound(self, CHAN_VOICE, gi.soundindex(va("player/male/death%d.wav", irandom2(1, 5))), 1, ATTN_IDLE, 0);

    self->deadflag = true;
    self->takedamage = true;

    if (self->spawnflags & SPAWNFLAG_INSANE_CRUCIFIED) {
        insane_dead(self);
    } else {
        if (((self->s.frame >= FRAME_crawl1) && (self->s.frame <= FRAME_crawl9)) ||
            ((self->s.frame >= FRAME_stand99) && (self->s.frame <= FRAME_stand160)))
            M_SetAnimation(self, &insane_move_crawl_death);
        else
            M_SetAnimation(self, &insane_move_stand_death);
    }
}

static void insane_precache1(void)
{
    sound_fist = gi.soundindex("insane/insane11.wav");
}

static void insane_precache2(void)
{
    sound_shake = gi.soundindex("insane/insane5.wav");
    sound_moan = gi.soundindex("insane/insane7.wav");
    sound_scream[0] = gi.soundindex("insane/insane1.wav");
    sound_scream[1] = gi.soundindex("insane/insane2.wav");
    sound_scream[2] = gi.soundindex("insane/insane3.wav");
    sound_scream[3] = gi.soundindex("insane/insane4.wav");
    sound_scream[4] = gi.soundindex("insane/insane6.wav");
    sound_scream[5] = gi.soundindex("insane/insane8.wav");
    sound_scream[6] = gi.soundindex("insane/insane9.wav");
    sound_scream[7] = gi.soundindex("insane/insane10.wav");
}

/*QUAKED misc_insane (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn CRAWL CRUCIFIED STAND_GROUND ALWAYS_STAND QUIET
 */
void SP_misc_insane(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    G_AddPrecache(insane_precache1);

    if (!(self->spawnflags & SPAWNFLAG_INSANE_QUIET))
        G_AddPrecache(insane_precache2);

    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;
    self->s.modelindex = gi.modelindex("models/monsters/insane/tris.md2");

    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, 32);

    self->health = 100 * st.health_multiplier;
    self->gib_health = -50;
    self->mass = 300;

    self->pain = insane_pain;
    self->die = insane_die;

    self->monsterinfo.stand = insane_stand;
    self->monsterinfo.walk = insane_walk;
    self->monsterinfo.run = insane_run;
    self->monsterinfo.dodge = NULL;
    self->monsterinfo.attack = NULL;
    self->monsterinfo.melee = NULL;
    self->monsterinfo.sight = NULL;
    self->monsterinfo.aiflags |= AI_GOOD_GUY;

    gi.linkentity(self);

    if (self->spawnflags & SPAWNFLAG_INSANE_STAND_GROUND) // Stand Ground
        self->monsterinfo.aiflags |= AI_STAND_GROUND;

    M_SetAnimation(self, &insane_move_stand_normal);

    self->monsterinfo.scale = MODEL_SCALE;

    if (self->spawnflags & SPAWNFLAG_INSANE_CRUCIFIED) { // Crucified ?
        self->flags |= FL_NO_KNOCKBACK | FL_STATIONARY;
        stationarymonster_start(self);
    } else
        walkmonster_start(self);

    self->s.skinnum = irandom1(3);
}
