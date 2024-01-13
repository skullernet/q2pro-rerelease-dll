// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

parasite

==============================================================================
*/

#include "g_local.h"
#include "m_parasite.h"

#define PARASITE_MISS_CHANCE        0.1f
#define PARASITE_PROBOSCIS_SPEED    1250
#define PARASITE_PROBOSCIS_RETRACT_MODIFIER 2.0f

static int sound_pain1;
static int sound_pain2;
static int sound_die;
static int sound_launch;
static int sound_impact;
static int sound_suck;
static int sound_reelin;
static int sound_sight;
static int sound_tap;
static int sound_scratch;
static int sound_search;

void parasite_stand(edict_t *self);
void parasite_start_run(edict_t *self);
static void parasite_run(edict_t *self);
static void parasite_walk(edict_t *self);
static void parasite_do_fidget(edict_t *self);
static void parasite_refidget(edict_t *self);

static void parasite_launch(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_launch, 1, ATTN_NORM, 0);
}

static void parasite_reel_in(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_reelin, 1, ATTN_NORM, 0);
}

void MONSTERINFO_SIGHT(parasite_sight)(edict_t *self, edict_t *other)
{
    gi.sound(self, CHAN_WEAPON, sound_sight, 1, ATTN_NORM, 0);
}

static void parasite_tap(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_tap, 0.75f, 2.75f, 0);
}

static void parasite_scratch(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_scratch, 0.75f, 2.75f, 0);
}

#if 0
static void parasite_search(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_search, 1, ATTN_IDLE, 0);
}
#endif

static const mframe_t parasite_frames_start_fidget[] = {
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand }
};
const mmove_t MMOVE_T(parasite_move_start_fidget) = { FRAME_stand18, FRAME_stand21, parasite_frames_start_fidget, parasite_do_fidget };

static const mframe_t parasite_frames_fidget[] = {
    { ai_stand, 0, parasite_scratch },
    { ai_stand },
    { ai_stand },
    { ai_stand, 0, parasite_scratch },
    { ai_stand },
    { ai_stand }
};
const mmove_t MMOVE_T(parasite_move_fidget) = { FRAME_stand22, FRAME_stand27, parasite_frames_fidget, parasite_refidget };

static const mframe_t parasite_frames_end_fidget[] = {
    { ai_stand, 0, parasite_scratch },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand }
};
const mmove_t MMOVE_T(parasite_move_end_fidget) = { FRAME_stand28, FRAME_stand35, parasite_frames_end_fidget, parasite_stand };

static void parasite_do_fidget(edict_t *self)
{
    M_SetAnimation(self, &parasite_move_fidget);
}

static void parasite_refidget(edict_t *self)
{
    if (frandom() <= 0.8f)
        M_SetAnimation(self, &parasite_move_fidget);
    else
        M_SetAnimation(self, &parasite_move_end_fidget);
}

void MONSTERINFO_IDLE(parasite_idle)(edict_t *self)
{
    if (self->enemy)
        return;

    M_SetAnimation(self, &parasite_move_start_fidget);
}

static const mframe_t parasite_frames_stand[] = {
    { ai_stand },
    { ai_stand },
    { ai_stand, 0, parasite_tap },
    { ai_stand },
    { ai_stand, 0, parasite_tap },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand, 0, parasite_tap },
    { ai_stand },
    { ai_stand, 0, parasite_tap },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand, 0, parasite_tap },
    { ai_stand },
    { ai_stand, 0, parasite_tap }
};
const mmove_t MMOVE_T(parasite_move_stand) = { FRAME_stand01, FRAME_stand17, parasite_frames_stand, parasite_stand };

void MONSTERINFO_STAND(parasite_stand)(edict_t *self)
{
    M_SetAnimation(self, &parasite_move_stand);
}

static const mframe_t parasite_frames_run[] = {
    { ai_run, 30 },
    { ai_run, 30 },
    { ai_run, 22, monster_footstep },
    { ai_run, 19, monster_footstep },
    { ai_run, 24 },
    { ai_run, 28, monster_footstep },
    { ai_run, 25, monster_footstep }
};
const mmove_t MMOVE_T(parasite_move_run) = { FRAME_run03, FRAME_run09, parasite_frames_run, NULL };

static const mframe_t parasite_frames_start_run[] = {
    { ai_run },
    { ai_run, 30 },
};
const mmove_t MMOVE_T(parasite_move_start_run) = { FRAME_run01, FRAME_run02, parasite_frames_start_run, parasite_run };

#if 0
static const mframe_t parasite_frames_stop_run[] = {
    { ai_run, 20 },
    { ai_run, 20 },
    { ai_run, 12 },
    { ai_run, 10 },
    { ai_run },
    { ai_run }
};
const mmove_t MMOVE_T(parasite_move_stop_run) = { FRAME_run10, FRAME_run15, parasite_frames_stop_run, NULL };
#endif

void MONSTERINFO_RUN(parasite_start_run)(edict_t *self)
{
    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        M_SetAnimation(self, &parasite_move_stand);
    else
        M_SetAnimation(self, &parasite_move_start_run);
}

static void proboscis_retract(edict_t *self);

static void parasite_run(edict_t *self)
{
    if (self->proboscus && self->proboscus->style != 2)
        proboscis_retract(self->proboscus);

    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        M_SetAnimation(self, &parasite_move_stand);
    else
        M_SetAnimation(self, &parasite_move_run);
}

static const mframe_t parasite_frames_walk[] = {
    { ai_walk, 30 },
    { ai_walk, 30 },
    { ai_walk, 22, monster_footstep },
    { ai_walk, 19, monster_footstep },
    { ai_walk, 24 },
    { ai_walk, 28, monster_footstep },
    { ai_walk, 25, monster_footstep }
};
const mmove_t MMOVE_T(parasite_move_walk) = { FRAME_run03, FRAME_run09, parasite_frames_walk, parasite_walk };

static const mframe_t parasite_frames_start_walk[] = {
    { ai_walk, 0 },
    { ai_walk, 30, parasite_walk }
};
const mmove_t MMOVE_T(parasite_move_start_walk) = { FRAME_run01, FRAME_run02, parasite_frames_start_walk, NULL };

#if 0
static const mframe_t parasite_frames_stop_walk[] = {
    { ai_walk, 20 },
    { ai_walk, 20 },
    { ai_walk, 12 },
    { ai_walk, 10 },
    { ai_walk },
    { ai_walk }
};
const mmove_t MMOVE_T(parasite_move_stop_walk) = { FRAME_run10, FRAME_run15, parasite_frames_stop_walk, NULL };
#endif

void MONSTERINFO_WALK(parasite_start_walk)(edict_t *self)
{
    M_SetAnimation(self, &parasite_move_start_walk);
}

static void parasite_walk(edict_t *self)
{
    M_SetAnimation(self, &parasite_move_walk);
}

// hard reset on proboscis; like we never existed
void THINK(proboscis_reset)(edict_t *self)
{
    self->owner->proboscus = NULL;
    G_FreeEdict(self->proboscus);
    G_FreeEdict(self);
}

void DIE(proboscis_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    if (mod.id == MOD_CRUSH)
        proboscis_reset(self);
}

const mmove_t parasite_move_fire_proboscis;

static void parasite_break_wait(edict_t *self)
{
    // prob exploded?
    if (self->proboscus && self->proboscus->style != 3)
        self->monsterinfo.nextframe = FRAME_break19;
    else if (brandom()) {
        // don't get hurt
        parasite_reel_in(self);
        self->monsterinfo.nextframe = FRAME_break31;
    }
}

static void proboscis_retract(edict_t *self)
{
    // start retract animation
    if (self->owner->monsterinfo.active_move == &parasite_move_fire_proboscis)
        self->owner->monsterinfo.nextframe = FRAME_drain12;

    // mark as retracting
    self->movetype = MOVETYPE_NONE;
    self->solid = SOLID_NOT;
    // come back real hard
    if (self->style != 2)
        self->speed *= PARASITE_PROBOSCIS_RETRACT_MODIFIER;
    self->style = 2;
    gi.linkentity(self);
}

static void parasite_break_retract(edict_t *self)
{
    if (self->proboscus)
        proboscis_retract(self->proboscus);
}

static void parasite_break_sound(edict_t *self)
{
    if (brandom())
        gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
    else
        gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

    self->pain_debounce_time = level.time + SEC(3);
}

void proboscis_segment_draw(edict_t *self);

static void parasite_charge_proboscis(edict_t *self, float dist)
{
    if (self->s.frame >= FRAME_break01 && self->s.frame <= FRAME_break32)
        ai_move(self, dist);
    else
        ai_charge(self, dist);

    if (self->proboscus)
        proboscis_segment_draw(self->proboscus->proboscus);
}

static void parasite_break_noise(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

static const mframe_t parasite_frames_break[] = {
    { parasite_charge_proboscis },
    { parasite_charge_proboscis, -3, parasite_break_noise },
    { parasite_charge_proboscis, 1 },
    { parasite_charge_proboscis, 2 },
    { parasite_charge_proboscis, -3 },
    { parasite_charge_proboscis, 1 },
    { parasite_charge_proboscis, 1 },
    { parasite_charge_proboscis, 3 },
    { parasite_charge_proboscis, 0, parasite_break_noise },
    { parasite_charge_proboscis, -18 },
    { parasite_charge_proboscis, 3 },
    { parasite_charge_proboscis, 9 },
    { parasite_charge_proboscis, 6 },
    { parasite_charge_proboscis },
    { parasite_charge_proboscis, -18 },
    { parasite_charge_proboscis },
    { parasite_charge_proboscis, 8, parasite_break_retract },
    { parasite_charge_proboscis, 9 },
    { parasite_charge_proboscis, 0, parasite_break_wait },
    { parasite_charge_proboscis, -18, parasite_break_sound },
    { parasite_charge_proboscis },
    { parasite_charge_proboscis }, // airborne
    { parasite_charge_proboscis }, // airborne
    { parasite_charge_proboscis }, // slides
    { parasite_charge_proboscis }, // slides
    { parasite_charge_proboscis }, // slides
    { parasite_charge_proboscis }, // slides
    { parasite_charge_proboscis, 4 },
    { parasite_charge_proboscis, 11 },
    { parasite_charge_proboscis, -2 },
    { parasite_charge_proboscis, -5 },
    { parasite_charge_proboscis, 1 }
};
const mmove_t MMOVE_T(parasite_move_break) = { FRAME_break01, FRAME_break32, parasite_frames_break, parasite_start_run };

void TOUCH(proboscis_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    // owner isn't trying to probe any more, don't touch anything
    if (self->owner->monsterinfo.active_move != &parasite_move_fire_proboscis)
        return;

    vec3_t p;

    // hit what we want to succ
    if ((other->svflags & SVF_PLAYER) || other == self->owner->enemy) {
        if (tr->startsolid)
            VectorCopy(tr->endpos, p);
        else {
            vec3_t dir;
            VectorSubtract(self->s.origin, tr->endpos, dir);
            VectorNormalize(dir);
            VectorMA(tr->endpos, -12, dir, p);
        }

        self->owner->monsterinfo.nextframe = FRAME_drain06;
        self->movetype = MOVETYPE_NONE;
        self->solid = SOLID_NOT;
        self->style = 1;
        // stick to this guy
        VectorSubtract(p, other->s.origin, self->move_origin);
        self->enemy = other;
        self->x.alpha = 0.35f;
        gi.sound(self, CHAN_WEAPON, sound_suck, 1, ATTN_NORM, 0);
    } else {
        VectorAdd(tr->endpos, tr->plane.normal, p);
        // hit monster, don't suck but do small damage
        // and retract immediately
        if (other->svflags & (SVF_MONSTER | SVF_DEADMONSTER))
            proboscis_retract(self);
        else {
            // hit wall; stick to it and do break animation
            self->owner->monsterinfo.active_move = &parasite_move_break;
            self->movetype = MOVETYPE_NONE;
            self->solid = SOLID_NOT;
            self->style = 1;
            self->owner->s.angles[YAW] = self->s.angles[YAW];
        }
    }

    if (other->takedamage)
        T_Damage(other, self, self->owner, tr->plane.normal, tr->endpos, tr->plane.normal, 5, 0, DAMAGE_NONE, (mod_t) { MOD_UNKNOWN });

    gi.positioned_sound(tr->endpos, self->owner, CHAN_AUTO, sound_impact, 1, ATTN_NORM, 0);

    VectorCopy(p, self->s.origin);
    self->nextthink = level.time + FRAME_TIME; // start doing stuff on next frame
    gi.linkentity(self);
}

// from break01
static const vec3_t parasite_break_offsets[] = {
    { 7.0f, 0, 7.0f },
    { 6.3f, 14.5f, 4.0f },
    { 8.5f, 0, 5.6f },
    { 5.0f, -15.25f, 4.0f, },
    { 9.5f, -1.8f, 5.9f },
    { 6.2f, 14.0f, 4.0f },
    { 12.25f, 7.5f, 1.4f },
    { 13.8f, 0, -2.4f },
    { 13.8f, 0, -4.0f },
    { 0.1f, 0, -0.7f },
    { 5.0f, 0, 3.7f },
    { 11.0f, 0, 4.0f },
    { 13.5f, 0, -4.0f },
    { 13.5f, 0, -4.0f },
    { 0.2f, 0, -0.7f },
    { 3.9f, 0, 3.6f },
    { 8.5f, 0, 5.0f },
    { 14.0f, 0, -4.0f },
    { 14.0f, 0, -4.0f },
    { 0.1f, 0, -0.5f }
};

// from drain01
static const vec3_t parasite_drain_offsets[] = {
    { -1.7f, 0, 1.2f },
    { -2.2f, 0, -0.6f },
    { 7.7f, 0, 7.2f },
    { 7.2f, 0, 5.7f },
    { 6.2f, 0, 7.8f },
    { 4.7f, 0, 6.7f },
    { 5.0f, 0, 9.0f },
    { 5.0f, 0, 7.0f },
    { 5.0f, 0, 10.5f },
    { 4.5f, 0, 9.7f },
    { 1.5f, 0, 12.0f },
    { 2.9f, 0, 11.0f },
    { 2.1f, 0, 7.6f },
};

static void parasite_get_proboscis_start(edict_t *self, vec3_t start)
{
    const vec_t *offset = (const vec3_t) { 8, 0, 6 };
    vec3_t f, r;

    AngleVectors(self->s.angles, f, r, NULL);

    if (self->s.frame >= FRAME_break01 && self->s.frame < FRAME_break01 + q_countof(parasite_break_offsets))
        offset = parasite_break_offsets[self->s.frame - FRAME_break01];
    else if (self->s.frame >= FRAME_drain01 && self->s.frame < FRAME_drain01 + q_countof(parasite_drain_offsets))
        offset = parasite_drain_offsets[self->s.frame - FRAME_drain01];

    M_ProjectFlashSource(self, offset, f, r, start);
}

void THINK(proboscis_think)(edict_t *self)
{
    self->nextthink = level.time + FRAME_TIME; // start doing stuff on next frame

    // retracting; keep pulling until we hit the parasite
    if (self->style == 2) {
        vec3_t start, dir;
        float dist;

        parasite_get_proboscis_start(self->owner, start);
        VectorSubtract(self->s.origin, start, dir);

        dist = VectorNormalize(dir);
        if (dist <= (self->speed * 2) * FRAME_TIME_SEC) {
            // reached target; free self on next frame, let parasite know
            self->style = 3;
            self->think = proboscis_reset;
            VectorCopy(start, self->s.origin);
            gi.linkentity(self);
            return;
        }

        // pull us in
        VectorMA(self->s.origin, -(self->speed * FRAME_TIME_SEC), dir, self->s.origin);
        gi.linkentity(self);
    // stuck on target; do damage, suck health
    // and check if target goes away
    } else if (self->style == 1) {
        if (!self->enemy) {
            // stuck in wall
        } else if (!self->enemy->inuse || self->enemy->health <= 0 || !self->enemy->takedamage) {
            // target gone, retract early
            proboscis_retract(self);
        } else {
            vec3_t start, dir;

            // update our position
            VectorAdd(self->enemy->s.origin, self->move_origin, self->s.origin);

            parasite_get_proboscis_start(self->owner, start);
            VectorSubtract(self->s.origin, start, dir);

            vectoangles(dir, self->s.angles);

            // see if we got cut by the world
            trace_t tr = gi.trace(start, NULL, NULL, self->s.origin, NULL, MASK_SOLID);

            if (tr.fraction != 1.0f) {
                // blocked, so retract
                proboscis_retract(self);
                VectorCopy(self->s.old_origin, self->s.origin);
            } else {
                // succ & drain
                if (self->timestamp <= level.time) {
                    T_Damage(self->enemy, self, self->owner, tr.plane.normal, tr.endpos, tr.plane.normal, 2, 0, DAMAGE_NONE, (mod_t) { MOD_UNKNOWN });
                    self->owner->health = min(self->owner->max_health, self->owner->health + 2);
                    self->owner->monsterinfo.setskin(self->owner);
                    self->timestamp = level.time + HZ(10);
                }
            }

            gi.linkentity(self);
        }
    // flying
    } else if (self->style == 0) {
        vec3_t to_target;
        float dist_to_target;

        // owner gone away?
        if (!self->owner->enemy || !self->owner->enemy->inuse || self->owner->enemy->health <= 0) {
            proboscis_retract(self);
            return;
        }

        // if we're well behind our target and missed by 2x velocity,
        // be smart enough to pull in automatically
        VectorSubtract(self->s.origin, self->owner->enemy->s.origin, to_target);
        dist_to_target = VectorNormalize(to_target);

        if (dist_to_target > (self->speed * 2) / 15.0f) {
            vec3_t from_owner;

            VectorSubtract(self->s.origin, self->owner->s.origin, from_owner);
            VectorNormalize(from_owner);

            if (DotProduct(to_target, from_owner) > 0) {
                proboscis_retract(self);
                return;
            }
        }
    }
}

void PRETHINK(proboscis_segment_draw)(edict_t *self)
{
    vec3_t start, dir;

    parasite_get_proboscis_start(self->owner->owner, start);

    VectorCopy(start, self->s.origin);
    VectorSubtract(self->owner->s.origin, start, dir);
    VectorNormalize(dir);
    VectorMA(self->owner->s.origin, -8, dir, self->s.old_origin);
    gi.linkentity(self);
}

static void fire_proboscis(edict_t *self, vec3_t start, vec3_t dir, float speed)
{
    edict_t *tip = G_Spawn();
    vectoangles(dir, tip->s.angles);
    tip->s.modelindex = gi.modelindex("models/monsters/parasite/tip/tris.md2");
    tip->movetype = MOVETYPE_FLYMISSILE;
    tip->owner = self;
    self->proboscus = tip;
    tip->clipmask = MASK_PROJECTILE & ~CONTENTS_DEADMONSTER;
    VectorCopy(start, tip->s.origin);
    VectorCopy(start, tip->s.old_origin);
    tip->speed = speed;
    VectorScale(dir, speed, tip->velocity);
    tip->solid = SOLID_BBOX;
    tip->takedamage = true;
    tip->flags |= FL_NO_DAMAGE_EFFECTS | FL_NO_KNOCKBACK;
    tip->die = proboscis_die;
    tip->touch = proboscis_touch;
    tip->think = proboscis_think;
    tip->nextthink = level.time + FRAME_TIME; // start doing stuff on next frame
    tip->svflags |= SVF_PROJECTILE;

    edict_t *segment = G_Spawn();
    segment->s.modelindex = gi.modelindex("models/monsters/parasite/segment/tris.md2");
    segment->s.renderfx = RF_BEAM;
    segment->postthink = proboscis_segment_draw;

    tip->proboscus = segment;
    segment->owner = tip;

    vec3_t pos;
    VectorMA(tip->s.origin, FRAME_TIME_SEC, tip->velocity, pos);
    trace_t tr = gi.trace(tip->s.origin, NULL, NULL, pos, self, tip->clipmask);
    if (tr.startsolid) {
        VectorNegate(dir, tr.plane.normal);
        VectorCopy(start, tr.endpos);
        tip->touch(tip, tr.ent, &tr, false);
    } else if (tr.fraction < 1.0f)
        tip->touch(tip, tr.ent, &tr, false);

    vec3_t dir2;
    VectorCopy(start, segment->s.origin);
    VectorSubtract(tip->s.origin, start, dir2);
    VectorNormalize(dir2);
    VectorMA(tip->s.origin, 8, dir2, segment->s.old_origin);

    gi.linkentity(tip);
    gi.linkentity(segment);
}

static void parasite_fire_proboscis(edict_t *self)
{
    if (self->proboscus && self->proboscus->style != 2)
        proboscis_reset(self->proboscus);

    vec3_t start;
    parasite_get_proboscis_start(self, start);

    vec3_t dir;
    PredictAim(self, self->enemy, start, PARASITE_PROBOSCIS_SPEED, false, crandom_open() * PARASITE_MISS_CHANCE, dir, NULL);

    fire_proboscis(self, start, dir, PARASITE_PROBOSCIS_SPEED);
}

static void parasite_proboscis_wait(edict_t *self)
{
    // loop frames while we wait
    if (self->s.frame == FRAME_drain04)
        self->monsterinfo.nextframe = FRAME_drain05;
    else
        self->monsterinfo.nextframe = FRAME_drain04;
}

static void parasite_proboscis_pull_wait(edict_t *self)
{
    // prob exploded?
    if (!self->proboscus || self->proboscus->style == 3) {
        self->monsterinfo.nextframe = FRAME_drain14;
        return;
    }

    // being pulled in, so wait until we get destroyed
    if (self->s.frame == FRAME_drain12)
        self->monsterinfo.nextframe = FRAME_drain13;
    else
        self->monsterinfo.nextframe = FRAME_drain12;

    if (self->proboscus->style != 2)
        proboscis_retract(self->proboscus);
}

static const mframe_t parasite_frames_fire_proboscis[] = {
    { parasite_charge_proboscis, 0, parasite_launch },
    { parasite_charge_proboscis },
    { parasite_charge_proboscis, 15, parasite_fire_proboscis }, // Target hits
    { parasite_charge_proboscis, 0, parasite_proboscis_wait },  // drain
    { parasite_charge_proboscis, 0, parasite_proboscis_wait },  // drain
    { parasite_charge_proboscis, 0 },  // drain
    { parasite_charge_proboscis, 0 },  // drain
    { parasite_charge_proboscis, -2 }, // drain
    { parasite_charge_proboscis, -2 }, // drain
    { parasite_charge_proboscis, -3 }, // drain
    { parasite_charge_proboscis, -2 }, // drain
    { parasite_charge_proboscis, 0, parasite_proboscis_pull_wait },  // drain
    { parasite_charge_proboscis, -1, parasite_proboscis_pull_wait }, // drain
    { parasite_charge_proboscis, 0, parasite_reel_in },       // let go
    { parasite_charge_proboscis, -2 },
    { parasite_charge_proboscis, -2 },
    { parasite_charge_proboscis, -3 },
    { parasite_charge_proboscis }
};
const mmove_t MMOVE_T(parasite_move_fire_proboscis) = { FRAME_drain01, FRAME_drain18, parasite_frames_fire_proboscis, parasite_start_run };

void MONSTERINFO_ATTACK(parasite_attack)(edict_t *self)
{
    if (!M_CheckClearShot(self, parasite_drain_offsets[0]))
        return;

    if (self->proboscus && self->proboscus->style != 2)
        proboscis_retract(self->proboscus);

    M_SetAnimation(self, &parasite_move_fire_proboscis);
}

//================
// ROGUE
static void parasite_jump_down(edict_t *self)
{
    vec3_t forward, up;

    AngleVectors(self->s.angles, forward, NULL, up);
    VectorMA(self->velocity, 100, forward, self->velocity);
    VectorMA(self->velocity, 300, up, self->velocity);
}

static void parasite_jump_up(edict_t *self)
{
    vec3_t forward, up;

    AngleVectors(self->s.angles, forward, NULL, up);
    VectorMA(self->velocity, 200, forward, self->velocity);
    VectorMA(self->velocity, 450, up, self->velocity);
}

static void parasite_jump_wait_land(edict_t *self)
{
    if (self->groundentity == NULL) {
        self->monsterinfo.nextframe = self->s.frame;

        if (monster_jump_finished(self))
            self->monsterinfo.nextframe = self->s.frame + 1;
    } else
        self->monsterinfo.nextframe = self->s.frame + 1;
}

static const mframe_t parasite_frames_jump_up[] = {
    { ai_move, -8 },
    { ai_move, -8 },
    { ai_move, -8 },
    { ai_move, -8, parasite_jump_up },
    { ai_move },
    { ai_move },
    { ai_move, 0, parasite_jump_wait_land },
    { ai_move }
};
const mmove_t MMOVE_T(parasite_move_jump_up) = { FRAME_jump01, FRAME_jump08, parasite_frames_jump_up, parasite_run };

static const mframe_t parasite_frames_jump_down[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, parasite_jump_down },
    { ai_move },
    { ai_move },
    { ai_move, 0, parasite_jump_wait_land },
    { ai_move }
};
const mmove_t MMOVE_T(parasite_move_jump_down) = { FRAME_jump01, FRAME_jump08, parasite_frames_jump_down, parasite_run };

static void parasite_jump(edict_t *self, blocked_jump_result_t result)
{
    if (!self->enemy)
        return;

    if (result == JUMP_JUMP_UP)
        M_SetAnimation(self, &parasite_move_jump_up);
    else
        M_SetAnimation(self, &parasite_move_jump_down);
}

/*
===
Blocked
===
*/
bool MONSTERINFO_BLOCKED(parasite_blocked)(edict_t *self, float dist)
{
    blocked_jump_result_t result = blocked_checkjump(self, dist);
    if (result != NO_JUMP) {
        if (result != JUMP_TURN)
            parasite_jump(self, result);
        return true;
    }

    if (blocked_checkplat(self, dist))
        return true;

    return false;
}
// ROGUE
//================

/*
===
Death Stuff Starts
===
*/

static void parasite_dead(edict_t *self)
{
    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, -8);
    monster_dead(self);
}

static void parasite_shrink(edict_t *self)
{
    self->maxs[2] = 0;
    self->svflags |= SVF_DEADMONSTER;
    gi.linkentity(self);
}

static const mframe_t parasite_frames_death[] = {
    { ai_move, 0, NULL, FRAME_stand01 },
    { ai_move },
    { ai_move },
    { ai_move, 0, parasite_shrink },
    { ai_move, 0, monster_footstep },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(parasite_move_death) = { FRAME_death101, FRAME_death107, parasite_frames_death, parasite_dead };

static const gib_def_t parasite_gibs[] = {
    { "models/objects/gibs/bone/tris.md2", 1 },
    { "models/objects/gibs/sm_meat/tris.md2", 3 },
    { "models/monsters/parasite/gibs/chest.md2", 1, GIB_SKINNED },
    { "models/monsters/parasite/gibs/bleg.md2", 2, GIB_SKINNED | GIB_UPRIGHT },
    { "models/monsters/parasite/gibs/fleg.md2", 2, GIB_SKINNED | GIB_UPRIGHT },
    { "models/monsters/parasite/gibs/head.md2", 1, GIB_SKINNED | GIB_HEAD },
    { 0 }
};

void DIE(parasite_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    if (self->proboscus && self->proboscus->style != 2)
        proboscis_reset(self->proboscus);

    // check for gib
    if (M_CheckGib(self, mod)) {
        gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
        self->s.skinnum /= 2;
        ThrowGibs(self, damage, parasite_gibs);
        self->deadflag = true;
        return;
    }

    if (self->deadflag)
        return;

    // regular death
    gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
    self->deadflag = true;
    self->takedamage = true;
    M_SetAnimation(self, &parasite_move_death);
}

/*
===
End Death Stuff
===
*/
static void parasite_skipframe(edict_t *self)
{
    self->monsterinfo.nextframe = FRAME_pain105;
}

static const mframe_t parasite_frames_pain1[] = {
    { ai_move, 0, NULL, FRAME_stand01 },
    { ai_move },
    { ai_move, 0, parasite_skipframe },
    { ai_move, 0, monster_footstep },
    { ai_move },
    { ai_move },
    { ai_move, 6, monster_footstep },
    { ai_move, 16 },
    { ai_move, -6, monster_footstep },
    { ai_move, -7 },
    { ai_move }
};
const mmove_t MMOVE_T(parasite_move_pain1) = { FRAME_pain101, FRAME_pain111, parasite_frames_pain1, parasite_start_run };

void PAIN(parasite_pain)(edict_t *self, edict_t *other, float kick, int damage, mod_t mod)
{
    if (level.time < self->pain_debounce_time)
        return;

    if (self->proboscus && self->proboscus->style != 2)
        proboscis_retract(self->proboscus);

    self->pain_debounce_time = level.time + SEC(3);

    if (brandom())
        gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
    else
        gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

    if (!M_ShouldReactToPain(self, mod))
        return; // no pain anims in nightmare

    M_SetAnimation(self, &parasite_move_pain1);
}

void MONSTERINFO_SETSKIN(parasite_setskin)(edict_t *self)
{
    if (self->health < (self->max_health / 2))
        self->s.skinnum = 1;
    else
        self->s.skinnum = 0;
}

static void parasite_precache(void)
{
    sound_pain1 = gi.soundindex("parasite/parpain1.wav");
    sound_pain2 = gi.soundindex("parasite/parpain2.wav");
    sound_die = gi.soundindex("parasite/pardeth1.wav");
    sound_launch = gi.soundindex("parasite/paratck1.wav");
    sound_impact = gi.soundindex("parasite/paratck2.wav");
    sound_suck = gi.soundindex("parasite/paratck3.wav");
    sound_reelin = gi.soundindex("parasite/paratck4.wav");
    sound_sight = gi.soundindex("parasite/parsght1.wav");
    sound_tap = gi.soundindex("parasite/paridle1.wav");
    sound_scratch = gi.soundindex("parasite/paridle2.wav");
    sound_search = gi.soundindex("parasite/parsrch1.wav");
}

#define SPAWNFLAG_PARASITE_NOJUMPING    8

/*QUAKED monster_parasite (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight NoJumping
 */
void SP_monster_parasite(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    G_AddPrecache(parasite_precache);

    gi.modelindex("models/monsters/parasite/tip/tris.md2");
    gi.modelindex("models/monsters/parasite/segment/tris.md2");

    self->s.modelindex = gi.modelindex("models/monsters/parasite/tris.md2");

    PrecacheGibs(parasite_gibs);

    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, 24);
    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;

    self->health = 175 * st.health_multiplier;
    self->gib_health = -50;
    self->mass = 250;

    self->pain = parasite_pain;
    self->die = parasite_die;

    self->monsterinfo.stand = parasite_stand;
    self->monsterinfo.walk = parasite_start_walk;
    self->monsterinfo.run = parasite_start_run;
    self->monsterinfo.attack = parasite_attack;
    self->monsterinfo.sight = parasite_sight;
    self->monsterinfo.idle = parasite_idle;
    self->monsterinfo.blocked = parasite_blocked; // PGM
    self->monsterinfo.setskin = parasite_setskin;

    gi.linkentity(self);

    M_SetAnimation(self, &parasite_move_stand);
    self->monsterinfo.scale = MODEL_SCALE;
    self->yaw_speed = 30;
    self->monsterinfo.can_jump = !(self->spawnflags & SPAWNFLAG_PARASITE_NOJUMPING);
    self->monsterinfo.drop_height = 256;
    self->monsterinfo.jump_height = 68;

    walkmonster_start(self);
}
