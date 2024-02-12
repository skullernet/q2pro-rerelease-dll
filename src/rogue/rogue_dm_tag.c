// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// dm_tag
// pmack
// june 1998

#include "g_local.h"

void SP_dm_tag_token(edict_t *self);

// ***********************
// Tag Specific Stuff
// ***********************

static edict_t *tag_token;
static edict_t *tag_owner;
static int      tag_count;

static void Tag_DropToken(edict_t *ent, const gitem_t *item);

static void Tag_PlayerDeath(edict_t *targ, edict_t *inflictor, edict_t *attacker)
{
    if (tag_token && targ && (targ == tag_owner)) {
        Tag_DropToken(targ, GetItemByIndex(IT_ITEM_TAG_TOKEN));
        tag_owner = NULL;
        tag_count = 0;
    }
}

static void Tag_KillItBonus(edict_t *self)
{
    edict_t *armor;

    // if the player is hurt, boost them up to max.
    if (self->health < self->max_health) {
        self->health += 200;
        if (self->health > self->max_health)
            self->health = self->max_health;
    }

    // give the player a body armor
    armor = G_Spawn();
    armor->spawnflags |= SPAWNFLAG_ITEM_DROPPED;
    armor->item = GetItemByIndex(IT_ARMOR_BODY);
    Touch_Item(armor, self, &null_trace, true);
    if (armor->inuse)
        G_FreeEdict(armor);
}

static void Tag_PlayerDisconnect(edict_t *self)
{
    if (tag_token && self && (self == tag_owner)) {
        Tag_DropToken(self, GetItemByIndex(IT_ITEM_TAG_TOKEN));
        tag_owner = NULL;
        tag_count = 0;
    }
}

static void Tag_Score(edict_t *attacker, edict_t *victim, int scoreChange, mod_t mod)
{
    const gitem_t *quad;

    if (tag_token && tag_owner) {
        // owner killed somone else
        if ((scoreChange > 0) && tag_owner == attacker) {
            scoreChange = 3;
            tag_count++;
            if (tag_count == 5) {
                quad = GetItemByIndex(IT_ITEM_QUAD);
                attacker->client->pers.inventory[IT_ITEM_QUAD]++;
                quad->use(attacker, quad);
                tag_count = 0;
            }
        // owner got killed. 5 points and switch owners
        } else if (tag_owner == victim && tag_owner != attacker) {
            scoreChange = 5;
            if ((mod.id == MOD_HUNTER_SPHERE) || (mod.id == MOD_DOPPLE_EXPLODE) ||
                (mod.id == MOD_DOPPLE_VENGEANCE) || (mod.id == MOD_DOPPLE_HUNTER) ||
                (attacker->health <= 0)) {
                Tag_DropToken(tag_owner, GetItemByIndex(IT_ITEM_TAG_TOKEN));
                tag_owner = NULL;
                tag_count = 0;
            } else {
                Tag_KillItBonus(attacker);
                tag_owner = attacker;
                tag_count = 0;
            }
        }
    }

    attacker->client->resp.score += scoreChange;
}

bool Tag_PickupToken(edict_t *ent, edict_t *other)
{
    if (gamerules->integer != RDM_TAG)
        return false;

    // sanity checking is good.
    if (tag_token != ent)
        tag_token = ent;

    other->client->pers.inventory[ent->item->id]++;

    tag_owner = other;
    tag_count = 0;

    Tag_KillItBonus(other);

    return true;
}

void THINK(Tag_Respawn)(edict_t *ent)
{
    edict_t *spot;

    spot = SelectDeathmatchSpawnPoint(true, false, true, NULL);
    if (spot == NULL) {
        ent->nextthink = level.time + SEC(1);
        return;
    }

    VectorCopy(spot->s.origin, ent->s.origin);
    gi.linkentity(ent);
}

void THINK(Tag_MakeTouchable)(edict_t *ent)
{
    ent->touch = Touch_Item;

    tag_token->think = Tag_Respawn;

    // check here to see if it's in lava or slime. if so, do a respawn sooner
    if (gi.pointcontents(ent->s.origin) & (CONTENTS_LAVA | CONTENTS_SLIME))
        tag_token->nextthink = level.time + SEC(3);
    else
        tag_token->nextthink = level.time + SEC(30);
}

static void Tag_DropToken(edict_t *ent, const gitem_t *item)
{
    trace_t trace;
    vec3_t  forward, right;
    static const vec3_t offset = { 24, 0, -16 };

    // reset the score count for next player
    tag_count = 0;
    tag_owner = NULL;

    tag_token = G_Spawn();

    tag_token->classname = item->classname;
    tag_token->item = item;
    tag_token->spawnflags = SPAWNFLAG_ITEM_DROPPED;
    tag_token->s.effects = EF_ROTATE | EF_TAGTRAIL;
    tag_token->s.renderfx = RF_GLOW | RF_NO_LOD;
    VectorSet(tag_token->mins, -15, -15, -15);
    VectorSet(tag_token->maxs, 15, 15, 15);
    gi.setmodel(tag_token, tag_token->item->world_model);
    tag_token->solid = SOLID_TRIGGER;
    tag_token->movetype = MOVETYPE_TOSS;
    tag_token->touch = NULL;
    tag_token->owner = ent;

    AngleVectors(ent->client->v_angle, forward, right, NULL);
    G_ProjectSource(ent->s.origin, offset, forward, right, tag_token->s.origin);
    trace = gi.trace(ent->s.origin, tag_token->mins, tag_token->maxs,
                     tag_token->s.origin, ent, CONTENTS_SOLID);
    VectorCopy(trace.endpos, tag_token->s.origin);

    VectorScale(forward, 100, tag_token->velocity);
    tag_token->velocity[2] = 300;

    tag_token->think = Tag_MakeTouchable;
    tag_token->nextthink = level.time + SEC(1);

    gi.linkentity(tag_token);

    //  tag_token = Drop_Item (ent, item);
    ent->client->pers.inventory[item->id]--;
}

static void Tag_PlayerEffects(edict_t *ent)
{
    if (ent == tag_owner)
        ent->s.effects |= EF_TAGTRAIL;
}

static void Tag_DogTag(edict_t *ent, edict_t *killer, const char **pic)
{
    if (ent == tag_owner)
        *pic = "tag3";
}

//=================
// Tag_ChangeDamage - damage done that does not involve the tag owner
//      is at 75% original to encourage folks to go after the tag owner.
//=================
static int Tag_ChangeDamage(edict_t *targ, edict_t *attacker, int damage, mod_t mod)
{
    if ((targ != tag_owner) && (attacker != tag_owner))
        return (damage * 3 / 4);

    return damage;
}

static void Tag_GameInit(void)
{
    tag_token = NULL;
    tag_owner = NULL;
    tag_count = 0;
}

static void Tag_PostInitSetup(void)
{
    edict_t *e;

    // automatic spawning of tag token if one is not present on map.
    e = G_Find(NULL, FOFS(classname), "dm_tag_token");
    if (e == NULL) {
        e = G_Spawn();
        e->classname = "dm_tag_token";

        SelectSpawnPoint(e, e->s.origin, e->s.angles, true);
        SP_dm_tag_token(e);
    }
}

/*QUAKED dm_tag_token (.3 .3 1) (-16 -16 -16) (16 16 16)
The tag token for deathmatch tag games.
*/
void SP_dm_tag_token(edict_t *self)
{
    if (!deathmatch->integer) {
        G_FreeEdict(self);
        return;
    }

    if (gamerules->integer != RDM_TAG) {
        G_FreeEdict(self);
        return;
    }

    // store the tag token edict pointer for later use.
    tag_token = self;
    tag_count = 0;

    self->classname = "dm_tag_token";
    self->model = "models/items/tagtoken/tris.md2";
    self->count = 1;
    SpawnItem(self, GetItemByIndex(IT_ITEM_TAG_TOKEN));
}

const dm_game_rt DMGame_Tag = {
    .GameInit = Tag_GameInit,
    .PostInitSetup = Tag_PostInitSetup,
    .PlayerDeath = Tag_PlayerDeath,
    .Score = Tag_Score,
    .PlayerEffects = Tag_PlayerEffects,
    .DogTag = Tag_DogTag,
    .PlayerDisconnect = Tag_PlayerDisconnect,
    .ChangeDamage = Tag_ChangeDamage,
};
