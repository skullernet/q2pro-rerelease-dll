// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"

//
// ROGUE
//

//
// Monster spawning code
//
// Used by the carrier, the medic_commander, and the black widow
//
// The sequence to create a flying monster is:
//
//  FindSpawnPoint - tries to find suitable spot to spawn the monster in
//  CreateFlyMonster  - this verifies the point as good and creates the monster

// To create a ground walking monster:
//
//  FindSpawnPoint - same thing
//  CreateGroundMonster - this checks the volume and makes sure the floor under the volume is suitable
//

// FIXME - for the black widow, if we want the stalkers coming in on the roof, we'll have to tweak some things

//
// CreateMonster
//
edict_t *CreateMonster(const vec3_t origin, const vec3_t angles, const char *classname)
{
    edict_t *newEnt;

    newEnt = G_Spawn();

    VectorCopy(origin, newEnt->s.origin);
    VectorCopy(angles, newEnt->s.angles);
    newEnt->classname = classname;
    newEnt->monsterinfo.aiflags |= AI_DO_NOT_COUNT;

    ED_InitSpawnVars();
    ED_CallSpawn(newEnt);
    newEnt->s.renderfx |= RF_IR_VISIBLE;

    return newEnt;
}

edict_t *CreateFlyMonster(const vec3_t origin, const vec3_t angles, const vec3_t mins, const vec3_t maxs, const char *classname)
{
    if (!CheckSpawnPoint(origin, mins, maxs))
        return NULL;

    return CreateMonster(origin, angles, classname);
}

// This is just a wrapper for CreateMonster that looks down height # of CMUs and sees if there
// are bad things down there or not

edict_t *CreateGroundMonster(const vec3_t origin, const vec3_t angles, const vec3_t entMins, const vec3_t entMaxs, const char *classname, float height)
{
    // check the ground to make sure it's there, it's relatively flat, and it's not toxic
    if (!CheckGroundSpawnPoint(origin, entMins, entMaxs, height, -1))
        return NULL;

    return CreateMonster(origin, angles, classname);
}

// FindSpawnPoint
// PMM - this is used by the medic commander (possibly by the carrier) to find a good spawn point
// if the startpoint is bad, try above the startpoint for a bit

bool FindSpawnPoint(const vec3_t startpoint, const vec3_t mins, const vec3_t maxs, vec3_t spawnpoint, float maxMoveUp, bool drop)
{
    VectorCopy(startpoint, spawnpoint);

    // drop first
    if (drop && M_droptofloor_generic(spawnpoint, mins, maxs, false, NULL, MASK_MONSTERSOLID, false))
        return true;

    VectorCopy(startpoint, spawnpoint);

    // fix stuck if we couldn't drop initially
    if (G_FixStuckObject_Generic(spawnpoint, mins, maxs, NULL, MASK_MONSTERSOLID) == NO_GOOD_POSITION)
        return false;

    // fixed, so drop again
    if (drop && !M_droptofloor_generic(spawnpoint, mins, maxs, false, NULL, MASK_MONSTERSOLID, false))
        return false; // ???

    return true;
}

// FIXME - all of this needs to be tweaked to handle the new gravity rules
// if we ever want to spawn stuff on the roof

//
// CheckSpawnPoint
//
// PMM - checks volume to make sure we can spawn a monster there (is it solid?)
//
// This is all fliers should need

bool CheckSpawnPoint(const vec3_t origin, const vec3_t mins, const vec3_t maxs)
{
    trace_t tr;

    if (VectorEmpty(mins) || VectorEmpty(maxs))
        return false;

    tr = gi.trace(origin, mins, maxs, origin, NULL, MASK_MONSTERSOLID);
    if (tr.startsolid || tr.allsolid)
        return false;

    if (tr.ent != world)
        return false;

    return true;
}

//
// CheckGroundSpawnPoint
//
// PMM - used for walking monsters
//  checks:
//      1)  is there a ground within the specified height of the origin?
//      2)  is the ground non-water?
//      3)  is the ground flat enough to walk on?
//

bool CheckGroundSpawnPoint(const vec3_t origin, const vec3_t entMins, const vec3_t entMaxs, float height, float gravity)
{
    vec3_t absmin, absmax;

    if (!CheckSpawnPoint(origin, entMins, entMaxs))
        return false;

    VectorAdd(origin, entMins, absmin);
    VectorAdd(origin, entMaxs, absmax);

    if (M_CheckBottom_Fast_Generic(absmin, absmax, false))
        return true;

    if (M_CheckBottom_Slow_Generic(origin, entMins, entMaxs, NULL, MASK_MONSTERSOLID, false, false))
        return true;

    return false;
}

// ****************************
// SPAWNGROW stuff
// ****************************

#define SPAWNGROW_LIFESPAN_SEC  1
#define SPAWNGROW_LIFESPAN      SEC(SPAWNGROW_LIFESPAN_SEC)

void THINK(spawngrow_think)(edict_t *self)
{
    if (level.time >= self->timestamp) {
        G_FreeEdict(self->target_ent);
        G_FreeEdict(self);
        return;
    }

    VectorMA(self->s.angles, FRAME_TIME_SEC, self->avelocity, self->s.angles);

    float t = 1.0f - TO_SEC(level.time - self->teleport_time) / self->wait;
    float s = lerp(self->decel, self->accel, t) / 16;

    self->x.scale = Q_clipf(s, 0.001f, 16);
    self->x.alpha = t * t;

    self->nextthink += FRAME_TIME;
}

static void SpawnGro_laser_pos(edict_t *ent, vec3_t pos)
{
    // pick random direction
    float theta = frandom1(2 * M_PI);
    float phi = acos(crandom());

    vec3_t d = {
        sin(phi) * cos(theta),
        sin(phi) * sin(theta),
        cos(phi)
    };

    VectorMA(ent->s.origin, ent->owner->x.scale * 9, d, pos);
}

void THINK(SpawnGro_laser_think)(edict_t *self)
{
    SpawnGro_laser_pos(self, self->s.old_origin);
    gi.linkentity(self);
    self->nextthink = level.time + FRAME_TIME;
}

void SpawnGrow_Spawn(const vec3_t startpos, float start_size, float end_size)
{
    edict_t *ent;

    ent = G_Spawn();
    VectorCopy(startpos, ent->s.origin);

    ent->s.angles[0] = irandom1(360);
    ent->s.angles[1] = irandom1(360);
    ent->s.angles[2] = irandom1(360);

    ent->avelocity[0] = frandom2(280, 360) * 2;
    ent->avelocity[1] = frandom2(280, 360) * 2;
    ent->avelocity[2] = frandom2(280, 360) * 2;

    ent->solid = SOLID_NOT;
    ent->s.renderfx |= RF_IR_VISIBLE;
    ent->movetype = MOVETYPE_NONE;
    ent->classname = "spawngro";

    ent->s.modelindex = gi.modelindex("models/items/spawngro3/tris.md2");
    ent->s.skinnum = 1;

    ent->accel = start_size;
    ent->decel = end_size;
    ent->think = spawngrow_think;

    ent->x.scale = Q_clipf(start_size / 16, 0.001f, 16);

    ent->teleport_time = level.time;
    ent->wait = SPAWNGROW_LIFESPAN_SEC;
    ent->timestamp = level.time + SPAWNGROW_LIFESPAN;

    ent->nextthink = level.time + FRAME_TIME;

    gi.linkentity(ent);

    // [Paril-KEX]
    edict_t *beam = ent->target_ent = G_Spawn();
    beam->s.modelindex = MODELINDEX_WORLD;
    beam->s.renderfx = RF_BEAM_LIGHTNING | RF_FRAMELERP;
    beam->s.frame = 1;
    beam->s.skinnum = 0x30303030;
    beam->classname = "spawngro_beam";
    beam->angle = end_size;
    beam->owner = ent;
    VectorCopy(ent->s.origin, beam->s.origin);
    beam->think = SpawnGro_laser_think;
    beam->nextthink = level.time + FRAME_TIME;
    SpawnGro_laser_pos(beam, beam->s.old_origin);
    gi.linkentity(beam);
}

// ****************************
// WidowLeg stuff
// ****************************

#define MAX_LEGSFRAME   23
#define LEG_WAIT_TIME   SEC(1)

void ThrowMoreStuff(edict_t *self, const vec3_t point);
void ThrowSmallStuff(edict_t *self, const vec3_t point);
void ThrowWidowGibLoc(edict_t *self, const char *gibname, int damage, gib_type_t type, const vec3_t startpos, bool fade);
void ThrowWidowGibSized(edict_t *self, const char *gibname, int damage, gib_type_t type, const vec3_t startpos, int hitsound, bool fade);

void THINK(widowlegs_think)(edict_t *self)
{
    vec3_t offset;
    vec3_t point;
    vec3_t f, r, u;

    if (self->s.frame == 17) {
        VectorSet(offset, 11.77f, -7.24f, 23.31f);
        AngleVectors(self->s.angles, f, r, u);
        G_ProjectSource2(self->s.origin, offset, f, r, u, point);
        gi.WriteByte(svc_temp_entity);
        gi.WriteByte(TE_EXPLOSION1);
        gi.WritePosition(point);
        gi.multicast(point, MULTICAST_ALL);
        ThrowSmallStuff(self, point);
    }

    if (self->s.frame < MAX_LEGSFRAME) {
        self->s.frame++;
        self->nextthink = level.time + HZ(10);
        return;
    }

    if (self->timestamp == 0)
        self->timestamp = level.time + LEG_WAIT_TIME;

    if (level.time > self->timestamp) {
        AngleVectors(self->s.angles, f, r, u);

        VectorSet(offset, -65.6f, -8.44f, 28.59f);
        G_ProjectSource2(self->s.origin, offset, f, r, u, point);
        gi.WriteByte(svc_temp_entity);
        gi.WriteByte(TE_EXPLOSION1);
        gi.WritePosition(point);
        gi.multicast(point, MULTICAST_ALL);
        ThrowSmallStuff(self, point);

        ThrowWidowGibSized(self, "models/monsters/blackwidow/gib1/tris.md2", 80 + frandom1(20.0f), GIB_METALLIC, point, 0, true);
        ThrowWidowGibSized(self, "models/monsters/blackwidow/gib2/tris.md2", 80 + frandom1(20.0f), GIB_METALLIC, point, 0, true);

        VectorSet(offset, -1.04f, -51.18f, 7.04f);
        G_ProjectSource2(self->s.origin, offset, f, r, u, point);
        gi.WriteByte(svc_temp_entity);
        gi.WriteByte(TE_EXPLOSION1);
        gi.WritePosition(point);
        gi.multicast(point, MULTICAST_ALL);
        ThrowSmallStuff(self, point);

        ThrowWidowGibSized(self, "models/monsters/blackwidow/gib1/tris.md2", 80 + frandom1(20.0f), GIB_METALLIC, point, 0, true);
        ThrowWidowGibSized(self, "models/monsters/blackwidow/gib2/tris.md2", 80 + frandom1(20.0f), GIB_METALLIC, point, 0, true);
        ThrowWidowGibSized(self, "models/monsters/blackwidow/gib3/tris.md2", 80 + frandom1(20.0f), GIB_METALLIC, point, 0, true);

        G_FreeEdict(self);
        return;
    }

    if ((level.time > self->timestamp - SEC(0.5f)) && (self->count == 0)) {
        self->count = 1;
        AngleVectors(self->s.angles, f, r, u);

        VectorSet(offset, 31, -88.7f, 10.96f);
        G_ProjectSource2(self->s.origin, offset, f, r, u, point);
        gi.WriteByte(svc_temp_entity);
        gi.WriteByte(TE_EXPLOSION1);
        gi.WritePosition(point);
        gi.multicast(point, MULTICAST_ALL);

        VectorSet(offset, -12.67f, -4.39f, 15.68f);
        G_ProjectSource2(self->s.origin, offset, f, r, u, point);
        gi.WriteByte(svc_temp_entity);
        gi.WriteByte(TE_EXPLOSION1);
        gi.WritePosition(point);
        gi.multicast(point, MULTICAST_ALL);
    }

    self->nextthink = level.time + HZ(10);
}

void Widowlegs_Spawn(const vec3_t startpos, const vec3_t angles)
{
    edict_t *ent;

    ent = G_Spawn();
    VectorCopy(startpos, ent->s.origin);
    VectorCopy(angles, ent->s.angles);
    ent->solid = SOLID_NOT;
    ent->s.renderfx = RF_IR_VISIBLE;
    ent->movetype = MOVETYPE_NONE;
    ent->classname = "widowlegs";

    ent->s.modelindex = gi.modelindex("models/monsters/legs/tris.md2");
    ent->think = widowlegs_think;

    ent->nextthink = level.time + HZ(10);
    gi.linkentity(ent);
}
