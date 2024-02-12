// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"

// ================
// PMM
bool Pickup_Nuke(edict_t *ent, edict_t *other)
{
    int quantity;

    quantity = other->client->pers.inventory[ent->item->id];

    if (quantity >= 1)
        return false;

    if (coop->integer && !P_UseCoopInstancedItems() && (ent->item->flags & IF_STAY_COOP) && (quantity > 0))
        return false;

    other->client->pers.inventory[ent->item->id]++;

    if (deathmatch->integer && !(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED))
        SetRespawn(ent, SEC(ent->item->quantity));

    return true;
}

// ================
// PGM
void Use_IR(edict_t *ent, const gitem_t *item)
{
    ent->client->pers.inventory[item->id]--;

    ent->client->ir_time = max(level.time, ent->client->ir_time) + SEC(60);

    gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/ir_start.wav"), 1, ATTN_NORM, 0);
}

void Use_Double(edict_t *ent, const gitem_t *item)
{
    ent->client->pers.inventory[item->id]--;

    ent->client->double_time = max(level.time, ent->client->double_time) + SEC(30);

    gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/ddamage1.wav"), 1, ATTN_NORM, 0);
}

void Use_Nuke(edict_t *ent, const gitem_t *item)
{
    vec3_t forward, right;

    ent->client->pers.inventory[item->id]--;

    AngleVectors(ent->client->v_angle, forward, right, NULL);

    fire_nuke(ent, ent->s.origin, forward, 100);
}

void Use_Doppleganger(edict_t *ent, const gitem_t *item)
{
    vec3_t forward, right;
    vec3_t createPt, spawnPt;
    vec3_t ang;

    ang[PITCH] = 0;
    ang[YAW] = ent->client->v_angle[YAW];
    ang[ROLL] = 0;
    AngleVectors(ang, forward, right, NULL);

    VectorMA(ent->s.origin, 48, forward, createPt);

    if (!FindSpawnPoint(createPt, ent->mins, ent->maxs, spawnPt, 32, true))
        return;

    if (!CheckGroundSpawnPoint(spawnPt, ent->mins, ent->maxs, 64, -1))
        return;

    ent->client->pers.inventory[item->id]--;

    SpawnGrow_Spawn(spawnPt, 24, 48);
    fire_doppleganger(ent, spawnPt, forward);
}

bool Pickup_Doppleganger(edict_t *ent, edict_t *other)
{
    int quantity;

    if (!deathmatch->integer) // item is DM only
        return false;

    quantity = other->client->pers.inventory[ent->item->id];
    if (quantity >= 1) // FIXME - apply max to dopplegangers
        return false;

    other->client->pers.inventory[ent->item->id]++;

    if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED))
        SetRespawn(ent, SEC(ent->item->quantity));

    return true;
}

bool Pickup_Sphere(edict_t *ent, edict_t *other)
{
    int quantity;

    if (other->client && other->client->owned_sphere)
        return false;

    quantity = other->client->pers.inventory[ent->item->id];
    if ((skill->integer == 1 && quantity >= 2) || (skill->integer >= 2 && quantity >= 1))
        return false;

    if ((coop->integer) && !P_UseCoopInstancedItems() && (ent->item->flags & IF_STAY_COOP) && (quantity > 0))
        return false;

    other->client->pers.inventory[ent->item->id]++;

    if (deathmatch->integer) {
        if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED))
            SetRespawn(ent, SEC(ent->item->quantity));
        if (g_dm_instant_items->integer) {
            // PGM
            if (ent->item->use)
                ent->item->use(other, ent->item);
            else
                gi.dprintf("Powerup has no use function!\n");
            // PGM
        }
    }

    return true;
}

void Use_Defender(edict_t *ent, const gitem_t *item)
{
    if (ent->client && ent->client->owned_sphere) {
        gi.cprintf(ent, PRINT_HIGH, "Only one sphere at a time!\n");
        return;
    }

    ent->client->pers.inventory[item->id]--;

    Defender_Launch(ent);
}

void Use_Hunter(edict_t *ent, const gitem_t *item)
{
    if (ent->client && ent->client->owned_sphere) {
        gi.cprintf(ent, PRINT_HIGH, "Only one sphere at a time!\n");
        return;
    }

    ent->client->pers.inventory[item->id]--;

    Hunter_Launch(ent);
}

void Use_Vengeance(edict_t *ent, const gitem_t *item)
{
    if (ent->client && ent->client->owned_sphere) {
        gi.cprintf(ent, PRINT_HIGH, "Only one sphere at a time!\n");
        return;
    }

    ent->client->pers.inventory[item->id]--;

    Vengeance_Launch(ent);
}

// PGM
// ================

//=================
// Item_TriggeredSpawn - create the item marked for spawn creation
//=================
void USE(Item_TriggeredSpawn)(edict_t *self, edict_t *other, edict_t *activator)
{
    self->svflags &= ~SVF_NOCLIENT;
    self->use = NULL;

    if (self->spawnflags & SPAWNFLAG_ITEM_TOSS_SPAWN) {
        self->movetype = MOVETYPE_TOSS;
        vec3_t forward, right;

        AngleVectors(self->s.angles, forward, right, NULL);
        self->s.origin[2] += 16;
        VectorScale(forward, 100, self->velocity);
        self->velocity[2] = 300;
    }

    if (self->item->id != IT_KEY_POWER_CUBE && self->item->id != IT_KEY_EXPLOSIVE_CHARGES) // leave them be on key_power_cube..
        self->spawnflags &= SPAWNFLAG_ITEM_NO_TOUCH;

    droptofloor(self);
}

//=================
// SetTriggeredSpawn - set up an item to spawn in later.
//=================
void SetTriggeredSpawn(edict_t *ent)
{
    // don't do anything on key_power_cubes.
    if (ent->item->id == IT_KEY_POWER_CUBE || ent->item->id == IT_KEY_EXPLOSIVE_CHARGES)
        return;

    ent->think = NULL;
    ent->nextthink = 0;
    ent->use = Item_TriggeredSpawn;
    ent->svflags |= SVF_NOCLIENT;
    ent->solid = SOLID_NOT;
}
