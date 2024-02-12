// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_sphere.c
// pmack
// april 1998

// defender - actively finds and shoots at enemies
// hunter - waits until < 25% health and vore ball tracks person who hurt you
// vengeance - kills person who killed you.

#include "g_local.h"

#define DEFENDER_LIFESPAN   SEC(30)
#define HUNTER_LIFESPAN     SEC(30)
#define VENGEANCE_LIFESPAN  SEC(30)
#define MINIMUM_FLY_TIME    SEC(15)

void vengeance_touch(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self);
void hunter_touch(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self);

// *************************
// General Sphere Code
// *************************

void THINK(sphere_think_explode)(edict_t *self)
{
    if (self->owner && self->owner->client && !(self->spawnflags & SPHERE_DOPPLEGANGER))
        self->owner->client->owned_sphere = NULL;

    BecomeExplosion1(self);
}

void DIE(sphere_explode)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    sphere_think_explode(self);
}

// if the sphere is not currently attacking, blow up.
void DIE(sphere_if_idle_die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t point, mod_t mod)
{
    if (!self->enemy)
        sphere_think_explode(self);
}

// *************************
// Sphere Movement
// *************************

static void sphere_fly(edict_t *self)
{
    vec3_t dest;
    vec3_t dir;

    if (level.time >= self->timestamp) {
        sphere_think_explode(self);
        return;
    }

    VectorCopy(self->owner->s.origin, dest);
    dest[2] = self->owner->absmax[2] + 4;

    if (!(level.time % HZ(1)) && !visible(self, self->owner)) {
        VectorCopy(dest, self->s.origin);
        gi.linkentity(self);
        return;
    }

    VectorSubtract(dest, self->s.origin, dir);
    VectorScale(dir, 5, self->velocity);
}

static void sphere_chase(edict_t *self, int stupidChase)
{
    vec3_t dest;
    vec3_t dir;
    float  dist;

    if ((level.time >= self->timestamp) || (self->enemy && self->enemy->health < 1)) {
        sphere_think_explode(self);
        return;
    }

    VectorCopy(self->enemy->s.origin, dest);
    if (self->enemy->client)
        dest[2] += self->enemy->viewheight;

    if (visible(self, self->enemy) || stupidChase) {
        // if moving, hunter sphere uses active sound
        if (!stupidChase)
            self->s.sound = gi.soundindex("spheres/h_active.wav");

        VectorSubtract(dest, self->s.origin, dir);
        VectorNormalize(dir);
        vectoangles(dir, self->s.angles);
        VectorScale(dir, 500, self->velocity);
        VectorCopy(dest, self->monsterinfo.saved_goal);
    } else if (VectorEmpty(self->monsterinfo.saved_goal)) {
        VectorSubtract(self->enemy->s.origin, self->s.origin, dir);
        vectoangles(dir, self->s.angles);

        // if lurking, hunter sphere uses lurking sound
        self->s.sound = gi.soundindex("spheres/h_lurk.wav");
        VectorClear(self->velocity);
    } else {
        VectorSubtract(self->monsterinfo.saved_goal, self->s.origin, dir);
        dist = VectorNormalize(dir);

        if (dist > 1) {
            vectoangles(dir, self->s.angles);

            if (dist > 500)
                VectorScale(dir, 500, self->velocity);
            else if (dist < 20)
                VectorScale(dir, dist / FRAME_TIME_SEC, self->velocity);
            else
                VectorScale(dir, dist, self->velocity);

            // if moving, hunter sphere uses active sound
            if (!stupidChase)
                self->s.sound = gi.soundindex("spheres/h_active.wav");
        } else {
            VectorSubtract(self->enemy->s.origin, self->s.origin, dir);
            vectoangles(dir, self->s.angles);

            // if not moving, hunter sphere uses lurk sound
            if (!stupidChase)
                self->s.sound = gi.soundindex("spheres/h_lurk.wav");

            VectorClear(self->velocity);
        }
    }
}

// *************************
// Attack related stuff
// *************************

#if 0
static void sphere_fire(edict_t *self, edict_t *enemy)
{
    vec3_t dest;
    vec3_t dir;

    if (!enemy || level.time >= self->timestamp) {
        sphere_think_explode(self);
        return;
    }

    VectorCopy(enemy->s.origin, dest);
    self->s.effects |= EF_ROCKET;

    VectorSubtract(dest, self->s.origin, dir);
    VectorNormalize(dir);
    vectoangles(dir, self->s.angles);
    VectorScale(dir, 1000, self->velocity);

    self->touch = vengeance_touch;
    self->think = sphere_think_explode;
    self->nextthink = self->timestamp;
}
#endif

static void sphere_touch(edict_t *self, edict_t *other, const trace_t *tr, mod_t mod)
{
    if (self->spawnflags & SPHERE_DOPPLEGANGER) {
        if (other == self->teammaster)
            return;

        self->takedamage = false;
        self->owner = self->teammaster;
        self->teammaster = NULL;
    } else {
        if (other == self->owner)
            return;
        // PMM - don't blow up on bodies
        if (!strcmp(other->classname, "bodyque"))
            return;
    }

    if (tr->surface && (tr->surface->flags & SURF_SKY)) {
        G_FreeEdict(self);
        return;
    }

    if (self->owner) {
        if (other->takedamage) {
            T_Damage(other, self, self->owner, self->velocity, self->s.origin, tr->plane.normal,
                     10000, 1, DAMAGE_DESTROY_ARMOR, mod);
        } else {
            T_RadiusDamage(self, self->owner, 512, self->owner, 256, DAMAGE_NONE, mod);
        }
    }

    sphere_think_explode(self);
}

void TOUCH(vengeance_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if (self->spawnflags & SPHERE_DOPPLEGANGER)
        sphere_touch(self, other, tr, (mod_t) { MOD_DOPPLE_VENGEANCE });
    else
        sphere_touch(self, other, tr, (mod_t) { MOD_VENGEANCE_SPHERE });
}

void TOUCH(hunter_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    edict_t *owner;

    // don't blow up if you hit the world.... sheesh.
    if (other == world)
        return;

    if (self->owner) {
        // if owner is flying with us, make sure they stop too.
        owner = self->owner;
        if (owner->flags & FL_SAM_RAIMI) {
            VectorClear(owner->velocity);
            owner->movetype = MOVETYPE_NONE;
            gi.linkentity(owner);
        }
    }

    if (self->spawnflags & SPHERE_DOPPLEGANGER)
        sphere_touch(self, other, tr, (mod_t) { MOD_DOPPLE_HUNTER });
    else
        sphere_touch(self, other, tr, (mod_t) { MOD_HUNTER_SPHERE });
}

static void defender_shoot(edict_t *self, edict_t *enemy)
{
    vec3_t dir;
    vec3_t start;

    if (!enemy->inuse || enemy->health <= 0)
        return;

    if (enemy == self->owner)
        return;

    if (self->monsterinfo.attack_finished > level.time)
        return;

    if (!visible(self, self->enemy))
        return;

    VectorSubtract(enemy->s.origin, self->s.origin, dir);
    VectorNormalize(dir);

    VectorCopy(self->s.origin, start);
    start[2] += 2;
    fire_blaster2(self->owner, start, dir, 10, 1000, EF_BLASTER, 0);

    self->monsterinfo.attack_finished = level.time + SEC(0.4f);
}

// *************************
// Activation Related Stuff
// *************************

static const gib_def_t sphere_gibs[] = {
    { "models/objects/gibs/sm_meat/tris.md2", 4 },
    { "models/objects/gibs/skull/tris.md2", 1 },
    { 0 }
};

static void body_gib(edict_t *self)
{
    gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
    ThrowGibs(self, 50, sphere_gibs);
}

void PAIN(hunter_pain)(edict_t *self, edict_t *other, float kick, int damage, mod_t mod)
{
    edict_t *owner;
    float    dist;

    if (self->enemy)
        return;

    owner = self->owner;

    if (!(self->spawnflags & SPHERE_DOPPLEGANGER)) {
        if (owner && (owner->health > 0))
            return;
        // PMM
        if (other == owner)
            return;
        // pmm
        self->timestamp = max(self->timestamp, level.time + MINIMUM_FLY_TIME);
    } else {
        // if fired by a doppleganger, set it to 10 second timeout
        self->timestamp = level.time + MINIMUM_FLY_TIME;
    }

    self->s.effects |= EF_BLASTER | EF_TRACKER;
    self->touch = hunter_touch;
    self->enemy = other;

    // if we're not owned by a player, no sam raimi
    // if we're spawned by a doppleganger, no sam raimi
    if ((self->spawnflags & SPHERE_DOPPLEGANGER) || !owner || !owner->client)
        return;

    // sam raimi cam is disabled if FORCE_RESPAWN is set.
    // sam raimi cam is also disabled if huntercam->value is 0.
    if (g_dm_force_respawn->integer || !huntercam->integer)
        return;

    dist = Distance(other->s.origin, self->s.origin);
    if (dist < 192)
        return;

    // detach owner from body and send him flying
    owner->movetype = MOVETYPE_FLYMISSILE;

    // gib like we just died, even though we didn't, really.
    body_gib(owner);

    // move the sphere to the owner's current viewpoint.
    // we know it's a valid spot (or will be momentarily)
    VectorCopy(owner->s.origin, self->s.origin);
    self->s.origin[2] += owner->viewheight;

    // move the player's origin to the sphere's new origin
    VectorCopy(self->s.origin, owner->s.origin);
    VectorCopy(self->s.angles, owner->s.angles);
    VectorCopy(self->s.angles, owner->client->v_angle);
    VectorSet(owner->mins, -5, -5, -5);
    VectorSet(owner->maxs, 5, 5, 5);
    owner->client->ps.fov = 140;
    owner->s.modelindex = 0;
    owner->s.modelindex2 = 0;
    owner->viewheight = 8;
    owner->solid = SOLID_NOT;
    owner->flags |= FL_SAM_RAIMI;
    gi.linkentity(owner);

    self->solid = SOLID_BBOX;
    gi.linkentity(self);
}

void PAIN(defender_pain)(edict_t *self, edict_t *other, float kick, int damage, mod_t mod)
{
    // PMM
    if (other == self->owner)
        return;
    // pmm
    self->enemy = other;
}

void PAIN(vengeance_pain)(edict_t *self, edict_t *other, float kick, int damage, mod_t mod)
{
    if (self->enemy)
        return;

    if (!(self->spawnflags & SPHERE_DOPPLEGANGER)) {
        if (self->owner && self->owner->health >= 25)
            return;
        // PMM
        if (other == self->owner)
            return;
        // pmm
        self->timestamp = max(self->timestamp, level.time + MINIMUM_FLY_TIME);
    } else {
        self->timestamp = level.time + MINIMUM_FLY_TIME;
    }

    self->s.effects |= EF_ROCKET;
    self->touch = vengeance_touch;
    self->enemy = other;
}

// *************************
// Think Functions
// *************************

void THINK(defender_think)(edict_t *self)
{
    if (!self->owner) {
        G_FreeEdict(self);
        return;
    }

    // if we've exited the level, just remove ourselves.
    if (level.intermissiontime) {
        sphere_think_explode(self);
        return;
    }

    if (self->owner->health <= 0) {
        sphere_think_explode(self);
        return;
    }

    self->s.frame++;
    if (self->s.frame > 19)
        self->s.frame = 0;

    if (self->enemy) {
        if (self->enemy->health > 0)
            defender_shoot(self, self->enemy);
        else
            self->enemy = NULL;
    }

    sphere_fly(self);

    if (self->inuse)
        self->nextthink = level.time + HZ(10);
}

void THINK(hunter_think)(edict_t *self)
{
    // if we've exited the level, just remove ourselves.
    if (level.intermissiontime) {
        sphere_think_explode(self);
        return;
    }

    edict_t *owner = self->owner;

    if (!owner && !(self->spawnflags & SPHERE_DOPPLEGANGER)) {
        G_FreeEdict(self);
        return;
    }

    if (owner)
        self->ideal_yaw = owner->s.angles[YAW];
    else if (self->enemy) { // fired by doppleganger
        vec3_t dir;
        VectorSubtract(self->enemy->s.origin, self->s.origin, dir);
        self->ideal_yaw = vectoyaw(dir);
    }

    M_ChangeYaw(self);

    if (self->enemy) {
        sphere_chase(self, 0);

        // deal with sam raimi cam
        if (owner && (owner->flags & FL_SAM_RAIMI)) {
            if (self->inuse) {
                owner->movetype = MOVETYPE_FLYMISSILE;
                LookAtKiller(owner, self, self->enemy);
                // owner is flying with us, move him too
                owner->movetype = MOVETYPE_FLYMISSILE;
                owner->viewheight = self->s.origin[2] - owner->s.origin[2];
                VectorCopy(self->s.origin, owner->s.origin);
                VectorCopy(self->velocity, owner->velocity);
                VectorClear(owner->mins);
                VectorClear(owner->maxs);
                gi.linkentity(owner);
            } else { // sphere timed out
                VectorClear(owner->velocity);
                owner->movetype = MOVETYPE_NONE;
                gi.linkentity(owner);
            }
        }
    } else
        sphere_fly(self);

    if (self->inuse)
        self->nextthink = level.time + HZ(10);
}

void THINK(vengeance_think)(edict_t *self)
{
    // if we've exited the level, just remove ourselves.
    if (level.intermissiontime) {
        sphere_think_explode(self);
        return;
    }

    if (!(self->owner) && !(self->spawnflags & SPHERE_DOPPLEGANGER)) {
        G_FreeEdict(self);
        return;
    }

    if (self->enemy)
        sphere_chase(self, 1);
    else
        sphere_fly(self);

    if (self->inuse)
        self->nextthink = level.time + HZ(10);
}

// *************************
// Spawning / Creation
// *************************

edict_t *Sphere_Spawn(edict_t *owner, spawnflags_t spawnflags)
{
    edict_t *sphere;

    sphere = G_Spawn();
    VectorCopy(owner->s.origin, sphere->s.origin);
    sphere->s.origin[2] = owner->absmax[2];
    sphere->s.angles[YAW] = owner->s.angles[YAW];
    sphere->solid = SOLID_BBOX;
    sphere->clipmask = MASK_PROJECTILE;
    sphere->s.renderfx = RF_FULLBRIGHT | RF_IR_VISIBLE;
    sphere->movetype = MOVETYPE_FLYMISSILE;

    if (spawnflags & SPHERE_DOPPLEGANGER)
        sphere->teammaster = owner->teammaster;
    else
        sphere->owner = owner;

    sphere->classname = "sphere";
    sphere->yaw_speed = 40;
    sphere->monsterinfo.attack_finished = 0;
    sphere->spawnflags = spawnflags; // need this for the HUD to recognize sphere
    // PMM
    sphere->takedamage = false;

    switch (spawnflags & SPHERE_TYPE) {
    case SPHERE_DEFENDER:
        sphere->s.modelindex = gi.modelindex("models/items/defender/tris.md2");
        sphere->s.modelindex2 = gi.modelindex("models/items/shell/tris.md2");
        sphere->s.sound = gi.soundindex("spheres/d_idle.wav");
        sphere->pain = defender_pain;
        sphere->timestamp = level.time + DEFENDER_LIFESPAN;
        sphere->die = sphere_explode;
        sphere->think = defender_think;
        break;
    case SPHERE_HUNTER:
        sphere->s.modelindex = gi.modelindex("models/items/hunter/tris.md2");
        sphere->s.sound = gi.soundindex("spheres/h_idle.wav");
        sphere->timestamp = level.time + HUNTER_LIFESPAN;
        sphere->pain = hunter_pain;
        sphere->die = sphere_if_idle_die;
        sphere->think = hunter_think;
        break;
    case SPHERE_VENGEANCE:
        sphere->s.modelindex = gi.modelindex("models/items/vengnce/tris.md2");
        sphere->s.sound = gi.soundindex("spheres/v_idle.wav");
        sphere->timestamp = level.time + VENGEANCE_LIFESPAN;
        sphere->pain = vengeance_pain;
        sphere->die = sphere_if_idle_die;
        sphere->think = vengeance_think;
        VectorSet(sphere->avelocity, 30, 30, 0);
        break;
    default:
        gi.dprintf("Tried to create an invalid sphere\n");
        G_FreeEdict(sphere);
        return NULL;
    }

    sphere->nextthink = level.time + HZ(10);

    gi.linkentity(sphere);

    return sphere;
}

// =================
// Own_Sphere - attach the sphere to the client so we can
//      directly access it later
// =================
static void Own_Sphere(edict_t *self, edict_t *sphere)
{
    if (!sphere)
        return;

    // ownership only for players
    if (self->client) {
        // if they don't have one
        if (!self->client->owned_sphere) {
            self->client->owned_sphere = sphere;
        // they already have one, take care of the old one
        } else {
            if (self->client->owned_sphere->inuse) {
                G_FreeEdict(self->client->owned_sphere);
                self->client->owned_sphere = sphere;
            } else {
                self->client->owned_sphere = sphere;
            }
        }
    }
}

void Defender_Launch(edict_t *self)
{
    edict_t *sphere;

    sphere = Sphere_Spawn(self, SPHERE_DEFENDER);
    Own_Sphere(self, sphere);
}

void Hunter_Launch(edict_t *self)
{
    edict_t *sphere;

    sphere = Sphere_Spawn(self, SPHERE_HUNTER);
    Own_Sphere(self, sphere);
}

void Vengeance_Launch(edict_t *self)
{
    edict_t *sphere;

    sphere = Sphere_Spawn(self, SPHERE_VENGEANCE);
    Own_Sphere(self, sphere);
}
