// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"
#include <float.h>

//===============================
// BLOCKED Logic
//===============================

bool face_wall(edict_t *self);

// blocked_checkplat
//  dist: how far they are trying to walk.
bool blocked_checkplat(edict_t *self, float dist)
{
    int      playerPosition;
    trace_t  trace;
    vec3_t   pt1, pt2;
    vec3_t   forward;
    edict_t *plat;

    if (!self->enemy)
        return false;

    // check player's relative altitude
    if (self->enemy->absmin[2] >= self->absmax[2])
        playerPosition = 1;
    else if (self->enemy->absmax[2] <= self->absmin[2])
        playerPosition = -1;
    else
        playerPosition = 0;

    // if we're close to the same position, don't bother trying plats.
    if (playerPosition == 0)
        return false;

    plat = NULL;

    // see if we're already standing on a plat.
    if (self->groundentity && self->groundentity != world && !strncmp(self->groundentity->classname, "func_plat", 8))
        plat = self->groundentity;

    // if we're not, check to see if we'll step onto one with this move
    if (!plat) {
        AngleVectors(self->s.angles, forward, NULL, NULL);
        VectorMA(self->s.origin, dist, forward, pt1);
        VectorCopy(pt1, pt2);
        pt2[2] -= 384;

        trace = gi.trace(pt1, NULL, NULL, pt2, self, MASK_MONSTERSOLID);
        if (trace.fraction < 1 && !trace.allsolid && !trace.startsolid && !strncmp(trace.ent->classname, "func_plat", 8))
            plat = trace.ent;
    }

    // if we've found a plat, trigger it.
    if (plat && plat->use) {
        if (playerPosition == 1) {
            if ((self->groundentity == plat && plat->moveinfo.state == STATE_BOTTOM) ||
                (self->groundentity != plat && plat->moveinfo.state == STATE_TOP)) {
                plat->use(plat, self, self);
                return true;
            }
        } else if (playerPosition == -1) {
            if ((self->groundentity == plat && plat->moveinfo.state == STATE_TOP) ||
                (self->groundentity != plat && plat->moveinfo.state == STATE_BOTTOM)) {
                plat->use(plat, self, self);
                return true;
            }
        }
    }

    return false;
}

//*******************
// JUMPING AIDS
//*******************

static void monster_jump_start(edict_t *self)
{
    monster_done_dodge(self);

    self->monsterinfo.jump_time = level.time + SEC(3);
}

bool monster_jump_finished(edict_t *self)
{
    vec3_t forward;
    AngleVectors(self->s.angles, forward, NULL, NULL);

    // if we lost our forward velocity, give us more
    if (DotProduct(self->velocity, forward) < 150) {
        float z_velocity = self->velocity[2];
        VectorScale(forward, 150, self->velocity);
        self->velocity[2] = z_velocity;
    }

    return self->monsterinfo.jump_time < level.time;
}

// blocked_checkjump
//  dist: how far they are trying to walk.
//  self->monsterinfo.drop_height/self->monsterinfo.jump_height: how far they'll ok a jump for. set to 0 to disable that direction.
blocked_jump_result_t blocked_checkjump(edict_t *self, float dist)
{
    // can't jump even if we physically can
    if (!self->monsterinfo.can_jump)
        return NO_JUMP;
    // no enemy to path to
    if (!self->enemy)
        return NO_JUMP;

    // we just jumped recently, don't try again
    if (self->monsterinfo.jump_time > level.time)
        return NO_JUMP;

    int     playerPosition;
    trace_t trace;
    vec3_t  pt1, pt2;
    vec3_t  forward, up;

    AngleVectors(self->s.angles, forward, NULL, up);

    if (self->enemy->absmin[2] > (self->absmin[2] + STEPSIZE))
        playerPosition = 1;
    else if (self->enemy->absmin[2] < (self->absmin[2] - STEPSIZE))
        playerPosition = -1;
    else
        playerPosition = 0;

    if (playerPosition == -1 && self->monsterinfo.drop_height) {
        // check to make sure we can even get to the spot we're going to "fall" from
        VectorMA(self->s.origin, 48, forward, pt1);
        trace = gi.trace(self->s.origin, self->mins, self->maxs, pt1, self, MASK_MONSTERSOLID);
        if (trace.fraction < 1)
            return NO_JUMP;

        VectorCopy(pt1, pt2);
        pt2[2] = self->absmin[2] - self->monsterinfo.drop_height - 1;

        trace = gi.trace(pt1, NULL, NULL, pt2, self, MASK_MONSTERSOLID | MASK_WATER);
        if (trace.fraction < 1 && !trace.allsolid && !trace.startsolid) {
            // check how deep the water is
            if (trace.contents & CONTENTS_WATER) {
                trace_t deep = gi.trace(trace.endpos, NULL, NULL, pt2, self, MASK_MONSTERSOLID);

                water_level_t waterlevel;
                contents_t watertype;
                M_CategorizePosition(self, deep.endpos, &waterlevel, &watertype);

                if (waterlevel > WATER_WAIST)
                    return NO_JUMP;
            }

            if ((self->absmin[2] - trace.endpos[2]) >= 24 && (trace.contents & (MASK_SOLID | CONTENTS_WATER))) {
                if ((self->enemy->absmin[2] - trace.endpos[2]) > 32)
                    return NO_JUMP;

                if (trace.plane.normal[2] < 0.9f)
                    return NO_JUMP;

                monster_jump_start(self);

                return JUMP_JUMP_DOWN;
            }
        }
    } else if (playerPosition == 1 && self->monsterinfo.jump_height) {
        VectorMA(self->s.origin, 48, forward, pt1);
        VectorCopy(pt1, pt2);
        pt1[2] = self->absmax[2] + self->monsterinfo.jump_height;

        trace = gi.trace(pt1, NULL, NULL, pt2, self, MASK_MONSTERSOLID | MASK_WATER);
        if (trace.fraction < 1 && !trace.allsolid && !trace.startsolid) {
            if ((trace.endpos[2] - self->absmin[2]) <= self->monsterinfo.jump_height && (trace.contents & (MASK_SOLID | CONTENTS_WATER))) {
                face_wall(self);

                monster_jump_start(self);

                return JUMP_JUMP_UP;
            }
        }
    }

    return NO_JUMP;
}

// *************************
// HINT PATHS
// *************************

#define SPAWNFLAG_HINT_ENDPOINT     0x01
#define MAX_HINT_CHAINS             100

static bool     hint_paths_present;
static edict_t *hint_path_start[MAX_HINT_CHAINS];
static int      num_hint_paths;

//
// AI code
//

// =============
// hintpath_go - starts a monster (self) moving towards the hintpath (point)
//      disables all contrary AI flags.
// =============
static void hintpath_go(edict_t *self, edict_t *point)
{
    vec3_t dir;

    VectorSubtract(point->s.origin, self->s.origin, dir);

    self->ideal_yaw = vectoyaw(dir);
    self->goalentity = self->movetarget = point;
    self->monsterinfo.pausetime = 0;
    self->monsterinfo.aiflags |= AI_HINT_PATH;
    self->monsterinfo.aiflags &= ~(AI_SOUND_TARGET | AI_PURSUIT_LAST_SEEN | AI_PURSUE_NEXT | AI_PURSUE_TEMP);
    // run for it
    self->monsterinfo.search_time = level.time;
    self->monsterinfo.run(self);
}

// =============
// hintpath_stop - bails a monster out of following hint paths
// =============
void hintpath_stop(edict_t *self)
{
    self->goalentity = NULL;
    self->movetarget = NULL;
    self->monsterinfo.last_hint_time = level.time;
    self->monsterinfo.goal_hint = NULL;
    self->monsterinfo.aiflags &= ~AI_HINT_PATH;
    if (has_valid_enemy(self)) {
        // if we can see our target, go nuts
        if (visible(self, self->enemy)) {
            FoundTarget(self);
            return;
        }
        // otherwise, keep chasing
        HuntTarget(self, true);
        return;
    }
    // if our enemy is no longer valid, forget about our enemy and go into stand
    self->enemy = NULL;
    // we need the pausetime otherwise the stand code
    // will just revert to walking with no target and
    // the monsters will wonder around aimlessly trying
    // to hunt the world entity
    self->monsterinfo.pausetime = HOLD_FOREVER;
    self->monsterinfo.stand(self);
}

// =============
// monsterlost_checkhint - the monster (self) will check around for valid hintpaths.
//      a valid hintpath is one where the two endpoints can see both the monster
//      and the monster's enemy. if only one person is visible from the endpoints,
//      it will not go for it.
// =============
bool monsterlost_checkhint(edict_t *self)
{
    edict_t *e, *monster_pathchain, *target_pathchain, *checkpoint = NULL;
    edict_t *closest;
    float    closest_range = 1000000;
    edict_t *start, *destination;
    int      count5 = 0;
    float    r;
    int      i;
    bool     hint_path_represented[MAX_HINT_CHAINS];

    // if there are no hint paths on this map, exit immediately.
    if (!hint_paths_present)
        return false;

    if (!self->enemy)
        return false;

    // [Paril-KEX] don't do hint paths if we're using nav nodes
    if (self->monsterinfo.aiflags & (AI_STAND_GROUND | AI_PATHING))
        return false;

    if (!strcmp(self->classname, "monster_turret"))
        return false;

    monster_pathchain = NULL;

    // find all the hint_paths.
    // FIXME - can we not do this every time?
    for (i = 0; i < num_hint_paths; i++) {
        e = hint_path_start[i];
        while (e) {
            if (e->monster_hint_chain)
                e->monster_hint_chain = NULL;

            if (monster_pathchain) {
                checkpoint->monster_hint_chain = e;
                checkpoint = e;
            } else {
                monster_pathchain = e;
                checkpoint = e;
            }
            e = e->hint_chain;
        }
    }

    // filter them by distance and visibility to the monster
    e = monster_pathchain;
    checkpoint = NULL;
    while (e) {
        r = realrange(self, e);

        if (r > 512 || !visible(self, e)) {
            if (checkpoint) {
                checkpoint->monster_hint_chain = e->monster_hint_chain;
                e->monster_hint_chain = NULL;
                e = checkpoint->monster_hint_chain;
            } else {
                // use checkpoint as temp pointer
                checkpoint = e;
                e = e->monster_hint_chain;
                checkpoint->monster_hint_chain = NULL;
                // and clear it again
                checkpoint = NULL;
                // since we have yet to find a valid one (or else checkpoint would be set) move the
                // start of monster_pathchain
                monster_pathchain = e;
            }
            continue;
        }

        count5++;
        checkpoint = e;
        e = e->monster_hint_chain;
    }

    // at this point, we have a list of all of the eligible hint nodes for the monster
    // we now take them, figure out what hint chains they're on, and traverse down those chains,
    // seeing whether any can see the player
    //
    // first, we figure out which hint chains we have represented in monster_pathchain
    if (count5 == 0)
        return false;

    for (i = 0; i < num_hint_paths; i++)
        hint_path_represented[i] = false;

    e = monster_pathchain;
    checkpoint = NULL;
    while (e) {
        if ((e->hint_chain_id < 0) || (e->hint_chain_id > num_hint_paths))
            return false;

        hint_path_represented[e->hint_chain_id] = true;
        e = e->monster_hint_chain;
    }

    count5 = 0;

    // now, build the target_pathchain which contains all of the hint_path nodes we need to check for
    // validity (within range, visibility)
    target_pathchain = NULL;
    checkpoint = NULL;
    for (i = 0; i < num_hint_paths; i++) {
        // if this hint chain is represented in the monster_hint_chain, add all of it's nodes to the target_pathchain
        // for validity checking
        if (hint_path_represented[i]) {
            e = hint_path_start[i];
            while (e) {
                if (target_pathchain) {
                    checkpoint->target_hint_chain = e;
                    checkpoint = e;
                } else {
                    target_pathchain = e;
                    checkpoint = e;
                }
                e = e->hint_chain;
            }
        }
    }

    // target_pathchain is a list of all of the hint_path nodes we need to check for validity relative to the target
    e = target_pathchain;
    checkpoint = NULL;
    while (e) {
        r = realrange(self->enemy, e);

        if (r > 512 || !visible(self->enemy, e)) {
            if (checkpoint) {
                checkpoint->target_hint_chain = e->target_hint_chain;
                e->target_hint_chain = NULL;
                e = checkpoint->target_hint_chain;
            } else {
                // use checkpoint as temp pointer
                checkpoint = e;
                e = e->target_hint_chain;
                checkpoint->target_hint_chain = NULL;
                // and clear it again
                checkpoint = NULL;
                target_pathchain = e;
            }
            continue;
        }

        count5++;
        checkpoint = e;
        e = e->target_hint_chain;
    }

    // at this point we should have:
    // monster_pathchain - a list of "monster valid" hint_path nodes linked together by monster_hint_chain
    // target_pathcain - a list of "target valid" hint_path nodes linked together by target_hint_chain.  these
    //                   are filtered such that only nodes which are on the same chain as "monster valid" nodes
    //
    // Now, we figure out which "monster valid" node we want to use
    //
    // To do this, we first off make sure we have some target nodes.  If we don't, there are no valid hint_path nodes
    // for us to take
    //
    // If we have some, we filter all of our "monster valid" nodes by which ones have "target valid" nodes on them
    //
    // Once this filter is finished, we select the closest "monster valid" node, and go to it.

    if (count5 == 0)
        return false;

    // reuse the hint_chain_represented array, this time to see which chains are represented by the target
    for (i = 0; i < num_hint_paths; i++)
        hint_path_represented[i] = false;

    e = target_pathchain;
    checkpoint = NULL;
    while (e) {
        if ((e->hint_chain_id < 0) || (e->hint_chain_id > num_hint_paths))
            return false;

        hint_path_represented[e->hint_chain_id] = true;
        e = e->target_hint_chain;
    }

    // traverse the monster_pathchain - if the hint_node isn't represented in the "target valid" chain list,
    // remove it
    // if it is on the list, check it for range from the monster.  If the range is the closest, keep it
    //
    closest = NULL;
    e = monster_pathchain;
    while (e) {
        if (!(hint_path_represented[e->hint_chain_id])) {
            checkpoint = e->monster_hint_chain;
            e->monster_hint_chain = NULL;
            e = checkpoint;
            continue;
        }
        r = realrange(self, e);
        if (r < closest_range)
            closest = e;
        e = e->monster_hint_chain;
    }

    if (!closest)
        return false;

    start = closest;
    // now we know which one is the closest to the monster .. this is the one the monster will go to
    // we need to finally determine what the DESTINATION node is for the monster .. walk down the hint_chain,
    // and find the closest one to the player

    closest = NULL;
    closest_range = 10000000;
    e = target_pathchain;
    while (e) {
        if (start->hint_chain_id == e->hint_chain_id) {
            r = realrange(self, e);
            if (r < closest_range)
                closest = e;
        }
        e = e->target_hint_chain;
    }

    if (!closest)
        return false;

    destination = closest;

    self->monsterinfo.goal_hint = destination;
    hintpath_go(self, start);

    return true;
}

//
// Path code
//

// =============
// hint_path_touch - someone's touched the hint_path
// =============
void TOUCH(hint_path_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    edict_t *e, *goal, *next = NULL;
    bool goalFound = false;

    // make sure we're the target of it's obsession
    if (other->movetarget != self)
        return;

    goal = other->monsterinfo.goal_hint;

    // if the monster is where he wants to be
    if (goal == self) {
        hintpath_stop(other);
        return;
    }

    // if we aren't, figure out which way we want to go
    e = hint_path_start[self->hint_chain_id];
    while (e) {
        // if we get up to ourselves on the hint chain, we're going down it
        if (e == self) {
            next = e->hint_chain;
            break;
        }
        if (e == goal)
            goalFound = true;
        // if we get to where the next link on the chain is this hint_path and have found the goal on the way
        // we're going upstream, so remember who the previous link is
        if ((e->hint_chain == self) && goalFound) {
            next = e;
            break;
        }
        e = e->hint_chain;
    }

    // if we couldn't find it, have the monster go back to normal hunting.
    if (!next) {
        hintpath_stop(other);
        return;
    }

    // send him on his way
    hintpath_go(other, next);

    // have the monster freeze if the hint path we just touched has a wait time
    // on it, for example, when riding a plat.
    if (self->wait)
        other->nextthink = level.time + SEC(self->wait);
}

/*QUAKED hint_path (.5 .3 0) (-8 -8 -8) (8 8 8) END
Target: next hint path

END - set this flag on the endpoints of each hintpath.

"wait" - set this if you want the monster to freeze when they touch this hintpath
*/
void SP_hint_path(edict_t *self)
{
    if (deathmatch->integer) {
        G_FreeEdict(self);
        return;
    }

    if (!self->targetname && !self->target) {
        gi.dprintf("%s: unlinked\n", etos(self));
        G_FreeEdict(self);
        return;
    }

    self->solid = SOLID_TRIGGER;
    self->touch = hint_path_touch;
    VectorSet(self->mins, -8, -8, -8);
    VectorSet(self->maxs, 8, 8, 8);
    self->svflags |= SVF_NOCLIENT;
    gi.linkentity(self);
}

// ============
// InitHintPaths - Called by InitGame (g_save) to enable quick exits if valid
// ============
void InitHintPaths(void)
{
    edict_t *e, *current;
    int      i;

    hint_paths_present = false;

    // check all the hint_paths.
    e = G_Find(NULL, FOFS(classname), "hint_path");
    if (!e)
        return;

    hint_paths_present = true;

    memset(hint_path_start, 0, sizeof(hint_path_start));
    num_hint_paths = 0;
    while (e) {
        if (e->spawnflags & SPAWNFLAG_HINT_ENDPOINT) {
            if (e->target) { // start point
                if (e->targetname) { // this is a bad end, ignore it
                    gi.dprintf("%s: marked as endpoint with both target (%s) and targetname (%s)\n",
                               etos(e), e->target, e->targetname);
                } else {
                    if (num_hint_paths >= MAX_HINT_CHAINS)
                        break;

                    hint_path_start[num_hint_paths++] = e;
                }
            }
        }
        e = G_Find(e, FOFS(classname), "hint_path");
    }

    for (i = 0; i < num_hint_paths; i++) {
        current = hint_path_start[i];
        current->hint_chain_id = i;
        e = G_Find(NULL, FOFS(targetname), current->target);
        if (G_Find(e, FOFS(targetname), current->target)) {
            gi.dprintf("%s: forked path detected for chain %d, target %s\n",
                       etos(current), num_hint_paths, current->target);
            hint_path_start[i]->hint_chain = NULL;
            continue;
        }
        while (e) {
            if (e->hint_chain) {
                gi.dprintf("%s: circular path detected for chain %d, targetname %s\n",
                           etos(e), num_hint_paths, e->targetname);
                hint_path_start[i]->hint_chain = NULL;
                break;
            }
            current->hint_chain = e;
            current = e;
            current->hint_chain_id = i;
            if (!current->target)
                break;
            e = G_Find(NULL, FOFS(targetname), current->target);
            if (G_Find(e, FOFS(targetname), current->target)) {
                gi.dprintf("%s: forked path detected for chain %d, target %s\n",
                           etos(current), num_hint_paths, current->target);
                hint_path_start[i]->hint_chain = NULL;
                break;
            }
        }
    }
}

// *****************************
//  MISCELLANEOUS STUFF
// *****************************

// PMM - inback
// use to see if opponent is behind you (not to side)
// if it looks a lot like infront, well, there's a reason

bool inback(edict_t *self, edict_t *other)
{
    vec3_t vec;
    vec3_t forward;

    AngleVectors(self->s.angles, forward, NULL, NULL);
    VectorSubtract(other->s.origin, self->s.origin, vec);
    VectorNormalize(vec);
    return DotProduct(vec, forward) < -0.3f;
}

float realrange(edict_t *self, edict_t *other)
{
    return Distance(self->s.origin, other->s.origin);
}

bool face_wall(edict_t *self)
{
    vec3_t  pt;
    vec3_t  forward;
    vec3_t  ang;
    trace_t tr;

    AngleVectors(self->s.angles, forward, NULL, NULL);
    VectorMA(self->s.origin, 64, forward, pt);
    tr = gi.trace(self->s.origin, NULL, NULL, pt, self, MASK_MONSTERSOLID);
    if (tr.fraction < 1 && !tr.allsolid && !tr.startsolid) {
        vectoangles(tr.plane.normal, ang);
        self->ideal_yaw = ang[YAW] + 180;
        if (self->ideal_yaw > 360)
            self->ideal_yaw -= 360;

        M_ChangeYaw(self);
        return true;
    }

    return false;
}

//
// Monster "Bad" Areas
//

void TOUCH(badarea_touch)(edict_t *ent, edict_t *other, const trace_t *tr, bool other_touching_self)
{
}

edict_t *SpawnBadArea(const vec3_t mins, const vec3_t maxs, gtime_t lifespan, edict_t *owner)
{
    edict_t *badarea;
    vec3_t   origin;

    VectorAvg(mins, maxs, origin);

    badarea = G_Spawn();
    VectorCopy(origin, badarea->s.origin);
    VectorSubtract(maxs, origin, badarea->maxs);
    VectorSubtract(mins, origin, badarea->mins);
    badarea->touch = badarea_touch;
    badarea->movetype = MOVETYPE_NONE;
    badarea->solid = SOLID_TRIGGER;
    badarea->classname = "bad_area";
    gi.linkentity(badarea);

    if (lifespan) {
        badarea->think = G_FreeEdict;
        badarea->nextthink = level.time + lifespan;
    }
    if (owner)
        badarea->owner = owner;

    return badarea;
}

// CheckForBadArea
//      This is a customized version of G_TouchTriggers that will check
//      for bad area triggers and return them if they're touched.
edict_t *CheckForBadArea(edict_t *ent)
{
    int         i, num;
    edict_t     *touch[MAX_EDICTS_OLD], *hit;
    vec3_t      mins, maxs;

    VectorAdd(ent->s.origin, ent->mins, mins);
    VectorAdd(ent->s.origin, ent->maxs, maxs);

    num = gi.BoxEdicts(mins, maxs, touch, q_countof(touch), AREA_TRIGGERS);

    // be careful, it is possible to have an entity in this
    // list removed before we get to it (killtriggered)
    for (i = 0; i < num; i++) {
        hit = touch[i];
        if (!hit->inuse)
            continue;
        if (hit->touch == badarea_touch)
            return hit;
    }

    return NULL;
}

#define TESLA_DAMAGE_RADIUS     128

bool MarkTeslaArea(edict_t *self, edict_t *tesla)
{
    edict_t *e;
    edict_t *tail;
    edict_t *area;

    if (!tesla || !self)
        return false;

    area = NULL;

    // make sure this tesla doesn't have a bad area around it already...
    e = tesla->teamchain;
    tail = tesla;
    while (e) {
        tail = tail->teamchain;
        if (!strcmp(e->classname, "bad_area"))
            return false;

        e = e->teamchain;
    }

    // see if we can grab the trigger directly
    if (tesla->teamchain && tesla->teamchain->inuse) {
        edict_t *trigger = tesla->teamchain;

        if (tesla->air_finished)
            area = SpawnBadArea(trigger->absmin, trigger->absmax, tesla->air_finished, tesla);
        else
            area = SpawnBadArea(trigger->absmin, trigger->absmax, tesla->nextthink, tesla);
    // otherwise we just guess at how long it'll last.
    } else {
        vec3_t mins = { -TESLA_DAMAGE_RADIUS, -TESLA_DAMAGE_RADIUS, tesla->mins[2] };
        vec3_t maxs = { TESLA_DAMAGE_RADIUS, TESLA_DAMAGE_RADIUS, TESLA_DAMAGE_RADIUS };

        VectorAdd(mins, tesla->s.origin, mins);
        VectorAdd(maxs, tesla->s.origin, maxs);

        area = SpawnBadArea(mins, maxs, SEC(30), tesla);
    }

    // if we spawned a bad area, then link it to the tesla
    if (area)
        tail->teamchain = area;

    return true;
}

// predictive calculator
// target is who you want to shoot
// start is where the shot comes from
// bolt_speed is how fast the shot is (or 0 for hitscan)
// eye_height is a boolean to say whether or not to adjust to targets eye_height
// offset is how much time to miss by
// aimdir is the resulting aim direction (pass in NULL if you don't want it)
// aimpoint is the resulting aimpoint (pass in NULL if don't want it)
void PredictAim(edict_t *self, edict_t *target, const vec3_t start, float bolt_speed, bool eye_height, float offset, vec3_t aimdir, vec3_t aimpoint)
{
    vec3_t dir, vec, aim;
    float  dist, time;

    if (!target || !target->inuse) {
        VectorClear(aimdir);
        return;
    }

    VectorSubtract(target->s.origin, start, dir);
    if (eye_height)
        dir[2] += target->viewheight;

    VectorAdd(start, dir, vec);

    // [Paril-KEX] if our current attempt is blocked, try the opposite one
    trace_t tr = gi.trace(start, NULL, NULL, vec, self, MASK_PROJECTILE);

    if (tr.fraction < 1.0f && tr.ent != target) {
        eye_height = !eye_height;
        VectorSubtract(target->s.origin, start, dir);
        if (eye_height)
            dir[2] += target->viewheight;
    }

    dist = VectorNormalize(dir);

    if (bolt_speed)
        time = dist / bolt_speed;
    else
        time = 0;

    VectorMA(target->s.origin, time - offset, target->velocity, vec);

    VectorSubtract(vec, start, aim);
    VectorNormalize(aim);

    // went backwards...
    if (DotProduct(dir, aim) < 0)
        VectorCopy(target->s.origin, vec);
    // if the shot is going to impact a nearby wall from our prediction, just fire it straight.
    else if (gi.trace(start, NULL, NULL, vec, NULL, MASK_SOLID).fraction < 0.9f)
        VectorCopy(target->s.origin, vec);

    if (eye_height)
        vec[2] += target->viewheight;

    if (aimdir) {
        VectorSubtract(vec, start, aimdir);
        VectorNormalize(aimdir);
    }

    if (aimpoint)
        VectorCopy(vec, aimpoint);
}

// [Paril-KEX] find a pitch that will at some point land on or near the player.
// very approximate. aim will be adjusted to the correct aim vector.
bool M_CalculatePitchToFire(edict_t *self, const vec3_t target, const vec3_t start, vec3_t aim,
                            float speed, float time_remaining, bool mortar, bool destroy_on_touch)
{
    static const float pitches[] = { -80, -70, -60, -50, -40, -30, -20, -10, -5 };
    float best_pitch = 0;
    float best_dist = FLT_MAX;
    const float sim_time = 0.1f;

    vec3_t pitched_aim;
    vectoangles(aim, pitched_aim);

    for (int i = 0; i < q_countof(pitches); i++) {
        float pitch = pitches[i];

        if (mortar && pitch >= -30)
            break;

        pitched_aim[PITCH] = pitch;

        vec3_t fwd;
        AngleVectors(pitched_aim, fwd, NULL, NULL);

        vec3_t velocity, origin;
        VectorScale(fwd, speed, velocity);
        VectorCopy(start, origin);

        float t = time_remaining;

        while (t > 0) {
            vec3_t end;

            velocity[2] -= sim_time * level.gravity;
            VectorMA(origin, sim_time, velocity, end);

            trace_t tr = gi.trace(origin, NULL, NULL, end, NULL, MASK_SHOT);
            VectorCopy(tr.endpos, origin);

            if (tr.fraction < 1.0f) {
                if (tr.surface->flags & SURF_SKY)
                    break;

                VectorAdd(origin, tr.plane.normal, origin);
                ClipVelocity(velocity, tr.plane.normal, velocity, 1.6f);

                float dist = DistanceSquared(origin, target);

                if (tr.ent == self->enemy || tr.ent->client || (tr.plane.normal[2] >= 0.7f && dist < (128 * 128) && dist < best_dist)) {
                    best_pitch = pitch;
                    best_dist = dist;
                }

                if (destroy_on_touch || (tr.contents & (CONTENTS_MONSTER | CONTENTS_PLAYER | CONTENTS_DEADMONSTER)))
                    break;
            }

            t -= sim_time;
        }
    }

    if (best_dist < FLT_MAX) {
        pitched_aim[PITCH] = best_pitch;
        AngleVectors(pitched_aim, aim, NULL, NULL);
        return true;
    }

    return false;
}

bool below(edict_t *self, edict_t *other)
{
    vec3_t vec;
    vec3_t down = { 0, 0, -1 };

    VectorSubtract(other->s.origin, self->s.origin, vec);
    VectorNormalize(vec);
    return DotProduct(vec, down) > 0.95f;   // 18 degree arc below
}

//
// New dodge code
//
void MONSTERINFO_DODGE(M_MonsterDodge)(edict_t *self, edict_t *attacker, gtime_t eta, trace_t *tr, bool gravity)
{
    // this needs to be here since this can be called after the monster has "died"
    if (self->health < 1)
        return;

    bool ducker = (self->monsterinfo.duck) && (self->monsterinfo.unduck) && !gravity;
    bool dodger = (self->monsterinfo.sidestep) && !(self->monsterinfo.aiflags & AI_STAND_GROUND);

    if ((!ducker) && (!dodger))
        return;

    if (!self->enemy) {
        self->enemy = attacker;
        FoundTarget(self);
    }

    // PMM - don't bother if it's going to hit anyway; fix for weird in-your-face etas (I was
    // seeing numbers like 13 and 14)
    if ((eta < FRAME_TIME) || (eta > SEC(2.5f)))
        return;

    // skill level determination..
    if (brandom())
        return;

    float height;

    if (ducker && tr) {
        height = self->absmax[2] - 32 - 1; // the -1 is because the absmax is s.origin + maxs + 1

        if ((!dodger) && ((tr->endpos[2] <= height) || (self->monsterinfo.aiflags & AI_DUCKED)))
            return;
    } else
        height = self->absmax[2];

    if (dodger) {
        // if we're already dodging, just finish the sequence, i.e. don't do anything else
        if (self->monsterinfo.aiflags & AI_DODGING)
            return;

        // if we're ducking already, or the shot is at our knees
        if ((!ducker || !tr || tr->endpos[2] <= height) || (self->monsterinfo.aiflags & AI_DUCKED)) {
            // on Easy & Normal, don't sidestep as often (25% on Easy, 50% on Normal)
            float chance = 1.0f;

            if (skill->integer == 0)
                chance = 0.25f;
            else if (skill->integer == 1)
                chance = 0.50f;

            if (frandom() > chance) {
                self->monsterinfo.dodge_time = level.time + random_time_sec(0.8f, 1.4f);
                return;
            }

            if (tr) {
                vec3_t right, diff;

                AngleVectors(self->s.angles, NULL, right, NULL);
                VectorSubtract(tr->endpos, self->s.origin, diff);

                if (DotProduct(right, diff) < 0)
                    self->monsterinfo.lefty = false;
                else
                    self->monsterinfo.lefty = true;
            } else
                self->monsterinfo.lefty = brandom();

            // call the monster specific code here
            if (self->monsterinfo.sidestep(self)) {
                // if we are currently ducked, unduck
                if ((ducker) && (self->monsterinfo.aiflags & AI_DUCKED))
                    self->monsterinfo.unduck(self);

                self->monsterinfo.aiflags |= AI_DODGING;
                self->monsterinfo.attack_state = AS_SLIDING;

                self->monsterinfo.dodge_time = level.time + random_time_sec(0.4f, 2.0f);
            }
            return;
        }
    }

    // [Paril-KEX] we don't need to duck until projectiles are going to hit us very
    // soon.
    if (ducker && tr && eta < SEC(0.5f)) {
        if (self->monsterinfo.next_duck_time > level.time)
            return;

        monster_done_dodge(self);

        if (self->monsterinfo.duck(self, eta)) {
            // if duck didn't set us yet, do it now
            if (self->monsterinfo.duck_wait_time < level.time)
                self->monsterinfo.duck_wait_time = level.time + eta;

            monster_duck_down(self);

            // on Easy & Normal mode, duck longer
            if (skill->integer == 0)
                self->monsterinfo.duck_wait_time += random_time_sec(0.5f, 1.0f);
            else if (skill->integer == 1)
                self->monsterinfo.duck_wait_time += random_time_sec(0.1f, 0.35f);
        }

        self->monsterinfo.dodge_time = level.time + random_time_sec(0.2f, 0.7f);
    }
}

void monster_duck_down(edict_t *self)
{
    self->monsterinfo.aiflags |= AI_DUCKED;

    self->maxs[2] = self->monsterinfo.base_height - 32;
    self->takedamage = true;
    self->monsterinfo.next_duck_time = level.time + DUCK_INTERVAL;
    gi.linkentity(self);
}

void monster_duck_hold(edict_t *self)
{
    if (level.time >= self->monsterinfo.duck_wait_time)
        self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
    else
        self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

void MONSTERINFO_UNDUCK(monster_duck_up)(edict_t *self)
{
    if (!(self->monsterinfo.aiflags & AI_DUCKED))
        return;

    self->monsterinfo.aiflags &= ~AI_DUCKED;
    self->maxs[2] = self->monsterinfo.base_height;
    self->takedamage = true;
    // we finished a duck-up successfully, so cut the time remaining in half
    if (self->monsterinfo.next_duck_time > level.time)
        self->monsterinfo.next_duck_time = level.time + ((self->monsterinfo.next_duck_time - level.time) / 2);
    gi.linkentity(self);
}

bool has_valid_enemy(edict_t *self)
{
    if (!self->enemy)
        return false;

    if (!self->enemy->inuse)
        return false;

    if (self->enemy->health < 1)
        return false;

    return true;
}

void TargetTesla(edict_t *self, edict_t *tesla)
{
    if ((!self) || (!tesla))
        return;

    // PMM - medic bails on healing things
    if (self->monsterinfo.aiflags & AI_MEDIC) {
        if (self->enemy)
            cleanupHealTarget(self->enemy);
        self->monsterinfo.aiflags &= ~AI_MEDIC;
    }

    // store the player enemy in case we lose track of him.
    if (self->enemy && self->enemy->client)
        self->monsterinfo.last_player_enemy = self->enemy;

    if (self->enemy != tesla) {
        self->oldenemy = self->enemy;
        self->enemy = tesla;
        if (self->monsterinfo.attack) {
            if (self->health <= 0)
                return;

            self->monsterinfo.attack(self);
        } else
            FoundTarget(self);
    }
}

// this returns a randomly selected coop player who is visible to self
// returns NULL if bad

edict_t *PickCoopTarget(edict_t *self)
{
    edict_t *targets[MAX_CLIENTS];
    int      num_targets = 0;
    edict_t *ent;

    // if we're not in coop, this is a noop
    if (!coop->integer)
        return NULL;

    for (int player = 1; player <= game.maxclients; player++) {
        ent = &g_edicts[player];
        if (!ent->inuse)
            continue;
        if (!ent->client)
            continue;
        if (visible(self, ent))
            targets[num_targets++] = ent;
    }

    if (!num_targets)
        return NULL;

    return targets[irandom1(num_targets)];
}

// only meant to be used in coop
int CountPlayers(void)
{
    edict_t *ent;
    int      count = 0;

    // if we're not in coop, this is a noop
    if (!coop->integer)
        return 1;

    for (int player = 1; player <= game.maxclients; player++) {
        ent = &g_edicts[player];
        if (!ent->inuse)
            continue;
        if (!ent->client)
            continue;
        count++;
    }

    return count;
}

void THINK(BossExplode_think)(edict_t *self)
{
    // owner gone or changed
    if (!self->owner->inuse || self->owner->s.modelindex != self->style || self->count != self->owner->spawn_count) {
        G_FreeEdict(self);
        return;
    }

    vec3_t org;
    VectorAdd(self->owner->s.origin, self->owner->mins, org);
    VectorMA(org, frandom(), self->owner->size, org);

    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(!(self->viewheight % 3) ? TE_EXPLOSION1 : TE_EXPLOSION1_NL);
    gi.WritePosition(org);
    gi.multicast(org, MULTICAST_PVS);

    self->viewheight++;
    self->nextthink = level.time + random_time_sec(0.05f, 0.2f);
}

void BossExplode(edict_t *self)
{
    // no blowy on deady
    if (self->spawnflags & SPAWNFLAG_MONSTER_DEAD)
        return;

    edict_t *exploder = G_Spawn();
    exploder->owner = self;
    exploder->count = self->spawn_count;
    exploder->style = self->s.modelindex;
    exploder->think = BossExplode_think;
    exploder->nextthink = level.time + random_time_sec(0.075f, 0.25f);
    exploder->viewheight = 0;
}
