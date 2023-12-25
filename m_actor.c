// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_actor.c

#include "g_local.h"
#include "m_actor.h"

static const char *const actor_names[] = {
    "Hellrot",
    "Tokay",
    "Killme",
    "Disruptor",
    "Adrianator",
    "Rambear",
    "Titus",
    "Bitterman"
};

static const mframe_t actor_frames_stand[] = {
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
const mmove_t MMOVE_T(actor_move_stand) = { FRAME_stand101, FRAME_stand140, actor_frames_stand, NULL };

void MONSTERINFO_STAND(actor_stand)(edict_t *self)
{
    M_SetAnimation(self, &actor_move_stand);

    // randomize on startup
    if (level.time < SEC(1))
        self->s.frame = irandom2(self->monsterinfo.active_move->firstframe, self->monsterinfo.active_move->lastframe + 1);
}

static const mframe_t actor_frames_walk[] = {
    { ai_walk },
    { ai_walk, 6 },
    { ai_walk, 10 },
    { ai_walk, 3 },
    { ai_walk, 2 },
    { ai_walk, 7 },
    { ai_walk, 10 },
    { ai_walk, 1 }
};
const mmove_t MMOVE_T(actor_move_walk) = { FRAME_walk01, FRAME_walk08, actor_frames_walk, NULL };

void MONSTERINFO_WALK(actor_walk)(edict_t *self)
{
    M_SetAnimation(self, &actor_move_walk);
}

static const mframe_t actor_frames_run[] = {
    { ai_run, 4 },
    { ai_run, 15 },
    { ai_run, 15 },
    { ai_run, 8 },
    { ai_run, 20 },
    { ai_run, 15 }
};
const mmove_t MMOVE_T(actor_move_run) = { FRAME_run02, FRAME_run07, actor_frames_run, NULL };

void MONSTERINFO_RUN(actor_run)(edict_t *self)
{
    if ((level.time < self->pain_debounce_time) && (!self->enemy)) {
        if (self->movetarget)
            actor_walk(self);
        else
            actor_stand(self);
        return;
    }

    if (self->monsterinfo.aiflags & AI_STAND_GROUND) {
        actor_stand(self);
        return;
    }

    M_SetAnimation(self, &actor_move_run);
}

static const mframe_t actor_frames_pain1[] = {
    { ai_move, -5 },
    { ai_move, 4 },
    { ai_move, 1 }
};
const mmove_t MMOVE_T(actor_move_pain1) = { FRAME_pain101, FRAME_pain103, actor_frames_pain1, actor_run };

static const mframe_t actor_frames_pain2[] = {
    { ai_move, -4 },
    { ai_move, 4 },
    { ai_move }
};
const mmove_t MMOVE_T(actor_move_pain2) = { FRAME_pain201, FRAME_pain203, actor_frames_pain2, actor_run };

static const mframe_t actor_frames_pain3[] = {
    { ai_move, -1 },
    { ai_move, 1 },
    { ai_move, 0 }
};
const mmove_t MMOVE_T(actor_move_pain3) = { FRAME_pain301, FRAME_pain303, actor_frames_pain3, actor_run };

static const mframe_t actor_frames_flipoff[] = {
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn }
};
const mmove_t MMOVE_T(actor_move_flipoff) = { FRAME_flip01, FRAME_flip14, actor_frames_flipoff, actor_run };

static const mframe_t actor_frames_taunt[] = {
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn },
    { ai_turn }
};
const mmove_t MMOVE_T(actor_move_taunt) = { FRAME_taunt01, FRAME_taunt17, actor_frames_taunt, actor_run };

static const char *const messages[] = {
    "Watch it",
    "#$@*&",
    "Idiot",
    "Check your targets"
};

void PAIN(actor_pain)(edict_t *self, edict_t *other, float kick, int damage, const mod_t mod)
{
    int n;

    if (level.time < self->pain_debounce_time)
        return;

    self->pain_debounce_time = level.time + SEC(3);
    //  gi.sound (self, CHAN_VOICE, actor.sound_pain, 1, ATTN_NORM, 0);

    if ((other->client) && (frandom() < 0.4f)) {
        vec3_t      v;
        const char *name;

        VectorSubtract(other->s.origin, self->s.origin, v);
        self->ideal_yaw = vectoyaw(v);
        if (brandom())
            M_SetAnimation(self, &actor_move_flipoff);
        else
            M_SetAnimation(self, &actor_move_taunt);
        name = actor_names[(self - g_edicts) % q_countof(actor_names)];
        gi.cprintf(other, PRINT_CHAT, "%s: %s!\n", name, random_element(messages));
        return;
    }

    n = irandom1(3);
    if (n == 0)
        M_SetAnimation(self, &actor_move_pain1);
    else if (n == 1)
        M_SetAnimation(self, &actor_move_pain2);
    else
        M_SetAnimation(self, &actor_move_pain3);
}

void MONSTERINFO_SETSKIN(actor_setskin)(edict_t *self)
{
    if (self->health < (self->max_health / 2))
        self->s.skinnum = 1;
    else
        self->s.skinnum = 0;
}

static void actorMachineGun(edict_t *self)
{
    vec3_t start, target;
    vec3_t forward, right;

    AngleVectors(self->s.angles, forward, right, NULL);
    G_ProjectSource(self->s.origin, monster_flash_offset[MZ2_ACTOR_MACHINEGUN_1], forward, right, start);
    if (self->enemy) {
        if (self->enemy->health > 0) {
            VectorMA(self->enemy->s.origin, -0.2f, self->enemy->velocity, target);
            target[2] += self->enemy->viewheight;
        } else {
            VectorCopy(self->enemy->absmin, target);
            target[2] += (self->enemy->size[2] / 2) + 1;
        }
        VectorSubtract(target, start, forward);
        VectorNormalize(forward);
    } else {
        AngleVectors(self->s.angles, forward, NULL, NULL);
    }
    monster_fire_bullet(self, start, forward, 3, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_ACTOR_MACHINEGUN_1);
}

static void actor_dead(edict_t *self)
{
    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, -8);
    self->movetype = MOVETYPE_TOSS;
    self->svflags |= SVF_DEADMONSTER;
    self->nextthink = 0;
    gi.linkentity(self);
}

static const mframe_t actor_frames_death1[] = {
    { ai_move },
    { ai_move },
    { ai_move, -13 },
    { ai_move, 14 },
    { ai_move, 3 },
    { ai_move, -2 },
    { ai_move, 1 }
};
const mmove_t MMOVE_T(actor_move_death1) = { FRAME_death101, FRAME_death107, actor_frames_death1, actor_dead };

static const mframe_t actor_frames_death2[] = {
    { ai_move },
    { ai_move, 7 },
    { ai_move, -6 },
    { ai_move, -5 },
    { ai_move, 1 },
    { ai_move },
    { ai_move, -1 },
    { ai_move, -2 },
    { ai_move, -1 },
    { ai_move, -9 },
    { ai_move, -13 },
    { ai_move, -13 },
    { ai_move }
};
const mmove_t MMOVE_T(actor_move_death2) = { FRAME_death201, FRAME_death213, actor_frames_death2, actor_dead };

static const gib_def_t actor_gibs[] = {
    { "models/objects/gibs/bone/tris.md2", 2 },
    { "models/objects/gibs/sm_meat/tris.md2", 4 },
    { "models/objects/gibs/head2/tris.md2", 1, GIB_HEAD },
    { 0 }
};

void DIE(actor_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, const mod_t mod)
{
    // check for gib
    if (self->health <= -80) {
        // gi.sound (self, CHAN_VOICE, actor.sound_gib, 1, ATTN_NORM, 0);
        ThrowGibs(self, damage, actor_gibs);
        self->deadflag = true;
        return;
    }

    if (self->deadflag)
        return;

    // regular death
    //  gi.sound (self, CHAN_VOICE, actor.sound_die, 1, ATTN_NORM, 0);
    self->deadflag = true;
    self->takedamage = true;

    if (brandom())
        M_SetAnimation(self, &actor_move_death1);
    else
        M_SetAnimation(self, &actor_move_death2);
}

static void actor_fire(edict_t *self)
{
    actorMachineGun(self);

    if (level.time >= self->monsterinfo.fire_wait)
        self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
    else
        self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static const mframe_t actor_frames_attack[] = {
    { ai_charge, -2, actor_fire },
    { ai_charge, -2 },
    { ai_charge, 3 },
    { ai_charge, 2 }
};
const mmove_t MMOVE_T(actor_move_attack) = { FRAME_attak01, FRAME_attak04, actor_frames_attack, actor_run };

void MONSTERINFO_ATTACK(actor_attack)(edict_t *self)
{
    M_SetAnimation(self, &actor_move_attack);
    self->monsterinfo.fire_wait = level.time + random_time_sec(1, 2.6f);
}

void USE(actor_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    vec3_t v;

    self->goalentity = self->movetarget = G_PickTarget(self->target);
    if ((!self->movetarget) || (strcmp(self->movetarget->classname, "target_actor") != 0)) {
        gi.dprintf("%s: bad target %s\n", etos(self), self->target);
        self->target = NULL;
        self->monsterinfo.pausetime = HOLD_FOREVER;
        self->monsterinfo.stand(self);
        return;
    }

    VectorSubtract(self->goalentity->s.origin, self->s.origin, v);
    self->ideal_yaw = self->s.angles[YAW] = vectoyaw(v);
    self->monsterinfo.walk(self);
    self->target = NULL;
}

/*QUAKED misc_actor (1 .5 0) (-16 -16 -24) (16 16 32)
 */

void SP_misc_actor(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    if (!self->targetname) {
        gi.dprintf("%s: no targetname\n", etos(self));
        G_FreeEdict(self);
        return;
    }

    if (!self->target) {
        gi.dprintf("%s: no target\n", etos(self));
        G_FreeEdict(self);
        return;
    }

    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;
    self->s.modelindex = gi.modelindex("players/male/tris.md2");
    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, 32);

    if (!self->health)
        self->health = 100;
    self->mass = 200;

    self->pain = actor_pain;
    self->die = actor_die;

    self->monsterinfo.stand = actor_stand;
    self->monsterinfo.walk = actor_walk;
    self->monsterinfo.run = actor_run;
    self->monsterinfo.attack = actor_attack;
    self->monsterinfo.melee = NULL;
    self->monsterinfo.sight = NULL;
    self->monsterinfo.setskin = actor_setskin;

    self->monsterinfo.aiflags |= AI_GOOD_GUY;

    gi.linkentity(self);

    M_SetAnimation(self, &actor_move_stand);
    self->monsterinfo.scale = MODEL_SCALE;

    walkmonster_start(self);

    // actors always start in a dormant state, they *must* be used to get going
    self->use = actor_use;
}

/*QUAKED target_actor (.5 .3 0) (-8 -8 -8) (8 8 8) JUMP SHOOT ATTACK x HOLD BRUTAL
JUMP            jump in set direction upon reaching this target
SHOOT           take a single shot at the pathtarget
ATTACK          attack pathtarget until it or actor is dead

"target"        next target_actor
"pathtarget"    target of any action to be taken at this point
"wait"          amount of time actor should pause at this point
"message"       actor will "say" this to the player

for JUMP only:
"speed"         speed thrown forward (default 200)
"height"        speed thrown upwards (default 200)
*/

#define SPAWNFLAG_TARGET_ACTOR_JUMP     1
#define SPAWNFLAG_TARGET_ACTOR_SHOOT    2
#define SPAWNFLAG_TARGET_ACTOR_ATTACK   4
#define SPAWNFLAG_TARGET_ACTOR_HOLD     16
#define SPAWNFLAG_TARGET_ACTOR_BRUTAL   32

void TOUCH(target_actor_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    vec3_t v;

    if (other->movetarget != self)
        return;

    if (other->enemy)
        return;

    other->goalentity = other->movetarget = NULL;

    if (self->message) {
        for (int n = 1; n <= game.maxclients; n++) {
            edict_t *ent = &g_edicts[n];
            if (!ent->inuse)
                continue;
            gi.cprintf(ent, PRINT_CHAT, "%s: %s\n", actor_names[(other - g_edicts) % q_countof(actor_names)], self->message);
        }
    }

    if (self->spawnflags & SPAWNFLAG_TARGET_ACTOR_JUMP) { // jump
        other->velocity[0] = self->movedir[0] * self->speed;
        other->velocity[1] = self->movedir[1] * self->speed;

        if (other->groundentity) {
            other->groundentity = NULL;
            other->velocity[2] = self->movedir[2];
            gi.sound(other, CHAN_VOICE, gi.soundindex("player/male/jump1.wav"), 1, ATTN_NORM, 0);
        }
    }

    if (self->spawnflags & SPAWNFLAG_TARGET_ACTOR_SHOOT) { // shoot
    } else if (self->spawnflags & SPAWNFLAG_TARGET_ACTOR_ATTACK) { // attack
        other->enemy = G_PickTarget(self->pathtarget);
        if (other->enemy) {
            other->goalentity = other->enemy;
            if (self->spawnflags & SPAWNFLAG_TARGET_ACTOR_BRUTAL)
                other->monsterinfo.aiflags |= AI_BRUTAL;
            if (self->spawnflags & SPAWNFLAG_TARGET_ACTOR_HOLD) {
                other->monsterinfo.aiflags |= AI_STAND_GROUND;
                actor_stand(other);
            } else {
                actor_run(other);
            }
        }
    }

    if (!(self->spawnflags & (SPAWNFLAG_TARGET_ACTOR_ATTACK | SPAWNFLAG_TARGET_ACTOR_SHOOT)) && (self->pathtarget)) {
        const char *savetarget = self->target;
        self->target = self->pathtarget;
        G_UseTargets(self, other);
        self->target = savetarget;
    }

    other->movetarget = G_PickTarget(self->target);

    if (!other->goalentity)
        other->goalentity = other->movetarget;

    if (!other->movetarget && !other->enemy) {
        other->monsterinfo.pausetime = HOLD_FOREVER;
        other->monsterinfo.stand(other);
    } else if (other->movetarget == other->goalentity) {
        VectorSubtract(other->movetarget->s.origin, other->s.origin, v);
        other->ideal_yaw = vectoyaw(v);
    }
}

void SP_target_actor(edict_t *self)
{
    if (!self->targetname)
        gi.dprintf("%s: no targetname\n", etos(self));

    self->solid = SOLID_TRIGGER;
    self->touch = target_actor_touch;
    VectorSet(self->mins, -8, -8, -8);
    VectorSet(self->maxs, 8, 8, 8);
    self->svflags = SVF_NOCLIENT;

    if (self->spawnflags & SPAWNFLAG_TARGET_ACTOR_JUMP) {
        if (!self->speed)
            self->speed = 200;
        if (!st.height)
            st.height = 200;
        if (self->s.angles[YAW] == 0)
            self->s.angles[YAW] = 360;
        G_SetMovedir(self->s.angles, self->movedir);
        self->movedir[2] = st.height;
    }

    gi.linkentity(self);
}
