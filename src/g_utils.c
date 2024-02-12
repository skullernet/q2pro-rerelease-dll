// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_utils.c -- misc utility functions for game module

#include "g_local.h"

void G_ProjectSource(const vec3_t point, const vec3_t distance, const vec3_t forward, const vec3_t right, vec3_t result)
{
    result[0] = point[0] + forward[0] * distance[0] + right[0] * distance[1];
    result[1] = point[1] + forward[1] * distance[0] + right[1] * distance[1];
    result[2] = point[2] + forward[2] * distance[0] + right[2] * distance[1] + distance[2];
}

void G_ProjectSource2(const vec3_t point, const vec3_t distance, const vec3_t forward, const vec3_t right, const vec3_t up, vec3_t result)
{
    result[0] = point[0] + forward[0] * distance[0] + right[0] * distance[1] + up[0] * distance[2];
    result[1] = point[1] + forward[1] * distance[0] + right[1] * distance[1] + up[1] * distance[2];
    result[2] = point[2] + forward[2] * distance[0] + right[2] * distance[1] + up[2] * distance[2];
}

void closest_point_to_box(const vec3_t from, const vec3_t mins, const vec3_t maxs, vec3_t point)
{
    point[0] = Q_clipf(from[0], mins[0], maxs[0]);
    point[1] = Q_clipf(from[1], mins[1], maxs[1]);
    point[2] = Q_clipf(from[2], mins[2], maxs[2]);
}

float distance_between_boxes(const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2)
{
    float len = 0;

    for (int i = 0; i < 3; i++) {
        if (maxs1[i] < mins2[i]) {
            float d = maxs1[i] - mins2[i];
            len += d * d;
        } else if (mins1[i] > maxs2[i]) {
            float d = mins1[i] - maxs2[i];
            len += d * d;
        }
    }

    return sqrtf(len);
}

bool boxes_intersect(const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2)
{
    for (int i = 0; i < 3; i++) {
        if (mins1[i] > maxs2[i])
            return false;
        if (maxs1[i] < mins2[i])
            return false;
    }

    return true;
}

/*
=============
G_Find

Searches all active entities for the next one that validates the given callback.

Searches beginning at the edict after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.
=============
*/
edict_t *G_Find(edict_t *from, int fieldofs, const char *match)
{
    char    *s;

    if (!from)
        from = g_edicts;
    else
        from++;

    for (; from < &g_edicts[globals.num_edicts]; from++) {
        if (!from->inuse)
            continue;
        s = *(char **)((byte *)from + fieldofs);
        if (!s)
            continue;
        if (!Q_stricmp(s, match))
            return from;
    }

    return NULL;
}

/*
=================
findradius

Returns entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
edict_t *findradius(edict_t *from, const vec3_t org, float rad)
{
    vec3_t eorg;
    vec3_t mid;

    if (!from)
        from = g_edicts;
    else
        from++;
    for (; from < &g_edicts[globals.num_edicts]; from++) {
        if (!from->inuse)
            continue;
        if (from->solid == SOLID_NOT)
            continue;
        VectorAvg(from->mins, from->maxs, mid);
        VectorAdd(from->s.origin, mid, eorg);
        if (Distance(eorg, org) > rad)
            continue;
        return from;
    }

    return NULL;
}

/*
=============
G_PickTarget

Searches all active entities for the next one that holds
the matching string at fieldofs in the structure.

Searches beginning at the edict after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.

=============
*/
#define MAXCHOICES  8

edict_t *G_PickTarget(const char *targetname)
{
    edict_t *ent = NULL;
    int      num_choices = 0;
    edict_t *choice[MAXCHOICES];

    if (!targetname) {
        gi.dprintf("G_PickTarget called with NULL targetname\n");
        return NULL;
    }

    while (1) {
        ent = G_Find(ent, FOFS(targetname), targetname);
        if (!ent)
            break;
        choice[num_choices++] = ent;
        if (num_choices == MAXCHOICES)
            break;
    }

    if (!num_choices) {
        gi.dprintf("G_PickTarget: target %s not found\n", targetname);
        return NULL;
    }

    return choice[irandom1(num_choices)];
}

void THINK(Think_Delay)(edict_t *ent)
{
    G_UseTargets(ent, ent->activator);
    G_FreeEdict(ent);
}

void G_PrintActivationMessage(edict_t *ent, edict_t *activator, bool coop_global)
{
    //
    // print the message
    //
    if ((ent->message) && !(activator->svflags & SVF_MONSTER)) {
        if (coop_global && coop->integer)
            gi.bprintf(PRINT_CENTER, "%s", ent->message);
        else
            gi.centerprintf(activator, "%s", ent->message);

        // [Paril-KEX] allow non-noisy centerprints
        if (ent->noise_index >= 0) {
            if (ent->noise_index)
                gi.sound(activator, CHAN_AUTO, ent->noise_index, 1, ATTN_NORM, 0);
            else
                gi.sound(activator, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
        }
    }
}

/*
==============================
G_UseTargets

the global "activator" should be set to the entity that initiated the firing.

If self.delay is set, a DelayedUse entity will be created that will actually
do the SUB_UseTargets after that many seconds have passed.

Centerprints any self.message to the activator.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function

==============================
*/
void G_UseTargets(edict_t *ent, edict_t *activator)
{
    edict_t *t;

    //
    // check for a delay
    //
    if (ent->delay) {
        // create a temp object to fire at a later time
        t = G_Spawn();
        t->classname = "DelayedUse";
        t->nextthink = level.time + SEC(ent->delay);
        t->think = Think_Delay;
        t->activator = activator;
        if (!activator)
            gi.dprintf("Think_Delay with no activator\n");
        t->message = ent->message;
        t->target = ent->target;
        t->killtarget = ent->killtarget;
        return;
    }

    //
    // print the message
    //
    G_PrintActivationMessage(ent, activator, true);

    //
    // kill killtargets
    //
    if (ent->killtarget) {
        t = NULL;
        while ((t = G_Find(t, FOFS(targetname), ent->killtarget))) {
            if (t->teammaster) {
                // PMM - if this entity is part of a chain, cleanly remove it
                if (t->flags & FL_TEAMSLAVE) {
                    for (edict_t *master = t->teammaster; master; master = master->teamchain) {
                        if (master->teamchain == t) {
                            master->teamchain = t->teamchain;
                            break;
                        }
                    }
                // [Paril-KEX] remove teammaster too
                } else if (t->flags & FL_TEAMMASTER) {
                    t->teammaster->flags &= ~FL_TEAMMASTER;

                    edict_t *new_master = t->teammaster->teamchain;

                    if (new_master) {
                        new_master->flags |= FL_TEAMMASTER;
                        new_master->flags &= ~FL_TEAMSLAVE;

                        for (edict_t *m = new_master; m; m = m->teamchain)
                            m->teammaster = new_master;
                    }
                }
            }

            // [Paril-KEX] if we killtarget a monster, clean up properly
            if ((t->svflags & SVF_MONSTER) && !t->deadflag &&
                !(t->monsterinfo.aiflags & AI_DO_NOT_COUNT) && !(t->spawnflags & SPAWNFLAG_MONSTER_DEAD))
                G_MonsterKilled(t);

            // PMM
            G_FreeEdict(t);

            if (!ent->inuse) {
                gi.dprintf("entity was removed while using killtargets\n");
                return;
            }
        }
    }

    //
    // fire targets
    //
    if (ent->target) {
        t = NULL;
        while ((t = G_Find(t, FOFS(targetname), ent->target))) {
            // doors fire area portals in a specific way
            if (!Q_strcasecmp(t->classname, "func_areaportal") &&
                (!Q_strcasecmp(ent->classname, "func_door") || !Q_strcasecmp(ent->classname, "func_door_rotating") ||
                 !Q_strcasecmp(ent->classname, "func_door_secret") || !Q_strcasecmp(ent->classname, "func_water")))
                continue;

            if (t == ent)
                gi.dprintf("WARNING: Entity used itself.\n");
            else if (t->use)
                t->use(t, ent, activator);

            if (!ent->inuse) {
                gi.dprintf("entity was removed while using targets\n");
                return;
            }
        }
    }
}

char *etos(edict_t *ent)
{
    if (ent->area.next) {
        vec3_t mid;
        VectorAvg(ent->absmin, ent->absmax, mid);
        return va("%s @ %s", ent->classname, vtos(mid));
    }
    return va("%s @ %s", ent->classname, vtos(ent->s.origin));
}

static const vec3_t VEC_UP = { 0, -1, 0 };
static const vec3_t MOVEDIR_UP = { 0, 0, 1 };
static const vec3_t VEC_DOWN = { 0, -2, 0 };
static const vec3_t MOVEDIR_DOWN = { 0, 0, -1 };

void G_SetMovedir(vec3_t angles, vec3_t movedir)
{
    if (VectorCompare(angles, VEC_UP))
        VectorCopy(MOVEDIR_UP, movedir);
    else if (VectorCompare(angles, VEC_DOWN))
        VectorCopy(MOVEDIR_DOWN, movedir);
    else
        AngleVectors(angles, movedir, NULL, NULL);

    VectorClear(angles);
}

float vectoyaw(const vec3_t vec)
{
    // PMM - fixed to correct for pitch of 0
    if (vec[PITCH] == 0) {
        if (vec[YAW] == 0)
            return 0;
        else if (vec[YAW] > 0)
            return 90;
        else
            return 270;
    }

    float yaw = RAD2DEG(atan2(vec[YAW], vec[PITCH]));

    if (yaw < 0)
        yaw += 360;

    return yaw;
}

void vectoangles(const vec3_t value1, vec3_t angles)
{
    float   forward;
    float   yaw, pitch;

    if (value1[1] == 0 && value1[0] == 0) {
        yaw = 0;
        if (value1[2] > 0)
            pitch = 90;
        else
            pitch = 270;
    } else {
        if (value1[0])
            yaw = RAD2DEG(atan2(value1[1], value1[0]));
        else if (value1[1] > 0)
            yaw = 90;
        else
            yaw = 270;
        if (yaw < 0)
            yaw += 360;

        forward = sqrtf(value1[0] * value1[0] + value1[1] * value1[1]);
        pitch = RAD2DEG(atan2(value1[2], forward));
        if (pitch < 0)
            pitch += 360;
    }

    angles[PITCH] = -pitch;
    angles[YAW] = yaw;
    angles[ROLL] = 0;
}

char *G_CopyString(const char *in, int tag)
{
    if (!in)
        return NULL;
    size_t len = strlen(in) + 1;
    char *out = gi.TagMalloc(len, tag);
    memcpy(out, in, len);
    return out;
}

void G_InitEdict(edict_t *e)
{
    // ROGUE
    // FIXME -
    //   this fixes a bug somewhere that is setting "nextthink" for an entity that has
    //   already been released.  nextthink is being set to FRAME_TIME after level.time,
    //   since freetime = nextthink - FRAME_TIME
    if (e->nextthink)
        e->nextthink = 0;
    // ROGUE

    e->inuse = qtrue;
    e->classname = "noclass";
    e->gravity = 1.0f;
    e->s.number = e - g_edicts;

    // PGM - do this before calling the spawn function so it can be overridden.
    VectorSet(e->gravityVector, 0, 0, -1);
    // PGM
}

/*
=================
G_Spawn

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_t *G_Spawn(void)
{
    int      i;
    edict_t *e;

    e = &g_edicts[game.maxclients + 1];
    for (i = game.maxclients + 1; i < globals.num_edicts; i++, e++) {
        // the first couple seconds of server time can involve a lot of
        // freeing and allocating, so relax the replacement policy
        if (!e->inuse && (e->freetime < SEC(2) || level.time - e->freetime > SEC(0.5f))) {
            G_InitEdict(e);
            return e;
        }
    }

    if (i == game.maxentities)
        gi.error("ED_Alloc: no free edicts");

    globals.num_edicts++;
    G_InitEdict(e);
    return e;
}

/*
=================
G_FreeEdict

Marks the edict as free
=================
*/
void THINK(G_FreeEdict)(edict_t *ed)
{
    // already freed
    if (!ed->inuse)
        return;

    gi.unlinkentity(ed); // unlink from world

    if ((ed - g_edicts) <= (game.maxclients + BODY_QUEUE_SIZE))
        return;

    int id = ed->spawn_count + 1;
    memset(ed, 0, sizeof(*ed));
    ed->s.number = ed - g_edicts;
    ed->classname = "freed";
    ed->freetime = level.time;
    ed->inuse = qfalse;
    ed->spawn_count = id;
}

/*
============
G_TouchTriggers

============
*/
void G_TouchTriggers(edict_t *ent)
{
    int      i, num;
    edict_t *touch[MAX_EDICTS_OLD];
    edict_t *hit;

    // dead things don't activate triggers!
    if ((ent->client || (ent->svflags & SVF_MONSTER)) && (ent->health <= 0))
        return;

    num = gi.BoxEdicts(ent->absmin, ent->absmax, touch, q_countof(touch), AREA_TRIGGERS);

    // be careful, it is possible to have an entity in this
    // list removed before we get to it (killtriggered)
    for (i = 0; i < num; i++) {
        hit = touch[i];
        if (!hit->inuse)
            continue;
        if (!hit->touch)
            continue;
        hit->touch(hit, ent, &null_trace, true);
    }
}

typedef struct {
    edict_t     *projectile;
    int          spawn_count;
} skipped_projectile_t;

// [Paril-KEX] scan for projectiles between our movement positions
// to see if we need to collide against them
void G_TouchProjectiles(edict_t *ent, const vec3_t previous_origin)
{
    // a bit ugly, but we'll store projectiles we are ignoring here.
    skipped_projectile_t skipped[MAX_EDICTS_OLD], *skip;
    int num_skipped = 0;

    while (num_skipped < q_countof(skipped)) {
        trace_t tr = gi.trace(previous_origin, ent->mins, ent->maxs, ent->s.origin, ent, ent->clipmask | CONTENTS_PROJECTILE);

        if (tr.fraction == 1.0f)
            break;
        if (!(tr.ent->svflags & SVF_PROJECTILE))
            break;

        // always skip this projectile since certain conditions may cause the projectile
        // to not disappear immediately
        tr.ent->svflags &= ~SVF_PROJECTILE;

        skip = &skipped[num_skipped++];
        skip->projectile = tr.ent;
        skip->spawn_count = tr.ent->spawn_count;

        // if we're both players and it's coop, allow the projectile to "pass" through
        if (ent->client && tr.ent->owner && tr.ent->owner->client && !G_ShouldPlayersCollide(true))
            continue;

        G_Impact(ent, &tr);
    }

    for (int i = 0; i < num_skipped; i++) {
        skip = &skipped[i];
        if (skip->projectile->inuse && skip->projectile->spawn_count == skip->spawn_count)
            skip->projectile->svflags |= SVF_PROJECTILE;
    }
}

/*
==============================================================================

Kill box

==============================================================================
*/

/*
=================
KillBox

Kills all entities that would touch the proposed new positioning
of ent.
=================
*/
bool KillBox(edict_t *ent, bool from_spawning, mod_id_t mod, bool bsp_clipping)
{
    // don't telefrag as spectator...
    if (ent->movetype == MOVETYPE_NOCLIP)
        return true;

    contents_t mask = CONTENTS_MONSTER | CONTENTS_PLAYER;

    // [Paril-KEX] don't gib other players in coop if we're not colliding
    if (from_spawning && ent->client && coop->integer && !G_ShouldPlayersCollide(false))
        mask &= ~CONTENTS_PLAYER;

    int      i, num;
    edict_t *touch[MAX_EDICTS_OLD];
    edict_t *hit;

    num = gi.BoxEdicts(ent->absmin, ent->absmax, touch, q_countof(touch), AREA_SOLID);

    for (i = 0; i < num; i++) {
        hit = touch[i];

        if (hit == ent)
            continue;
        if (!hit->inuse || !hit->takedamage || !hit->solid || hit->solid == SOLID_TRIGGER || hit->solid == SOLID_BSP)
            continue;
        if (hit->client && !(mask & CONTENTS_PLAYER))
            continue;

        if ((ent->solid == SOLID_BSP || (ent->svflags & SVF_HULL)) && bsp_clipping) {
            trace_t clip = gix.clip(hit->s.origin, hit->mins, hit->maxs, hit->s.origin, ent, G_GetClipMask(hit));

            if (clip.fraction == 1.0f)
                continue;
        }

        // [Paril-KEX] don't allow telefragging of friends in coop.
        // the player that is about to be telefragged will have collision
        // disabled until another time.
        if (ent->client && hit->client && coop->integer) {
            hit->clipmask &= ~CONTENTS_PLAYER;
            ent->clipmask &= ~CONTENTS_PLAYER;
            continue;
        }

        T_Damage(hit, ent, ent, vec3_origin, ent->s.origin, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, (mod_t) { mod });
    }

    return true; // all clear
}
