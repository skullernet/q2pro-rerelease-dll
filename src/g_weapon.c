// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"

/*
=================
fire_hit

Used for all impact (hit/punch/slash) attacks
=================
*/
bool fire_hit(edict_t *self, vec3_t aim, int damage, int kick)
{
    trace_t tr;
    vec3_t  forward, right, up;
    vec3_t  v;
    vec3_t  point;
    float   range;
    vec3_t  dir;

    if (!self->enemy)
        return false;

    // see if enemy is in range
    range = distance_between_boxes(self->enemy->absmin, self->enemy->absmax, self->absmin, self->absmax);
    if (range > aim[0])
        return false;

    if (!(aim[1] > self->mins[0] && aim[1] < self->maxs[0])) {
        // this is a side hit so adjust the "right" value out to the edge of their bbox
        if (aim[1] < 0)
            aim[1] = self->enemy->mins[0];
        else
            aim[1] = self->enemy->maxs[0];
    }

    closest_point_to_box(self->s.origin, self->enemy->absmin, self->enemy->absmax, point);

    // check that we can hit the point on the bbox
    tr = gi.trace(self->s.origin, NULL, NULL, point, self, MASK_PROJECTILE);

    if (tr.fraction < 1) {
        if (!tr.ent->takedamage)
            return false;
        // if it will hit any client/monster then hit the one we wanted to hit
        if ((tr.ent->svflags & SVF_MONSTER) || (tr.ent->client))
            tr.ent = self->enemy;
    }

    // check that we can hit the player from the point
    tr = gi.trace(point, NULL, NULL, self->enemy->s.origin, self, MASK_PROJECTILE);

    if (tr.fraction < 1) {
        if (!tr.ent->takedamage)
            return false;
        // if it will hit any client/monster then hit the one we wanted to hit
        if ((tr.ent->svflags & SVF_MONSTER) || (tr.ent->client))
            tr.ent = self->enemy;
    }

    AngleVectors(self->s.angles, forward, right, up);
    VectorMA(self->s.origin, range, forward, point);
    VectorMA(point, aim[1], right, point);
    VectorMA(point, aim[2], up, point);
    VectorSubtract(point, self->enemy->s.origin, dir);

    // do the damage
    T_Damage(tr.ent, self, self, dir, point, vec3_origin, damage, kick / 2, DAMAGE_NO_KNOCKBACK, (mod_t) { MOD_HIT });

    if (!(tr.ent->svflags & SVF_MONSTER) && (!tr.ent->client))
        return false;

    // do our special form of knockback here
    VectorAvg(self->enemy->absmin, self->enemy->absmax, v);
    VectorSubtract(v, point, v);
    VectorNormalize(v);
    VectorMA(self->enemy->velocity, kick, v, self->enemy->velocity);
    if (self->enemy->velocity[2] > 0)
        self->enemy->groundentity = NULL;
    return true;
}

static trace_t fire_lead_pierce(edict_t *self, const vec3_t start, const vec3_t end_, const vec3_t aimdir,
                                int damage, int kick, int te_impact, int hspread, int vspread, mod_t mod,
                                contents_t *mask, bool *water, vec3_t water_start)
{
    trace_t tr;
    pierce_t pierce;
    pierce_begin(&pierce);

    vec3_t end;
    VectorCopy(end_, end);
    while (1) {
        tr = gi.trace(start, NULL, NULL, end, self, *mask);

        // didn't hit anything, so we're done
        if (!tr.ent || tr.fraction == 1.0f)
            break;

        // see if we hit water
        if (tr.contents & MASK_WATER) {
            *water = true;
            VectorCopy(tr.endpos, water_start);

            // CHECK: is this compare ever true?
            if (te_impact != -1 && !VectorCompare(start, tr.endpos)) {
                int color;

                if (tr.contents & CONTENTS_WATER) {
                    // FIXME: this effectively does nothing..
                    if (strcmp(tr.surface->name, "brwater") == 0)
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
                vec3_t dir, forward, right, up;
                VectorSubtract(end, start, dir);
                vectoangles(dir, dir);
                AngleVectors(dir, forward, right, up);
                float r = crandom() * hspread * 2;
                float u = crandom() * vspread * 2;
                VectorMA(water_start, 8192, forward, end);
                VectorMA(end, r, right, end);
                VectorMA(end, u, up, end);
            }

            // re-trace ignoring water this time
            *mask &= ~MASK_WATER;
            continue;
        }

        // did we hit an hurtable entity?
        if (tr.ent->takedamage) {
            T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, mod.id == MOD_TESLA ? DAMAGE_ENERGY : DAMAGE_BULLET, mod);

            // only deadmonster is pierceable, or actual dead monsters
            // that haven't been made non-solid yet
            if ((tr.ent->svflags & SVF_DEADMONSTER) || (tr.ent->health <= 0 && (tr.ent->svflags & SVF_MONSTER))) {
                if (pierce_mark(&pierce, tr.ent))
                    continue;
            }
        } else {
            // send gun puff / flash
            // don't mark the sky
            if (te_impact != -1 && !(tr.surface && ((tr.surface->flags & SURF_SKY) || strncmp(tr.surface->name, "sky", 3) == 0))) {
                gi.WriteByte(svc_temp_entity);
                gi.WriteByte(te_impact);
                gi.WritePosition(tr.endpos);
                gi.WriteDir(tr.plane.normal);
                gi.multicast(tr.endpos, MULTICAST_PVS);

                if (self->client)
                    PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
            }
        }

        // hit a solid, so we're stopping here
        break;
    }

    pierce_end(&pierce);
    return tr;
}

/*
=================
fire_lead

This is an internal support routine used for bullet/pellet based weapons.
=================
*/
static void fire_lead(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread, mod_t mod)
{
    contents_t   mask = MASK_PROJECTILE | MASK_WATER;
    bool         water = false;
    vec3_t       water_start = { 0 };

    // [Paril-KEX]
    if (self->client && !G_ShouldPlayersCollide(true))
        mask &= ~CONTENTS_PLAYER;

    // special case: we started in water.
    if (gi.pointcontents(start) & MASK_WATER) {
        water = true;
        VectorCopy(start, water_start);
        mask &= ~MASK_WATER;
    }

    // check initial firing position
    trace_t tr = fire_lead_pierce(self, self->s.origin, start, aimdir, damage, kick, te_impact, hspread, vspread, mod, &mask, &water, water_start);

    // we're clear, so do the second pierce
    if (tr.fraction == 1.0f) {
        vec3_t end, dir, forward, right, up;
        vectoangles(aimdir, dir);
        AngleVectors(dir, forward, right, up);

        float r = crandom() * hspread;
        float u = crandom() * vspread;
        VectorMA(start, 8192, forward, end);
        VectorMA(end, r, right, end);
        VectorMA(end, u, up, end);

        tr = fire_lead_pierce(self, start, end, aimdir, damage, kick, te_impact, hspread, vspread, mod, &mask, &water, water_start);
    }

    // if went through water, determine where the end is and make a bubble trail
    if (water && te_impact != -1) {
        vec3_t pos, dir;

        VectorSubtract(tr.endpos, water_start, dir);
        VectorNormalize(dir);
        VectorMA(tr.endpos, -2, dir, pos);
        if (gi.pointcontents(pos) & MASK_WATER)
            VectorCopy(pos, tr.endpos);
        else
            tr = gi.trace(pos, NULL, NULL, water_start, tr.ent != world ? tr.ent : NULL, MASK_WATER);

        VectorAvg(water_start, tr.endpos, pos);

        gi.WriteByte(svc_temp_entity);
        gi.WriteByte(TE_BUBBLETRAIL);
        gi.WritePosition(water_start);
        gi.WritePosition(tr.endpos);
        gi.multicast(pos, MULTICAST_PVS);
    }
}

/*
=================
fire_bullet

Fires a single round.  Used for machinegun and chaingun.  Would be fine for
pistols, rifles, etc....
=================
*/
void fire_bullet(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int kick, int hspread, int vspread, mod_t mod)
{
    fire_lead(self, start, aimdir, damage, kick, mod.id == MOD_TESLA ? -1 : TE_GUNSHOT, hspread, vspread, mod);
}

/*
=================
fire_shotgun

Shoots shotgun pellets.  Used by shotgun and super shotgun.
=================
*/
void fire_shotgun(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, mod_t mod)
{
    while (count--)
        fire_lead(self, start, aimdir, damage, kick, TE_SHOTGUN, hspread, vspread, mod);
}

/*
=================
fire_blaster

Fires a single blaster bolt.  Used by the blaster and hyper blaster.
=================
*/
void TOUCH(blaster_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if (other == self->owner)
        return;

    if (tr->surface && (tr->surface->flags & SURF_SKY)) {
        G_FreeEdict(self);
        return;
    }

    // PMM - crash prevention
    if (self->owner && self->owner->client)
        PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

    if (other->takedamage)
        T_Damage(other, self, self->owner, self->velocity, self->s.origin, tr->plane.normal, self->dmg, 1, DAMAGE_ENERGY, (mod_t) { self->style });
    else {
        gi.WriteByte(svc_temp_entity);
        gi.WriteByte((self->style != MOD_BLUEBLASTER) ? TE_BLASTER : TE_BLUEHYPERBLASTER_2);
        gi.WritePosition(self->s.origin);
        gi.WriteDir(tr->plane.normal);
        gi.multicast(self->s.origin, MULTICAST_PHS);
    }

    G_FreeEdict(self);
}

edict_t *fire_blaster(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed, effects_t effect, mod_t mod)
{
    edict_t *bolt;
    trace_t  tr;

    bolt = G_Spawn();
    bolt->svflags = SVF_PROJECTILE;
    VectorCopy(start, bolt->s.origin);
    VectorCopy(start, bolt->s.old_origin);
    vectoangles(dir, bolt->s.angles);
    VectorScale(dir, speed, bolt->velocity);
    bolt->movetype = MOVETYPE_FLYMISSILE;
    bolt->clipmask = MASK_PROJECTILE;
    // [Paril-KEX]
    if (self->client && !G_ShouldPlayersCollide(true))
        bolt->clipmask &= ~CONTENTS_PLAYER;
    bolt->flags |= FL_DODGE;
    bolt->solid = SOLID_BBOX;
    bolt->s.effects |= effect;
    bolt->s.modelindex = gi.modelindex("models/objects/laser/tris.md2");
    bolt->s.sound = gi.soundindex("misc/lasfly.wav");
    bolt->owner = self;
    bolt->touch = blaster_touch;
    bolt->nextthink = level.time + SEC(2);
    bolt->think = G_FreeEdict;
    bolt->dmg = damage;
    bolt->classname = "bolt";
    bolt->style = mod.id;
    gi.linkentity(bolt);

    tr = gi.trace(self->s.origin, NULL, NULL, bolt->s.origin, bolt, bolt->clipmask);
    if (tr.fraction < 1.0f) {
        VectorAdd(tr.endpos, tr.plane.normal, bolt->s.origin);
        bolt->touch(bolt, tr.ent, &tr, false);
    }

    return bolt;
}

#define SPAWNFLAG_GRENADE_HAND  1
#define SPAWNFLAG_GRENADE_HELD  2

/*
=================
fire_grenade
=================
*/
static void Grenade_ExplodeReal(edict_t *ent, edict_t *other, const vec3_t normal)
{
    vec3_t   origin;
    mod_id_t mod;

    if (ent->owner->client)
        PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

    // FIXME: if we are onground then raise our Z just a bit since we are a point?
    if (other) {
        vec3_t dir;

        VectorSubtract(other->s.origin, ent->s.origin, dir);
        if (ent->spawnflags & SPAWNFLAG_GRENADE_HAND)
            mod = MOD_HANDGRENADE;
        else
            mod = MOD_GRENADE;
        T_Damage(other, ent, ent->owner, dir, ent->s.origin, normal, ent->dmg, ent->dmg, mod == MOD_HANDGRENADE ? DAMAGE_RADIUS : DAMAGE_NONE, (mod_t) { mod });
    }

    if (ent->spawnflags & SPAWNFLAG_GRENADE_HELD)
        mod = MOD_HELD_GRENADE;
    else if (ent->spawnflags & SPAWNFLAG_GRENADE_HAND)
        mod = MOD_HG_SPLASH;
    else
        mod = MOD_G_SPLASH;
    T_RadiusDamage(ent, ent->owner, ent->dmg, other, ent->dmg_radius, DAMAGE_NONE, (mod_t) { mod });

    VectorAdd(ent->s.origin, normal, origin);
    gi.WriteByte(svc_temp_entity);
    if (ent->waterlevel) {
        if (ent->groundentity)
            gi.WriteByte(TE_GRENADE_EXPLOSION_WATER);
        else
            gi.WriteByte(TE_ROCKET_EXPLOSION_WATER);
    } else {
        if (ent->groundentity)
            gi.WriteByte(TE_GRENADE_EXPLOSION);
        else
            gi.WriteByte(TE_ROCKET_EXPLOSION);
    }
    gi.WritePosition(origin);
    gi.multicast(ent->s.origin, MULTICAST_PHS);

    G_FreeEdict(ent);
}

void THINK(Grenade_Explode)(edict_t *ent)
{
    vec3_t normal;
    VectorScale(ent->velocity, -0.02f, normal);
    Grenade_ExplodeReal(ent, NULL, normal);
}

void TOUCH(Grenade_Touch)(edict_t *ent, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if (other == ent->owner)
        return;

    if (tr->surface && (tr->surface->flags & SURF_SKY)) {
        G_FreeEdict(ent);
        return;
    }

    if (!other->takedamage) {
        if (!(ent->spawnflags & SPAWNFLAG_GRENADE_HAND))
            gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/grenlb1b.wav"), 1, ATTN_NORM, 0);
        else if (brandom())
            gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/hgrenb1a.wav"), 1, ATTN_NORM, 0);
        else
            gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/hgrenb2a.wav"), 1, ATTN_NORM, 0);
        return;
    }

    Grenade_ExplodeReal(ent, other, tr->plane.normal);
}

void THINK(Grenade4_Think)(edict_t *self)
{
    if (level.time >= self->timestamp) {
        Grenade_Explode(self);
        return;
    }

    if (!VectorEmpty(self->velocity)) {
        float p = self->s.angles[0];
        float r = self->s.angles[2];
        float speed_frac = Q_clipf(VectorLengthSquared(self->velocity) / (self->speed * self->speed), 0, 1);
        vectoangles(self->velocity, self->s.angles);
        self->s.angles[0] = LerpAngle(p, self->s.angles[0], speed_frac);
        self->s.angles[2] = r + (FRAME_TIME_SEC * 360 * speed_frac);
    }

    self->nextthink = level.time + FRAME_TIME;
}

void fire_grenade(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int speed, gtime_t timer, float damage_radius, float right_adjust, float up_adjust, bool monster)
{
    edict_t *grenade;
    vec3_t   dir;
    vec3_t   forward, right, up;

    vectoangles(aimdir, dir);
    AngleVectors(dir, forward, right, up);

    grenade = G_Spawn();
    VectorCopy(start, grenade->s.origin);
    VectorScale(aimdir, speed, grenade->velocity);

    if (up_adjust) {
        up_adjust *= level.gravity / 800.0f;
        VectorMA(grenade->velocity, up_adjust, up, grenade->velocity);
    }

    if (right_adjust)
        VectorMA(grenade->velocity, right_adjust, right, grenade->velocity);

    grenade->movetype = MOVETYPE_BOUNCE;
    grenade->clipmask = MASK_PROJECTILE;
    // [Paril-KEX]
    if (self->client && !G_ShouldPlayersCollide(true))
        grenade->clipmask &= ~CONTENTS_PLAYER;
    grenade->solid = SOLID_BBOX;
    grenade->svflags |= SVF_PROJECTILE;
    grenade->flags |= (FL_DODGE | FL_TRAP);
    grenade->s.effects |= EF_GRENADE;
    grenade->speed = speed;
    if (monster) {
        crandom_vec(grenade->avelocity, 360);
        grenade->s.modelindex = gi.modelindex("models/objects/grenade/tris.md2");
        grenade->nextthink = level.time + timer;
        grenade->think = Grenade_Explode;
        grenade->x.morefx |= EFX_GRENADE_LIGHT;
    } else {
        grenade->s.modelindex = gi.modelindex("models/objects/grenade4/tris.md2");
        vectoangles(grenade->velocity, grenade->s.angles);
        grenade->nextthink = level.time + FRAME_TIME;
        grenade->timestamp = level.time + timer;
        grenade->think = Grenade4_Think;
        grenade->s.renderfx |= RF_MINLIGHT;
    }
    grenade->owner = self;
    grenade->touch = Grenade_Touch;
    grenade->dmg = damage;
    grenade->dmg_radius = damage_radius;
    grenade->classname = "grenade";

    gi.linkentity(grenade);
}

void fire_grenade2(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int speed, gtime_t timer, float damage_radius, bool held)
{
    edict_t *grenade;
    vec3_t   dir;
    vec3_t   forward, right, up;

    vectoangles(aimdir, dir);
    AngleVectors(dir, forward, right, up);

    grenade = G_Spawn();
    VectorCopy(start, grenade->s.origin);
    VectorScale(aimdir, speed, grenade->velocity);

    float scale = (200 + crandom() * 10.0f) * (level.gravity / 800.0f);
    VectorMA(grenade->velocity, scale, up, grenade->velocity);

    scale = crandom() * 10.0f;
    VectorMA(grenade->velocity, scale, right, grenade->velocity);

    crandom_vec(grenade->avelocity, 360);

    grenade->movetype = MOVETYPE_BOUNCE;
    grenade->clipmask = MASK_PROJECTILE;
    // [Paril-KEX]
    if (self->client && !G_ShouldPlayersCollide(true))
        grenade->clipmask &= ~CONTENTS_PLAYER;
    grenade->solid = SOLID_BBOX;
    grenade->svflags |= SVF_PROJECTILE;
    grenade->flags |= (FL_DODGE | FL_TRAP);
    grenade->s.effects |= EF_GRENADE;

    grenade->s.modelindex = gi.modelindex("models/objects/grenade3/tris.md2");
    grenade->owner = self;
    grenade->touch = Grenade_Touch;
    grenade->nextthink = level.time + timer;
    grenade->think = Grenade_Explode;
    grenade->dmg = damage;
    grenade->dmg_radius = damage_radius;
    grenade->classname = "hand_grenade";
    grenade->spawnflags = SPAWNFLAG_GRENADE_HAND;
    if (held)
        grenade->spawnflags |= SPAWNFLAG_GRENADE_HELD;
    grenade->s.sound = gi.soundindex("weapons/hgrenc1b.wav");

    if (timer <= 0)
        Grenade_Explode(grenade);
    else {
        gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/hgrent1a.wav"), 1, ATTN_NORM, 0);
        gi.linkentity(grenade);
    }
}

/*
=================
fire_rocket
=================
*/
void TOUCH(rocket_touch)(edict_t *ent, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    vec3_t origin;

    if (other == ent->owner)
        return;

    if (tr->surface && (tr->surface->flags & SURF_SKY)) {
        G_FreeEdict(ent);
        return;
    }

    if (ent->owner->client)
        PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

    // calculate position for the explosion entity
    VectorAdd(ent->s.origin, tr->plane.normal, origin);

    if (other->takedamage) {
        T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, tr->plane.normal, ent->dmg, ent->dmg, DAMAGE_NONE, (mod_t) { MOD_ROCKET });
        // don't throw any debris in net games
    } else if (!deathmatch->integer && !coop->integer && tr->surface &&
               !(tr->surface->flags & (SURF_WARP | SURF_TRANS33 | SURF_TRANS66 | SURF_FLOWING))) {
        int n = irandom1(5);
        while (n--)
            ThrowGib(ent, "models/objects/debris2/tris.md2", 2, GIB_METALLIC | GIB_DEBRIS);
    }

    T_RadiusDamage(ent, ent->owner, ent->radius_dmg, other, ent->dmg_radius, DAMAGE_NONE, (mod_t) { MOD_R_SPLASH });

    gi.WriteByte(svc_temp_entity);
    if (ent->waterlevel)
        gi.WriteByte(TE_ROCKET_EXPLOSION_WATER);
    else
        gi.WriteByte(TE_ROCKET_EXPLOSION);
    gi.WritePosition(origin);
    gi.multicast(ent->s.origin, MULTICAST_PHS);

    G_FreeEdict(ent);
}

edict_t *fire_rocket(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed, float damage_radius, int radius_damage)
{
    edict_t *rocket;

    rocket = G_Spawn();
    VectorCopy(start, rocket->s.origin);
    vectoangles(dir, rocket->s.angles);
    VectorScale(dir, speed, rocket->velocity);
    rocket->movetype = MOVETYPE_FLYMISSILE;
    rocket->svflags |= SVF_PROJECTILE;
    rocket->flags |= FL_DODGE;
    rocket->clipmask = MASK_PROJECTILE;
    // [Paril-KEX]
    if (self->client && !G_ShouldPlayersCollide(true))
        rocket->clipmask &= ~CONTENTS_PLAYER;
    rocket->solid = SOLID_BBOX;
    rocket->s.effects |= EF_ROCKET;
    rocket->s.modelindex = gi.modelindex("models/objects/rocket/tris.md2");
    rocket->owner = self;
    rocket->touch = rocket_touch;
    rocket->nextthink = level.time + SEC(8000.0f / speed);
    rocket->think = G_FreeEdict;
    rocket->dmg = damage;
    rocket->radius_dmg = radius_damage;
    rocket->dmg_radius = damage_radius;
    rocket->s.sound = gi.soundindex("weapons/rockfly.wav");
    rocket->classname = "rocket";

    gi.linkentity(rocket);

    return rocket;
}

typedef qboolean (*search_callback_t)(const vec3_t p1, const vec3_t p2); 

static bool binary_positional_search_r(const vec3_t viewer, const vec3_t start, const vec3_t end, search_callback_t cb, int split_num)
{
    // check half-way point
    vec3_t mid;
    VectorAvg(start, end, mid);

    if (cb(viewer, mid))
        return true;

    // no more splits
    if (!split_num)
        return false;

    // recursively check both sides
    if (binary_positional_search_r(viewer, start, mid, cb, split_num - 1))
        return true;

    return binary_positional_search_r(viewer, mid, end, cb, split_num - 1);
}

// [Paril-KEX] simple binary search through a line to see if any points along
// the line (in a binary split) pass the callback
static bool binary_positional_search(const vec3_t viewer, const vec3_t start, const vec3_t end, search_callback_t cb, int num_splits)
{
    // check start/end first
    if (cb(viewer, start) || cb(viewer, end))
        return true;

    // recursive split
    return binary_positional_search_r(viewer, start, end, cb, num_splits);
}

/*
=================
fire_rail
=================
*/
bool fire_rail(edict_t *self, const vec3_t start, const vec3_t aimdir, int damage, int kick)
{
    contents_t mask = MASK_PROJECTILE;

    // [Paril-KEX]
    if (self->client && !G_ShouldPlayersCollide(true))
        mask &= ~CONTENTS_PLAYER;

    vec3_t end;
    VectorMA(start, 8192, aimdir, end);

    pierce_t pierce;
    trace_t tr;

    pierce_begin(&pierce);

    while (1) {
        tr = gi.trace(start, NULL, NULL, end, self, mask);

        // didn't hit anything, so we're done
        if (!tr.ent || tr.fraction == 1.0f)
            break;

        // try to kill it first
        if ((tr.ent != self) && (tr.ent->takedamage))
            T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, DAMAGE_NONE, (mod_t) { MOD_RAILGUN });

        // dead, so we don't need to care about checking pierce
        if (!tr.ent->inuse || (!tr.ent->solid || tr.ent->solid == SOLID_TRIGGER))
            continue;

        // ZOID--added so rail goes through SOLID_BBOX entities (gibs, etc)
        if ((tr.ent->svflags & SVF_MONSTER) || (tr.ent->client) ||
            // ROGUE
            (tr.ent->flags & FL_DAMAGEABLE) ||
            // ROGUE
            (tr.ent->solid == SOLID_BBOX)) {
            if (pierce_mark(&pierce, tr.ent))
                continue;
        }

        // hit a solid, so we're stopping here
        break;
    }

    pierce_end(&pierce);

    // send gun puff / flash
    temp_event_t te = (deathmatch->integer && g_instagib->integer) ? TE_RAILTRAIL2 : TE_RAILTRAIL;

    // [Paril-KEX] this often makes double noise, so trying
    // a slightly different approach...
    for (int i = 1; i <= game.maxclients; i++) {
        edict_t *player = &g_edicts[i];
        if (!player->inuse)
            continue;

        vec3_t org;
        VectorAdd(player->s.origin, player->client->ps.viewoffset, org);

        if (!binary_positional_search(org, start, tr.endpos, gi.inPHS, 3))
            continue;

        gi.WriteByte(svc_temp_entity);
        gi.WriteByte(te);
        gi.WritePosition(start);
        gi.WritePosition(tr.endpos);
        gi.unicast(player, false);
    }

    if (self->client)
        PlayerNoise(self, tr.endpos, PNOISE_IMPACT);

    return pierce.count;
}

static void bfg_laser_pos(const vec3_t p, float dist, vec3_t out)
{
    float theta = frandom1(2 * M_PIf);
    float phi = acosf(crandom());

    vec3_t d = {
        sinf(phi) * cosf(theta),
        sinf(phi) * sinf(theta),
        cosf(phi)
    };

    VectorMA(p, dist, d, out);
}

void THINK(bfg_laser_update)(edict_t *self)
{
    if (level.time > self->timestamp || !self->owner->inuse) {
        G_FreeEdict(self);
        return;
    }

    VectorCopy(self->owner->s.origin, self->s.origin);
    self->nextthink = level.time + FRAME_TIME;
    gi.linkentity(self);
}

static void bfg_spawn_laser(edict_t *self)
{
    vec3_t end;
    bfg_laser_pos(self->s.origin, 256, end);
    trace_t tr = gi.trace(self->s.origin, NULL, NULL, end, self, MASK_OPAQUE | CONTENTS_PROJECTILECLIP);

    if (tr.fraction == 1.0f)
        return;

    edict_t *laser = G_Spawn();
    laser->s.frame = 3;
    laser->s.renderfx = RF_BEAM_LIGHTNING;
    laser->movetype = MOVETYPE_NONE;
    laser->solid = SOLID_NOT;
    laser->s.modelindex = MODELINDEX_WORLD; // must be non-zero
    VectorCopy(self->s.origin, laser->s.origin);
    VectorCopy(tr.endpos, laser->s.old_origin);
    laser->s.skinnum = 0xD0D0D0D0;
    laser->think = bfg_laser_update;
    laser->nextthink = level.time + FRAME_TIME;
    laser->timestamp = level.time + SEC(0.3f);
    laser->owner = self;
    gi.linkentity(laser);
}

/*
=================
fire_bfg
=================
*/
void THINK(bfg_explode)(edict_t *self)
{
    edict_t *ent;
    float    points;
    vec3_t   v;
    float    dist;

    bfg_spawn_laser(self);

    if (self->s.frame == 0) {
        // the BFG effect
        ent = NULL;
        while ((ent = findradius(ent, self->s.origin, self->dmg_radius)) != NULL) {
            if (!ent->takedamage)
                continue;
            if (ent == self->owner)
                continue;
            if (!CanDamage(ent, self))
                continue;
            if (!CanDamage(ent, self->owner))
                continue;
            // ROGUE - make tesla hurt by bfg
            if (!(ent->svflags & SVF_MONSTER) && !(ent->flags & FL_DAMAGEABLE) && (!ent->client) && (strcmp(ent->classname, "misc_explobox") != 0))
                continue;
            // ZOID
            // don't target players in CTF
            if (CheckTeamDamage(ent, self->owner))
                continue;
            // ZOID

            vec3_t centroid;
            VectorAvg(ent->mins, ent->maxs, v);
            VectorAdd(ent->s.origin, v, centroid);
            dist = Distance(self->s.origin, centroid);
            points = self->radius_dmg * (1.0f - sqrtf(dist / self->dmg_radius));

            T_Damage(ent, self, self->owner, self->velocity, centroid, vec3_origin, points, 0, DAMAGE_ENERGY, (mod_t) { MOD_BFG_EFFECT });

            // Paril: draw BFG lightning laser to enemies
            gi.WriteByte(svc_temp_entity);
            gi.WriteByte(TE_BFG_ZAP);
            gi.WritePosition(self->s.origin);
            gi.WritePosition(centroid);
            gi.multicast(self->s.origin, MULTICAST_PHS);
        }
    }

    self->nextthink = level.time + HZ(10);
    self->s.frame++;
    if (self->s.frame == 5)
        self->think = G_FreeEdict;
}

void TOUCH(bfg_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if (other == self->owner)
        return;

    if (tr->surface && (tr->surface->flags & SURF_SKY)) {
        G_FreeEdict(self);
        return;
    }

    if (self->owner->client)
        PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

    // core explosion - prevents firing it into the wall/floor
    if (other->takedamage)
        T_Damage(other, self, self->owner, self->velocity, self->s.origin, tr->plane.normal, 200, 0, DAMAGE_ENERGY, (mod_t) { MOD_BFG_BLAST });
    T_RadiusDamage(self, self->owner, 200, other, 100, DAMAGE_ENERGY, (mod_t) { MOD_BFG_BLAST });

    gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/bfg__x1b.wav"), 1, ATTN_NORM, 0);
    self->solid = SOLID_NOT;
    self->touch = NULL;
    VectorMA(self->s.origin, -1 * FRAME_TIME_SEC, self->velocity, self->s.origin);
    VectorClear(self->velocity);
    self->s.modelindex = gi.modelindex("sprites/s_bfg3.sp2");
    self->s.frame = 0;
    self->s.sound = 0;
    self->s.effects &= ~EF_ANIM_ALLFAST;
    self->think = bfg_explode;
    self->nextthink = level.time + HZ(10);
    self->enemy = other;

    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_BFG_BIGEXPLOSION);
    gi.WritePosition(self->s.origin);
    gi.multicast(self->s.origin, MULTICAST_PHS);
}

void THINK(bfg_think)(edict_t *self)
{
    edict_t *ent;
    vec3_t   point;
    vec3_t   dir;
    vec3_t   start;
    vec3_t   end;
    int      dmg;
    trace_t  tr;

    if (deathmatch->integer)
        dmg = 5;
    else
        dmg = 10;

    bfg_spawn_laser(self);

    ent = NULL;
    while ((ent = findradius(ent, self->s.origin, 256)) != NULL) {
        if (ent == self)
            continue;

        if (ent == self->owner)
            continue;

        if (!ent->takedamage)
            continue;

        // ROGUE - make tesla hurt by bfg
        if (!(ent->svflags & SVF_MONSTER) && !(ent->flags & FL_DAMAGEABLE) && (!ent->client) && (strcmp(ent->classname, "misc_explobox") != 0))
            continue;
        // ZOID
        // don't target players in CTF
        if (CheckTeamDamage(ent, self->owner))
            continue;
        // ZOID

        VectorAvg(ent->absmin, ent->absmax, point);

        VectorSubtract(point, self->s.origin, dir);
        VectorNormalize(dir);

        VectorCopy(self->s.origin, start);
        VectorMA(start, 2048, dir, end);

        // [Paril-KEX] don't fire a laser if we're blocked by the world
        tr = gi.trace(start, NULL, NULL, point, NULL, MASK_SOLID | CONTENTS_PROJECTILECLIP);

        if (tr.fraction < 1.0f)
            continue;

        pierce_t pierce;
        pierce_begin(&pierce);

        do {
            tr = gi.trace(start, NULL, NULL, end, self, CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_PLAYER | CONTENTS_DEADMONSTER | CONTENTS_PROJECTILECLIP);

            // didn't hit anything, so we're done
            if (!tr.ent || tr.fraction == 1.0f)
                break;

            // hurt it if we can
            if ((tr.ent->takedamage) && !(tr.ent->flags & FL_IMMUNE_LASER) && (tr.ent != self->owner))
                T_Damage(tr.ent, self, self->owner, dir, tr.endpos, vec3_origin, dmg, 1, DAMAGE_ENERGY, (mod_t) { MOD_BFG_LASER });

            // if we hit something that's not a monster or player we're done
            if (!(tr.ent->svflags & SVF_MONSTER) && !(tr.ent->flags & FL_DAMAGEABLE) && (!tr.ent->client)) {
                vec3_t pos;
                VectorAdd(tr.endpos, tr.plane.normal, pos);

                gi.WriteByte(svc_temp_entity);
                gi.WriteByte(TE_LASER_SPARKS);
                gi.WriteByte(4);
                gi.WritePosition(pos);
                gi.WriteDir(tr.plane.normal);
                gi.WriteByte(208);
                gi.multicast(pos, MULTICAST_PVS);
                break;
            }
        } while (pierce_mark(&pierce, tr.ent));

        pierce_end(&pierce);

        gi.WriteByte(svc_temp_entity);
        gi.WriteByte(TE_BFG_LASER);
        gi.WritePosition(self->s.origin);
        gi.WritePosition(tr.endpos);
        gi.multicast(self->s.origin, MULTICAST_PHS);
    }

    self->nextthink = level.time + HZ(10);
}

void fire_bfg(edict_t *self, const vec3_t start, const vec3_t dir, int damage, int speed, float damage_radius)
{
    edict_t *bfg;

    bfg = G_Spawn();
    VectorCopy(start, bfg->s.origin);
    vectoangles(dir, bfg->s.angles);
    VectorScale(dir, speed, bfg->velocity);
    bfg->movetype = MOVETYPE_FLYMISSILE;
    bfg->clipmask = MASK_PROJECTILE;
    bfg->svflags = SVF_PROJECTILE;
    // [Paril-KEX]
    if (self->client && !G_ShouldPlayersCollide(true))
        bfg->clipmask &= ~CONTENTS_PLAYER;
    bfg->solid = SOLID_BBOX;
    bfg->s.effects |= EF_BFG | EF_ANIM_ALLFAST;
    bfg->s.modelindex = gi.modelindex("sprites/s_bfg1.sp2");
    bfg->owner = self;
    bfg->touch = bfg_touch;
    bfg->nextthink = level.time + SEC(8000.0f / speed);
    bfg->think = G_FreeEdict;
    bfg->radius_dmg = damage;
    bfg->dmg_radius = damage_radius;
    bfg->classname = "bfg blast";
    bfg->s.sound = gi.soundindex("weapons/bfg__l1a.wav");

    bfg->think = bfg_think;
    bfg->nextthink = level.time + FRAME_TIME;
    bfg->teammaster = bfg;
    bfg->teamchain = NULL;

    gi.linkentity(bfg);
}

void TOUCH(disintegrator_touch)(edict_t *self, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    vec3_t pos;
    VectorMA(self->s.origin, -0.01f, self->velocity, pos);

    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_WIDOWSPLASH);
    gi.WritePosition(pos);
    gi.multicast(self->s.origin, MULTICAST_PHS);

    G_FreeEdict(self);

    if (other->svflags & (SVF_MONSTER | SVF_PLAYER)) {
        other->disintegrator_time += SEC(50);
        other->disintegrator = self->owner;
    }
}

void fire_disintegrator(edict_t *self, const vec3_t start, const vec3_t forward, int speed)
{
    edict_t *bfg;

    bfg = G_Spawn();
    VectorCopy(start, bfg->s.origin);
    vectoangles(forward, bfg->s.angles);
    VectorScale(forward, speed, bfg->velocity);
    bfg->movetype = MOVETYPE_FLYMISSILE;
    bfg->clipmask = MASK_PROJECTILE;
    // [Paril-KEX]
    if (self->client && !G_ShouldPlayersCollide(true))
        bfg->clipmask &= ~CONTENTS_PLAYER;
    bfg->solid = SOLID_BBOX;
    bfg->s.effects |= EF_TAGTRAIL | EF_ANIM_ALL;
    bfg->s.renderfx |= RF_TRANSLUCENT;
    bfg->svflags |= SVF_PROJECTILE;
    bfg->flags |= FL_DODGE;
    bfg->s.modelindex = gi.modelindex("sprites/s_bfg1.sp2");
    bfg->owner = self;
    bfg->touch = disintegrator_touch;
    bfg->nextthink = level.time + SEC(8000.0f / speed);
    bfg->think = G_FreeEdict;
    bfg->classname = "disint ball";
    bfg->s.sound = gi.soundindex("weapons/bfg__l1a.wav");

    gi.linkentity(bfg);
}
