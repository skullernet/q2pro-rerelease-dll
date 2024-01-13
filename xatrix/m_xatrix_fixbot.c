// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
    fixbot.c
*/

#include "../g_local.h"
#include "m_xatrix_fixbot.h"

static int sound_pain1;
static int sound_die;
static int sound_weld1;
static int sound_weld2;
static int sound_weld3;

void fixbot_run(edict_t *self);
void fixbot_attack(edict_t *self);
void fixbot_stand(edict_t *self);
static void fixbot_fire_blaster(edict_t *self);
static void fixbot_fire_welder(edict_t *self);

static void use_scanner(edict_t *self);
static void change_to_roam(edict_t *self);
static void fly_vertical(edict_t *self);

static void roam_goal(edict_t *self);

const mmove_t fixbot_move_forward;
const mmove_t fixbot_move_stand;
const mmove_t fixbot_move_stand2;
const mmove_t fixbot_move_roamgoal;

const mmove_t fixbot_move_weld_start;
const mmove_t fixbot_move_weld;
const mmove_t fixbot_move_weld_end;
const mmove_t fixbot_move_takeoff;
const mmove_t fixbot_move_landing;
const mmove_t fixbot_move_turn;

// [Paril-KEX] clean up bot goals if we get interrupted
void THINK(bot_goal_check)(edict_t *self)
{
    if (!self->owner || !self->owner->inuse || self->owner->goalentity != self) {
        G_FreeEdict(self);
        return;
    }

    self->nextthink = level.time + FRAME_TIME;
}

static edict_t *fixbot_FindDeadMonster(edict_t *self)
{
    edict_t *ent = NULL;
    edict_t *best = NULL;

    while ((ent = findradius(ent, self->s.origin, 1024)) != NULL) {
        if (ent == self)
            continue;
        if (!(ent->svflags & SVF_MONSTER))
            continue;
        if (ent->monsterinfo.aiflags & AI_GOOD_GUY)
            continue;
        // check to make sure we haven't bailed on this guy already
        if ((ent->monsterinfo.badMedic1 == self) || (ent->monsterinfo.badMedic2 == self))
            continue;
        if (ent->monsterinfo.healer)
            // FIXME - this is correcting a bug that is somewhere else
            // if the healer is a monster, and it's in medic mode .. continue .. otherwise
            //   we will override the healer, if it passes all the other tests
            if ((ent->monsterinfo.healer->inuse) && (ent->monsterinfo.healer->health > 0) &&
                (ent->monsterinfo.healer->svflags & SVF_MONSTER) && (ent->monsterinfo.healer->monsterinfo.aiflags & AI_MEDIC))
                continue;
        if (ent->health > 0)
            continue;
        if ((ent->nextthink) && (ent->think != monster_dead_think))
            continue;
        if (!visible(self, ent))
            continue;
        if (!best) {
            best = ent;
            continue;
        }
        if (ent->max_health <= best->max_health)
            continue;
        best = ent;
    }

    return best;
}

static void fixbot_set_fly_parameters(edict_t *self, bool heal, bool weld)
{
    self->monsterinfo.fly_position_time = 0;
    self->monsterinfo.fly_acceleration = 5;
    self->monsterinfo.fly_speed = 110;
    self->monsterinfo.fly_buzzard = false;

    if (heal) {
        self->monsterinfo.fly_min_distance = 100;
        self->monsterinfo.fly_max_distance = 100;
        self->monsterinfo.fly_thrusters = true;
    } else if (weld) {
        self->monsterinfo.fly_min_distance = 24;
        self->monsterinfo.fly_max_distance = 24;
    } else {
        // timid bot
        self->monsterinfo.fly_min_distance = 300;
        self->monsterinfo.fly_max_distance = 500;
    }
}

static bool fixbot_search(edict_t *self)
{
    edict_t *ent;

    if (!self->enemy) {
        ent = fixbot_FindDeadMonster(self);
        if (ent) {
            self->oldenemy = self->enemy;
            self->enemy = ent;
            self->enemy->monsterinfo.healer = self;
            self->monsterinfo.aiflags |= AI_MEDIC;
            FoundTarget(self);
            fixbot_set_fly_parameters(self, true, false);
            return true;
        }
    }
    return false;
}

static void landing_goal(edict_t *self)
{
    trace_t  tr;
    vec3_t   forward, right, up;
    vec3_t   end;
    edict_t *ent;

    ent = G_Spawn();
    ent->classname = "bot_goal";
    ent->solid = SOLID_BBOX;
    ent->owner = self;
    ent->think = bot_goal_check;
    gi.linkentity(ent);

    VectorSet(ent->mins, -32, -32, -24);
    VectorSet(ent->maxs, 32, 32, 24);

    AngleVectors(self->s.angles, forward, right, up);
    VectorMA(self->s.origin, 32, forward, end); // FIXME
    VectorMA(self->s.origin, -8096, up, end);

    tr = gi.trace(self->s.origin, ent->mins, ent->maxs, end, self, MASK_MONSTERSOLID);

    VectorCopy(tr.endpos, ent->s.origin);

    self->goalentity = self->enemy = ent;
    M_SetAnimation(self, &fixbot_move_landing);
}

static void takeoff_goal(edict_t *self)
{
    trace_t  tr;
    vec3_t   forward, right, up;
    vec3_t   end;
    edict_t *ent;

    ent = G_Spawn();
    ent->classname = "bot_goal";
    ent->solid = SOLID_BBOX;
    ent->owner = self;
    ent->think = bot_goal_check;
    gi.linkentity(ent);

    VectorSet(ent->mins, -32, -32, -24);
    VectorSet(ent->maxs, 32, 32, 24);

    AngleVectors(self->s.angles, forward, right, up);
    VectorMA(self->s.origin, 32, forward, end); // FIXME
    VectorMA(self->s.origin, 128, up, end);

    tr = gi.trace(self->s.origin, ent->mins, ent->maxs, end, self, MASK_MONSTERSOLID);

    VectorCopy(tr.endpos, ent->s.origin);

    self->goalentity = self->enemy = ent;
    M_SetAnimation(self, &fixbot_move_takeoff);
}

static void change_to_roam(edict_t *self)
{
    if (fixbot_search(self))
        return;

    M_SetAnimation(self, &fixbot_move_roamgoal);

    if (self->spawnflags & SPAWNFLAG_FIXBOT_LANDING) {
        landing_goal(self);
        M_SetAnimation(self, &fixbot_move_landing);
        self->spawnflags &= ~SPAWNFLAG_FIXBOT_LANDING;
        self->spawnflags = SPAWNFLAG_FIXBOT_WORKING;
    }
    if (self->spawnflags & SPAWNFLAG_FIXBOT_TAKEOFF) {
        takeoff_goal(self);
        M_SetAnimation(self, &fixbot_move_takeoff);
        self->spawnflags &= ~SPAWNFLAG_FIXBOT_TAKEOFF;
        self->spawnflags = SPAWNFLAG_FIXBOT_WORKING;
    }
    if (self->spawnflags & SPAWNFLAG_FIXBOT_FIXIT) {
        M_SetAnimation(self, &fixbot_move_roamgoal);
        self->spawnflags &= ~SPAWNFLAG_FIXBOT_FIXIT;
        self->spawnflags = SPAWNFLAG_FIXBOT_WORKING;
    }
    if (!self->spawnflags)
        M_SetAnimation(self, &fixbot_move_stand2);
}

static void roam_goal(edict_t *self)
{
    trace_t  tr;
    vec3_t   forward, right, up;
    vec3_t   end;
    edict_t *ent;
    vec3_t   dang;
    float    len, oldlen;
    int      i;
    vec3_t   whichvec = { 0 };

    ent = G_Spawn();
    ent->classname = "bot_goal";
    ent->solid = SOLID_BBOX;
    ent->owner = self;
    ent->think = bot_goal_check;
    ent->nextthink = level.time + FRAME_TIME;
    gi.linkentity(ent);

    oldlen = 0;

    for (i = 0; i < 12; i++) {
        VectorCopy(self->s.angles, dang);

        if (i < 6)
            dang[YAW] += 30 * i;
        else
            dang[YAW] -= 30 * (i - 6);

        AngleVectors(dang, forward, right, up);
        VectorMA(self->s.origin, 8192, forward, end);

        tr = gi.trace(self->s.origin, NULL, NULL, end, self, MASK_PROJECTILE);

        len = Distance(self->s.origin, tr.endpos);
        if (len > oldlen) {
            oldlen = len;
            VectorCopy(tr.endpos, whichvec);
        }
    }

    VectorCopy(whichvec, ent->s.origin);
    self->goalentity = self->enemy = ent;

    M_SetAnimation(self, &fixbot_move_turn);
}

static void use_scanner(edict_t *self)
{
    edict_t *ent = NULL;
    float   radius = 1024;

    while ((ent = findradius(ent, self->s.origin, radius)) != NULL) {
        if (ent->health < 100)
            continue;
        if (strcmp(ent->classname, "object_repair") != 0)
            continue;
        if (!visible(self, ent))
            continue;

        // remove the old one
        if (strcmp(self->goalentity->classname, "bot_goal") == 0) {
            self->goalentity->nextthink = level.time + FRAME_TIME;
            self->goalentity->think = G_FreeEdict;
        }

        self->goalentity = self->enemy = ent;

        fixbot_set_fly_parameters(self, false, true);

        if (Distance(self->s.origin, self->goalentity->s.origin) < 32)
            M_SetAnimation(self, &fixbot_move_weld_start);
        return;
    }

    if (!self->goalentity) {
        M_SetAnimation(self, &fixbot_move_stand);
        return;
    }

    if (Distance(self->s.origin, self->goalentity->s.origin) < 32) {
        if (strcmp(self->goalentity->classname, "object_repair") == 0) {
            M_SetAnimation(self, &fixbot_move_weld_start);
        } else {
            self->goalentity->nextthink = level.time + FRAME_TIME;
            self->goalentity->think = G_FreeEdict;
            self->goalentity = self->enemy = NULL;
            M_SetAnimation(self, &fixbot_move_stand);
        }
        return;
    }

    /*
      bot is stuck get new goalentity
    */
    if (VectorCompare(self->s.origin, self->s.old_origin)) {
        if (strcmp(self->goalentity->classname, "object_repair") == 0) {
            M_SetAnimation(self, &fixbot_move_stand);
        } else {
            self->goalentity->nextthink = level.time + FRAME_TIME;
            self->goalentity->think = G_FreeEdict;
            self->goalentity = self->enemy = NULL;
            M_SetAnimation(self, &fixbot_move_stand);
        }
    }
}

/*
    when the bot has found a landing pad
    it will proceed to its goalentity
    just above the landing pad and
    decend translated along the z the current
    frames are at 10fps
*/
static void blastoff(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread)
{
    trace_t    tr;
    vec3_t     dir;
    vec3_t     forward, right, up;
    vec3_t     end;
    float      r;
    float      u;
    vec3_t     water_start;
    bool       water = false;
    contents_t content_mask = MASK_PROJECTILE | MASK_WATER;

    hspread += (self->s.frame - FRAME_takeoff_01);
    vspread += (self->s.frame - FRAME_takeoff_01);

    tr = gi.trace(self->s.origin, NULL, NULL, start, self, MASK_PROJECTILE);
    if (!(tr.fraction < 1.0f)) {
        vectoangles(aimdir, dir);
        AngleVectors(dir, forward, right, up);

        r = crandom() * hspread;
        u = crandom() * vspread;
        VectorMA(start, 8192, forward, end);
        VectorMA(end, r, right, end);
        VectorMA(end, u, up, end);

        if (gi.pointcontents(start) & MASK_WATER) {
            water = true;
            VectorCopy(start, water_start);
            content_mask &= ~MASK_WATER;
        }

        tr = gi.trace(start, NULL, NULL, end, self, content_mask);

        // see if we hit water
        if (tr.contents & MASK_WATER) {
            int color;

            water = true;
            VectorCopy(tr.endpos, water_start);

            if (!VectorCompare(start, tr.endpos)) {
                if (tr.contents & CONTENTS_WATER) {
                    if (strcmp(tr.surface->name, "*brwater") == 0)
                        color = SPLASH_BROWN_WATER;
                    else
                        color = SPLASH_BLUE_WATER;
                } else if (tr.contents & CONTENTS_SLIME)
                    color = SPLASH_SLIME;
                else if (tr.contents & CONTENTS_LAVA)
                    color = SPLASH_LAVA;
                else
                    color = SPLASH_UNKNOWN;

                if (color != SPLASH_UNKNOWN) {
                    gi.WriteByte(svc_temp_entity);
                    gi.WriteByte(TE_SPLASH);
                    gi.WriteByte(8);
                    gi.WritePosition(tr.endpos);
                    gi.WriteDir(tr.plane.normal);
                    gi.WriteByte(color);
                    gi.multicast(tr.endpos, MULTICAST_PVS);
                }

                // change bullet's course when it enters water
                VectorSubtract(end, start, dir);
                vectoangles(dir, dir);
                AngleVectors(dir, forward, right, up);
                r = crandom() * hspread * 2;
                u = crandom() * vspread * 2;
                VectorMA(water_start, 8192, forward, end);
                VectorMA(end, r, right, end);
                VectorMA(end, u, up, end);
            }

            // re-trace ignoring water this time
            tr = gi.trace(water_start, NULL, NULL, end, self, MASK_PROJECTILE);
        }
    }

    // send gun puff / flash
    if (!((tr.surface) && (tr.surface->flags & SURF_SKY))) {
        if (tr.fraction < 1.0f) {
            if (tr.ent->takedamage) {
                T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, DAMAGE_BULLET, (mod_t) { MOD_BLASTOFF });
            } else {
                if (!(tr.surface->flags & SURF_SKY)) {
                    gi.WriteByte(svc_temp_entity);
                    gi.WriteByte(te_impact);
                    gi.WritePosition(tr.endpos);
                    gi.WriteDir(tr.plane.normal);
                    gi.multicast(tr.endpos, MULTICAST_PVS);

                    if (self->client)
                        PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
                }
            }
        }
    }

    // if went through water, determine where the end and make a bubble trail
    if (water) {
        vec3_t pos;

        VectorSubtract(tr.endpos, water_start, dir);
        VectorNormalize(dir);
        VectorMA(tr.endpos, -2, dir, pos);
        if (gi.pointcontents(pos) & MASK_WATER)
            VectorCopy(pos, tr.endpos);
        else
            tr = gi.trace(pos, NULL, NULL, water_start, tr.ent, MASK_WATER);

        VectorAvg(water_start, tr.endpos, pos);

        gi.WriteByte(svc_temp_entity);
        gi.WriteByte(TE_BUBBLETRAIL);
        gi.WritePosition(water_start);
        gi.WritePosition(tr.endpos);
        gi.multicast(pos, MULTICAST_PVS);
    }
}

static void fly_vertical(edict_t *self)
{
    int    i;
    vec3_t v;
    vec3_t forward, right, up;
    vec3_t start;
    vec3_t tempvec;

    VectorSubtract(self->goalentity->s.origin, self->s.origin, v);
    self->ideal_yaw = vectoyaw(v);
    M_ChangeYaw(self);

    if (self->s.frame == FRAME_landing_58 || self->s.frame == FRAME_takeoff_16) {
        self->goalentity->nextthink = level.time + FRAME_TIME;
        self->goalentity->think = G_FreeEdict;
        M_SetAnimation(self, &fixbot_move_stand);
        self->goalentity = self->enemy = NULL;
    }

    // kick up some particles
    VectorCopy(self->s.angles, tempvec);
    tempvec[PITCH] += 90;

    AngleVectors(tempvec, forward, right, up);
    VectorCopy(self->s.origin, start);

    for (i = 0; i < 10; i++)
        blastoff(self, start, forward, 2, 1, TE_SHOTGUN, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD);

    // needs sound
}

static void fly_vertical2(edict_t *self)
{
    vec3_t v;
    float  len;

    VectorSubtract(self->goalentity->s.origin, self->s.origin, v);
    len = VectorLength(v);
    self->ideal_yaw = vectoyaw(v);
    M_ChangeYaw(self);

    if (len < 32) {
        self->goalentity->nextthink = level.time + FRAME_TIME;
        self->goalentity->think = G_FreeEdict;
        M_SetAnimation(self, &fixbot_move_stand);
        self->goalentity = self->enemy = NULL;
    }

    // needs sound
}

static const mframe_t fixbot_frames_landing[] = {
    { ai_move },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },

    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },

    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },

    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },

    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },

    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 },
    { ai_move, 0, fly_vertical2 }
};
const mmove_t MMOVE_T(fixbot_move_landing) = { FRAME_landing_01, FRAME_landing_58, fixbot_frames_landing, NULL };

/*
    generic ambient stand
*/
static const mframe_t fixbot_frames_stand[] = {
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
    { ai_move, 0, change_to_roam }

};
const mmove_t MMOVE_T(fixbot_move_stand) = { FRAME_ambient_01, FRAME_ambient_19, fixbot_frames_stand, NULL };

static const mframe_t fixbot_frames_stand2[] = {
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
    { ai_stand, 0, change_to_roam }
};
const mmove_t MMOVE_T(fixbot_move_stand2) = { FRAME_ambient_01, FRAME_ambient_19, fixbot_frames_stand2, NULL };

#if 0
/*
    will need the pickup offset for the front pincers
    object will need to stop forward of the object
    and take the object with it ( this may require a variant of liftoff and landing )
*/
static const mframe_t fixbot_frames_pickup[] = {
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
    { ai_move },
    { ai_move },
    { ai_move }

};
const mmove_t MMOVE_T(fixbot_move_pickup) = { FRAME_pickup_01, FRAME_pickup_27, fixbot_frames_pickup, NULL };
#endif

/*
    generic frame to move bot
*/
static const mframe_t fixbot_frames_roamgoal[] = {
    { ai_move, 0, roam_goal }
};
const mmove_t MMOVE_T(fixbot_move_roamgoal) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_roamgoal, NULL };

static void ai_facing(edict_t *self, float dist)
{
    if (!self->goalentity) {
        fixbot_stand(self);
        return;
    }

    if (infront(self, self->goalentity))
        M_SetAnimation(self, &fixbot_move_forward);
    else {
        vec3_t v;
        VectorSubtract(self->goalentity->s.origin, self->s.origin, v);
        self->ideal_yaw = vectoyaw(v);
        M_ChangeYaw(self);
    }
};

static const mframe_t fixbot_frames_turn[] = {
    { ai_facing }
};
const mmove_t MMOVE_T(fixbot_move_turn) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_turn, NULL };

/*
    takeoff
*/
static const mframe_t fixbot_frames_takeoff[] = {
    { ai_move, 0.01f, fly_vertical },
    { ai_move, 0.01f, fly_vertical },
    { ai_move, 0.01f, fly_vertical },
    { ai_move, 0.01f, fly_vertical },
    { ai_move, 0.01f, fly_vertical },
    { ai_move, 0.01f, fly_vertical },
    { ai_move, 0.01f, fly_vertical },
    { ai_move, 0.01f, fly_vertical },
    { ai_move, 0.01f, fly_vertical },
    { ai_move, 0.01f, fly_vertical },

    { ai_move, 0.01f, fly_vertical },
    { ai_move, 0.01f, fly_vertical },
    { ai_move, 0.01f, fly_vertical },
    { ai_move, 0.01f, fly_vertical },
    { ai_move, 0.01f, fly_vertical },
    { ai_move, 0.01f, fly_vertical }
};
const mmove_t MMOVE_T(fixbot_move_takeoff) = { FRAME_takeoff_01, FRAME_takeoff_16, fixbot_frames_takeoff, NULL };

/* findout what this is */
static const mframe_t fixbot_frames_paina[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(fixbot_move_paina) = { FRAME_paina_01, FRAME_paina_06, fixbot_frames_paina, fixbot_run };

/* findout what this is */
static const mframe_t fixbot_frames_painb[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
const mmove_t MMOVE_T(fixbot_move_painb) = { FRAME_painb_01, FRAME_painb_08, fixbot_frames_painb, fixbot_run };

/*
    backup from pain
    call a generic painsound
    some spark effects
*/
static const mframe_t fixbot_frames_pain3[] = {
    { ai_move, -1 }
};
const mmove_t MMOVE_T(fixbot_move_pain3) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_pain3, fixbot_run };

#if 0
/*
    bot has compleated landing
    and is now on the grownd
    ( may need second land if the bot is releasing jib into jib vat )
*/
static const mframe_t fixbot_frames_land[] = {
    { ai_move }
};
const mmove_t MMOVE_T(fixbot_move_land) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_land, NULL };
#endif

void M_MoveToGoal(edict_t *ent, float dist);

static void ai_movetogoal(edict_t *self, float dist)
{
    M_MoveToGoal(self, dist);
}
/*

*/
static const mframe_t fixbot_frames_forward[] = {
    { ai_movetogoal, 5, use_scanner }
};
const mmove_t MMOVE_T(fixbot_move_forward) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_forward, NULL };

/*

*/
static const mframe_t fixbot_frames_walk[] = {
    { ai_walk, 5 }
};
const mmove_t MMOVE_T(fixbot_move_walk) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_walk, NULL };

/*

*/
static const mframe_t fixbot_frames_run[] = {
    { ai_run, 10 }
};
const mmove_t MMOVE_T(fixbot_move_run) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_run, NULL };

#if 0
/*
    raf
    note to self
    they could have a timer that will cause
    the bot to explode on countdown
*/
static const mframe_t fixbot_frames_death1[] = {
    { ai_move }
};
const mmove_t MMOVE_T(fixbot_move_death1) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_death1, fixbot_dead };

//
static const mframe_t fixbot_frames_backward[] = {
    { ai_move }
};
const mmove_t MMOVE_T(fixbot_move_backward) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_backward, NULL };
#endif

//
static const mframe_t fixbot_frames_start_attack[] = {
    { ai_charge }
};
const mmove_t MMOVE_T(fixbot_move_start_attack) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_start_attack, fixbot_attack };

#if 0
/*
    TBD:
    need to get laser attack anim
    attack with the laser blast
*/
static const mframe_t fixbot_frames_attack1[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, -10, fixbot_fire_blaster }
};
const mmove_t MMOVE_T(fixbot_move_attack1) = { FRAME_shoot_01, FRAME_shoot_06, fixbot_frames_attack1, NULL };
#endif

void abortHeal(edict_t *self, bool change_frame, bool gib, bool mark);

void PRETHINK(fixbot_laser_update)(edict_t *laser)
{
    edict_t *self = laser->owner;

    vec3_t start, dir;
    AngleVectors(self->s.angles, dir, NULL, NULL);
    VectorMA(self->s.origin, 16, dir, start);

    if (self->enemy && self->health > 0) {
        vec3_t point;
        VectorAvg(self->enemy->absmin, self->enemy->absmax, point);
        if (self->monsterinfo.aiflags & AI_MEDIC)
            point[0] += sinf(TO_SEC(level.time)) * 8;
        VectorSubtract(point, self->s.origin, dir);
        VectorNormalize(dir);
    }

    VectorCopy(start, laser->s.origin);
    VectorCopy(dir, laser->movedir);
    gi.linkentity(laser);
    dabeam_update(laser, true);
}

static void fixbot_fire_laser(edict_t *self)
{
    // critter dun got blown up while bein' fixed
    if (!self->enemy || !self->enemy->inuse || self->enemy->health <= self->enemy->gib_health) {
        M_SetAnimation(self, &fixbot_move_stand);
        self->monsterinfo.aiflags &= ~AI_MEDIC;
        return;
    }

    monster_fire_dabeam(self, -1, false, fixbot_laser_update);

    if (self->enemy->health > (self->enemy->mass / 10)) {
        vec3_t maxs;
        self->enemy->spawnflags = SPAWNFLAG_NONE;
        self->enemy->monsterinfo.aiflags &= AI_STINKY | AI_SPAWNED_MASK;
        self->enemy->target = NULL;
        self->enemy->targetname = NULL;
        self->enemy->combattarget = NULL;
        self->enemy->deathtarget = NULL;
        self->enemy->healthtarget = NULL;
        self->enemy->itemtarget = NULL;
        self->enemy->monsterinfo.healer = self;

        VectorCopy(self->enemy->maxs, maxs);
        maxs[2] += 48; // compensate for change when they die

        trace_t tr = gi.trace(self->enemy->s.origin, self->enemy->mins, maxs, self->enemy->s.origin, self->enemy, MASK_MONSTERSOLID);
        if (tr.startsolid || tr.allsolid) {
            abortHeal(self, false, true, false);
            return;
        }
        if (tr.ent != world) {
            abortHeal(self, false, true, false);
            return;
        }

        self->enemy->monsterinfo.aiflags |= AI_IGNORE_SHOTS | AI_DO_NOT_COUNT;

        // backup & restore health stuff, because of multipliers
        int old_max_health = self->enemy->max_health;
        item_id_t old_power_armor_type = self->enemy->monsterinfo.initial_power_armor_type;
        int old_power_armor_power = self->enemy->monsterinfo.max_power_armor_power;
        int old_base_health = self->enemy->monsterinfo.base_health;
        int old_health_scaling = self->enemy->monsterinfo.health_scaling;
        reinforcement_list_t reinforcements = self->enemy->monsterinfo.reinforcements;
        int monster_slots = self->enemy->monsterinfo.monster_slots;
        int monster_used = self->enemy->monsterinfo.monster_used;
        int old_gib_health = self->enemy->gib_health;

        ED_InitSpawnVars();
        ED_SetKeySpecified("reinforcements");
        st.reinforcements = "";

        ED_CallSpawn(self->enemy);

        self->enemy->monsterinfo.reinforcements = reinforcements;
        self->enemy->monsterinfo.monster_slots = monster_slots;
        self->enemy->monsterinfo.monster_used = monster_used;

        self->enemy->gib_health = old_gib_health / 2;
        self->enemy->health = self->enemy->max_health = old_max_health;
        self->enemy->monsterinfo.power_armor_power = self->enemy->monsterinfo.max_power_armor_power = old_power_armor_power;
        self->enemy->monsterinfo.power_armor_type = self->enemy->monsterinfo.initial_power_armor_type = old_power_armor_type;
        self->enemy->monsterinfo.base_health = old_base_health;
        self->enemy->monsterinfo.health_scaling = old_health_scaling;

        if (self->enemy->monsterinfo.setskin)
            self->enemy->monsterinfo.setskin(self->enemy);

        if (self->enemy->think) {
            self->enemy->nextthink = level.time;
            self->enemy->think(self->enemy);
        }
        self->enemy->monsterinfo.aiflags &= ~AI_RESURRECTING;
        self->enemy->monsterinfo.aiflags |= AI_IGNORE_SHOTS | AI_DO_NOT_COUNT;
        // turn off flies
        self->enemy->s.effects &= ~EF_FLIES;
        self->enemy->monsterinfo.healer = NULL;

        // clean up target, if we have one and it's legit
        if (self->enemy && self->enemy->inuse) {
            cleanupHealTarget(self->enemy);

            if ((self->oldenemy) && (self->oldenemy->inuse) && (self->oldenemy->health > 0)) {
                self->enemy->enemy = self->oldenemy;
                FoundTarget(self->enemy);
            } else {
                self->enemy->enemy = NULL;
                if (!FindTarget(self->enemy)) {
                    // no valid enemy, so stop acting
                    self->enemy->monsterinfo.pausetime = HOLD_FOREVER;
                    self->enemy->monsterinfo.stand(self->enemy);
                }
                self->enemy = NULL;
                self->oldenemy = NULL;
                if (!FindTarget(self)) {
                    // no valid enemy, so stop acting
                    self->monsterinfo.pausetime = HOLD_FOREVER;
                    self->monsterinfo.stand(self);
                    return;
                }
            }
        }

        M_SetAnimation(self, &fixbot_move_stand);
    } else
        self->enemy->monsterinfo.aiflags |= AI_RESURRECTING;
}

static const mframe_t fixbot_frames_laserattack[] = {
    { ai_charge, 0, fixbot_fire_laser },
    { ai_charge, 0, fixbot_fire_laser },
    { ai_charge, 0, fixbot_fire_laser },
    { ai_charge, 0, fixbot_fire_laser },
    { ai_charge, 0, fixbot_fire_laser },
    { ai_charge, 0, fixbot_fire_laser }
};
const mmove_t MMOVE_T(fixbot_move_laserattack) = { FRAME_shoot_01, FRAME_shoot_06, fixbot_frames_laserattack, NULL };

/*
    need to get forward translation data
    for the charge attack
*/
static const mframe_t fixbot_frames_attack2[] = {
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

    { ai_charge, -10 },
    { ai_charge, -10 },
    { ai_charge, -10 },
    { ai_charge, -10 },
    { ai_charge, -10 },
    { ai_charge, -10 },
    { ai_charge, -10 },
    { ai_charge, -10 },
    { ai_charge, -10 },
    { ai_charge, -10 },

    { ai_charge, 0, fixbot_fire_blaster },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },

    { ai_charge }
};
const mmove_t MMOVE_T(fixbot_move_attack2) = { FRAME_charging_01, FRAME_charging_31, fixbot_frames_attack2, fixbot_run };

static void weldstate(edict_t *self)
{
    if (self->s.frame == FRAME_weldstart_10)
        M_SetAnimation(self, &fixbot_move_weld);
    else if (self->goalentity && self->s.frame == FRAME_weldmiddle_07) {
        if (self->goalentity->health <= 0) {
            self->enemy->owner = NULL;
            M_SetAnimation(self, &fixbot_move_weld_end);
        } else
            self->goalentity->health -= 10;
    } else {
        self->goalentity = self->enemy = NULL;
        M_SetAnimation(self, &fixbot_move_stand);
    }
}

static void ai_move2(edict_t *self, float dist)
{
    if (!self->goalentity) {
        fixbot_stand(self);
        return;
    }

    M_walkmove(self, self->s.angles[YAW], dist);

    vec3_t v;
    VectorSubtract(self->goalentity->s.origin, self->s.origin, v);
    self->ideal_yaw = vectoyaw(v);
    M_ChangeYaw(self);
};

static const mframe_t fixbot_frames_weld_start[] = {
    { ai_move2, 0 },
    { ai_move2, 0 },
    { ai_move2, 0 },
    { ai_move2, 0 },
    { ai_move2, 0 },
    { ai_move2, 0 },
    { ai_move2, 0 },
    { ai_move2, 0 },
    { ai_move2, 0 },
    { ai_move2, 0, weldstate }
};
const mmove_t MMOVE_T(fixbot_move_weld_start) = { FRAME_weldstart_01, FRAME_weldstart_10, fixbot_frames_weld_start, NULL };

static const mframe_t fixbot_frames_weld[] = {
    { ai_move2, 0, fixbot_fire_welder },
    { ai_move2, 0, fixbot_fire_welder },
    { ai_move2, 0, fixbot_fire_welder },
    { ai_move2, 0, fixbot_fire_welder },
    { ai_move2, 0, fixbot_fire_welder },
    { ai_move2, 0, fixbot_fire_welder },
    { ai_move2, 0, weldstate }
};
const mmove_t MMOVE_T(fixbot_move_weld) = { FRAME_weldmiddle_01, FRAME_weldmiddle_07, fixbot_frames_weld, NULL };

static const mframe_t fixbot_frames_weld_end[] = {
    { ai_move2, -2 },
    { ai_move2, -2 },
    { ai_move2, -2 },
    { ai_move2, -2 },
    { ai_move2, -2 },
    { ai_move2, -2 },
    { ai_move2, -2, weldstate }
};
const mmove_t MMOVE_T(fixbot_move_weld_end) = { FRAME_weldend_01, FRAME_weldend_07, fixbot_frames_weld_end, NULL };

static void fixbot_fire_welder(edict_t *self)
{
    vec3_t start;
    vec3_t forward, right, up;
    static const vec3_t vec = { 24.0f, -0.8f, -10.0f };
    float  r;

    if (!self->enemy)
        return;

    AngleVectors(self->s.angles, forward, right, up);
    M_ProjectFlashSource(self, vec, forward, right, start);

    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_WELDING_SPARKS);
    gi.WriteByte(10);
    gi.WritePosition(start);
    gi.WriteDir(vec3_origin);
    gi.WriteByte(irandom2(0xe0, 0xe8));
    gi.multicast(self->s.origin, MULTICAST_PVS);

    if (frandom() > 0.8f) {
        r = frandom();

        if (r < 0.33f)
            gi.sound(self, CHAN_VOICE, sound_weld1, 1, ATTN_IDLE, 0);
        else if (r < 0.66f)
            gi.sound(self, CHAN_VOICE, sound_weld2, 1, ATTN_IDLE, 0);
        else
            gi.sound(self, CHAN_VOICE, sound_weld3, 1, ATTN_IDLE, 0);
    }
}

static void fixbot_fire_blaster(edict_t *self)
{
    vec3_t start;
    vec3_t forward, right, up;
    vec3_t end;
    vec3_t dir;

    if (!visible(self, self->enemy))
        M_SetAnimation(self, &fixbot_move_run);

    AngleVectors(self->s.angles, forward, right, up);
    M_ProjectFlashSource(self, monster_flash_offset[MZ2_HOVER_BLASTER_1], forward, right, start);

    VectorCopy(self->enemy->s.origin, end);
    end[2] += self->enemy->viewheight;
    VectorSubtract(end, start, dir);
    VectorNormalize(dir);

    monster_fire_blaster(self, start, dir, 15, 1000, MZ2_HOVER_BLASTER_1, EF_BLASTER);
}

void MONSTERINFO_STAND(fixbot_stand)(edict_t *self)
{
    M_SetAnimation(self, &fixbot_move_stand);
}

void MONSTERINFO_RUN(fixbot_run)(edict_t *self)
{
    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        M_SetAnimation(self, &fixbot_move_stand);
    else
        M_SetAnimation(self, &fixbot_move_run);
}

void MONSTERINFO_WALK(fixbot_walk)(edict_t *self)
{
    if (self->goalentity
        && strcmp(self->goalentity->classname, "object_repair") == 0
        && Distance(self->s.origin, self->goalentity->s.origin) < 32)
        M_SetAnimation(self, &fixbot_move_weld_start);
    else
        M_SetAnimation(self, &fixbot_move_walk);
}

void MONSTERINFO_ATTACK(fixbot_attack)(edict_t *self)
{
    if (self->monsterinfo.aiflags & AI_MEDIC) {
        if (!visible(self, self->enemy))
            return;
        if (Distance(self->s.origin, self->enemy->s.origin) > 128)
            return;
        M_SetAnimation(self, &fixbot_move_laserattack);
    } else {
        fixbot_set_fly_parameters(self, false, false);
        M_SetAnimation(self, &fixbot_move_attack2);
    }
}

void PAIN(fixbot_pain)(edict_t *self, edict_t *other, float kick, int damage, mod_t mod)
{
    if (level.time < self->pain_debounce_time)
        return;

    fixbot_set_fly_parameters(self, false, false);
    self->pain_debounce_time = level.time + SEC(3);
    gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);

    if (damage <= 10)
        M_SetAnimation(self, &fixbot_move_pain3);
    else if (damage <= 25)
        M_SetAnimation(self, &fixbot_move_painb);
    else
        M_SetAnimation(self, &fixbot_move_paina);

    abortHeal(self, false, false, false);
}

#if 0
static void fixbot_dead(edict_t *self)
{
    VectorSet(self->mins, -16, -16, -24);
    VectorSet(self->maxs, 16, 16, -8);
    self->movetype = MOVETYPE_TOSS;
    self->svflags |= SVF_DEADMONSTER;
    self->nextthink = 0;
    gi.linkentity(self);
}
#endif

void DIE(fixbot_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
    BecomeExplosion1(self);

    // shards
}

static void fixbot_precache(void)
{
    sound_pain1 = gi.soundindex("flyer/flypain1.wav");
    sound_die = gi.soundindex("flyer/flydeth1.wav");

    sound_weld1 = gi.soundindex("misc/welder1.wav");
    sound_weld2 = gi.soundindex("misc/welder2.wav");
    sound_weld3 = gi.soundindex("misc/welder3.wav");
}

/*QUAKED monster_fixbot (1 .5 0) (-32 -32 -24) (32 32 24) Ambush Trigger_Spawn Fixit Takeoff Landing
 */
void SP_monster_fixbot(edict_t *self)
{
    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    G_AddPrecache(fixbot_precache);

    self->s.modelindex = gi.modelindex("models/monsters/fixbot/tris.md2");

    VectorSet(self->mins, -32, -32, -24);
    VectorSet(self->maxs, 32, 32, 24);

    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;

    self->health = 150 * st.health_multiplier;
    self->mass = 150;

    self->pain = fixbot_pain;
    self->die = fixbot_die;

    self->monsterinfo.stand = fixbot_stand;
    self->monsterinfo.walk = fixbot_walk;
    self->monsterinfo.run = fixbot_run;
    self->monsterinfo.attack = fixbot_attack;

    gi.linkentity(self);

    M_SetAnimation(self, &fixbot_move_stand);
    self->monsterinfo.scale = MODEL_SCALE;
    self->monsterinfo.aiflags |= AI_ALTERNATE_FLY;
    fixbot_set_fly_parameters(self, false, false);

    flymonster_start(self);
}
