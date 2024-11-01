// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

GUARDIAN

==============================================================================
*/

#include "g_local.h"
#include "m_guardian.h"

static int sound_step;
static int sound_charge;
static int sound_spin_loop;
static int sound_laser;
static int sound_pew;
static int sound_sight;
static int sound_pain1;
static int sound_pain2;
static int sound_death;

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

#define SPAWNFLAG_GUARDIAN_SLEEPY 8

/*
=============
ai_sleep

honk shoo honk shoo
==============
*/
static void ai_sleep(edict_t *self, float dist)
{
}

static const mframe_t guardian_frames_sleep[] = {
    { ai_sleep },
    { ai_sleep },
    { ai_sleep },
    { ai_sleep },
    { ai_sleep },
    { ai_sleep },
    { ai_sleep },
    { ai_sleep },
    { ai_sleep },
    { ai_sleep },
    { ai_sleep },
    { ai_sleep },
    { ai_sleep },
    { ai_sleep }
};
const mmove_t MMOVE_T(guardian_move_sleep) = { FRAME_sleep1, FRAME_sleep14, guardian_frames_sleep, NULL };

void MONSTERINFO_STAND(guardian_stand)(edict_t *self)
{
    if (self->spawnflags & SPAWNFLAG_GUARDIAN_SLEEPY)
        M_SetAnimation(self, &guardian_move_sleep);
    else
        M_SetAnimation(self, &guardian_move_stand);
}

void guardian_run(edict_t *self);

static const mframe_t guardian_frames_wake[] = {
    { ai_sleep },
    { ai_sleep },
    { ai_sleep },
    { ai_sleep },
    { ai_sleep }
};
const mmove_t MMOVE_T(guardian_move_wake) = { FRAME_wake1, FRAME_wake5, guardian_frames_wake, guardian_run };

void USE(guardian_use)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->spawnflags &= ~SPAWNFLAG_GUARDIAN_SLEEPY;
    M_SetAnimation(self, &guardian_move_wake);
    self->use = monster_use;
    gi.sound(self, CHAN_BODY, sound_sight, 1, 0.1f, 0);
}

//
// walk
//

static void guardian_footstep(edict_t *self)
{
    gi.sound(self, CHAN_BODY, sound_step, 1, 0.1f, 0);
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

    if (brandom())
        gi.sound(self, CHAN_BODY, sound_pain1, 1, 0.1f, 0);
    else
        gi.sound(self, CHAN_BODY, sound_pain2, 1, 0.1f, 0);

    if (!M_ShouldReactToPain(self, mod))
        return; // no pain anims in nightmare

    M_SetAnimation(self, &guardian_move_pain1);
    self->monsterinfo.weapon_sound = 0;
    self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
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

static void guardian_atk1_charge(edict_t *self)
{
    self->monsterinfo.weapon_sound = sound_spin_loop;
    gi.sound(self, CHAN_WEAPON, sound_charge, 1, ATTN_NORM, 0);
}

static void guardian_fire_blaster(edict_t *self)
{
    vec3_t forward, right, up;
    vec3_t start;
    monster_muzzleflash_id_t id = MZ2_GUARDIAN_BLASTER;
    float offset;

    if (!self->enemy || !self->enemy->inuse) {
        self->monsterinfo.nextframe = FRAME_atk1_spin13;
        return;
    }

    AngleVectors(self->s.angles, forward, right, up);
    M_ProjectFlashSource(self, monster_flash_offset[id], forward, right, start);
    PredictAim(self, self->enemy, start, 1000, false, crandom() * 0.1f, forward, NULL);
    offset = crandom() * 0.02f;
    VectorMA(forward, offset, right, forward);
    offset = crandom() * 0.02f;
    VectorMA(forward, offset, up, forward);
    VectorNormalize(forward);

    edict_t *bolt = monster_fire_blaster(self, start, forward, 3, 1100, id, (self->s.frame % 4) ? EF_NONE : EF_HYPERBLASTER);
    bolt->x.scale = 2.0f;

    if (self->enemy && self->enemy->health > 0 &&
        self->s.frame == FRAME_atk1_spin12 && self->timestamp > level.time && visible(self, self->enemy))
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

static const vec3_t laser_positions[] = {
    { 125, -70, 60 },
    { 112, -62, 60 }
};

void PRETHINK(guardian_fire_update)(edict_t *laser)
{
    if (!(laser->spawnflags & SPAWNFLAG_DABEAM_SPAWNED)) {
        edict_t *self = laser->owner;
        bool sec = laser->spawnflags & SPAWNFLAG_DABEAM_SECONDARY;

        vec3_t forward, right, target;
        vec3_t start;

        AngleVectors(self->s.angles, forward, right, NULL);
        M_ProjectFlashSource(self, laser_positions[sec], forward, right, start);
        PredictAim(self, self->enemy, start, 0, false, 0.3f, forward, target);

        VectorCopy(start, laser->s.origin);
        forward[0] += crandom() * 0.02f;
        forward[1] += crandom() * 0.02f;
        VectorNormalize2(forward, laser->movedir);
        gi.linkentity(laser);
    }

    dabeam_update(laser, false);
}

static void guardian_laser_fire(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_laser, 1, ATTN_NORM, 0);
    monster_fire_dabeam(self, 15, self->s.frame & 1, guardian_fire_update);
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
    if (!fire_hit(self, (vec3_t) { 160, 0, -80 }, 85, 700))
        self->monsterinfo.melee_debounce_time = level.time + SEC(3.5f);
}

static const mframe_t guardian_frames_kick[] = {
    { ai_charge, 12 },
    { ai_charge, 18, guardian_footstep },
    { ai_charge, 11 },
    { ai_charge, 9 },
    { ai_charge, 8 },
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


// RAFAEL
/*
fire_heat
*/

static void heat_guardian_get_dist_vec(edict_t *heat, edict_t *target, float dist_to_target, vec3_t vec)
{
    VectorCopy(target->s.origin, vec);
    vec[2] += target->mins[2];

    float f = Q_clipf(dist_to_target / 500.0f, 0, 1) * 0.5f;
    VectorMA(vec, f, target->velocity, vec);

    VectorSubtract(vec, heat->s.origin, vec);
    VectorNormalize(vec);
}

void THINK(heat_guardian_think)(edict_t *self)
{
    edict_t *acquire = NULL;
    float    olddot = 1;

    if (self->timestamp < level.time) {
        vec3_t fwd;
        AngleVectors(self->s.angles, fwd, NULL, NULL);

        if (self->oldenemy) {
            self->enemy = self->oldenemy;
            self->oldenemy = NULL;
        }

        if (self->enemy) {
            acquire = self->enemy;

            if (acquire->health <= 0 || !visible(self, acquire)) {
                self->enemy = acquire = NULL;
            } else {
                float dist_to_target = Distance(self->s.origin, acquire->s.origin);
                heat_guardian_get_dist_vec(self, acquire, dist_to_target, self->pos1);
            }
        }

        if (!acquire) {
            // acquire new target
            edict_t *target = NULL;

            while ((target = findradius(target, self->s.origin, 1024)) != NULL) {
                if (self->owner == target)
                    continue;
                if (!target->client)
                    continue;
                if (target->health <= 0)
                    continue;
                if (!visible(self, target))
                    continue;

                float dist_to_target = Distance(self->s.origin, target->s.origin);
                vec3_t vec;
                heat_guardian_get_dist_vec(self, target, dist_to_target, vec);

                float dot = DotProduct(vec, fwd);

                // targets that require us to turn less are preferred
                if (dot >= olddot)
                    continue;

                if (!acquire || dot < olddot) {
                    acquire = target;
                    olddot = dot;
                    VectorCopy(vec, self->pos1);
                }
            }
        }
    }

    vec3_t preferred_dir;
    VectorCopy(self->pos1, preferred_dir);

    if (acquire) {
        if (self->enemy != acquire) {
            gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/railgr1a.wav"), 1, 0.25f, 0);
            self->enemy = acquire;
        }
    } else
        self->enemy = NULL;

    float t = self->accel;

    if (self->enemy)
        t *= 0.85f;

    //float d = DotProduct(self->movedir, preferred_dir);

    slerp(self->movedir, preferred_dir, t, self->movedir);
    VectorNormalize(self->movedir);
    vectoangles(self->movedir, self->s.angles);

    if (self->speed < self->yaw_speed)
        self->speed += self->yaw_speed * FRAME_TIME_SEC;

    VectorScale(self->movedir, self->speed, self->velocity);
    self->nextthink = level.time + FRAME_TIME;
}

void DIE(guardian_heat_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    BecomeExplosion1(self);
}

// RAFAEL
static void fire_guardian_heat(edict_t *self, const vec3_t start, const vec3_t dir, const vec3_t rest_dir, int damage, int speed, float damage_radius, int radius_damage, float turn_fraction)
{
    edict_t *heat;

    heat = G_Spawn();
    VectorCopy(start, heat->s.origin);
    VectorCopy(dir, heat->movedir);
    vectoangles(dir, heat->s.angles);
    VectorScale(dir, speed, heat->velocity);
    heat->movetype = MOVETYPE_FLYMISSILE;
    heat->clipmask = MASK_PROJECTILE;
    heat->flags |= FL_DAMAGEABLE;
    heat->solid = SOLID_BBOX;
    heat->s.effects |= EF_ROCKET;
    heat->s.modelindex = gi.modelindex("models/objects/rocket/tris.md2");
    heat->x.scale = 1.5f;
    heat->owner = self;
    heat->touch = rocket_touch;
    heat->speed = speed / 2;
    heat->yaw_speed = speed * 2;
    heat->accel = turn_fraction;
    VectorCopy(rest_dir, heat->pos1);
    VectorSet(heat->mins, -5, -5, -5);
    VectorSet(heat->maxs, 5, 5, 5);
    heat->health = 15;
    heat->takedamage = true;
    heat->die = guardian_heat_die;

    heat->nextthink = level.time + SEC(0.2f);
    heat->think = heat_guardian_think;

    heat->dmg = damage;
    heat->radius_dmg = radius_damage;
    heat->dmg_radius = damage_radius;
    heat->s.sound = gi.soundindex("weapons/rockfly.wav");

    if (visible(heat, self->enemy)) {
        heat->oldenemy = self->enemy;
        heat->timestamp = level.time + SEC(0.6f);
        gi.sound(heat, CHAN_WEAPON, gi.soundindex("weapons/railgr1a.wav"), 1, 0.25f, 0);
    }

    gi.linkentity(heat);
}
// RAFAEL

static void guardian_fire_rocket(edict_t *self, float offset)
{
    vec3_t forward, right, up;
    vec3_t start;

    AngleVectors(self->s.angles, forward, right, up);
    VectorMA(self->s.origin, -8, forward, start);
    VectorMA(start, offset, right, start);
    VectorMA(start, 50, up, start);

    AngleVectors((vec3_t) { 20.0f, self->s.angles[1] - offset, 0 }, forward, NULL, NULL);

    fire_guardian_heat(self, start, up, forward, 20, 250, 150, 35, 0.085f);
    gi.sound(self, CHAN_WEAPON, sound_pew, 1.f, 0.5f, 0.0f);
}

static void guardian_fire_rocket_l(edict_t *self)
{
    guardian_fire_rocket(self, -14.0f);
}

static void guardian_fire_rocket_r(edict_t *self)
{
    guardian_fire_rocket(self, 14.0f);
}

static void guardian_blind_fire_check(edict_t *self)
{
    if (self->monsterinfo.aiflags & AI_MANUAL_STEERING) {
        vec3_t aim;
        VectorSubtract(self->monsterinfo.blind_fire_target, self->s.origin, aim);
        self->ideal_yaw = vectoyaw(aim);
    }
}

static const mframe_t guardian_frames_rocket[] = {
    { ai_charge, 0, guardian_blind_fire_check },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, guardian_fire_rocket_l },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, guardian_fire_rocket_r },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, guardian_fire_rocket_l },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, guardian_fire_rocket_r },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge }
};
const mmove_t MMOVE_T(guardian_move_rocket) = { FRAME_turnl_1, FRAME_turnr_11, guardian_frames_rocket, guardian_run };

void MONSTERINFO_ATTACK(guardian_attack)(edict_t *self)
{
    if (!self->enemy || !self->enemy->inuse)
        return;

    if (self->monsterinfo.attack_state == AS_BLIND) {
        float chance;

        // setup shot probabilities
        if (self->count == 0)
            chance = 1.0;
        else if (self->count <= 2)
            chance = 0.4f;
        else
            chance = 0.1f;

        float r = frandom();

        self->monsterinfo.blind_fire_delay += random_time_sec(8.5f, 15.5f);

        // don't shoot at the origin
        if (VectorEmpty(self->monsterinfo.blind_fire_target))
            return;

        // shot the rockets way too soon
        if (self->count) {
            self->count--;
            return;
        }

        // don't shoot if the dice say not to
        if (r > chance)
            return;

        // turn on manual steering to signal both manual steering and blindfire
        self->monsterinfo.aiflags |= AI_MANUAL_STEERING;
        M_SetAnimation(self, &guardian_move_rocket);
        self->monsterinfo.attack_finished = level.time + random_time_sec(0, 3);
        return;
    }

    if (self->bad_area) {
        M_SetAnimation(self, &guardian_move_atk1_in);
        return;
    }

    float r = range_to(self, self->enemy);
    bool changedAttack = false;

    if (self->monsterinfo.melee_debounce_time < level.time && r < 160) {
        M_SetAnimation(self, &guardian_move_kick);
        changedAttack = true;
        self->style = 0;
    } else if (r > 300 && frandom() < (max(r, 1000) / 1200)) {
        if (self->count <= 0 && frandom() < 0.25f) {
            M_SetAnimation(self, &guardian_move_rocket);
            self->count = 6;
            self->style = 0;
            return;
        }
        if (M_CheckClearShot(self, laser_positions[0]) && self->style != 1) {
            M_SetAnimation(self, &guardian_move_atk2_in);
            self->style = 1;
            changedAttack = true;

            if (skill->integer >= 2)
                self->monsterinfo.nextframe = FRAME_atk2_in8;
        } else if (M_CheckClearShot(self, monster_flash_offset[MZ2_GUARDIAN_BLASTER])) {
            M_SetAnimation(self, &guardian_move_atk1_in);
            changedAttack = true;
            self->style = 0;
        }
    } else if (M_CheckClearShot(self, monster_flash_offset[MZ2_GUARDIAN_BLASTER])) {
        M_SetAnimation(self, &guardian_move_atk1_in);
        changedAttack = true;
        self->style = 0;
    }

    if (changedAttack && self->count)
        self->count--;
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
    self->monsterinfo.weapon_sound = 0;
    self->deadflag = true;
    self->takedamage = true;

    M_SetAnimation(self, &guardian_move_death);
    gi.sound(self, CHAN_BODY, sound_death, 1, 0.1f, 0);
}

static void GuardianPowerArmor(edict_t *self)
{
    self->monsterinfo.power_armor_type = IT_ITEM_POWER_SHIELD;
    // I don't like this, but it works
    if (self->monsterinfo.power_armor_power <= 0)
        self->monsterinfo.power_armor_power += 200 * skill->integer;
}

static void GuardianRespondPowerup(edict_t *self, edict_t *other)
{
    if ((other->s.effects & (EF_QUAD | EF_DOUBLE | EF_PENT)) || (other->x.morefx & EFX_DUALFIRE))
        GuardianPowerArmor(self);
}

static void GuardianPowerups(edict_t *self)
{
    edict_t *ent;

    if (!coop->integer) {
        GuardianRespondPowerup(self, self->enemy);
    } else {
        for (int player = 1; player <= game.maxclients; player++) {
            ent = &g_edicts[player];
            if (!ent->inuse)
                continue;
            if (!ent->client)
                continue;
            GuardianRespondPowerup(self, ent);
        }
    }
}

bool MONSTERINFO_CHECKATTACK(Guardian_CheckAttack)(edict_t *self)
{
    if (!self->enemy)
        return false;

    GuardianPowerups(self);

    return M_CheckAttack_Base(self, 0.4f, 0.8f, 0.6f, 0.7f, 0.85f, 0);
}

void MONSTERINFO_SETSKIN(guardian_setskin)(edict_t *self)
{
    if (self->health < (self->max_health / 2))
        self->s.skinnum = 1;
    else
        self->s.skinnum = 0;
}

static void guadian_precache(void)
{
    sound_step = gi.soundindex("zortemp/step.wav");
    sound_charge = gi.soundindex("weapons/hyprbu1a.wav");
    sound_spin_loop = gi.soundindex("weapons/hyprbl1a.wav");
    sound_laser = gi.soundindex("weapons/laser2.wav");
    sound_pew = gi.soundindex("makron/blaster.wav");
    sound_sight = gi.soundindex("guardian/sight.wav");
    sound_pain1 = gi.soundindex ("guardian/pain1.wav");
    sound_pain2 = gi.soundindex("guardian/pain2.wav");
    sound_death = gi.soundindex("guardian/death.wav");
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
    VectorSet(self->mins, -78, -78, -66);
    VectorSet(self->maxs, 78, 78, 76);
    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;

    self->health = 2500 * st.health_multiplier;
    self->gib_health = -200;

    if (skill->integer >= 3 || coop->integer)
        self->health *= 2;
    else if (skill->integer == 2)
        self->health *= 1.5f;

    self->monsterinfo.scale = MODEL_SCALE;

    self->mass = 1650;

    self->pain = guardian_pain;
    self->die = guardian_die;
    self->monsterinfo.stand = guardian_stand;
    self->monsterinfo.walk = guardian_walk;
    self->monsterinfo.run = guardian_run;
    self->monsterinfo.attack = guardian_attack;
    self->monsterinfo.checkattack = Guardian_CheckAttack;
    self->monsterinfo.setskin = guardian_setskin;

    self->monsterinfo.aiflags |= AI_IGNORE_SHOTS;
    self->monsterinfo.blindfire = true;

    gi.linkentity(self);

    guardian_stand(self);

    walkmonster_start(self);

    self->use = guardian_use;
}
