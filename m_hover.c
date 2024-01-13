// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

hover

==============================================================================
*/

#include "g_local.h"
#include "m_hover.h"

static int sound_pain1;
static int sound_pain2;
static int sound_death1;
static int sound_death2;
static int sound_sight;
static int sound_search1;
static int sound_search2;

// ROGUE
// daedalus sounds
static int daed_sound_pain1;
static int daed_sound_pain2;
static int daed_sound_death1;
static int daed_sound_death2;
static int daed_sound_sight;
static int daed_sound_search1;
static int daed_sound_search2;
// ROGUE

void MONSTERINFO_SIGHT(hover_sight)(edict_t *self, edict_t *other)
{
    // PMM - daedalus sounds
    if (self->mass < 225)
        gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
    else
        gi.sound(self, CHAN_VOICE, daed_sound_sight, 1, ATTN_NORM, 0);
}

void MONSTERINFO_SEARCH(hover_search)(edict_t *self)
{
    // PMM - daedalus sounds
    if (self->mass < 225) {
        if (brandom())
            gi.sound(self, CHAN_VOICE, sound_search1, 1, ATTN_NORM, 0);
        else
            gi.sound(self, CHAN_VOICE, sound_search2, 1, ATTN_NORM, 0);
    } else {
        if (brandom())
            gi.sound(self, CHAN_VOICE, daed_sound_search1, 1, ATTN_NORM, 0);
        else
            gi.sound(self, CHAN_VOICE, daed_sound_search2, 1, ATTN_NORM, 0);
    }
}

void hover_run(edict_t *self);
static void hover_dead(edict_t *self);
static void hover_attack(edict_t *self);
static void hover_reattack(edict_t *self);
static void hover_fire_blaster(edict_t *self);

static const mframe_t hover_frames_stand[] = {
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
const mmove_t MMOVE_T(hover_move_stand) = { FRAME_stand01, FRAME_stand30, hover_frames_stand, NULL };

static const mframe_t hover_frames_pain3[] = {
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
const mmove_t MMOVE_T(hover_move_pain3) = { FRAME_pain301, FRAME_pain309, hover_frames_pain3, hover_run };

static const mframe_t hover_frames_pain2[] = {
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
const mmove_t MMOVE_T(hover_move_pain2) = { FRAME_pain201, FRAME_pain212, hover_frames_pain2, hover_run };

static const mframe_t hover_frames_pain1[] = {
    { ai_move },
    { ai_move },
    { ai_move, 2 },
    { ai_move, -8 },
    { ai_move, -4 },
    { ai_move, -6 },
    { ai_move, -4 },
    { ai_move, -3 },
    { ai_move, 1 },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 3 },
    { ai_move, 1 },
    { ai_move },
    { ai_move, 2 },
    { ai_move, 3 },
    { ai_move, 2 },
    { ai_move, 7 },
    { ai_move, 1 },
    { ai_move },
    { ai_move },
    { ai_move, 2 },
    { ai_move },
    { ai_move },
    { ai_move, 5 },
    { ai_move, 3 },
    { ai_move, 4 }
};
const mmove_t MMOVE_T(hover_move_pain1) = { FRAME_pain101, FRAME_pain128, hover_frames_pain1, hover_run };

static const mframe_t hover_frames_walk[] = {
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 4 }
};
const mmove_t MMOVE_T(hover_move_walk) = { FRAME_forwrd01, FRAME_forwrd35, hover_frames_walk, NULL };

static const mframe_t hover_frames_run[] = {
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
const mmove_t MMOVE_T(hover_move_run) = { FRAME_forwrd01, FRAME_forwrd35, hover_frames_run, NULL };

static const gib_def_t hover_gibs[] = {
    { "models/objects/gibs/sm_meat/tris.md2", 2 },
    { "models/objects/gibs/sm_metal/tris.md2", 2, GIB_METALLIC },
    { "models/monsters/hover/gibs/chest.md2", 1, GIB_SKINNED },
    { "models/monsters/hover/gibs/ring.md2", 2, GIB_SKINNED | GIB_METALLIC },
    { "models/monsters/hover/gibs/foot.md2", 2, GIB_SKINNED },
    { "models/monsters/hover/gibs/head.md2", 1, GIB_SKINNED | GIB_HEAD },
    { 0 }
};

static void hover_gib(edict_t *self)
{
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_EXPLOSION1);
    gi.WritePosition(self->s.origin);
    gi.multicast(self->s.origin, MULTICAST_PHS);

    self->s.skinnum /= 2;

    ThrowGibs(self, 150, hover_gibs);
}

void THINK(hover_deadthink)(edict_t *self)
{
    if (!self->groundentity && level.time < self->timestamp) {
        self->nextthink = level.time + FRAME_TIME;
        return;
    }

    hover_gib(self);
}

static void hover_dying(edict_t *self)
{
    if (self->groundentity) {
        hover_deadthink(self);
        return;
    }

    if (brandom())
        return;

    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_PLAIN_EXPLOSION);
    gi.WritePosition(self->s.origin);
    gi.multicast(self->s.origin, MULTICAST_PHS);

    if (brandom())
        ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", 120, GIB_NONE, self->x.scale);
    else
        ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", 120, GIB_METALLIC, self->x.scale);
}

static const mframe_t hover_frames_death1[] = {
    { ai_move },
    { ai_move, 0, hover_dying },
    { ai_move },
    { ai_move, 0, hover_dying },
    { ai_move },
    { ai_move, 0, hover_dying },
    { ai_move, -10, hover_dying },
    { ai_move, 3 },
    { ai_move, 5, hover_dying },
    { ai_move, 4, hover_dying },
    { ai_move, 7 }
};
const mmove_t MMOVE_T(hover_move_death1) = { FRAME_death101, FRAME_death111, hover_frames_death1, hover_dead };

static const mframe_t hover_frames_start_attack[] = {
    { ai_charge, 1 },
    { ai_charge, 1 },
    { ai_charge, 1 }
};
const mmove_t MMOVE_T(hover_move_start_attack) = { FRAME_attak101, FRAME_attak103, hover_frames_start_attack, hover_attack };

static const mframe_t hover_frames_attack1[] = {
    { ai_charge, -10, hover_fire_blaster },
    { ai_charge, -10, hover_fire_blaster },
    { ai_charge, 0, hover_reattack },
};
const mmove_t MMOVE_T(hover_move_attack1) = { FRAME_attak104, FRAME_attak106, hover_frames_attack1, NULL };

static const mframe_t hover_frames_end_attack[] = {
    { ai_charge, 1 },
    { ai_charge, 1 }
};
const mmove_t MMOVE_T(hover_move_end_attack) = { FRAME_attak107, FRAME_attak108, hover_frames_end_attack, hover_run };

/* PMM - circle strafing code */
#if 0
static const mframe_t hover_frames_start_attack2[] = {
    { ai_charge, 15 },
    { ai_charge, 15 },
    { ai_charge, 15 }
};
const mmove_t MMOVE_T(hover_move_start_attack2) = { FRAME_attak101, FRAME_attak103, hover_frames_start_attack2, hover_attack };
#endif

static const mframe_t hover_frames_attack2[] = {
    { ai_charge, 10, hover_fire_blaster },
    { ai_charge, 10, hover_fire_blaster },
    { ai_charge, 10, hover_reattack },
};
const mmove_t MMOVE_T(hover_move_attack2) = { FRAME_attak104, FRAME_attak106, hover_frames_attack2, NULL };

#if 0
static const mframe_t hover_frames_end_attack2[] = {
    { ai_charge, 15 },
    { ai_charge, 15 }
};
const mmove_t MMOVE_T(hover_move_end_attack2) = { FRAME_attak107, FRAME_attak108, hover_frames_end_attack2, hover_run };
#endif

// end of circle strafe

static void hover_reattack(edict_t *self)
{
    if (self->enemy->health > 0 && visible(self, self->enemy) && frandom() <= 0.6f) {
        if (self->monsterinfo.attack_state == AS_STRAIGHT)
            M_SetAnimation(self, &hover_move_attack1);
        else if (self->monsterinfo.attack_state == AS_SLIDING)
            M_SetAnimation(self, &hover_move_attack2);
        else
            gi.dprintf("hover_reattack: unexpected state %d\n", self->monsterinfo.attack_state);
        return;
    }
    M_SetAnimation(self, &hover_move_end_attack);
}

static void hover_fire_blaster(edict_t *self)
{
    vec3_t    start;
    vec3_t    forward, right;
    vec3_t    end;
    vec3_t    dir;

    if (!self->enemy || !self->enemy->inuse) // PGM
        return;                              // PGM

    AngleVectors(self->s.angles, forward, right, NULL);
    M_ProjectFlashSource(self, monster_flash_offset[(self->s.frame & 1) ? MZ2_HOVER_BLASTER_2 : MZ2_HOVER_BLASTER_1], forward, right, start);

    VectorCopy(self->enemy->s.origin, end);
    end[2] += self->enemy->viewheight;
    VectorSubtract(end, start, dir);
    VectorNormalize(dir);

    // PGM  - daedalus fires blaster2
    if (self->mass < 200)
        monster_fire_blaster(self, start, dir, 1, 1000, (self->s.frame & 1) ? MZ2_HOVER_BLASTER_2 : MZ2_HOVER_BLASTER_1, (self->s.frame % 4) ? EF_NONE : EF_HYPERBLASTER);
    else
        monster_fire_blaster2(self, start, dir, 1, 1000, (self->s.frame & 1) ? MZ2_DAEDALUS_BLASTER_2 : MZ2_DAEDALUS_BLASTER, (self->s.frame % 4) ? EF_NONE : EF_BLASTER);
    // PGM
}

void MONSTERINFO_STAND(hover_stand)(edict_t *self)
{
    M_SetAnimation(self, &hover_move_stand);
}

void MONSTERINFO_RUN(hover_run)(edict_t *self)
{
    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        M_SetAnimation(self, &hover_move_stand);
    else
        M_SetAnimation(self, &hover_move_run);
}

void MONSTERINFO_WALK(hover_walk)(edict_t *self)
{
    M_SetAnimation(self, &hover_move_walk);
}

void MONSTERINFO_ATTACK(hover_start_attack)(edict_t *self)
{
    M_SetAnimation(self, &hover_move_start_attack);
}

static void hover_attack(edict_t *self)
{
    float chance = 0.5f;

    if (self->mass > 150) // the daedalus strafes more
        chance += 0.1f;

    if (frandom() > chance) {
        M_SetAnimation(self, &hover_move_attack1);
        self->monsterinfo.attack_state = AS_STRAIGHT;
    } else { // circle strafe
        if (brandom()) // switch directions
            self->monsterinfo.lefty = !self->monsterinfo.lefty;
        M_SetAnimation(self, &hover_move_attack2);
        self->monsterinfo.attack_state = AS_SLIDING;
    }
}

void PAIN(hover_pain)(edict_t *self, edict_t *other, float kick, int damage, mod_t mod)
{
    if (level.time < self->pain_debounce_time)
        return;

    self->pain_debounce_time = level.time + SEC(3);

    //====
    if (brandom()) {
        // PMM - daedalus sounds
        if (self->mass < 225)
            gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
        else
            gi.sound(self, CHAN_VOICE, daed_sound_pain1, 1, ATTN_NORM, 0);
    } else {
        // PMM - daedalus sounds
        if (self->mass < 225)
            gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
        else
            gi.sound(self, CHAN_VOICE, daed_sound_pain2, 1, ATTN_NORM, 0);
    }
    // PGM
    //====

    if (!M_ShouldReactToPain(self, mod))
        return; // no pain anims in nightmare

    if (damage <= 25) {
        if (brandom())
            M_SetAnimation(self, &hover_move_pain3);
        else
            M_SetAnimation(self, &hover_move_pain2);
    } else {
        //====
        // PGM pain sequence is WAY too long
        if (frandom() < 0.3f)
            M_SetAnimation(self, &hover_move_pain1);
        else
            M_SetAnimation(self, &hover_move_pain2);
        // PGM
        //====
    }
}

void MONSTERINFO_SETSKIN(hover_setskin)(edict_t *self)
{
    if (self->health < (self->max_health / 2))
        self->s.skinnum |= 1; // PGM support for skins 2 & 3.
    else
        self->s.skinnum &= ~1; // PGM support for skins 2 & 3.
}

static void hover_dead(edict_t *self)
{
    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, -8);
    self->movetype = MOVETYPE_TOSS;
    self->think = hover_deadthink;
    self->nextthink = level.time + FRAME_TIME;
    self->timestamp = level.time + SEC(15);
    gi.linkentity(self);
}

void DIE(hover_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    self->s.effects = EF_NONE;
    self->monsterinfo.power_armor_type = IT_NULL;

    if (M_CheckGib(self, mod)) {
        hover_gib(self);
        return;
    }

    if (self->deadflag)
        return;

    // regular death
    // PMM - daedalus sounds
    if (self->mass < 225) {
        if (brandom())
            gi.sound(self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);
        else
            gi.sound(self, CHAN_VOICE, sound_death2, 1, ATTN_NORM, 0);
    } else {
        if (brandom())
            gi.sound(self, CHAN_VOICE, daed_sound_death1, 1, ATTN_NORM, 0);
        else
            gi.sound(self, CHAN_VOICE, daed_sound_death2, 1, ATTN_NORM, 0);
    }
    self->deadflag = true;
    self->takedamage = true;
    M_SetAnimation(self, &hover_move_death1);
}

static void hover_set_fly_parameters(edict_t *self)
{
    self->monsterinfo.fly_thrusters = false;
    self->monsterinfo.fly_acceleration = 20;
    self->monsterinfo.fly_speed = 120;
    // Icarus prefers to keep its distance, but flies slower than the flyer.
    // he never pins because of this.
    self->monsterinfo.fly_min_distance = 150;
    self->monsterinfo.fly_max_distance = 350;
}

static void daed_precache(void)
{
    daed_sound_pain1 = gi.soundindex("daedalus/daedpain1.wav");
    daed_sound_pain2 = gi.soundindex("daedalus/daedpain2.wav");
    daed_sound_death1 = gi.soundindex("daedalus/daeddeth1.wav");
    daed_sound_death2 = gi.soundindex("daedalus/daeddeth2.wav");
    daed_sound_sight = gi.soundindex("daedalus/daedsght1.wav");
    daed_sound_search1 = gi.soundindex("daedalus/daedsrch1.wav");
    daed_sound_search2 = gi.soundindex("daedalus/daedsrch2.wav");
}

static void hover_precache(void)
{
    sound_pain1 = gi.soundindex("hover/hovpain1.wav");
    sound_pain2 = gi.soundindex("hover/hovpain2.wav");
    sound_death1 = gi.soundindex("hover/hovdeth1.wav");
    sound_death2 = gi.soundindex("hover/hovdeth2.wav");
    sound_sight = gi.soundindex("hover/hovsght1.wav");
    sound_search1 = gi.soundindex("hover/hovsrch1.wav");
    sound_search2 = gi.soundindex("hover/hovsrch2.wav");
}

/*QUAKED monster_hover (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
 */
/*QUAKED monster_daedalus (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
This is the improved icarus monster.
*/
void SP_monster_hover(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;
    self->s.modelindex = gi.modelindex("models/monsters/hover/tris.md2");

    PrecacheGibs(hover_gibs);

    VectorSet(self->mins, -24, -24, -24);
    VectorSet(self->maxs, 24, 24, 32);

    self->health = 240 * st.health_multiplier;
    self->gib_health = -100;
    self->mass = 150;

    self->pain = hover_pain;
    self->die = hover_die;

    self->monsterinfo.stand = hover_stand;
    self->monsterinfo.walk = hover_walk;
    self->monsterinfo.run = hover_run;
    self->monsterinfo.attack = hover_start_attack;
    self->monsterinfo.sight = hover_sight;
    self->monsterinfo.search = hover_search;
    self->monsterinfo.setskin = hover_setskin;

    // PGM
    if (strcmp(self->classname, "monster_daedalus") == 0) {
        self->health = 450 * st.health_multiplier;
        self->mass = 225;
        self->yaw_speed = 23;
        if (!ED_WasKeySpecified("power_armor_type"))
            self->monsterinfo.power_armor_type = IT_ITEM_POWER_SCREEN;
        if (!ED_WasKeySpecified("power_armor_power"))
            self->monsterinfo.power_armor_power = 100;
        // PMM - daedalus sounds
        self->monsterinfo.engine_sound = gi.soundindex("daedalus/daedidle1.wav");
        G_AddPrecache(daed_precache);
        gi.soundindex("tank/tnkatck3.wav");
        // pmm
    } else {
        self->yaw_speed = 18;
        G_AddPrecache(hover_precache);
        gi.soundindex("hover/hovatck1.wav");

        self->monsterinfo.engine_sound = gi.soundindex("hover/hovidle1.wav");
    }
    // PGM

    gi.linkentity(self);

    M_SetAnimation(self, &hover_move_stand);
    self->monsterinfo.scale = MODEL_SCALE;

    flymonster_start(self);

    // PGM
    if (strcmp(self->classname, "monster_daedalus") == 0)
        self->s.skinnum = 2;
    // PGM

    self->monsterinfo.aiflags |= AI_ALTERNATE_FLY;
    hover_set_fly_parameters(self);
}
