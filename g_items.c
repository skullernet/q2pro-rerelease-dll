// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"

bool Pickup_Weapon(edict_t *ent, edict_t *other);
void Use_Weapon(edict_t *ent, const gitem_t *inv);
void Drop_Weapon(edict_t *ent, const gitem_t *inv);

void Weapon_Blaster(edict_t *ent);
void Weapon_Shotgun(edict_t *ent);
void Weapon_SuperShotgun(edict_t *ent);
void Weapon_Machinegun(edict_t *ent);
void Weapon_Chaingun(edict_t *ent);
void Weapon_HyperBlaster(edict_t *ent);
void Weapon_RocketLauncher(edict_t *ent);
void Weapon_Grenade(edict_t *ent);
void Weapon_GrenadeLauncher(edict_t *ent);
void Weapon_Railgun(edict_t *ent);
void Weapon_BFG(edict_t *ent);
// RAFAEL
void Weapon_Ionripper(edict_t *ent);
void Weapon_Phalanx(edict_t *ent);
void Weapon_Trap(edict_t *ent);
// RAFAEL
// ROGUE
void Weapon_ChainFist(edict_t *ent);
void Weapon_Disintegrator(edict_t *ent);
void Weapon_ETF_Rifle(edict_t *ent);
void Weapon_Heatbeam(edict_t *ent);
void Weapon_Prox(edict_t *ent);
void Weapon_Tesla(edict_t *ent);
void Weapon_ProxLauncher(edict_t *ent);
// ROGUE
void Weapon_Beta_Disintegrator(edict_t *ent);

static void Use_Quad(edict_t *ent, const gitem_t *item);
static gtime_t quad_drop_timeout_hack;

// RAFAEL
static void Use_QuadFire(edict_t *ent, const gitem_t *item);
static gtime_t quad_fire_drop_timeout_hack;
// RAFAEL

const gitem_armor_t jacketarmor_info = { 25, 50, 0.3f, 0 };
const gitem_armor_t combatarmor_info = { 50, 100, 0.6f, 0.3f };
const gitem_armor_t bodyarmor_info = { 100, 200, 0.8f, 0.6f };

//======================================================================

/*
===============
GetItemByIndex
===============
*/
const gitem_t *GetItemByIndex(item_id_t index)
{
    if (index <= IT_NULL || index >= IT_TOTAL)
        return NULL;

    return &itemlist[index];
}

static const gitem_t *ammolist[AMMO_MAX];

const gitem_t *GetItemByAmmo(ammo_t ammo)
{
    return ammolist[ammo];
}

static const gitem_t *poweruplist[POWERUP_MAX];

const gitem_t *GetItemByPowerup(powerup_t powerup)
{
    return poweruplist[powerup];
}

/*
===============
FindItemByClassname

===============
*/
const gitem_t *FindItemByClassname(const char *classname)
{
    int            i;
    const gitem_t *it;

    it = itemlist;
    for (i = 0; i < IT_TOTAL; i++, it++) {
        if (!it->classname)
            continue;
        if (!Q_strcasecmp(it->classname, classname))
            return it;
    }

    return NULL;
}

/*
===============
FindItem

===============
*/
const gitem_t *FindItem(const char *pickup_name)
{
    int            i;
    const gitem_t *it;

    it = itemlist;
    for (i = 0; i < IT_TOTAL; i++, it++) {
        if (!it->use_name)
            continue;
        if (!Q_strcasecmp(it->use_name, pickup_name))
            return it;
    }

    return NULL;
}

//======================================================================

void THINK(DoRespawn)(edict_t *ent)
{
    if (ent->team) {
        edict_t *master;
        int      count;
        int      choice;

        master = ent->teammaster;

        // ZOID
        // in ctf, when we are weapons stay, only the master of a team of weapons
        // is spawned
        if (ctf->integer && g_dm_weapons_stay->integer && master->item && (master->item->flags & IF_WEAPON))
            ent = master;
        else {
            // ZOID
            ent->svflags |= SVF_NOCLIENT;
            ent->solid = SOLID_NOT;
            gi.linkentity(ent);

            for (count = 0, ent = master; ent; ent = ent->chain, count++)
                ;

            choice = irandom1(count);

            for (count = 0, ent = master; count < choice; ent = ent->chain, count++)
                ;
        }
    }

    ent->svflags &= ~SVF_NOCLIENT;
    ent->svflags &= ~SVF_RESPAWNING;
    ent->solid = SOLID_TRIGGER;
    gi.linkentity(ent);

    // send an effect
    ent->s.event = EV_ITEM_RESPAWN;

    // ROGUE
    if (g_dm_random_items->integer) {
        item_id_t new_item = DoRandomRespawn(ent);

        // if we've changed entities, then do some sleight of hand.
        // otherwise, the old entity will respawn
        if (new_item) {
            ent->item = GetItemByIndex(new_item);

            ent->classname = ent->item->classname;
            ent->s.effects = ent->item->world_model_flags;
            gi.setmodel(ent, ent->item->world_model);
        }
    }
    // ROGUE
}

void SetRespawnEx(edict_t *ent, gtime_t delay, bool hide_self)
{
    // already respawning
    if (ent->think == DoRespawn && ent->nextthink >= level.time)
        return;

    ent->flags |= FL_RESPAWN;

    if (hide_self) {
        ent->svflags |= (SVF_NOCLIENT | SVF_RESPAWNING);
        ent->solid = SOLID_NOT;
        gi.linkentity(ent);
    }

    ent->nextthink = level.time + delay;
    ent->think = DoRespawn;
}

void SetRespawn(edict_t *ent, gtime_t delay)
{
    SetRespawnEx(ent, delay, true);
}

//======================================================================

static bool IsInstantItemsEnabled(void)
{
    if (deathmatch->integer && g_dm_instant_items->integer)
        return true;

    if (!deathmatch->integer && level.instantitems)
        return true;

    return false;
}

static bool Pickup_Powerup(edict_t *ent, edict_t *other)
{
    int quantity;

    quantity = other->client->pers.inventory[ent->item->id];
    if ((skill->integer == 0 && quantity >= 3) ||
        (skill->integer == 1 && quantity >= 2) ||
        (skill->integer >= 2 && quantity >= 1))
        return false;

    if (coop->integer && !P_UseCoopInstancedItems() && (ent->item->flags & IF_STAY_COOP) && (quantity > 0))
        return false;

    other->client->pers.inventory[ent->item->id]++;

    bool is_dropped_from_death = (ent->spawnflags & SPAWNFLAG_ITEM_DROPPED_PLAYER) && !(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED);

    if (IsInstantItemsEnabled() ||
        ((ent->item->use == Use_Quad) && is_dropped_from_death) ||
        ((ent->item->use == Use_QuadFire) && is_dropped_from_death)) {
        if ((ent->item->use == Use_Quad) && is_dropped_from_death)
            quad_drop_timeout_hack = (ent->nextthink - level.time);
        else if ((ent->item->use == Use_QuadFire) && is_dropped_from_death)
            quad_fire_drop_timeout_hack = (ent->nextthink - level.time);

        if (ent->item->use)
            ent->item->use(other, ent->item);
    }

    if (deathmatch->integer && !(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED) && !is_dropped_from_death)
        SetRespawn(ent, SEC(ent->item->quantity));

    return true;
}

static bool Pickup_General(edict_t *ent, edict_t *other)
{
    if (other->client->pers.inventory[ent->item->id])
        return false;

    other->client->pers.inventory[ent->item->id]++;

    if (deathmatch->integer && !(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED))
        SetRespawn(ent, SEC(ent->item->quantity));

    return true;
}

static void Drop_General(edict_t *ent, const gitem_t *item)
{
    edict_t *dropped = Drop_Item(ent, item);
    dropped->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
    dropped->svflags &= ~SVF_INSTANCED;
    ent->client->pers.inventory[item->id]--;
}

//======================================================================

static void Use_Adrenaline(edict_t *ent, const gitem_t *item)
{
    if (!deathmatch->integer)
        ent->max_health += 1;

    if (ent->health < ent->max_health)
        ent->health = ent->max_health;

    gi.sound(ent, CHAN_ITEM, gi.soundindex("items/n_health.wav"), 1, ATTN_NORM, 0);

    ent->client->pers.inventory[item->id]--;
}

static bool Pickup_LegacyHead(edict_t *ent, edict_t *other)
{
    other->max_health += 5;
    other->health += 5;

    if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED) && deathmatch->integer)
        SetRespawn(ent, SEC(ent->item->quantity));

    return true;
}

void G_CheckPowerArmor(edict_t *ent)
{
    bool has_enough_cells;

    if (!ent->client->pers.inventory[IT_AMMO_CELLS])
        has_enough_cells = false;
    else if (ent->client->pers.autoshield >= AUTO_SHIELD_AUTO)
        has_enough_cells = (ent->flags & FL_WANTS_POWER_ARMOR) && ent->client->pers.inventory[IT_AMMO_CELLS] > ent->client->pers.autoshield;
    else
        has_enough_cells = true;

    if (ent->flags & FL_POWER_ARMOR) {
        if (!has_enough_cells) {
            // ran out of cells for power armor
            ent->flags &= ~FL_POWER_ARMOR;
            gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
        }
    } else {
        // special case for power armor, for auto-shields
        if (ent->client->pers.autoshield != AUTO_SHIELD_MANUAL &&
            has_enough_cells && (ent->client->pers.inventory[IT_ITEM_POWER_SCREEN] ||
                                 ent->client->pers.inventory[IT_ITEM_POWER_SHIELD])) {
            ent->flags |= FL_POWER_ARMOR;
            gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power1.wav"), 1, ATTN_NORM, 0);
        }
    }
}

static bool G_AddAmmoAndCap(edict_t *other, item_id_t item, int max, int quantity)
{
    if (other->client->pers.inventory[item] >= max)
        return false;

    other->client->pers.inventory[item] += quantity;
    if (other->client->pers.inventory[item] > max)
        other->client->pers.inventory[item] = max;

    G_CheckPowerArmor(other);

    return true;
}

static bool G_AddAmmoAndCapQuantity(edict_t *other, ammo_t ammo)
{
    const gitem_t *item = GetItemByAmmo(ammo);
    return G_AddAmmoAndCap(other, item->id, other->client->pers.max_ammo[ammo], item->quantity);
}

static void G_AdjustAmmoCap(edict_t *other, ammo_t ammo, int new_max)
{
    other->client->pers.max_ammo[ammo] = max(other->client->pers.max_ammo[ammo], new_max);
}

static bool Pickup_Bandolier(edict_t *ent, edict_t *other)
{
    G_AdjustAmmoCap(other, AMMO_BULLETS, 250);
    G_AdjustAmmoCap(other, AMMO_SHELLS, 150);
    G_AdjustAmmoCap(other, AMMO_CELLS, 250);
    G_AdjustAmmoCap(other, AMMO_SLUGS, 75);
    G_AdjustAmmoCap(other, AMMO_MAGSLUG, 75);
    G_AdjustAmmoCap(other, AMMO_FLECHETTES, 250);
    G_AdjustAmmoCap(other, AMMO_DISRUPTOR, 21);

    G_AddAmmoAndCapQuantity(other, AMMO_BULLETS);
    G_AddAmmoAndCapQuantity(other, AMMO_SHELLS);

    if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED) && deathmatch->integer)
        SetRespawn(ent, SEC(ent->item->quantity));

    return true;
}

static bool Pickup_Pack(edict_t *ent, edict_t *other)
{
    G_AdjustAmmoCap(other, AMMO_BULLETS, 300);
    G_AdjustAmmoCap(other, AMMO_SHELLS, 200);
    G_AdjustAmmoCap(other, AMMO_ROCKETS, 100);
    G_AdjustAmmoCap(other, AMMO_GRENADES, 100);
    G_AdjustAmmoCap(other, AMMO_CELLS, 300);
    G_AdjustAmmoCap(other, AMMO_SLUGS, 100);
    G_AdjustAmmoCap(other, AMMO_MAGSLUG, 100);
    G_AdjustAmmoCap(other, AMMO_FLECHETTES, 300);
    G_AdjustAmmoCap(other, AMMO_DISRUPTOR, 30);

    G_AddAmmoAndCapQuantity(other, AMMO_BULLETS);
    G_AddAmmoAndCapQuantity(other, AMMO_SHELLS);
    G_AddAmmoAndCapQuantity(other, AMMO_CELLS);
    G_AddAmmoAndCapQuantity(other, AMMO_GRENADES);
    G_AddAmmoAndCapQuantity(other, AMMO_ROCKETS);
    G_AddAmmoAndCapQuantity(other, AMMO_SLUGS);

    // RAFAEL
    G_AddAmmoAndCapQuantity(other, AMMO_MAGSLUG);
    // RAFAEL

    // ROGUE
    G_AddAmmoAndCapQuantity(other, AMMO_FLECHETTES);
    G_AddAmmoAndCapQuantity(other, AMMO_DISRUPTOR);
    // ROGUE

    if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED) && deathmatch->integer)
        SetRespawn(ent, SEC(ent->item->quantity));

    return true;
}

//======================================================================

static void Use_Quad(edict_t *ent, const gitem_t *item)
{
    gtime_t timeout;

    ent->client->pers.inventory[item->id]--;

    if (quad_drop_timeout_hack) {
        timeout = quad_drop_timeout_hack;
        quad_drop_timeout_hack = 0;
    } else {
        timeout = SEC(30);
    }

    ent->client->quad_time = max(level.time, ent->client->quad_time) + timeout;

    gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}
// =====================================================================

// RAFAEL
static void Use_QuadFire(edict_t *ent, const gitem_t *item)
{
    gtime_t timeout;

    ent->client->pers.inventory[item->id]--;

    if (quad_fire_drop_timeout_hack) {
        timeout = quad_fire_drop_timeout_hack;
        quad_fire_drop_timeout_hack = 0;
    } else {
        timeout = SEC(30);
    }

    ent->client->quadfire_time = max(level.time, ent->client->quadfire_time) + timeout;

    gi.sound(ent, CHAN_ITEM, gi.soundindex("items/quadfire1.wav"), 1, ATTN_NORM, 0);
}
// RAFAEL

//======================================================================

static void Use_Breather(edict_t *ent, const gitem_t *item)
{
    ent->client->pers.inventory[item->id]--;

    ent->client->breather_time = max(level.time, ent->client->breather_time) + SEC(30);

    //  gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

static void Use_Envirosuit(edict_t *ent, const gitem_t *item)
{
    ent->client->pers.inventory[item->id]--;

    ent->client->enviro_time = max(level.time, ent->client->enviro_time) + SEC(30);

    //  gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

static void Use_Invulnerability(edict_t *ent, const gitem_t *item)
{
    ent->client->pers.inventory[item->id]--;

    ent->client->invincible_time = max(level.time, ent->client->invincible_time) + SEC(30);

    gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect.wav"), 1, ATTN_NORM, 0);
}

static void Use_Invisibility(edict_t *ent, const gitem_t *item)
{
    ent->client->pers.inventory[item->id]--;

    ent->client->invisible_time = max(level.time, ent->client->invisible_time) + SEC(30);

    gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

static void Use_Silencer(edict_t *ent, const gitem_t *item)
{
    ent->client->pers.inventory[item->id]--;
    ent->client->silencer_shots += 30;

    //  gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

static bool Pickup_Key(edict_t *ent, edict_t *other)
{
    if (coop->integer) {
        if (ent->item->id == IT_KEY_POWER_CUBE || ent->item->id == IT_KEY_EXPLOSIVE_CHARGES) {
            if (other->client->pers.power_cubes & ((ent->spawnflags & SPAWNFLAG_EDITOR_MASK) >> 8))
                return false;
            other->client->pers.inventory[ent->item->id]++;
            other->client->pers.power_cubes |= ((ent->spawnflags & SPAWNFLAG_EDITOR_MASK) >> 8);
        } else {
            if (other->client->pers.inventory[ent->item->id])
                return false;
            other->client->pers.inventory[ent->item->id] = 1;
        }
        return true;
    }
    other->client->pers.inventory[ent->item->id]++;
    return true;
}

//======================================================================

bool Add_Ammo(edict_t *ent, const gitem_t *item, int count)
{
    if (!ent->client || item->tag < AMMO_BULLETS || item->tag >= AMMO_MAX)
        return false;

    return G_AddAmmoAndCap(ent, item->id, ent->client->pers.max_ammo[item->tag], count);
}

// we just got weapon `item`, check if we should switch to it
void G_CheckAutoSwitch(edict_t *ent, const gitem_t *item, bool is_new)
{
    // already using or switching to
    if (ent->client->pers.weapon == item || ent->client->newweapon == item)
        return;
    // need ammo
    if (item->ammo) {
        int required_ammo = (item->flags & IF_AMMO) ? 1 : item->quantity;

        if (ent->client->pers.inventory[item->ammo] < required_ammo)
            return;
    }

    // check autoswitch setting
    if (ent->client->pers.autoswitch == AUTOSW_NEVER)
        return;
    if ((item->flags & IF_AMMO) && ent->client->pers.autoswitch == AUTOSW_ALWAYS_NO_AMMO)
        return;
    if (ent->client->pers.autoswitch == AUTOSW_SMART) {
        bool using_blaster = ent->client->pers.weapon && ent->client->pers.weapon->id == IT_WEAPON_BLASTER;

        // smartness algorithm: in DM, we will always switch if we have the blaster out
        // otherwise leave our active weapon alone
        if (deathmatch->integer && !using_blaster)
            return;
        // in SP, only switch if it's a new weapon, or we have the blaster out
        if (!deathmatch->integer && !using_blaster && !is_new)
            return;
    }

    // switch!
    ent->client->newweapon = item;
}

static bool Pickup_Ammo(edict_t *ent, edict_t *other)
{
    int  oldcount;
    int  count;
    bool weapon;

    weapon = (ent->item->flags & IF_WEAPON);
    if (weapon && G_CheckInfiniteAmmo(ent->item))
        count = 1000;
    else if (ent->count)
        count = ent->count;
    else
        count = ent->item->quantity;

    oldcount = other->client->pers.inventory[ent->item->id];

    if (!Add_Ammo(other, ent->item, count))
        return false;

    if (weapon)
        G_CheckAutoSwitch(other, ent->item, !oldcount);

    if (!(ent->spawnflags & (SPAWNFLAG_ITEM_DROPPED | SPAWNFLAG_ITEM_DROPPED_PLAYER)) && deathmatch->integer)
        SetRespawn(ent, SEC(30));
    return true;
}

static void Drop_Ammo(edict_t *ent, const gitem_t *item)
{
    // [Paril-KEX]
    if (G_CheckInfiniteAmmo(item))
        return;

    item_id_t index = item->id;
    edict_t *dropped = Drop_Item(ent, item);
    dropped->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
    dropped->svflags &= ~SVF_INSTANCED;

    if (ent->client->pers.inventory[index] >= item->quantity)
        dropped->count = item->quantity;
    else
        dropped->count = ent->client->pers.inventory[index];

    if (ent->client->pers.weapon && ent->client->pers.weapon == item && (item->flags & IF_AMMO) &&
        ent->client->pers.inventory[index] - dropped->count <= 0) {
        gi.cprintf(ent, PRINT_HIGH, "Can't drop current weapon\n");
        G_FreeEdict(dropped);
        return;
    }

    ent->client->pers.inventory[index] -= dropped->count;
    G_CheckPowerArmor(ent);
}

//======================================================================

void THINK(MegaHealth_think)(edict_t *self)
{
    if (self->owner->health > self->owner->max_health
        //ZOID
        && !CTFHasRegeneration(self->owner)
        //ZOID
       )
    {
        self->nextthink = level.time + SEC(1);
        self->owner->health -= 1;
        return;
    }

    if (!(self->spawnflags & SPAWNFLAG_ITEM_DROPPED) && deathmatch->integer)
        SetRespawn(self, SEC(20));
    else
        G_FreeEdict(self);
}

static bool Pickup_Health(edict_t *ent, edict_t *other)
{
    int health_flags = (ent->style ? ent->style : ent->item->tag);

    if (!(health_flags & HEALTH_IGNORE_MAX) && (other->health >= other->max_health))
        return false;

    int count = ent->count ? ent->count : ent->item->quantity;

    // ZOID
    if (deathmatch->integer && other->health >= 250 && count > 25)
        return false;
    // ZOID

    other->health += count;

    //ZOID
    if (ctf->integer && other->health > 250 && count > 25)
        other->health = 250;
    //ZOID

    if (!(health_flags & HEALTH_IGNORE_MAX) && (other->health > other->max_health))
        other->health = other->max_health;

    if ((ent->item->tag & HEALTH_TIMED)
        //ZOID
        && !CTFHasRegeneration(other)
        //ZOID
       )
    {
        if (!deathmatch->integer) {
            // mega health doesn't need to be special in SP
            // since it never respawns.
            other->client->pers.megahealth_time = SEC(5);
        } else {
            ent->think = MegaHealth_think;
            ent->nextthink = level.time + SEC(5);
            ent->owner = other;
            ent->flags |= FL_RESPAWN;
            ent->svflags |= SVF_NOCLIENT;
            ent->solid = SOLID_NOT;
        }
    } else {
        if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED) && deathmatch->integer)
            SetRespawn(ent, SEC(30));
    }

    return true;
}

//======================================================================

item_id_t ArmorIndex(edict_t *ent)
{
    if (ent->svflags & SVF_MONSTER)
        return ent->monsterinfo.armor_type;

    if (ent->client) {
        if (ent->client->pers.inventory[IT_ARMOR_JACKET] > 0)
            return IT_ARMOR_JACKET;
        if (ent->client->pers.inventory[IT_ARMOR_COMBAT] > 0)
            return IT_ARMOR_COMBAT;
        if (ent->client->pers.inventory[IT_ARMOR_BODY] > 0)
            return IT_ARMOR_BODY;
    }

    return IT_NULL;
}

static bool Pickup_Armor(edict_t *ent, edict_t *other)
{
    item_id_t            old_armor_index;
    const gitem_armor_t *oldinfo;
    const gitem_armor_t *newinfo;
    int                  newcount;
    float                salvage;
    int                  salvagecount;

    // get info on new armor
    newinfo = ent->item->armor_info;

    old_armor_index = ArmorIndex(other);

    // [Paril-KEX] for g_start_items
    int base_count = ent->count ? ent->count : newinfo ? newinfo->base_count : 0;

    // handle armor shards specially
    if (ent->item->id == IT_ARMOR_SHARD) {
        if (!old_armor_index)
            other->client->pers.inventory[IT_ARMOR_JACKET] = 2;
        else
            other->client->pers.inventory[old_armor_index] += 2;
    // if player has no armor, just use it
    } else if (!old_armor_index) {
        other->client->pers.inventory[ent->item->id] = base_count;
    // use the better armor
    } else {
        // get info on old armor
        if (old_armor_index == IT_ARMOR_JACKET)
            oldinfo = &jacketarmor_info;
        else if (old_armor_index == IT_ARMOR_COMBAT)
            oldinfo = &combatarmor_info;
        else
            oldinfo = &bodyarmor_info;

        if (newinfo->normal_protection > oldinfo->normal_protection) {
            // calc new armor values
            salvage = oldinfo->normal_protection / newinfo->normal_protection;
            salvagecount = (int)(salvage * other->client->pers.inventory[old_armor_index]);
            newcount = base_count + salvagecount;
            if (newcount > newinfo->max_count)
                newcount = newinfo->max_count;

            // zero count of old armor so it goes away
            other->client->pers.inventory[old_armor_index] = 0;

            // change armor to new item with computed value
            other->client->pers.inventory[ent->item->id] = newcount;
        } else {
            // calc new armor values
            salvage = newinfo->normal_protection / oldinfo->normal_protection;
            salvagecount = (int)(salvage * base_count);
            newcount = other->client->pers.inventory[old_armor_index] + salvagecount;
            if (newcount > oldinfo->max_count)
                newcount = oldinfo->max_count;

            // if we're already maxed out then we don't need the new armor
            if (other->client->pers.inventory[old_armor_index] >= newcount)
                return false;

            // update current armor value
            other->client->pers.inventory[old_armor_index] = newcount;
        }
    }

    if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED) && deathmatch->integer)
        SetRespawn(ent, SEC(20));

    return true;
}

//======================================================================

item_id_t PowerArmorType(edict_t *ent)
{
    if (!ent->client)
        return IT_NULL;

    if (!(ent->flags & FL_POWER_ARMOR))
        return IT_NULL;

    if (ent->client->pers.inventory[IT_ITEM_POWER_SHIELD] > 0)
        return IT_ITEM_POWER_SHIELD;

    if (ent->client->pers.inventory[IT_ITEM_POWER_SCREEN] > 0)
        return IT_ITEM_POWER_SCREEN;

    return IT_NULL;
}

static void Use_PowerArmor(edict_t *ent, const gitem_t *item)
{
    if (ent->flags & FL_POWER_ARMOR) {
        ent->flags &= ~(FL_POWER_ARMOR | FL_WANTS_POWER_ARMOR);
        gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
    } else {
        if (!ent->client->pers.inventory[IT_AMMO_CELLS]) {
            gi.cprintf(ent, PRINT_HIGH, "No cells for power armor.\n");
            return;
        }

        ent->flags |= FL_POWER_ARMOR;

        if (ent->client->pers.autoshield != AUTO_SHIELD_MANUAL &&
            ent->client->pers.inventory[IT_AMMO_CELLS] > ent->client->pers.autoshield)
            ent->flags |= FL_WANTS_POWER_ARMOR;

        gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power1.wav"), 1, ATTN_NORM, 0);
    }
}

static bool Pickup_PowerArmor(edict_t *ent, edict_t *other)
{
    int quantity;

    quantity = other->client->pers.inventory[ent->item->id];

    other->client->pers.inventory[ent->item->id]++;

    if (deathmatch->integer) {
        if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED))
            SetRespawn(ent, SEC(ent->item->quantity));
        // auto-use for DM only if we didn't already have one
        if (!quantity)
            G_CheckPowerArmor(other);
    } else
        G_CheckPowerArmor(other);

    return true;
}

static void Drop_PowerArmor(edict_t *ent, const gitem_t *item)
{
    if ((ent->flags & FL_POWER_ARMOR) && (ent->client->pers.inventory[item->id] == 1))
        Use_PowerArmor(ent, item);
    Drop_General(ent, item);
}

//======================================================================

bool Entity_IsVisibleToPlayer(edict_t *ent, edict_t *player)
{
    return !ent->item_picked_up_by[player->s.number - 1];
}

/*
===============
Touch_Item
===============
*/
void TOUCH(Touch_Item)(edict_t *ent, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    bool taken;

    if (!other->client)
        return;
    if (other->health < 1)
        return; // dead people can't pickup
    if (!ent->item->pickup)
        return; // not a grabbable item?

    // already got this instanced item
    if (coop->integer && P_UseCoopInstancedItems() && ent->item_picked_up_by[other->s.number - 1])
        return;

    // ZOID
    if (CTFMatchSetup())
        return; // can't pick stuff up right now
    // ZOID

    taken = ent->item->pickup(ent, other);

    ValidateSelectedItem(other);

    if (taken) {
        // flash the screen
        other->client->bonus_alpha = 0.25f;

        // show icon and name on status bar
        other->client->ps.stats[STAT_PICKUP_ICON] = gi.imageindex(ent->item->icon);
        other->client->ps.stats[STAT_PICKUP_STRING] = CS_ITEMS + ent->item->id;
        other->client->pickup_msg_time = level.time + SEC(3);

        // change selected item if we still have it
        if (ent->item->use && other->client->pers.inventory[ent->item->id]) {
            other->client->ps.stats[STAT_SELECTED_ITEM] = other->client->pers.selected_item = ent->item->id;
            other->client->ps.stats[STAT_SELECTED_ITEM_NAME] = 0; // don't set name on pickup item since it's already there
        }

        if (ent->noise_index)
            gi.sound(other, CHAN_ITEM, ent->noise_index, 1, ATTN_NORM, 0);
        else if (ent->item->pickup_sound)
            gi.sound(other, CHAN_ITEM, gi.soundindex(ent->item->pickup_sound), 1, ATTN_NORM, 0);

        int player_number = other->s.number - 1;

        if (coop->integer && P_UseCoopInstancedItems() && !ent->item_picked_up_by[player_number]) {
            ent->item_picked_up_by[player_number] = true;

            // [Paril-KEX] this is to fix a coop quirk where items
            // that send a message on pick up will only print on the
            // player that picked them up, and never anybody else;
            // when instanced items are enabled we don't need to limit
            // ourselves to this, but it does mean that relays that trigger
            // messages won't work, so we'll have to fix those
            if (ent->message)
                G_PrintActivationMessage(ent, other, false);
        }
    }

    if (!(ent->spawnflags & SPAWNFLAG_ITEM_TARGETS_USED)) {
        // [Paril-KEX] see above msg; this also disables the message in DM
        // since there's no need to print pickup messages in DM (this wasn't
        // even a documented feature, relays were traditionally used for this)
        const char *message_backup = ent->message;

        if (deathmatch->integer || (coop->integer && P_UseCoopInstancedItems()))
            ent->message = NULL;

        G_UseTargets(ent, other);

        if (deathmatch->integer || (coop->integer && P_UseCoopInstancedItems()))
            ent->message = message_backup;

        ent->spawnflags |= SPAWNFLAG_ITEM_TARGETS_USED;
    }

    if (taken) {
        bool should_remove = false;

        if (coop->integer) {
            // in coop with instanced items, *only* dropped
            // player items will ever get deleted permanently.
            if (P_UseCoopInstancedItems())
                should_remove = (ent->spawnflags & SPAWNFLAG_ITEM_DROPPED_PLAYER);
            // in coop without instanced items, IF_STAY_COOP items remain
            // if not dropped
            else
                should_remove = (ent->spawnflags & (SPAWNFLAG_ITEM_DROPPED | SPAWNFLAG_ITEM_DROPPED_PLAYER)) || !(ent->item->flags & IF_STAY_COOP);
        } else
            should_remove = !deathmatch->integer || (ent->spawnflags & (SPAWNFLAG_ITEM_DROPPED | SPAWNFLAG_ITEM_DROPPED_PLAYER));

        if (should_remove) {
            if (ent->flags & FL_RESPAWN)
                ent->flags &= ~FL_RESPAWN;
            else
                G_FreeEdict(ent);
        }
    }
}

//======================================================================

void TOUCH(drop_temp_touch)(edict_t *ent, edict_t *other, const trace_t *tr, bool other_touching_self)
{
    if (other == ent->owner)
        return;

    Touch_Item(ent, other, tr, other_touching_self);
}

void THINK(drop_make_touchable)(edict_t *ent)
{
    ent->touch = Touch_Item;
    if (deathmatch->integer) {
        ent->nextthink = level.time + SEC(29);
        ent->think = G_FreeEdict;
    }
}

edict_t *Drop_Item(edict_t *ent, const gitem_t *item)
{
    edict_t *dropped;
    vec3_t   forward, right;

    dropped = G_Spawn();

    dropped->item = item;
    dropped->spawnflags = SPAWNFLAG_ITEM_DROPPED;
    dropped->classname = item->classname;
    dropped->s.effects = item->world_model_flags;
    gi.setmodel(dropped, dropped->item->world_model);
    dropped->s.renderfx = RF_GLOW | RF_NO_LOD | RF_IR_VISIBLE; // PGM
    VectorSet(dropped->mins, -15, -15, -15);
    VectorSet(dropped->maxs, 15, 15, 15);
    dropped->solid = SOLID_TRIGGER;
    dropped->movetype = MOVETYPE_TOSS;
    dropped->touch = drop_temp_touch;
    dropped->owner = ent;

    if (ent->client) {
        trace_t trace;

        AngleVectors(ent->client->v_angle, forward, right, NULL);
        G_ProjectSource(ent->s.origin, (const vec3_t) { 24, 0, -16 }, forward, right, dropped->s.origin);
        trace = gi.trace(ent->s.origin, dropped->mins, dropped->maxs, dropped->s.origin, ent, CONTENTS_SOLID);
        VectorCopy(trace.endpos, dropped->s.origin);
    } else {
        AngleVectors(ent->s.angles, forward, right, NULL);
        VectorAvg(ent->absmin, ent->absmax, dropped->s.origin);
    }

    G_FixStuckObject(dropped, dropped->s.origin);

    VectorScale(forward, 100, dropped->velocity);
    dropped->velocity[2] = 300;

    dropped->think = drop_make_touchable;
    dropped->nextthink = level.time + SEC(1);

    if (coop->integer && P_UseCoopInstancedItems())
        dropped->svflags |= SVF_INSTANCED;

    gi.linkentity(dropped);
    return dropped;
}

void USE(Use_Item)(edict_t *ent, edict_t *other, edict_t *activator)
{
    ent->svflags &= ~SVF_NOCLIENT;
    ent->use = NULL;

    if (ent->spawnflags & SPAWNFLAG_ITEM_NO_TOUCH) {
        ent->solid = SOLID_BBOX;
        ent->touch = NULL;
    } else {
        ent->solid = SOLID_TRIGGER;
        ent->touch = Touch_Item;
    }

    gi.linkentity(ent);
}

//======================================================================

static const vec3_t fc_mins = { -8, -8, -8 };
static const vec3_t fc_maxs = { 8, 8, 8 };

/*
================
droptofloor
================
*/
void THINK(droptofloor)(edict_t *ent)
{
    trace_t tr;
    vec3_t  dest;

    // [Paril-KEX] scale foodcube based on how much we ingested
    if (strcmp(ent->classname, "item_foodcube") == 0) {
        VectorScale(fc_mins, ent->x.scale, ent->mins);
        VectorScale(fc_maxs, ent->x.scale, ent->maxs);
    } else {
        VectorSet(ent->mins, -15, -15, -15);
        VectorSet(ent->maxs, 15, 15, 15);
    }

    if (ent->model)
        gi.setmodel(ent, ent->model);
    else
        gi.setmodel(ent, ent->item->world_model);
    ent->solid = SOLID_TRIGGER;
    ent->movetype = MOVETYPE_TOSS;
    ent->touch = Touch_Item;

    VectorCopy(ent->s.origin, dest);
    dest[2] -= 128;

    tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_SOLID);
    if (tr.startsolid) {
        if (G_FixStuckObject(ent, ent->s.origin) == NO_GOOD_POSITION) {
            // RAFAEL
            if (strcmp(ent->classname, "item_foodcube") == 0)
                ent->velocity[2] = 0;
            else {
            // RAFAEL
                gi.dprintf("%s: droptofloor: startsolid\n", etos(ent));
                G_FreeEdict(ent);
                return;
            // RAFAEL
            }
            // RAFAEL
        }
    } else
        VectorCopy(tr.endpos, ent->s.origin);

    if (ent->team) {
        ent->flags &= ~FL_TEAMSLAVE;
        ent->chain = ent->teamchain;
        ent->teamchain = NULL;

        ent->svflags |= SVF_NOCLIENT;
        ent->solid = SOLID_NOT;

        if (ent == ent->teammaster) {
            ent->nextthink = level.time + HZ(10);
            ent->think = DoRespawn;
        }
    }

    if (ent->spawnflags & SPAWNFLAG_ITEM_NO_TOUCH) {
        ent->solid = SOLID_BBOX;
        ent->touch = NULL;
        ent->s.effects &= ~(EF_ROTATE | EF_BOB);
        ent->s.renderfx &= ~RF_GLOW;
    }

    if (ent->spawnflags & SPAWNFLAG_ITEM_TRIGGER_SPAWN) {
        ent->svflags |= SVF_NOCLIENT;
        ent->solid = SOLID_NOT;
        ent->use = Use_Item;
    }

    ent->watertype = gi.pointcontents(ent->s.origin);
    gi.linkentity(ent);
}

/*
===============
PrecacheItem

Precaches all data needed for a given item.
This will be called for each item spawned in a level,
and for each item in each client's inventory.
===============
*/
void PrecacheItem(const gitem_t *it)
{
    const char *s, *start;
    char        data[MAX_QPATH];
    int         len;

    if (!it)
        return;

    if (it->pickup_sound)
        gi.soundindex(it->pickup_sound);
    if (it->world_model)
        gi.modelindex(it->world_model);
    if (it->view_model)
        gi.modelindex(it->view_model);
    if (it->icon)
        gi.imageindex(it->icon);

    // parse everything for its ammo
    if (it->ammo) {
        const gitem_t *ammo = GetItemByIndex(it->ammo);
        if (ammo != it)
            PrecacheItem(ammo);
    }

    // parse the space seperated precache string for other items
    s = it->precaches;
    if (!s || !s[0])
        return;

    while (*s) {
        start = s;
        while (*s && *s != ' ')
            s++;

        len = s - start;
        if (len >= MAX_QPATH || len < 5)
            gi.error("PrecacheItem: %s has bad precache string", it->classname);
        memcpy(data, start, len);
        data[len] = 0;
        if (*s)
            s++;

        // determine type based on extension
        if (!strcmp(data + len - 3, "md2"))
            gi.modelindex(data);
        else if (!strcmp(data + len - 3, "sp2"))
            gi.modelindex(data);
        else if (!strcmp(data + len - 3, "wav"))
            gi.soundindex(data);
        else if (!strcmp(data + len - 3, "pcx"))
            gi.imageindex(data);
    }
}

/*
============
SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void SpawnItem(edict_t *ent, const gitem_t *item)
{
    // [Sam-KEX]
    // Paril: allow all keys to be trigger_spawn'd (N64 uses this
    // a few different times)
    if (item->flags & IF_KEY) {
        if (ent->spawnflags & SPAWNFLAG_ITEM_TRIGGER_SPAWN) {
            ent->svflags |= SVF_NOCLIENT;
            ent->solid = SOLID_NOT;
            ent->use = Use_Item;
        }
        if (ent->spawnflags & SPAWNFLAG_ITEM_NO_TOUCH) {
            ent->solid = SOLID_BBOX;
            ent->touch = NULL;
            ent->s.effects &= ~(EF_ROTATE | EF_BOB);
            ent->s.renderfx &= ~RF_GLOW;
        }
    } else if (ent->spawnflags >= SPAWNFLAG_ITEM_MAX) { // PGM
        ent->spawnflags = SPAWNFLAG_NONE;
        gi.dprintf("%s has invalid spawnflags set\n", etos(ent));
    }

    // some items will be prevented in deathmatch
    if (deathmatch->integer) {
        // [Kex] In instagib, spawn no pickups!
        if (g_instagib->integer) {
            if (item->pickup == Pickup_Armor || item->pickup == Pickup_PowerArmor ||
                item->pickup == Pickup_Powerup || item->pickup == Pickup_Sphere || item->pickup == Pickup_Doppleganger ||
                (item->flags & IF_HEALTH) || (item->flags & IF_AMMO) || item->pickup == Pickup_Weapon || item->pickup == Pickup_Pack ||
                item->id == IT_ITEM_BANDOLIER || item->id == IT_ITEM_PACK || item->id == IT_AMMO_NUKE) {
                G_FreeEdict(ent);
                return;
            }
        }

        if (g_no_armor->integer) {
            if (item->pickup == Pickup_Armor || item->pickup == Pickup_PowerArmor) {
                G_FreeEdict(ent);
                return;
            }
        }
        if (g_no_items->integer) {
            if (item->pickup == Pickup_Powerup) {
                G_FreeEdict(ent);
                return;
            }
            //=====
            // ROGUE
            if (item->pickup == Pickup_Sphere) {
                G_FreeEdict(ent);
                return;
            }
            if (item->pickup == Pickup_Doppleganger) {
                G_FreeEdict(ent);
                return;
            }
            // ROGUE
            //=====
        }
        if (g_no_health->integer) {
            if (item->flags & IF_HEALTH) {
                G_FreeEdict(ent);
                return;
            }
        }
        if (G_CheckInfiniteAmmo(item)) {
            if (item->flags == IF_AMMO) {
                G_FreeEdict(ent);
                return;
            }

            // [Paril-KEX] some item swappage
            // BFG too strong in Infinite Ammo mode
            if (item->id == IT_WEAPON_BFG)
                item = GetItemByIndex(IT_WEAPON_DISRUPTOR);
        }

        //==========
        // ROGUE
        if (g_no_mines->integer) {
            if (item->id == IT_WEAPON_PROXLAUNCHER || item->id == IT_AMMO_PROX || item->id == IT_AMMO_TESLA || item->id == IT_AMMO_TRAP) {
                G_FreeEdict(ent);
                return;
            }
        }
        if (g_no_nukes->integer) {
            if (item->id == IT_AMMO_NUKE) {
                G_FreeEdict(ent);
                return;
            }
        }
        if (g_no_spheres->integer) {
            if (item->pickup == Pickup_Sphere) {
                G_FreeEdict(ent);
                return;
            }
        }
        // ROGUE
        //==========
    }

    //==========
    // ROGUE
    // DM only items
    if (!deathmatch->integer) {
        if (item->pickup == Pickup_Doppleganger || item->pickup == Pickup_Nuke) {
            gi.dprintf("%s spawned in non-DM; freeing...\n", etos(ent));
            G_FreeEdict(ent);
            return;
        }
        if ((item->use == Use_Vengeance) || (item->use == Use_Hunter)) {
            gi.dprintf("%s spawned in non-DM; freeing...\n", etos(ent));
            G_FreeEdict(ent);
            return;
        }
    }
    // ROGUE
    //==========

    // [Paril-KEX] power armor breaks infinite ammo
    if (G_CheckInfiniteAmmo(item) && (item->id == IT_ITEM_POWER_SHIELD || item->id == IT_ITEM_POWER_SCREEN))
        item = GetItemByIndex(IT_ARMOR_BODY);

    // ZOID
    // Don't spawn the flags unless enabled
    if (!ctf->integer && (item->id == IT_FLAG1 || item->id == IT_FLAG2)) {
        G_FreeEdict(ent);
        return;
    }
    // ZOID

    // set final classname now
    ent->classname = item->classname;

    PrecacheItem(item);

    if (coop->integer && (item->id == IT_KEY_POWER_CUBE || item->id == IT_KEY_EXPLOSIVE_CHARGES)) {
        ent->spawnflags |= (1 << (8 + level.power_cubes));
        level.power_cubes++;
    }

    // mark all items as instanced
    if (coop->integer && P_UseCoopInstancedItems())
        ent->svflags |= SVF_INSTANCED;

    ent->item = item;
    ent->nextthink = level.time + HZ(5); // items start after other solids
    ent->think = droptofloor;
    ent->s.effects = item->world_model_flags;
    ent->s.renderfx = RF_GLOW | RF_NO_LOD;
    if (ent->model)
        gi.modelindex(ent->model);

    if (ent->spawnflags & SPAWNFLAG_ITEM_TRIGGER_SPAWN)
        SetTriggeredSpawn(ent);

    // ZOID
    // flags are server animated and have special handling
    if (item->id == IT_FLAG1 || item->id == IT_FLAG2)
        ent->think = CTFFlagSetup;
    // ZOID
}

void P_ToggleFlashlight(edict_t *ent, bool state)
{
    if (!!(ent->flags & FL_FLASHLIGHT) == state)
        return;

    ent->flags ^= FL_FLASHLIGHT;

    gi.sound(ent, CHAN_AUTO, gi.soundindex(ent->flags & FL_FLASHLIGHT ? "items/flashlight_on.wav" : "items/flashlight_off.wav"), 1, ATTN_STATIC, 0);
}

static void Use_Flashlight(edict_t *ent, const gitem_t *inv)
{
    P_ToggleFlashlight(ent, !(ent->flags & FL_FLASHLIGHT));
}

//======================================================================

// clang-format off
const gitem_t itemlist[] = {
    { 0 },    // leave index 0 alone

    //
    // ARMOR
    //


    /*QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16)
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/items/armor/body/tris.md2"
    */
    {
        /* id */ IT_ARMOR_BODY,
        /* classname */ "item_armor_body",
        /* pickup */ Pickup_Armor,
        /* use */ NULL,
        /* drop */ NULL,
        /* weaponthink */ NULL,
        /* pickup_sound */ "misc/ar3_pkup.wav",
        /* world_model */ "models/items/armor/body/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "i_bodyarmor",
        /* use_name */   "Body Armor",
        /* pickup_name */   "Body Armor",
        /* pickup_name_definite */ "the Body Armor",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_ARMOR,
        /* vwep_model */ NULL,
        /* armor_info */ &bodyarmor_info
    },

    /*QUAKED item_armor_combat (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_ARMOR_COMBAT,
        /* classname */ "item_armor_combat",
        /* pickup */ Pickup_Armor,
        /* use */ NULL,
        /* drop */ NULL,
        /* weaponthink */ NULL,
        /* pickup_sound */ "misc/ar1_pkup.wav",
        /* world_model */ "models/items/armor/combat/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "i_combatarmor",
        /* use_name */  "Combat Armor",
        /* pickup_name */  "Combat Armor",
        /* pickup_name_definite */ "the Combat Armor",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_ARMOR,
        /* vwep_model */ NULL,
        /* armor_info */ &combatarmor_info
    },

    /*QUAKED item_armor_jacket (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_ARMOR_JACKET,
        /* classname */ "item_armor_jacket",
        /* pickup */ Pickup_Armor,
        /* use */ NULL,
        /* drop */ NULL,
        /* weaponthink */ NULL,
        /* pickup_sound */ "misc/ar1_pkup.wav",
        /* world_model */ "models/items/armor/jacket/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "i_jacketarmor",
        /* use_name */  "Jacket Armor",
        /* pickup_name */  "Jacket Armor",
        /* pickup_name_definite */ "the Jacket Armor",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_ARMOR,
        /* vwep_model */ NULL,
        /* armor_info */ &jacketarmor_info
    },

    /*QUAKED item_armor_shard (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_ARMOR_SHARD,
        /* classname */ "item_armor_shard",
        /* pickup */ Pickup_Armor,
        /* use */ NULL,
        /* drop */ NULL,
        /* weaponthink */ NULL,
        /* pickup_sound */ "misc/ar2_pkup.wav",
        /* world_model */ "models/items/armor/shard/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "i_armor_shard",
        /* use_name */  "Armor Shard",
        /* pickup_name */  "Armor Shard",
        /* pickup_name_definite */ "the Armor Shard",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_ARMOR
    },

    /*QUAKED item_power_screen (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_ITEM_POWER_SCREEN,
        /* classname */ "item_power_screen",
        /* pickup */ Pickup_PowerArmor,
        /* use */ Use_PowerArmor,
        /* drop */ Drop_PowerArmor,
        /* weaponthink */ NULL,
        /* pickup_sound */ "misc/ar3_pkup.wav",
        /* world_model */ "models/items/armor/screen/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "i_powerscreen",
        /* use_name */  "Power Screen",
        /* pickup_name */  "Power Screen",
        /* pickup_name_definite */ "the Power Screen",
        /* quantity */ 60,
        /* ammo */ IT_AMMO_CELLS,
        /* chain */ IT_NULL,
        /* flags */ IF_ARMOR | IF_POWERUP_WHEEL | IF_POWERUP_ONOFF,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_SCREEN,
        /* precaches */ "misc/power2.wav misc/power1.wav"
    },

    /*QUAKED item_power_shield (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_ITEM_POWER_SHIELD,
        /* classname */ "item_power_shield",
        /* pickup */ Pickup_PowerArmor,
        /* use */ Use_PowerArmor,
        /* drop */ Drop_PowerArmor,
        /* weaponthink */ NULL,
        /* pickup_sound */ "misc/ar3_pkup.wav",
        /* world_model */ "models/items/armor/shield/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "i_powershield",
        /* use_name */  "Power Shield",
        /* pickup_name */  "Power Shield",
        /* pickup_name_definite */ "the Power Shield",
        /* quantity */ 60,
        /* ammo */ IT_AMMO_CELLS,
        /* chain */ IT_NULL,
        /* flags */ IF_ARMOR | IF_POWERUP_WHEEL | IF_POWERUP_ONOFF,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_SHIELD,
        /* precaches */ "misc/power2.wav misc/power1.wav"
    },

    //
    // WEAPONS
    //

    /* weapon_grapple (.3 .3 1) (-16 -16 -16) (16 16 16)
    always owned, never in the world
    */
    {
        /* id */ IT_WEAPON_GRAPPLE,
        /* classname */ "weapon_grapple",
        /* pickup */ NULL,
        /* use */ Use_Weapon,
        /* drop */ NULL,
        /* weaponthink */ CTFWeapon_Grapple,
        /* pickup_sound */ NULL,
        /* world_model */ NULL,
        /* world_model_flags */ EF_NONE,
        /* view_model */ "models/weapons/grapple/tris.md2",
        /* icon */ "w_grapple",
        /* use_name */  "Grapple",
        /* pickup_name */  "Grapple",
        /* pickup_name_definite */ "the Grapple",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_WEAPON_CHAINFIST,
        /* flags */ IF_WEAPON | IF_NO_HASTE | IF_POWERUP_WHEEL | IF_NOT_RANDOM,
        /* vwep_model */ "#w_grapple.md2",
        /* armor_info */ NULL,
        /* tag */ 0,
        /* precaches */ "weapons/grapple/grfire.wav weapons/grapple/grpull.wav weapons/grapple/grhang.wav weapons/grapple/grreset.wav weapons/grapple/grhit.wav weapons/grapple/grfly.wav"
    },

    /* weapon_blaster (.3 .3 1) (-16 -16 -16) (16 16 16)
    always owned, never in the world
    */
    {
        /* id */ IT_WEAPON_BLASTER,
        /* classname */ "weapon_blaster",
        /* pickup */ Pickup_Weapon,
        /* use */ Use_Weapon,
        /* drop */ NULL,
        /* weaponthink */ Weapon_Blaster,
        /* pickup_sound */ "misc/w_pkup.wav",
        /* world_model */ "models/weapons/g_blast/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ "models/weapons/v_blast/tris.md2",
        /* icon */ "w_blaster",
        /* use_name */  "Blaster",
        /* pickup_name */  "Blaster",
        /* pickup_name_definite */ "the Blaster",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_WEAPON_GRAPPLE,
        /* flags */ IF_WEAPON | IF_STAY_COOP | IF_NOT_RANDOM,
        /* vwep_model */ "#w_blaster.md2",
        /* armor_info */ NULL,
        /* tag */ 0,
        /* precaches */ "weapons/blastf1a.wav misc/lasfly.wav"
    },

    /*QUAKED weapon_chainfist (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
    */
    {
        /* id */ IT_WEAPON_CHAINFIST,
        /* classname */ "weapon_chainfist",
        /* pickup */ Pickup_Weapon,
        /* use */ Use_Weapon,
        /* drop */ Drop_Weapon,
        /* weaponthink */ Weapon_ChainFist,
        /* pickup_sound */ "misc/w_pkup.wav",
        /* world_model */ "models/weapons/g_chainf/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ "models/weapons/v_chainf/tris.md2",
        /* icon */ "w_chainfist",
        /* use_name */  "Chainfist",
        /* pickup_name */  "Chainfist",
        /* pickup_name_definite */ "the Chainfist",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_WEAPON_BLASTER,
        /* flags */ IF_WEAPON | IF_STAY_COOP | IF_NO_HASTE,
        /* vwep_model */ "#w_chainfist.md2",
        /* armor_info */ NULL,
        /* tag */ 0,
        /* precaches */ "weapons/sawidle.wav weapons/sawhit.wav weapons/sawslice.wav",
    },

    /*QUAKED weapon_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16)
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons/g_shotg/tris.md2"
    */
    {
        /* id */ IT_WEAPON_SHOTGUN,
        /* classname */ "weapon_shotgun",
        /* pickup */ Pickup_Weapon,
        /* use */ Use_Weapon,
        /* drop */ Drop_Weapon,
        /* weaponthink */ Weapon_Shotgun,
        /* pickup_sound */ "misc/w_pkup.wav",
        /* world_model */ "models/weapons/g_shotg/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ "models/weapons/v_shotg/tris.md2",
        /* icon */ "w_shotgun",
        /* use_name */  "Shotgun",
        /* pickup_name */  "Shotgun",
        /* pickup_name_definite */ "the Shotgun",
        /* quantity */ 1,
        /* ammo */ IT_AMMO_SHELLS,
        /* chain */ IT_NULL,
        /* flags */ IF_WEAPON | IF_STAY_COOP,
        /* vwep_model */ "#w_shotgun.md2",
        /* armor_info */ NULL,
        /* tag */ 0,
        /* precaches */ "weapons/shotgf1b.wav weapons/shotgr1b.wav"
    },

    /*QUAKED weapon_supershotgun (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_WEAPON_SSHOTGUN,
        /* classname */ "weapon_supershotgun",
        /* pickup */ Pickup_Weapon,
        /* use */ Use_Weapon,
        /* drop */ Drop_Weapon,
        /* weaponthink */ Weapon_SuperShotgun,
        /* pickup_sound */ "misc/w_pkup.wav",
        /* world_model */ "models/weapons/g_shotg2/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ "models/weapons/v_shotg2/tris.md2",
        /* icon */ "w_sshotgun",
        /* use_name */  "Super Shotgun",
        /* pickup_name */  "Super Shotgun",
        /* pickup_name_definite */ "the Super Shotgun",
        /* quantity */ 2,
        /* ammo */ IT_AMMO_SHELLS,
        /* chain */ IT_NULL,
        /* flags */ IF_WEAPON | IF_STAY_COOP,
        /* vwep_model */ "#w_sshotgun.md2",
        /* armor_info */ NULL,
        /* tag */ 0,
        /* precaches */ "weapons/sshotf1b.wav",
        /* sort_id */ 0,
        /* quantity_warn */ 10
    },

    /*QUAKED weapon_machinegun (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_WEAPON_MACHINEGUN,
        /* classname */ "weapon_machinegun",
        /* pickup */ Pickup_Weapon,
        /* use */ Use_Weapon,
        /* drop */ Drop_Weapon,
        /* weaponthink */ Weapon_Machinegun,
        /* pickup_sound */ "misc/w_pkup.wav",
        /* world_model */ "models/weapons/g_machn/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ "models/weapons/v_machn/tris.md2",
        /* icon */ "w_machinegun",
        /* use_name */  "Machinegun",
        /* pickup_name */  "Machinegun",
        /* pickup_name_definite */ "the Machinegun",
        /* quantity */ 1,
        /* ammo */ IT_AMMO_BULLETS,
        /* chain */ IT_WEAPON_ETF_RIFLE,
        /* flags */ IF_WEAPON | IF_STAY_COOP,
        /* vwep_model */ "#w_machinegun.md2",
        /* armor_info */ NULL,
        /* tag */ 0,
        /* precaches */ "weapons/machgf1b.wav weapons/machgf2b.wav weapons/machgf3b.wav weapons/machgf4b.wav weapons/machgf5b.wav",
        /* sort_id */ 0,
        /* quantity_warn */ 30
    },

    // ROGUE
    /*QUAKED weapon_etf_rifle (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
    */
    {
        /* id */ IT_WEAPON_ETF_RIFLE,
        /* classname */ "weapon_etf_rifle",
        /* pickup */ Pickup_Weapon,
        /* use */ Use_Weapon,
        /* drop */ Drop_Weapon,
        /* weaponthink */ Weapon_ETF_Rifle,
        /* pickup_sound */ "misc/w_pkup.wav",
        /* world_model */ "models/weapons/g_etf_rifle/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ "models/weapons/v_etf_rifle/tris.md2",
        /* icon */ "w_etf_rifle",
        /* use_name */  "ETF Rifle",
        /* pickup_name */  "ETF Rifle",
        /* pickup_name_definite */ "the ETF Rifle",
        /* quantity */ 1,
        /* ammo */ IT_AMMO_FLECHETTES,
        /* chain */ IT_WEAPON_MACHINEGUN,
        /* flags */ IF_WEAPON | IF_STAY_COOP,
        /* vwep_model */ "#w_etfrifle.md2",
        /* armor_info */ NULL,
        /* tag */ 0,
        /* precaches */ "weapons/nail1.wav models/proj/flechette/tris.md2",
        /* sort_id */ 0,
        /* quantity_warn */ 30
    },
    // ROGUE

    /*QUAKED weapon_chaingun (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_WEAPON_CHAINGUN,
        /* classname */ "weapon_chaingun",
        /* pickup */ Pickup_Weapon,
        /* use */ Use_Weapon,
        /* drop */ Drop_Weapon,
        /* weaponthink */ Weapon_Chaingun,
        /* pickup_sound */ "misc/w_pkup.wav",
        /* world_model */ "models/weapons/g_chain/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ "models/weapons/v_chain/tris.md2",
        /* icon */ "w_chaingun",
        /* use_name */  "Chaingun",
        /* pickup_name */  "Chaingun",
        /* pickup_name_definite */ "the Chaingun",
        /* quantity */ 1,
        /* ammo */ IT_AMMO_BULLETS,
        /* chain */ IT_NULL,
        /* flags */ IF_WEAPON | IF_STAY_COOP,
        /* vwep_model */ "#w_chaingun.md2",
        /* armor_info */ NULL,
        /* tag */ 0,
        /* precaches */ "weapons/chngnu1a.wav weapons/chngnl1a.wav weapons/machgf3b.wav weapons/chngnd1a.wav",
        /* sort_id */ 0,
        /* quantity_warn */ 60
    },

    /*QUAKED ammo_grenades (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_AMMO_GRENADES,
        /* classname */ "ammo_grenades",
        /* pickup */ Pickup_Ammo,
        /* use */ Use_Weapon,
        /* drop */ Drop_Ammo,
        /* weaponthink */ Weapon_Grenade,
        /* pickup_sound */ "misc/am_pkup.wav",
        /* world_model */ "models/items/ammo/grenades/medium/tris.md2",
        /* world_model_flags */ EF_NONE,
        /* view_model */ "models/weapons/v_handgr/tris.md2",
        /* icon */ "a_grenades",
        /* use_name */  "Grenades",
        /* pickup_name */  "Grenades",
        /* pickup_name_definite */ "the Grenades",
        /* quantity */ 5,
        /* ammo */ IT_AMMO_GRENADES,
        /* chain */ IT_AMMO_TRAP,
        /* flags */ IF_AMMO | IF_WEAPON,
        /* vwep_model */ "#a_grenades.md2",
        /* armor_info */ NULL,
        /* tag */ AMMO_GRENADES,
        /* precaches */ "weapons/hgrent1a.wav weapons/hgrena1b.wav weapons/hgrenc1b.wav weapons/hgrenb1a.wav weapons/hgrenb2a.wav models/objects/grenade3/tris.md2",
        /* sort_id */ 0,
        /* quantity_warn */ 2
    },

// RAFAEL
    /*QUAKED ammo_trap (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_AMMO_TRAP,
        /* classname */ "ammo_trap",
        /* pickup */ Pickup_Ammo,
        /* use */ Use_Weapon,
        /* drop */ Drop_Ammo,
        /* weaponthink */ Weapon_Trap,
        /* pickup_sound */ "misc/am_pkup.wav",
        /* world_model */ "models/weapons/g_trap/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ "models/weapons/v_trap/tris.md2",
        /* icon */ "a_trap",
        /* use_name */  "Trap",
        /* pickup_name */  "Trap",
        /* pickup_name_definite */ "the Trap",
        /* quantity */ 1,
        /* ammo */ IT_AMMO_TRAP,
        /* chain */ IT_AMMO_TESLA,
        /* flags */ IF_AMMO | IF_WEAPON | IF_NO_INFINITE_AMMO,
        /* vwep_model */ "#a_trap.md2",
        /* armor_info */ NULL,
        /* tag */ AMMO_TRAP,
        /* precaches */ "misc/fhit3.wav weapons/trapcock.wav weapons/traploop.wav weapons/trapsuck.wav weapons/trapdown.wav items/s_health.wav items/n_health.wav items/l_health.wav items/m_health.wav models/weapons/z_trap/tris.md2",
        /* sort_id */ 0,
        /* quantity_warn */ 1
    },
// RAFAEL

    /*QUAKED ammo_tesla (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
    */
    {
        /* id */ IT_AMMO_TESLA,
        /* classname */ "ammo_tesla",
        /* pickup */ Pickup_Ammo,
        /* use */ Use_Weapon,
        /* drop */ Drop_Ammo,
        /* weaponthink */ Weapon_Tesla,
        /* pickup_sound */ "misc/am_pkup.wav",
        /* world_model */ "models/ammo/am_tesl/tris.md2",
        /* world_model_flags */ EF_NONE,
        /* view_model */ "models/weapons/v_tesla/tris.md2",
        /* icon */ "a_tesla",
        /* use_name */  "Tesla",
        /* pickup_name */  "Tesla",
        /* pickup_name_definite */ "the Tesla",
        /* quantity */ 3,
        /* ammo */ IT_AMMO_TESLA,
        /* chain */ IT_AMMO_GRENADES,
        /* flags */ IF_AMMO | IF_WEAPON | IF_NO_INFINITE_AMMO,
        /* vwep_model */ "#a_tesla.md2",
        /* armor_info */ NULL,
        /* tag */ AMMO_TESLA,
        /* precaches */ "weapons/teslaopen.wav weapons/hgrenb1a.wav weapons/hgrenb2a.wav models/weapons/g_tesla/tris.md2",
        /* sort_id */ 0,
        /* quantity_warn */ 1
    },

    /*QUAKED weapon_grenadelauncher (.3 .3 1) (-16 -16 -16) (16 16 16)
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons/g_launch/tris.md2"
    */
    {
        /* id */ IT_WEAPON_GLAUNCHER,
        /* classname */ "weapon_grenadelauncher",
        /* pickup */ Pickup_Weapon,
        /* use */ Use_Weapon,
        /* drop */ Drop_Weapon,
        /* weaponthink */ Weapon_GrenadeLauncher,
        /* pickup_sound */ "misc/w_pkup.wav",
        /* world_model */ "models/weapons/g_launch/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ "models/weapons/v_launch/tris.md2",
        /* icon */ "w_glauncher",
        /* use_name */  "Grenade Launcher",
        /* pickup_name */  "Grenade Launcher",
        /* pickup_name_definite */ "the Grenade Launcher",
        /* quantity */ 1,
        /* ammo */ IT_AMMO_GRENADES,
        /* chain */ IT_WEAPON_PROXLAUNCHER,
        /* flags */ IF_WEAPON | IF_STAY_COOP,
        /* vwep_model */ "#w_glauncher.md2",
        /* armor_info */ NULL,
        /* tag */ 0,
        /* precaches */ "models/objects/grenade4/tris.md2 weapons/grenlf1a.wav weapons/grenlr1b.wav weapons/grenlb1b.wav"
    },

    // ROGUE
    /*QUAKED weapon_proxlauncher (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
    */
    {
        /* id */ IT_WEAPON_PROXLAUNCHER,
        /* classname */ "weapon_proxlauncher",
        /* pickup */ Pickup_Weapon,
        /* use */ Use_Weapon,
        /* drop */ Drop_Weapon,
        /* weaponthink */ Weapon_ProxLauncher,
        /* pickup_sound */ "misc/w_pkup.wav",
        /* world_model */ "models/weapons/g_plaunch/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ "models/weapons/v_plaunch/tris.md2",
        /* icon */ "w_proxlaunch",
        /* use_name */  "Prox Launcher",
        /* pickup_name */  "Prox Launcher",
        /* pickup_name_definite */ "the Prox Launcher",
        /* quantity */ 1,
        /* ammo */ IT_AMMO_PROX,
        /* chain */ IT_WEAPON_GLAUNCHER,
        /* flags */ IF_WEAPON | IF_STAY_COOP,
        /* vwep_model */ "#w_plauncher.md2",
        /* armor_info */ NULL,
        /* tag */ AMMO_PROX,
        /* precaches */ "weapons/grenlf1a.wav weapons/grenlr1b.wav weapons/grenlb1b.wav weapons/proxwarn.wav weapons/proxopen.wav",
    },
    // ROGUE

    /*QUAKED weapon_rocketlauncher (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_WEAPON_RLAUNCHER,
        /* classname */ "weapon_rocketlauncher",
        /* pickup */ Pickup_Weapon,
        /* use */ Use_Weapon,
        /* drop */ Drop_Weapon,
        /* weaponthink */ Weapon_RocketLauncher,
        /* pickup_sound */ "misc/w_pkup.wav",
        /* world_model */ "models/weapons/g_rocket/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ "models/weapons/v_rocket/tris.md2",
        /* icon */ "w_rlauncher",
        /* use_name */  "Rocket Launcher",
        /* pickup_name */  "Rocket Launcher",
        /* pickup_name_definite */ "the Rocket Launcher",
        /* quantity */ 1,
        /* ammo */ IT_AMMO_ROCKETS,
        /* chain */ IT_NULL,
        /* flags */ IF_WEAPON | IF_STAY_COOP,
        /* vwep_model */ "#w_rlauncher.md2",
        /* armor_info */ NULL,
        /* tag */ 0,
        /* precaches */ "models/objects/rocket/tris.md2 weapons/rockfly.wav weapons/rocklf1a.wav weapons/rocklr1b.wav models/objects/debris2/tris.md2"
    },

    /*QUAKED weapon_hyperblaster (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_WEAPON_HYPERBLASTER,
        /* classname */ "weapon_hyperblaster",
        /* pickup */ Pickup_Weapon,
        /* use */ Use_Weapon,
        /* drop */ Drop_Weapon,
        /* weaponthink */ Weapon_HyperBlaster,
        /* pickup_sound */ "misc/w_pkup.wav",
        /* world_model */ "models/weapons/g_hyperb/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ "models/weapons/v_hyperb/tris.md2",
        /* icon */ "w_hyperblaster",
        /* use_name */  "HyperBlaster",
        /* pickup_name */  "HyperBlaster",
        /* pickup_name_definite */ "the HyperBlaster",
        /* quantity */ 1,
        /* ammo */ IT_AMMO_CELLS,
        /* chain */ IT_WEAPON_IONRIPPER,
        /* flags */ IF_WEAPON | IF_STAY_COOP,
        /* vwep_model */ "#w_hyperblaster.md2",
        /* armor_info */ NULL,
        /* tag */ 0,
        /* precaches */ "weapons/hyprbu1a.wav weapons/hyprbl1a.wav weapons/hyprbf1a.wav weapons/hyprbd1a.wav misc/lasfly.wav",
        /* sort_id */ 0,
        /* quantity_warn */ 30
    },

// RAFAEL
    /*QUAKED weapon_boomer (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_WEAPON_IONRIPPER,
        /* classname */ "weapon_boomer",
        /* pickup */ Pickup_Weapon,
        /* use */ Use_Weapon,
        /* drop */ Drop_Weapon,
        /* weaponthink */ Weapon_Ionripper,
        /* pickup_sound */ "misc/w_pkup.wav",
        /* world_model */ "models/weapons/g_boom/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ "models/weapons/v_boomer/tris.md2",
        /* icon */ "w_ripper",
        /* use_name */  "Ionripper",
        /* pickup_name */  "Ionripper",
        /* pickup_name_definite */ "the Ionripper",
        /* quantity */ 2,
        /* ammo */ IT_AMMO_CELLS,
        /* chain */ IT_WEAPON_PLASMABEAM,
        /* flags */ IF_WEAPON | IF_STAY_COOP,
        /* vwep_model */ "#w_ripper.md2",
        /* armor_info */ NULL,
        /* tag */ 0,
        /* precaches */ "weapons/rippfire.wav models/objects/boomrang/tris.md2 misc/lasfly.wav",
        /* sort_id */ 0,
        /* quantity_warn */ 30
    },
// RAFAEL

// ROGUE
    /*QUAKED weapon_plasmabeam (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
    */
    {
        /* id */ IT_WEAPON_PLASMABEAM,
        /* classname */ "weapon_plasmabeam",
        /* pickup */ Pickup_Weapon,
        /* use */ Use_Weapon,
        /* drop */ Drop_Weapon,
        /* weaponthink */ Weapon_Heatbeam,
        /* pickup_sound */ "misc/w_pkup.wav",
        /* world_model */ "models/weapons/g_beamer/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ "models/weapons/v_beamer/tris.md2",
        /* icon */ "w_heatbeam",
        /* use_name */  "Plasma Beam",
        /* pickup_name */  "Plasma Beam",
        /* pickup_name_definite */ "the Plasma Beam",
        /* quantity */ 2,
        /* ammo */ IT_AMMO_CELLS,
        /* chain */ IT_WEAPON_HYPERBLASTER,
        /* flags */ IF_WEAPON | IF_STAY_COOP,
        /* vwep_model */ "#w_plasma.md2",
        /* armor_info */ NULL,
        /* tag */ 0,
        /* precaches */ "weapons/bfg__l1a.wav",
        /* sort_id */ 0,
        /* quantity_warn */ 50
    },
//rogue

    /*QUAKED weapon_railgun (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_WEAPON_RAILGUN,
        /* classname */ "weapon_railgun",
        /* pickup */ Pickup_Weapon,
        /* use */ Use_Weapon,
        /* drop */ Drop_Weapon,
        /* weaponthink */ Weapon_Railgun,
        /* pickup_sound */ "misc/w_pkup.wav",
        /* world_model */ "models/weapons/g_rail/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ "models/weapons/v_rail/tris.md2",
        /* icon */ "w_railgun",
        /* use_name */  "Railgun",
        /* pickup_name */  "Railgun",
        /* pickup_name_definite */ "the Railgun",
        /* quantity */ 1,
        /* ammo */ IT_AMMO_SLUGS,
        /* chain */ IT_WEAPON_PHALANX,
        /* flags */ IF_WEAPON | IF_STAY_COOP,
        /* vwep_model */ "#w_railgun.md2",
        /* armor_info */ NULL,
        /* tag */ 0,
        /* precaches */ "weapons/rg_hum.wav"
    },

// RAFAEL 14-APR-98
    /*QUAKED weapon_phalanx (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_WEAPON_PHALANX,
        /* classname */ "weapon_phalanx",
        /* pickup */ Pickup_Weapon,
        /* use */ Use_Weapon,
        /* drop */ Drop_Weapon,
        /* weaponthink */ Weapon_Phalanx,
        /* pickup_sound */ "misc/w_pkup.wav",
        /* world_model */ "models/weapons/g_shotx/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ "models/weapons/v_shotx/tris.md2",
        /* icon */ "w_phallanx",
        /* use_name */  "Phalanx",
        /* pickup_name */  "Phalanx",
        /* pickup_name_definite */ "the Phalanx",
        /* quantity */ 1,
        /* ammo */ IT_AMMO_MAGSLUG,
        /* chain */ IT_WEAPON_RAILGUN,
        /* flags */ IF_WEAPON | IF_STAY_COOP,
        /* vwep_model */ "#w_phalanx.md2",
        /* armor_info */ NULL,
        /* tag */ 0,
        /* precaches */ "weapons/plasshot.wav sprites/s_photon.sp2 weapons/rockfly.wav"
    },
// RAFAEL

    /*QUAKED weapon_bfg (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_WEAPON_BFG,
        /* classname */ "weapon_bfg",
        /* pickup */ Pickup_Weapon,
        /* use */ Use_Weapon,
        /* drop */ Drop_Weapon,
        /* weaponthink */ Weapon_BFG,
        /* pickup_sound */ "misc/w_pkup.wav",
        /* world_model */ "models/weapons/g_bfg/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ "models/weapons/v_bfg/tris.md2",
        /* icon */ "w_bfg",
        /* use_name */  "BFG10K",
        /* pickup_name */  "BFG10K",
        /* pickup_name_definite */ "the BFG10K",
        /* quantity */ 50,
        /* ammo */ IT_AMMO_CELLS,
        /* chain */ IT_WEAPON_DISRUPTOR,
        /* flags */ IF_WEAPON | IF_STAY_COOP,
        /* vwep_model */ "#w_bfg.md2",
        /* armor_info */ NULL,
        /* tag */ 0,
        /* precaches */ "sprites/s_bfg1.sp2 sprites/s_bfg2.sp2 sprites/s_bfg3.sp2 weapons/bfg__f1y.wav weapons/bfg__l1a.wav weapons/bfg__x1b.wav weapons/bfg_hum.wav",
        /* sort_id */ 0,
        /* quantity_warn */ 50
    },

    // =========================
    // ROGUE WEAPONS
    /*QUAKED weapon_disintegrator (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
    */
    {
        /* id */ IT_WEAPON_DISRUPTOR,
        /* classname */ "weapon_disintegrator",
        /* pickup */ Pickup_Weapon,
        /* use */ Use_Weapon,
        /* drop */ Drop_Weapon,
        /* weaponthink */ Weapon_Disintegrator,
        /* pickup_sound */ "misc/w_pkup.wav",
        /* world_model */ "models/weapons/g_dist/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ "models/weapons/v_dist/tris.md2",
        /* icon */ "w_disintegrator",
        /* use_name */  "Disruptor",
        /* pickup_name */  "Disruptor",
        /* pickup_name_definite */ "the Disruptor",
        /* quantity */ 1,
        /* ammo */ IT_AMMO_ROUNDS,
        /* chain */ IT_WEAPON_BFG,
        /* flags */ IF_WEAPON | IF_STAY_COOP,
        /* vwep_model */ "#w_disrupt.md2",
        /* armor_info */ NULL,
        /* tag */ 0,
        /* precaches */ "models/proj/disintegrator/tris.md2 weapons/disrupt.wav weapons/disint2.wav weapons/disrupthit.wav",
    },
    // ROGUE WEAPONS
    // =========================

    //
    // AMMO ITEMS
    //

    /*QUAKED ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_AMMO_SHELLS,
        /* classname */ "ammo_shells",
        /* pickup */ Pickup_Ammo,
        /* use */ NULL,
        /* drop */ Drop_Ammo,
        /* weaponthink */ NULL,
        /* pickup_sound */ "misc/am_pkup.wav",
        /* world_model */ "models/items/ammo/shells/medium/tris.md2",
        /* world_model_flags */ EF_NONE,
        /* view_model */ NULL,
        /* icon */ "a_shells",
        /* use_name */  "Shells",
        /* pickup_name */  "Shells",
        /* pickup_name_definite */ "the Shells",
        /* quantity */ 10,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_AMMO,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ AMMO_SHELLS
    },

    /*QUAKED ammo_bullets (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_AMMO_BULLETS,
        /* classname */ "ammo_bullets",
        /* pickup */ Pickup_Ammo,
        /* use */ NULL,
        /* drop */ Drop_Ammo,
        /* weaponthink */ NULL,
        /* pickup_sound */ "misc/am_pkup.wav",
        /* world_model */ "models/items/ammo/bullets/medium/tris.md2",
        /* world_model_flags */ EF_NONE,
        /* view_model */ NULL,
        /* icon */ "a_bullets",
        /* use_name */  "Bullets",
        /* pickup_name */  "Bullets",
        /* pickup_name_definite */ "the Bullets",
        /* quantity */ 50,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_AMMO,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ AMMO_BULLETS
    },

    /*QUAKED ammo_cells (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_AMMO_CELLS,
        /* classname */ "ammo_cells",
        /* pickup */ Pickup_Ammo,
        /* use */ NULL,
        /* drop */ Drop_Ammo,
        /* weaponthink */ NULL,
        /* pickup_sound */ "misc/am_pkup.wav",
        /* world_model */ "models/items/ammo/cells/medium/tris.md2",
        /* world_model_flags */ EF_NONE,
        /* view_model */ NULL,
        /* icon */ "a_cells",
        /* use_name */  "Cells",
        /* pickup_name */  "Cells",
        /* pickup_name_definite */ "the Cells",
        /* quantity */ 50,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_AMMO,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ AMMO_CELLS
    },

    /*QUAKED ammo_rockets (.3 .3 1) (-16 -16 -16) (16 16 16)
    model="models/items/ammo/rockets/medium/tris.md2"
    */
    {
        /* id */ IT_AMMO_ROCKETS,
        /* classname */ "ammo_rockets",
        /* pickup */ Pickup_Ammo,
        /* use */ NULL,
        /* drop */ Drop_Ammo,
        /* weaponthink */ NULL,
        /* pickup_sound */ "misc/am_pkup.wav",
        /* world_model */ "models/items/ammo/rockets/medium/tris.md2",
        /* world_model_flags */ EF_NONE,
        /* view_model */ NULL,
        /* icon */ "a_rockets",
        /* use_name */  "Rockets",
        /* pickup_name */  "Rockets",
        /* pickup_name_definite */ "the Rockets",
        /* quantity */ 5,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_AMMO,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ AMMO_ROCKETS
    },

    /*QUAKED ammo_slugs (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_AMMO_SLUGS,
        /* classname */ "ammo_slugs",
        /* pickup */ Pickup_Ammo,
        /* use */ NULL,
        /* drop */ Drop_Ammo,
        /* weaponthink */ NULL,
        /* pickup_sound */ "misc/am_pkup.wav",
        /* world_model */ "models/items/ammo/slugs/medium/tris.md2",
        /* world_model_flags */ EF_NONE,
        /* view_model */ NULL,
        /* icon */ "a_slugs",
        /* use_name */  "Slugs",
        /* pickup_name */  "Slugs",
        /* pickup_name_definite */ "the Slugs",
        /* quantity */ 10,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_AMMO,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ AMMO_SLUGS
    },

// RAFAEL
    /*QUAKED ammo_magslug (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_AMMO_MAGSLUG,
        /* classname */ "ammo_magslug",
        /* pickup */ Pickup_Ammo,
        /* use */ NULL,
        /* drop */ Drop_Ammo,
        /* weaponthink */ NULL,
        /* pickup_sound */ "misc/am_pkup.wav",
        /* world_model */ "models/objects/ammo/tris.md2",
        /* world_model_flags */ EF_NONE,
        /* view_model */ NULL,
        /* icon */ "a_mslugs",
        /* use_name */  "Mag Slug",
        /* pickup_name */  "Mag Slug",
        /* pickup_name_definite */ "the Mag Slug",
        /* quantity */ 10,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_AMMO,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ AMMO_MAGSLUG
    },
// RAFAEL

// =======================================
// ROGUE AMMO

    /*QUAKED ammo_flechettes (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
    */
    {
        /* id */ IT_AMMO_FLECHETTES,
        /* classname */ "ammo_flechettes",
        /* pickup */ Pickup_Ammo,
        /* use */ NULL,
        /* drop */ Drop_Ammo,
        /* weaponthink */ NULL,
        /* pickup_sound */ "misc/am_pkup.wav",
        /* world_model */ "models/ammo/am_flechette/tris.md2",
        /* world_model_flags */ EF_NONE,
        /* view_model */ NULL,
        /* icon */ "a_flechettes",
        /* use_name */  "Flechettes",
        /* pickup_name */  "Flechettes",
        /* pickup_name_definite */ "the Flechettes",
        /* quantity */ 50,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_AMMO,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ AMMO_FLECHETTES
    },

    /*QUAKED ammo_prox (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
    */
    {
        /* id */ IT_AMMO_PROX,
        /* classname */ "ammo_prox",
        /* pickup */ Pickup_Ammo,
        /* use */ NULL,
        /* drop */ Drop_Ammo,
        /* weaponthink */ NULL,
        /* pickup_sound */ "misc/am_pkup.wav",
        /* world_model */ "models/ammo/am_prox/tris.md2",
        /* world_model_flags */ EF_NONE,
        /* view_model */ NULL,
        /* icon */ "a_prox",
        /* use_name */  "Prox",
        /* pickup_name */  "Prox",
        /* pickup_name_definite */ "the Prox",
        /* quantity */ 5,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_AMMO,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ AMMO_PROX,
        /* precaches */ "models/weapons/g_prox/tris.md2 weapons/proxwarn.wav"
    },

    /*QUAKED ammo_nuke (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
    */
    {
        /* id */ IT_AMMO_NUKE,
        /* classname */ "ammo_nuke",
        /* pickup */ Pickup_Nuke,
        /* use */ Use_Nuke,
        /* drop */ Drop_Ammo,
        /* weaponthink */ NULL,
        /* pickup_sound */ "misc/am_pkup.wav",
        /* world_model */ "models/weapons/g_nuke/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "p_nuke",
        /* use_name */  "A-M Bomb",
        /* pickup_name */  "A-M Bomb",
        /* pickup_name_definite */ "the A-M Bomb",
        /* quantity */ 300,
        /* ammo */ IT_AMMO_NUKE,
        /* chain */ IT_NULL,
        /* flags */ IF_POWERUP | IF_POWERUP_WHEEL,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_AM_BOMB,
        /* precaches */ "weapons/nukewarn2.wav world/rumble.wav"
    },

    /*QUAKED ammo_disruptor (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
    */
    {
        /* id */ IT_AMMO_ROUNDS,
        /* classname */ "ammo_disruptor",
        /* pickup */ Pickup_Ammo,
        /* use */ NULL,
        /* drop */ Drop_Ammo,
        /* weaponthink */ NULL,
        /* pickup_sound */ "misc/am_pkup.wav",
        /* world_model */ "models/ammo/am_disr/tris.md2",
        /* world_model_flags */ EF_NONE,
        /* view_model */ NULL,
        /* icon */ "a_disruptor",
        /* use_name */  "Rounds",
        /* pickup_name */  "Rounds",
        /* pickup_name_definite */ "the Rounds",
        /* quantity */ 3,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_AMMO,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ AMMO_DISRUPTOR
    },
// ROGUE AMMO
// =======================================

    //
    // POWERUP ITEMS
    //
    /*QUAKED item_quad (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_ITEM_QUAD,
        /* classname */ "item_quad",
        /* pickup */ Pickup_Powerup,
        /* use */ Use_Quad,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/quaddama/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "p_quad",
        /* use_name */  "Quad Damage",
        /* pickup_name */  "Quad Damage",
        /* pickup_name_definite */ "the Quad Damage",
        /* quantity */ 60,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_POWERUP | IF_POWERUP_WHEEL,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_QUAD,
        /* precaches */ "items/damage.wav items/damage2.wav items/damage3.wav ctf/tech2x.wav"
    },

// RAFAEL
    /*QUAKED item_quadfire (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_ITEM_QUADFIRE,
        /* classname */ "item_quadfire",
        /* pickup */ Pickup_Powerup,
        /* use */ Use_QuadFire,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/quadfire/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "p_quadfire",
        /* use_name */  "DualFire Damage",
        /* pickup_name */  "DualFire Damage",
        /* pickup_name_definite */ "the DualFire Damage",
        /* quantity */ 60,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_POWERUP | IF_POWERUP_WHEEL,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_QUADFIRE,
        /* precaches */ "items/quadfire1.wav items/quadfire2.wav items/quadfire3.wav"
    },
// RAFAEL

    /*QUAKED item_invulnerability (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_ITEM_INVULNERABILITY,
        /* classname */ "item_invulnerability",
        /* pickup */ Pickup_Powerup,
        /* use */ Use_Invulnerability,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/invulner/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "p_invulnerability",
        /* use_name */  "Invulnerability",
        /* pickup_name */  "Invulnerability",
        /* pickup_name_definite */ "the Invulnerability",
        /* quantity */ 300,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_POWERUP | IF_POWERUP_WHEEL,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_INVULNERABILITY,
        /* precaches */ "items/protect.wav items/protect2.wav items/protect4.wav"
    },

    /*QUAKED item_invisibility (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_ITEM_INVISIBILITY,
        /* classname */ "item_invisibility",
        /* pickup */ Pickup_Powerup,
        /* use */ Use_Invisibility,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/cloaker/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "p_cloaker",
        /* use_name */  "Invisibility",
        /* pickup_name */  "Invisibility",
        /* pickup_name_definite */ "the Invisibility",
        /* quantity */ 300,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_POWERUP | IF_POWERUP_WHEEL,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_INVISIBILITY,
    },

    /*QUAKED item_silencer (.3 .3 1) (-16 -16 -16) (16 16 16)
    model="models/items/silencer/tris.md2"
    */
    {
        /* id */ IT_ITEM_SILENCER,
        /* classname */ "item_silencer",
        /* pickup */ Pickup_Powerup,
        /* use */ Use_Silencer,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/silencer/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "p_silencer",
        /* use_name */  "Silencer",
        /* pickup_name */  "Silencer",
        /* pickup_name_definite */ "the Silencer",
        /* quantity */ 60,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_POWERUP | IF_POWERUP_WHEEL,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_SILENCER,
    },

    /*QUAKED item_breather (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_ITEM_REBREATHER,
        /* classname */ "item_breather",
        /* pickup */ Pickup_Powerup,
        /* use */ Use_Breather,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/breather/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "p_rebreather",
        /* use_name */  "Rebreather",
        /* pickup_name */  "Rebreather",
        /* pickup_name_definite */ "the Rebreather",
        /* quantity */ 60,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_STAY_COOP | IF_POWERUP | IF_POWERUP_WHEEL,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_REBREATHER,
        /* precaches */ "items/airout.wav"
    },

    /*QUAKED item_enviro (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_ITEM_ENVIROSUIT,
        /* classname */ "item_enviro",
        /* pickup */ Pickup_Powerup,
        /* use */ Use_Envirosuit,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/enviro/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "p_envirosuit",
        /* use_name */  "Environment Suit",
        /* pickup_name */  "Environment Suit",
        /* pickup_name_definite */ "the Environment Suit",
        /* quantity */ 60,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_STAY_COOP | IF_POWERUP | IF_POWERUP_WHEEL,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_ENVIROSUIT,
        /* precaches */ "items/airout.wav"
    },

    /*QUAKED item_ancient_head (.3 .3 1) (-16 -16 -16) (16 16 16)
    Special item that gives +2 to maximum health
    model="models/items/c_head/tris.md2"
    */
    {
        /* id */ IT_ITEM_ANCIENT_HEAD,
        /* classname */ "item_ancient_head",
        /* pickup */ Pickup_LegacyHead,
        /* use */ NULL,
        /* drop */ NULL,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/c_head/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "i_fixme",
        /* use_name */  "Ancient Head",
        /* pickup_name */  "Ancient Head",
        /* pickup_name_definite */ "the Ancient Head",
        /* quantity */ 60,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_HEALTH | IF_NOT_RANDOM,
    },

    /*QUAKED item_legacy_head (.3 .3 1) (-16 -16 -16) (16 16 16)
    Special item that gives +5 to maximum health
    model="models/items/legacyhead/tris.md2"
    */
    {
        /* id */ IT_ITEM_LEGACY_HEAD,
        /* classname */ "item_legacy_head",
        /* pickup */ Pickup_LegacyHead,
        /* use */ NULL,
        /* drop */ NULL,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/legacyhead/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "i_fixme",
        /* use_name */  "Legacy Head",
        /* pickup_name */  "Legacy Head",
        /* pickup_name_definite */ "the Legacy Head",
        /* quantity */ 60,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_HEALTH | IF_NOT_RANDOM,
    },

    /*QUAKED item_adrenaline (.3 .3 1) (-16 -16 -16) (16 16 16)
    gives +1 to maximum health
    */
    {
        /* id */ IT_ITEM_ADRENALINE,
        /* classname */ "item_adrenaline",
        /* pickup */ Pickup_Powerup,
        /* use */ Use_Adrenaline,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/adrenal/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "p_adrenaline",
        /* use_name */  "Adrenaline",
        /* pickup_name */  "Adrenaline",
        /* pickup_name_definite */ "the Adrenaline",
        /* quantity */ 60,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_HEALTH | IF_POWERUP_WHEEL,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_ADRENALINE,
        /* precache */ "items/n_health.wav"
    },

    /*QUAKED item_bandolier (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_ITEM_BANDOLIER,
        /* classname */ "item_bandolier",
        /* pickup */ Pickup_Bandolier,
        /* use */ NULL,
        /* drop */ NULL,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/band/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "p_bandolier",
        /* use_name */  "Bandolier",
        /* pickup_name */  "Bandolier",
        /* pickup_name_definite */ "the Bandolier",
        /* quantity */ 60,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_POWERUP
    },

    /*QUAKED item_pack (.3 .3 1) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_ITEM_PACK,
        /* classname */ "item_pack",
        /* pickup */ Pickup_Pack,
        /* use */ NULL,
        /* drop */ NULL,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/pack/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "i_pack",
        /* use_name */  "Ammo Pack",
        /* pickup_name */  "Ammo Pack",
        /* pickup_name_definite */ "the Ammo Pack",
        /* quantity */ 180,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_POWERUP
    },


// ======================================
// PGM

    /*QUAKED item_ir_goggles (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
    gives +1 to maximum health
    */
    {
        /* id */ IT_ITEM_IR_GOGGLES,
        /* classname */ "item_ir_goggles",
        /* pickup */ Pickup_Powerup,
        /* use */ Use_IR,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/goggles/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "p_ir",
        /* use_name */  "IR Goggles",
        /* pickup_name */  "IR Goggles",
        /* pickup_name_definite */ "the IR Goggles",
        /* quantity */ 60,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_POWERUP | IF_POWERUP_WHEEL,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_IR_GOGGLES,
        /* precaches */ "misc/ir_start.wav"
    },

    /*QUAKED item_double (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
    */
    {
        /* id */ IT_ITEM_DOUBLE,
        /* classname */ "item_double",
        /* pickup */ Pickup_Powerup,
        /* use */ Use_Double,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/ddamage/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "p_double",
        /* use_name */  "Double Damage",
        /* pickup_name */  "Double Damage",
        /* pickup_name_definite */ "the Double Damage",
        /* quantity */ 60,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_POWERUP | IF_POWERUP_WHEEL,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_DOUBLE,
        /* precaches */ "misc/ddamage1.wav misc/ddamage2.wav misc/ddamage3.wav ctf/tech2x.wav"
    },

    /*QUAKED item_sphere_vengeance (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
    */
    {
        /* id */ IT_ITEM_SPHERE_VENGEANCE,
        /* classname */ "item_sphere_vengeance",
        /* pickup */ Pickup_Sphere,
        /* use */ Use_Vengeance,
        /* drop */ NULL,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/vengnce/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "p_vengeance",
        /* use_name */  "vengeance sphere",
        /* pickup_name */  "Vengeance Sphere",
        /* pickup_name_definite */ "the Vengeance Sphere",
        /* quantity */ 60,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_POWERUP | IF_POWERUP_WHEEL,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_SPHERE_VENGEANCE,
        /* precaches */ "spheres/v_idle.wav"
    },

    /*QUAKED item_sphere_hunter (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
    */
    {
        /* id */ IT_ITEM_SPHERE_HUNTER,
        /* classname */ "item_sphere_hunter",
        /* pickup */ Pickup_Sphere,
        /* use */ Use_Hunter,
        /* drop */ NULL,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/hunter/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "p_hunter",
        /* use_name */  "hunter sphere",
        /* pickup_name */  "Hunter Sphere",
        /* pickup_name_definite */ "the Hunter Sphere",
        /* quantity */ 120,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_POWERUP | IF_POWERUP_WHEEL,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_SPHERE_HUNTER,
        /* precaches */ "spheres/h_idle.wav spheres/h_active.wav spheres/h_lurk.wav"
    },

    /*QUAKED item_sphere_defender (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
    */
    {
        /* id */ IT_ITEM_SPHERE_DEFENDER,
        /* classname */ "item_sphere_defender",
        /* pickup */ Pickup_Sphere,
        /* use */ Use_Defender,
        /* drop */ NULL,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/defender/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "p_defender",
        /* use_name */  "defender sphere",
        /* pickup_name */  "Defender Sphere",
        /* pickup_name_definite */ "the Defender Sphere",
        /* quantity */ 60,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_POWERUP | IF_POWERUP_WHEEL,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_SPHERE_DEFENDER,
        /* precaches */ "models/objects/laser/tris.md2 models/items/shell/tris.md2 spheres/d_idle.wav"
    },

    /*QUAKED item_doppleganger (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
    */
    {
        /* id */ IT_ITEM_DOPPELGANGER,
        /* classname */ "item_doppleganger",
        /* pickup */ Pickup_Doppleganger,
        /* use */ Use_Doppleganger,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/dopple/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "p_doppleganger",
        /* use_name */  "Doppelganger",
        /* pickup_name */  "Doppelganger",
        /* pickup_name_definite */ "the Doppelganger",
        /* quantity */ 90,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_POWERUP | IF_POWERUP_WHEEL,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_DOPPELGANGER,
        /* precaches */ "models/objects/dopplebase/tris.md2 models/items/spawngro3/tris.md2 medic_commander/monsterspawn1.wav models/items/hunter/tris.md2 models/items/vengnce/tris.md2",
    },

    {
        /* id */ IT_ITEM_TAG_TOKEN,
        /* classname */ NULL,
        /* pickup */ Tag_PickupToken,
        /* use */ NULL,
        /* drop */ NULL,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/tagtoken/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB | EF_TAGTRAIL,
        /* view_model */ NULL,
        /* icon */ "i_tagtoken",
        /* use_name */  "Tag Token",
        /* pickup_name */  "Tag Token",
        /* pickup_name_definite */ "the Tag Token",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_POWERUP | IF_NOT_GIVEABLE
    },

// PGM
// ======================================

    //
    // KEYS
    //
    /*QUAKED key_data_cd (0 .5 .8) (-16 -16 -16) (16 16 16)
    key for computer centers
    */
    {
        /* id */ IT_KEY_DATA_CD,
        /* classname */ "key_data_cd",
        /* pickup */ Pickup_Key,
        /* use */ NULL,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/keys/data_cd/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "k_datacd",
        /* use_name */  "Data CD",
        /* pickup_name */  "Data CD",
        /* pickup_name_definite */ "the Data CD",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_STAY_COOP | IF_KEY
    },

    /*QUAKED key_power_cube (0 .5 .8) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN NO_TOUCH
    warehouse circuits
    */
    {
        /* id */ IT_KEY_POWER_CUBE,
        /* classname */ "key_power_cube",
        /* pickup */ Pickup_Key,
        /* use */ NULL,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/keys/power/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "k_powercube",
        /* use_name */  "Power Cube",
        /* pickup_name */  "Power Cube",
        /* pickup_name_definite */ "the Power Cube",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_STAY_COOP | IF_KEY
    },

    /*QUAKED key_explosive_charges (0 .5 .8) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN NO_TOUCH
    warehouse circuits
    */
    {
        /* id */ IT_KEY_EXPLOSIVE_CHARGES,
        /* classname */ "key_explosive_charges",
        /* pickup */ Pickup_Key,
        /* use */ NULL,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/n64/charge/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "n64/i_charges",
        /* use_name */  "Explosive Charges",
        /* pickup_name */  "Explosive Charges",
        /* pickup_name_definite */ "the Explosive Charges",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_STAY_COOP | IF_KEY
    },

    /*QUAKED key_yellow_key (0 .5 .8) (-16 -16 -16) (16 16 16)
    normal door key - yellow
    [Sam-KEX] New key type for Q2 N64
    */
    {
        /* id */ IT_KEY_YELLOW,
        /* classname */ "key_yellow_key",
        /* pickup */ Pickup_Key,
        /* use */ NULL,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/n64/yellow_key/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "n64/i_yellow_key",
        /* use_name */  "Yellow Key",
        /* pickup_name */  "Yellow Key",
        /* pickup_name_definite */ "the Yellow Key",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_STAY_COOP | IF_KEY
    },

    /*QUAKED key_power_core (0 .5 .8) (-16 -16 -16) (16 16 16)
    key for N64
    */
    {
        /* id */ IT_KEY_POWER_CORE,
        /* classname */ "key_power_core",
        /* pickup */ Pickup_Key,
        /* use */ NULL,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/n64/power_core/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "k_pyramid",
        /* use_name */  "Power Core",
        /* pickup_name */  "Power Core",
        /* pickup_name_definite */ "the Power Core",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_STAY_COOP | IF_KEY
    },

    /*QUAKED key_pyramid (0 .5 .8) (-16 -16 -16) (16 16 16)
    key for the entrance of jail3
    */
    {
        /* id */ IT_KEY_PYRAMID,
        /* classname */ "key_pyramid",
        /* pickup */ Pickup_Key,
        /* use */ NULL,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/keys/pyramid/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "k_pyramid",
        /* use_name */  "Pyramid Key",
        /* pickup_name */  "Pyramid Key",
        /* pickup_name_definite */ "the Pyramid Key",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_STAY_COOP | IF_KEY
    },

    /*QUAKED key_data_spinner (0 .5 .8) (-16 -16 -16) (16 16 16)
    key for the city computer
    model="models/items/keys/spinner/tris.md2"
    */
    {
        /* id */ IT_KEY_DATA_SPINNER,
        /* classname */ "key_data_spinner",
        /* pickup */ Pickup_Key,
        /* use */ NULL,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/keys/spinner/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "k_dataspin",
        /* use_name */  "Data Spinner",
        /* pickup_name */  "Data Spinner",
        /* pickup_name_definite */ "the Data Spinner",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_STAY_COOP | IF_KEY
    },

    /*QUAKED key_pass (0 .5 .8) (-16 -16 -16) (16 16 16)
    security pass for the security level
    model="models/items/keys/pass/tris.md2"
    */
    {
        /* id */ IT_KEY_PASS,
        /* classname */ "key_pass",
        /* pickup */ Pickup_Key,
        /* use */ NULL,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/keys/pass/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "k_security",
        /* use_name */  "Security Pass",
        /* pickup_name */  "Security Pass",
        /* pickup_name_definite */ "the Security Pass",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_STAY_COOP | IF_KEY
    },

    /*QUAKED key_blue_key (0 .5 .8) (-16 -16 -16) (16 16 16)
    normal door key - blue
    */
    {
        /* id */ IT_KEY_BLUE_KEY,
        /* classname */ "key_blue_key",
        /* pickup */ Pickup_Key,
        /* use */ NULL,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/keys/key/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "k_bluekey",
        /* use_name */  "Blue Key",
        /* pickup_name */  "Blue Key",
        /* pickup_name_definite */ "the Blue Key",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_STAY_COOP | IF_KEY
    },

    /*QUAKED key_red_key (0 .5 .8) (-16 -16 -16) (16 16 16)
    normal door key - red
    */
    {
        /* id */ IT_KEY_RED_KEY,
        /* classname */ "key_red_key",
        /* pickup */ Pickup_Key,
        /* use */ NULL,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/keys/red_key/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "k_redkey",
        /* use_name */  "Red Key",
        /* pickup_name */  "Red Key",
        /* pickup_name_definite */ "the Red Key",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_STAY_COOP | IF_KEY
    },

// RAFAEL
    /*QUAKED key_green_key (0 .5 .8) (-16 -16 -16) (16 16 16)
    normal door key - blue
    */
    {
        /* id */ IT_KEY_GREEN_KEY,
        /* classname */ "key_green_key",
        /* pickup */ Pickup_Key,
        /* use */ NULL,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/keys/green_key/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "k_green",
        /* use_name */  "Green Key",
        /* pickup_name */  "Green Key",
        /* pickup_name_definite */ "the Green Key",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_STAY_COOP | IF_KEY
    },
// RAFAEL

    /*QUAKED key_commander_head (0 .5 .8) (-16 -16 -16) (16 16 16)
    tank commander's head
    */
    {
        /* id */ IT_KEY_COMMANDER_HEAD,
        /* classname */ "key_commander_head",
        /* pickup */ Pickup_Key,
        /* use */ NULL,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/monsters/commandr/head/tris.md2",
        /* world_model_flags */ EF_GIB,
        /* view_model */ NULL,
        /* icon */ "k_comhead",
        /* use_name */  "Commander's Head",
        /* pickup_name */  "Commander's Head",
        /* pickup_name_definite */ "the Commander's Head",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_STAY_COOP | IF_KEY
    },

    /*QUAKED key_airstrike_target (0 .5 .8) (-16 -16 -16) (16 16 16)
    */
    {
        /* id */ IT_KEY_AIRSTRIKE,
        /* classname */ "key_airstrike_target",
        /* pickup */ Pickup_Key,
        /* use */ NULL,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/keys/target/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "i_airstrike",
        /* use_name */  "Airstrike Marker",
        /* pickup_name */  "Airstrike Marker",
        /* pickup_name_definite */ "the Airstrike Marker",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_STAY_COOP | IF_KEY
    },

// ======================================
// PGM

    /*QUAKED key_nuke_container (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
    */
    {
        /* id */ IT_KEY_NUKE_CONTAINER,
        /* classname */ "key_nuke_container",
        /* pickup */ Pickup_Key,
        /* use */ NULL,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/weapons/g_nuke/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "i_contain",
        /* use_name */  "Antimatter Pod",
        /* pickup_name */  "Antimatter Pod",
        /* pickup_name_definite */ "the Antimatter Pod",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_STAY_COOP | IF_KEY,
    },

    /*QUAKED key_nuke (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
    */
    {
        /* id */ IT_KEY_NUKE,
        /* classname */ "key_nuke",
        /* pickup */ Pickup_Key,
        /* use */ NULL,
        /* drop */ Drop_General,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/weapons/g_nuke/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "i_nuke",
        /* use_name */  "Antimatter Bomb",
        /* pickup_name */  "Antimatter Bomb",
        /* pickup_name_definite */ "the Antimatter Bomb",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_STAY_COOP | IF_KEY,
    },

// PGM
//

// PGM
// ======================================

    /*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16)
    model="models/items/healing/stimpack/tris.md2"
    */
    // Paril: split the healths up so they are always valid classnames
    {
        /* id */ IT_HEALTH_SMALL,
        /* classname */ "item_health_small",
        /* pickup */ Pickup_Health,
        /* use */ NULL,
        /* drop */ NULL,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/s_health.wav",
        /* world_model */ "models/items/healing/stimpack/tris.md2",
        /* world_model_flags */ EF_NONE,
        /* view_model */ NULL,
        /* icon */ "i_health",
        /* use_name */  "Health",
        /* pickup_name */  "Stimpack",
        /* pickup_name_definite */ "the Stimpack",
        /* quantity */ 2,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_HEALTH,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ HEALTH_IGNORE_MAX
    },
    /*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16)
    model="models/items/healing/medium/tris.md2"
    */
    {
        /* id */ IT_HEALTH_MEDIUM,
        /* classname */ "item_health",
        /* pickup */ Pickup_Health,
        /* use */ NULL,
        /* drop */ NULL,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/n_health.wav",
        /* world_model */ "models/items/healing/medium/tris.md2",
        /* world_model_flags */ EF_NONE,
        /* view_model */ NULL,
        /* icon */ "i_health",
        /* use_name */  "Health",
        /* pickup_name */  "First Aid",
        /* pickup_name_definite */ "the First Aid",
        /* quantity */ 10,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_HEALTH
    },
    /*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16)
    model="models/items/healing/large/tris.md2"
    */
    {
        /* id */ IT_HEALTH_LARGE,
        /* classname */ "item_health_large",
        /* pickup */ Pickup_Health,
        /* use */ NULL,
        /* drop */ NULL,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/l_health.wav",
        /* world_model */ "models/items/healing/large/tris.md2",
        /* world_model_flags */ EF_NONE,
        /* view_model */ NULL,
        /* icon */ "i_health",
        /* use_name */  "Health",
        /* pickup_name */  "Medkit",
        /* pickup_name_definite */ "Medkit",
        /* quantity */ 25,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_HEALTH
    },
    /*QUAKED item_health_mega (.3 .3 1) (-16 -16 -16) (16 16 16)
    model="models/items/mega_h/tris.md2"
    */
    {
        /* id */ IT_HEALTH_MEGA,
        /* classname */ "item_health_mega",
        /* pickup */ Pickup_Health,
        /* use */ NULL,
        /* drop */ NULL,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/m_health.wav",
        /* world_model */ "models/items/mega_h/tris.md2",
        /* world_model_flags */ EF_NONE,
        /* view_model */ NULL,
        /* icon */ "p_megahealth",
        /* use_name */  "Health",
        /* pickup_name */  "Megahealth",
        /* pickup_name_definite */ "the Megahealth",
        /* quantity */ 100,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_HEALTH,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ HEALTH_IGNORE_MAX | HEALTH_TIMED
    },

//ZOID
    /*QUAKED item_flag_team1 (1 0.2 0) (-16 -16 -24) (16 16 32)
    */
    {
        /* id */ IT_FLAG1,
        /* classname */ "item_flag_team1",
        /* pickup */ CTFPickup_Flag,
        /* use */ NULL,
        /* drop */ CTFDrop_Flag, //Should this be null if we don't want players to drop it manually?
        /* weaponthink */ NULL,
        /* pickup_sound */ "ctf/flagtk.wav",
        /* world_model */ "players/male/flag1.md2",
        /* world_model_flags */ EF_FLAG1,
        /* view_model */ NULL,
        /* icon */ "i_ctf1",
        /* use_name */  "Red Flag",
        /* pickup_name */  "Red Flag",
        /* pickup_name_definite */ "the Red Flag",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_NONE,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ 0,
        /* precaches */ "ctf/flagcap.wav"
    },

    /*QUAKED item_flag_team2 (1 0.2 0) (-16 -16 -24) (16 16 32)
    */
    {
        /* id */ IT_FLAG2,
        /* classname */ "item_flag_team2",
        /* pickup */ CTFPickup_Flag,
        /* use */ NULL,
        /* drop */ CTFDrop_Flag,
        /* weaponthink */ NULL,
        /* pickup_sound */ "ctf/flagtk.wav",
        /* world_model */ "players/male/flag2.md2",
        /* world_model_flags */ EF_FLAG2,
        /* view_model */ NULL,
        /* icon */ "i_ctf2",
        /* use_name */  "Blue Flag",
        /* pickup_name */  "Blue Flag",
        /* pickup_name_definite */ "the Blue Flag",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_NONE,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ 0,
        /* precaches */ "ctf/flagcap.wav"
    },

    /* Resistance Tech */
    {
        /* id */ IT_TECH_RESISTANCE,
        /* classname */ "item_tech1",
        /* pickup */ CTFPickup_Tech,
        /* use */ NULL,
        /* drop */ CTFDrop_Tech,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/ctf/resistance/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "tech1",
        /* use_name */  "Disruptor Shield",
        /* pickup_name */  "Disruptor Shield",
        /* pickup_name_definite */ "the Disruptor Shield",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_TECH | IF_POWERUP_WHEEL,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_TECH1,
        /* precaches */ "ctf/tech1.wav"
    },

    /* Strength Tech */
    {
        /* id */ IT_TECH_STRENGTH,
        /* classname */ "item_tech2",
        /* pickup */ CTFPickup_Tech,
        /* use */ NULL,
        /* drop */ CTFDrop_Tech,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/ctf/strength/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "tech2",
        /* use_name */  "Power Amplifier",
        /* pickup_name */  "Power Amplifier",
        /* pickup_name_definite */ "the Power Amplifier",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_TECH | IF_POWERUP_WHEEL,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_TECH2,
        /* precaches */ "ctf/tech2.wav ctf/tech2x.wav"
    },

    /* Haste Tech */
    {
        /* id */ IT_TECH_HASTE,
        /* classname */ "item_tech3",
        /* pickup */ CTFPickup_Tech,
        /* use */ NULL,
        /* drop */ CTFDrop_Tech,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/ctf/haste/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "tech3",
        /* use_name */  "Time Accel",
        /* pickup_name */  "Time Accel",
        /* pickup_name_definite */ "the Time Accel",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_TECH | IF_POWERUP_WHEEL,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_TECH3,
        /* precaches */ "ctf/tech3.wav"
    },

    /* Regeneration Tech */
    {
        /* id */ IT_TECH_REGENERATION,
        /* classname */ "item_tech4",
        /* pickup */ CTFPickup_Tech,
        /* use */ NULL,
        /* drop */ CTFDrop_Tech,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/ctf/regeneration/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "tech4",
        /* use_name */  "AutoDoc",
        /* pickup_name */  "AutoDoc",
        /* pickup_name_definite */ "the AutoDoc",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_TECH | IF_POWERUP_WHEEL,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_TECH4,
        /* precaches */ "ctf/tech4.wav"
    },

    {
        /* id */ IT_ITEM_FLASHLIGHT,
        /* classname */ "item_flashlight",
        /* pickup */ Pickup_General,
        /* use */ Use_Flashlight,
        /* drop */ NULL,
        /* weaponthink */ NULL,
        /* pickup_sound */ "items/pkup.wav",
        /* world_model */ "models/items/flashlight/tris.md2",
        /* world_model_flags */ EF_ROTATE | EF_BOB,
        /* view_model */ NULL,
        /* icon */ "p_torch",
        /* use_name */  "Flashlight",
        /* pickup_name */  "Flashlight",
        /* pickup_name_definite */ "the Flashlight",
        /* quantity */ 0,
        /* ammo */ IT_NULL,
        /* chain */ IT_NULL,
        /* flags */ IF_STAY_COOP | IF_POWERUP_WHEEL | IF_POWERUP_ONOFF | IF_NOT_RANDOM,
        /* vwep_model */ NULL,
        /* armor_info */ NULL,
        /* tag */ POWERUP_FLASHLIGHT,
        /* precaches */ "items/flashlight_on.wav items/flashlight_off.wav",
        /* sort_id */ -1
    },
};
// clang-format on

void InitItems(void)
{
    // set up ammo
    for (item_id_t i = IT_NULL; i < IT_TOTAL; i++) {
        const gitem_t *item = &itemlist[i];
        Q_assert(item->id == i);
        if ((item->flags & IF_AMMO) && item->tag >= AMMO_BULLETS && item->tag < AMMO_MAX)
            ammolist[item->tag] = item;
        else if ((item->flags & IF_POWERUP_WHEEL) && !(item->flags & IF_WEAPON) && item->tag >= POWERUP_SCREEN && item->tag < POWERUP_MAX)
            poweruplist[item->tag] = item;
    }

#if 0
    // in coop or DM with Weapons' Stay, remove drop ptr
    for (item_id_t i = IT_NULL; i < IT_TOTAL; i++) {
        const gitem_t *item = &itemlist[i];
        if (coop->integer) {
            if (!P_UseCoopInstancedItems() && (item->flags & IF_STAY_COOP))
                item->drop = NULL;
        } else if (deathmatch->integer) {
            if (g_dm_weapons_stay->integer && item->drop == Drop_Weapon)
                item->drop = NULL;
        }
    }
#endif
}

/*
===============
SetItemNames

Called by worldspawn
===============
*/
void SetItemNames(void)
{
    for (item_id_t i = IT_NULL; i < IT_TOTAL; i++)
        gi.configstring(CS_ITEMS + i, itemlist[i].pickup_name);
}
