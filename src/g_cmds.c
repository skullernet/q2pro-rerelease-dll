// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include "m_player.h"

static void SelectNextItem(edict_t *ent, item_flags_t itflags, bool menu)
{
    gclient_t *cl;
    item_id_t  i, index;
    const gitem_t *it;

    cl = ent->client;

    // ZOID
    if (menu) {
        if (cl->menu) {
            PMenu_Next(ent);
            return;
        }
        if (cl->chase_target) {
            ChaseNext(ent);
            return;
        }
    }
    // ZOID

    // scan  for the next valid one
    for (i = IT_NULL + 1; i <= IT_TOTAL; i++) {
        index = (cl->pers.selected_item + i) % IT_TOTAL;
        if (!cl->pers.inventory[index])
            continue;
        it = &itemlist[index];
        if (!it->use)
            continue;
        if (!(it->flags & itflags))
            continue;

        cl->pers.selected_item = index;
        cl->pers.selected_item_time = level.time + SELECTED_ITEM_TIME;
        cl->ps.stats[STAT_SELECTED_ITEM_NAME] = CS_ITEMS + index;
        return;
    }

    cl->pers.selected_item = IT_NULL;
}

static void SelectPrevItem(edict_t *ent, item_flags_t itflags)
{
    gclient_t *cl;
    item_id_t  i, index;
    const gitem_t *it;

    cl = ent->client;

    // ZOID
    if (cl->menu) {
        PMenu_Prev(ent);
        return;
    }
    if (cl->chase_target) {
        ChasePrev(ent);
        return;
    }
    // ZOID

    // scan  for the next valid one
    for (i = IT_NULL + 1; i <= IT_TOTAL; i++) {
        index = (cl->pers.selected_item + IT_TOTAL - i) % IT_TOTAL;
        if (!cl->pers.inventory[index])
            continue;
        it = &itemlist[index];
        if (!it->use)
            continue;
        if (!(it->flags & itflags))
            continue;

        cl->pers.selected_item = index;
        cl->pers.selected_item_time = level.time + SELECTED_ITEM_TIME;
        cl->ps.stats[STAT_SELECTED_ITEM_NAME] = CS_ITEMS + index;
        return;
    }

    cl->pers.selected_item = IT_NULL;
}

void ValidateSelectedItem(edict_t *ent)
{
    gclient_t *cl;

    cl = ent->client;

    if (cl->pers.inventory[cl->pers.selected_item])
        return; // valid

    SelectNextItem(ent, IF_ANY, false);
}

//=================================================================================

static bool G_CheatCheck(edict_t *ent)
{
    if (game.maxclients > 1 && !sv_cheats->integer) {
        gi.cprintf(ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
        return false;
    }

    return true;
}

static void SpawnAndGiveItem(edict_t *ent, item_id_t id)
{
    const gitem_t *it = GetItemByIndex(id);

    if (!it)
        return;

    edict_t *it_ent = G_Spawn();
    it_ent->classname = it->classname;
    SpawnItem(it_ent, it);

    if (it_ent->inuse) {
        Touch_Item(it_ent, ent, &null_trace, true);
        if (it_ent->inuse)
            G_FreeEdict(it_ent);
    }
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
static void Cmd_Give_f(edict_t *ent)
{
    const char    *name;
    const gitem_t *it;
    item_id_t index;
    int       i;
    bool      give_all;
    edict_t *it_ent;

    if (!G_CheatCheck(ent))
        return;

    name = gi.args();

    if (Q_strcasecmp(name, "all") == 0)
        give_all = true;
    else
        give_all = false;

    if (give_all || Q_strcasecmp(gi.argv(1), "health") == 0) {
        if (gi.argc() == 3)
            ent->health = Q_atoi(gi.argv(2));
        else
            ent->health = ent->max_health;
        if (!give_all)
            return;
    }

    if (give_all || Q_strcasecmp(name, "weapons") == 0) {
        for (i = 0; i < IT_TOTAL; i++) {
            it = itemlist + i;
            if (!it->pickup)
                continue;
            if (!(it->flags & IF_WEAPON))
                continue;
            ent->client->pers.inventory[i] += 1;
        }
        if (!give_all)
            return;
    }

    if (give_all || Q_strcasecmp(name, "ammo") == 0) {
        if (give_all)
            SpawnAndGiveItem(ent, IT_ITEM_PACK);

        for (i = 0; i < IT_TOTAL; i++) {
            it = itemlist + i;
            if (!it->pickup)
                continue;
            if (!(it->flags & IF_AMMO))
                continue;
            Add_Ammo(ent, it, 1000);
        }
        if (!give_all)
            return;
    }

    if (give_all || Q_strcasecmp(name, "armor") == 0) {
        ent->client->pers.inventory[IT_ARMOR_JACKET] = 0;
        ent->client->pers.inventory[IT_ARMOR_COMBAT] = 0;
        ent->client->pers.inventory[IT_ARMOR_BODY] = GetItemByIndex(IT_ARMOR_BODY)->armor_info->max_count;

        if (!give_all)
            return;
    }

    if (give_all) {
        SpawnAndGiveItem(ent, IT_ITEM_POWER_SHIELD);

        if (!give_all)
            return;
    }

    if (give_all) {
        for (i = 0; i < IT_TOTAL; i++) {
            it = itemlist + i;
            if (!it->pickup)
                continue;
            // ROGUE
            if (it->flags & (IF_ARMOR | IF_WEAPON | IF_AMMO | IF_NOT_GIVEABLE | IF_TECH))
                continue;
            if (it->pickup == CTFPickup_Flag)
                continue;
            if ((it->flags & IF_HEALTH) && !it->use)
                continue;
            // ROGUE
            ent->client->pers.inventory[i] = (it->flags & IF_KEY) ? 8 : 1;
        }

        G_CheckPowerArmor(ent);
        ent->client->pers.power_cubes = 0xFF;
        return;
    }

    it = FindItem(name);
    if (!it) {
        name = gi.argv(1);
        it = FindItem(name);
    }
    if (!it)
        it = FindItemByClassname(name);

    if (!it) {
        gi.cprintf(ent, PRINT_HIGH, "Unknown item\n");
        return;
    }

    // ROGUE
    if (it->flags & IF_NOT_GIVEABLE) {
        gi.cprintf(ent, PRINT_HIGH, "Item cannot be given\n");
        return;
    }
    // ROGUE

    index = it->id;

    if (!it->pickup) {
        ent->client->pers.inventory[index] = 1;
        return;
    }

    if (it->flags & IF_AMMO) {
        if (gi.argc() == 3)
            ent->client->pers.inventory[index] = Q_atoi(gi.argv(2));
        else
            ent->client->pers.inventory[index] += it->quantity;
    } else {
        it_ent = G_Spawn();
        it_ent->classname = it->classname;
        SpawnItem(it_ent, it);
        // PMM - since some items don't actually spawn when you say to ..
        if (!it_ent->inuse)
            return;
        // pmm
        Touch_Item(it_ent, ent, &null_trace, true);
        if (it_ent->inuse)
            G_FreeEdict(it_ent);
    }
}

// [Paril-KEX]
static void Cmd_Target_f(edict_t *ent)
{
    if (!G_CheatCheck(ent))
        return;

    ent->target = gi.argv(1);
    G_UseTargets(ent, ent);
    ent->target = NULL;
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
static void Cmd_God_f(edict_t *ent)
{
    if (!G_CheatCheck(ent))
        return;

    ent->flags ^= FL_GODMODE;
    if (!(ent->flags & FL_GODMODE))
        gi.cprintf(ent, PRINT_HIGH, "godmode OFF\n");
    else
        gi.cprintf(ent, PRINT_HIGH, "godmode ON\n");
}

/*
==================
Cmd_Immortal_f

Sets client to immortal - take damage but never go below 1 hp

argv(0) immortal
==================
*/
static void Cmd_Immortal_f(edict_t *ent)
{
    if (!G_CheatCheck(ent))
        return;

    ent->flags ^= FL_IMMORTAL;
    if (!(ent->flags & FL_IMMORTAL))
        gi.cprintf(ent, PRINT_HIGH, "immortal OFF\n");
    else
        gi.cprintf(ent, PRINT_HIGH, "immortal ON\n");
}

/*
=================
Cmd_Spawn_f

Spawn class name

argv(0) spawn
argv(1) <classname>
argv(2+n) "key"...
argv(3+n) "value"...
=================
*/
static void Cmd_Spawn_f(edict_t *ent)
{
    if (!G_CheatCheck(ent))
        return;

    solid_t backup = ent->solid;
    ent->solid = SOLID_NOT;
    gi.linkentity(ent);

    edict_t *other = G_Spawn();
    other->classname = gi.argv(1);

    vec3_t forward;
    AngleVectors(ent->client->v_angle, forward, NULL, NULL);

    VectorMA(ent->s.origin, 24, forward, other->s.origin);
    other->s.angles[1] = ent->s.angles[1];

    ED_InitSpawnVars();

    if (gi.argc() > 3)
        for (int i = 2; i < gi.argc(); i += 2)
            ED_ParseField(gi.argv(i), gi.argv(i + 1), other);

    ED_CallSpawn(other);

    if (other->inuse) {
        vec3_t start, end;

        VectorCopy(ent->s.origin, start);
        start[2] += ent->viewheight;

        VectorMA(start, 8192, forward, end);

        trace_t tr = gi.trace(start, NULL, NULL, end, other, MASK_SHOT | CONTENTS_MONSTERCLIP);
        VectorCopy(tr.endpos, other->s.origin);

        for (int i = 0; i < 3; i++) {
            if (tr.plane.normal[i] > 0)
                other->s.origin[i] -= other->mins[i] * tr.plane.normal[i];
            else
                other->s.origin[i] += other->maxs[i] * -tr.plane.normal[i];
        }

        while (gi.trace(other->s.origin, other->mins, other->maxs, other->s.origin, other, MASK_SHOT | CONTENTS_MONSTERCLIP).startsolid) {
            float dx = other->maxs[0] - other->mins[0];
            float dy = other->maxs[1] - other->mins[1];
            float f = -sqrtf(dx * dx + dy * dy);
            vec3_t dir;

            VectorMA(other->s.origin, f, forward, other->s.origin);
            VectorSubtract(other->s.origin, ent->s.origin, dir);

            if (DotProduct(dir, forward) < 0) {
                gi.cprintf(ent, PRINT_HIGH, "Couldn't find a suitable spawn location\n");
                G_FreeEdict(other);
                break;
            }
        }

        if (other->inuse)
            gi.linkentity(other);

        if ((other->svflags & SVF_MONSTER) && other->think)
            other->think(other);
    }

    ent->solid = backup;
    gi.linkentity(ent);
}

/*
=================
Cmd_Spawn_f

Telepo'

argv(0) teleport
argv(1) x
argv(2) y
argv(3) z
=================
*/
static void Cmd_Teleport_f(edict_t *ent)
{
    if (!G_CheatCheck(ent))
        return;

    if (gi.argc() < 4) {
        gi.cprintf(ent, PRINT_HIGH, "Not enough args; teleport x y z\n");
        return;
    }

    ent->s.origin[0] = atof(gi.argv(1));
    ent->s.origin[1] = atof(gi.argv(2));
    ent->s.origin[2] = atof(gi.argv(3));

    if (gi.argc() > 4) {
        float pitch = atof(gi.argv(4));
        float yaw = atof(gi.argv(5));
        float roll = atof(gi.argv(6));
        vec3_t ang = { pitch, yaw, roll };

        for (int i = 0; i < 3; i++)
            ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(ang[i] - ent->client->resp.cmd_angles[i]);
        VectorCopy(ang, ent->client->ps.viewangles);
        VectorCopy(ang, ent->client->v_angle);
    }

    gi.linkentity(ent);
}

/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
static void Cmd_Notarget_f(edict_t *ent)
{
    if (!G_CheatCheck(ent))
        return;

    ent->flags ^= FL_NOTARGET;
    if (!(ent->flags & FL_NOTARGET))
        gi.cprintf(ent, PRINT_HIGH, "notarget OFF\n");
    else
        gi.cprintf(ent, PRINT_HIGH, "notarget ON\n");
}

/*
==================
Cmd_Novisible_f

Sets client to "super notarget"

argv(0) notarget
==================
*/
static void Cmd_Novisible_f(edict_t *ent)
{
    if (!G_CheatCheck(ent))
        return;

    ent->flags ^= FL_NOVISIBLE;
    if (!(ent->flags & FL_NOVISIBLE))
        gi.cprintf(ent, PRINT_HIGH, "novisible OFF\n");
    else
        gi.cprintf(ent, PRINT_HIGH, "novisible ON\n");
}

static void Cmd_AlertAll_f(edict_t *ent)
{
    if (!G_CheatCheck(ent))
        return;

    for (int i = game.maxclients + BODY_QUEUE_SIZE + 1; i < globals.num_edicts; i++) {
        edict_t *t = &g_edicts[i];

        if (!t->inuse || t->health <= 0 || !(t->svflags & SVF_MONSTER))
            continue;

        t->enemy = ent;
        FoundTarget(t);
    }
}

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
static void Cmd_Noclip_f(edict_t *ent)
{
    if (!G_CheatCheck(ent))
        return;

    if (ent->movetype == MOVETYPE_NOCLIP) {
        ent->movetype = MOVETYPE_WALK;
        gi.cprintf(ent, PRINT_HIGH, "noclip OFF\n");
    } else {
        ent->movetype = MOVETYPE_NOCLIP;
        gi.cprintf(ent, PRINT_HIGH, "noclip ON\n");
    }
}

/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
static void Cmd_Use_f(edict_t *ent)
{
    item_id_t index;
    const gitem_t *it;
    const char *s;

    if (ent->health <= 0 || ent->deadflag)
        return;

    s = gi.args();

    const char *cmd = gi.argv(0);
    if (!Q_strcasecmp(cmd, "use_index") || !Q_strcasecmp(cmd, "use_index_only"))
        it = GetItemByIndex(Q_atoi(s));
    else
        it = FindItem(s);

    if (!it) {
        gi.cprintf(ent, PRINT_HIGH, "Unknown item: %s\n", s);
        return;
    }
    if (!it->use) {
        gi.cprintf(ent, PRINT_HIGH, "Item is not usable.\n");
        return;
    }
    index = it->id;

    // Paril: Use_Weapon handles weapon availability
    if (!(it->flags & IF_WEAPON) && !ent->client->pers.inventory[index]) {
        gi.cprintf(ent, PRINT_HIGH, "Out of item: %s\n", it->pickup_name);
        return;
    }

    // allow weapon chains for use
    ent->client->no_weapon_chains = !!strcmp(gi.argv(0), "use") && !!strcmp(gi.argv(0), "use_index");

    it->use(ent, it);

    ValidateSelectedItem(ent);
}

/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
static void Cmd_Drop_f(edict_t *ent)
{
    item_id_t index;
    const gitem_t *it;
    const char *s;

    if (ent->health <= 0 || ent->deadflag)
        return;

    // ZOID--special case for tech powerups
    if (Q_strcasecmp(gi.args(), "tech") == 0) {
        it = CTFWhat_Tech(ent);

        if (it) {
            it->drop(ent, it);
            ValidateSelectedItem(ent);
        }

        return;
    }
    // ZOID

    s = gi.args();

    const char *cmd = gi.argv(0);

    if (!Q_strcasecmp(cmd, "drop_index"))
        it = GetItemByIndex(Q_atoi(s));
    else
        it = FindItem(s);

    if (!it) {
        gi.cprintf(ent, PRINT_HIGH, "Unknown item : %s\n", s);
        return;
    }
    if (!G_CanDropItem(it)) {
        gi.cprintf(ent, PRINT_HIGH, "Item is not droppable.\n");
        return;
    }
    index = it->id;
    if (!ent->client->pers.inventory[index]) {
        gi.cprintf(ent, PRINT_HIGH, "Out of item: %s\n", it->pickup_name);
        return;
    }

    it->drop(ent, it);

    ValidateSelectedItem(ent);
}

/*
=================
Cmd_Inven_f
=================
*/
static void Cmd_Inven_f(edict_t *ent)
{
    int        i;
    gclient_t *cl;

    cl = ent->client;

    cl->showscores = false;
    cl->showhelp = false;

    // ZOID
    if (ent->client->menu) {
        PMenu_Close(ent);
        ent->client->update_chase = true;
        return;
    }
    // ZOID

    if (cl->showinventory) {
        cl->showinventory = false;
        return;
    }

    // ZOID
    if (G_TeamplayEnabled() && cl->resp.ctf_team == CTF_NOTEAM) {
        CTFOpenJoinMenu(ent);
        return;
    }
    // ZOID

    cl->showinventory = true;

    gi.WriteByte(svc_inventory);
    for (i = 0; i < IT_TOTAL; i++)
        gi.WriteShort(cl->pers.inventory[i]);
    for (; i < MAX_ITEMS; i++)
        gi.WriteShort(0);
    gi.unicast(ent, true);
}

/*
=================
Cmd_InvUse_f
=================
*/
static void Cmd_InvUse_f(edict_t *ent)
{
    const gitem_t *it;

    // ZOID
    if (ent->client->menu) {
        PMenu_Select(ent);
        return;
    }
    // ZOID

    if (ent->health <= 0 || ent->deadflag)
        return;

    ValidateSelectedItem(ent);

    if (ent->client->pers.selected_item == IT_NULL) {
        gi.cprintf(ent, PRINT_HIGH, "No item to use.\n");
        return;
    }

    it = &itemlist[ent->client->pers.selected_item];
    if (!it->use) {
        gi.cprintf(ent, PRINT_HIGH, "Item is not usable.\n");
        return;
    }

    // don't allow weapon chains for invuse
    ent->client->no_weapon_chains = true;
    it->use(ent, it);

    ValidateSelectedItem(ent);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
static void Cmd_WeapPrev_f(edict_t *ent)
{
    gclient_t *cl;
    item_id_t  i, index;
    const gitem_t *it;
    item_id_t  selected_weapon;

    cl = ent->client;

    if (ent->health <= 0 || ent->deadflag)
        return;
    if (!cl->pers.weapon)
        return;

    // don't allow weapon chains for weapprev
    cl->no_weapon_chains = true;

    selected_weapon = cl->pers.weapon->id;

    // scan  for the next valid one
    for (i = IT_NULL + 1; i <= IT_TOTAL; i++) {
        // PMM - prevent scrolling through ALL weapons
        index = (selected_weapon + IT_TOTAL - i) % IT_TOTAL;
        if (!cl->pers.inventory[index])
            continue;
        it = &itemlist[index];
        if (!it->use)
            continue;
        if (!(it->flags & IF_WEAPON))
            continue;
        it->use(ent, it);
        // ROGUE
        if (cl->newweapon == it)
            return; // successful
        // ROGUE
    }
}

/*
=================
Cmd_WeapNext_f
=================
*/
static void Cmd_WeapNext_f(edict_t *ent)
{
    gclient_t *cl;
    item_id_t  i, index;
    const gitem_t *it;
    item_id_t  selected_weapon;

    cl = ent->client;

    if (ent->health <= 0 || ent->deadflag)
        return;
    if (!cl->pers.weapon)
        return;

    // don't allow weapon chains for weapnext
    cl->no_weapon_chains = true;

    selected_weapon = cl->pers.weapon->id;

    // scan  for the next valid one
    for (i = IT_NULL + 1; i <= IT_TOTAL; i++) {
        // PMM - prevent scrolling through ALL weapons
        index = (selected_weapon + i) % IT_TOTAL;
        if (!cl->pers.inventory[index])
            continue;
        it = &itemlist[index];
        if (!it->use)
            continue;
        if (!(it->flags & IF_WEAPON))
            continue;
        it->use(ent, it);
        // PMM - prevent scrolling through ALL weapons

        // ROGUE
        if (cl->newweapon == it)
            return;
        // ROGUE
    }
}

/*
=================
Cmd_WeapLast_f
=================
*/
static void Cmd_WeapLast_f(edict_t *ent)
{
    gclient_t *cl;
    int        index;
    const gitem_t *it;

    cl = ent->client;

    if (ent->health <= 0 || ent->deadflag)
        return;
    if (!cl->pers.weapon || !cl->pers.lastweapon)
        return;

    // don't allow weapon chains for weaplast
    cl->no_weapon_chains = true;

    index = cl->pers.lastweapon->id;
    if (!cl->pers.inventory[index])
        return;
    it = &itemlist[index];
    if (!it->use)
        return;
    if (!(it->flags & IF_WEAPON))
        return;
    it->use(ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
static void Cmd_InvDrop_f(edict_t *ent)
{
    const gitem_t *it;

    if (ent->health <= 0 || ent->deadflag)
        return;

    ValidateSelectedItem(ent);

    if (ent->client->pers.selected_item == IT_NULL) {
        gi.cprintf(ent, PRINT_HIGH, "No item to drop.\n");
        return;
    }

    it = &itemlist[ent->client->pers.selected_item];
    if (!G_CanDropItem(it)) {
        gi.cprintf(ent, PRINT_HIGH, "Item is not droppable.\n");
        return;
    }
    it->drop(ent, it);

    ValidateSelectedItem(ent);
}

/*
=================
Cmd_Kill_f
=================
*/
static void Cmd_Kill_f(edict_t *ent)
{
    // ZOID
    if (ent->client->resp.spectator)
        return;
    // ZOID

    if ((level.time - ent->client->respawn_time) < SEC(5))
        return;

    ent->flags &= ~FL_GODMODE;
    ent->health = 0;

    // ROGUE
    //  make sure no trackers are still hurting us.
    if (ent->client->tracker_pain_time)
        RemoveAttackingPainDaemons(ent);

    if (ent->client->owned_sphere) {
        G_FreeEdict(ent->client->owned_sphere);
        ent->client->owned_sphere = NULL;
    }
    // ROGUE

    // [Paril-KEX] don't allow kill to take points away in TDM
    player_die(ent, ent, ent, 100000, vec3_origin, (mod_t) { MOD_SUICIDE, teamplay->integer });
}

/*
=================
Cmd_Kill_AI_f
=================
*/
static void Cmd_Kill_AI_f(edict_t * ent)
{
    if (!sv_cheats->integer) {
        gi.cprintf(ent, PRINT_HIGH, "Kill_AI: Cheats Must Be Enabled!\n");
        return;
    }

    // except the one we're looking at...
    edict_t *looked_at = NULL;
    vec3_t start, end;

    VectorCopy(ent->s.origin, start);
    start[2] += ent->viewheight;

    VectorMA(start, 1024, ent->client->v_forward, end);

    looked_at = gi.trace(start, NULL, NULL, end, ent, MASK_SHOT).ent;

    for (int i = game.maxclients + BODY_QUEUE_SIZE + 1; i < globals.num_edicts; i++) {
        edict_t *edict = &g_edicts[i];
        if (!edict->inuse || edict == looked_at)
            continue;
        if (!(edict->svflags & SVF_MONSTER))
            continue;
        G_FreeEdict(edict);
    }

    gi.cprintf(ent, PRINT_HIGH, "Kill_AI: All AI Are Dead...\n");
}

/*
=================
Cmd_Where_f
=================
*/
static void Cmd_Where_f(edict_t *ent)
{
    gi.cprintf(ent, PRINT_HIGH, "Location: %s %s\n", vtos(ent->s.origin), vtos(ent->client->ps.viewangles));
}

/*
=================
Cmd_Clear_AI_Enemy_f
=================
*/
static void Cmd_Clear_AI_Enemy_f(edict_t *ent)
{
    if (!sv_cheats->integer) {
        gi.cprintf(ent, PRINT_HIGH, "Cmd_Clear_AI_Enemy: Cheats Must Be Enabled!\n");
        return;
    }

    for (int i = game.maxclients + BODY_QUEUE_SIZE + 1; i < globals.num_edicts; i++) {
        edict_t *edict = &g_edicts[i];
        if (!edict->inuse)
            continue;
        if (!(edict->svflags & SVF_MONSTER))
            continue;
        edict->monsterinfo.aiflags |= AI_FORGET_ENEMY;
    }

    gi.cprintf(ent, PRINT_HIGH, "Cmd_Clear_AI_Enemy: Clear All AI Enemies...\n");
}

/*
=================
Cmd_PutAway_f
=================
*/
static void Cmd_PutAway_f(edict_t *ent)
{
    ent->client->showscores = false;
    ent->client->showhelp = false;
    ent->client->showinventory = false;

    // ZOID
    if (ent->client->menu)
        PMenu_Close(ent);
    ent->client->update_chase = true;
    // ZOID
}

static int PlayerSort(const void *a, const void *b)
{
    int anum, bnum;

    anum = *(const int *)a;
    bnum = *(const int *)b;

    anum = game.clients[anum].ps.stats[STAT_FRAGS];
    bnum = game.clients[bnum].ps.stats[STAT_FRAGS];

    if (anum < bnum)
        return -1;
    if (anum > bnum)
        return 1;
    return 0;
}

#define MAX_IDEAL_PACKET_SIZE   1024

/*
=================
Cmd_Players_f
=================
*/
static void Cmd_Players_f(edict_t *ent)
{
    int     i;
    int     count;
    char    small[64];
    char    large[MAX_IDEAL_PACKET_SIZE];
    int     index[MAX_CLIENTS];

    count = 0;
    for (i = 0; i < game.maxclients; i++)
        if (game.clients[i].pers.connected) {
            index[count] = i;
            count++;
        }

    // sort by frags
    qsort(index, count, sizeof(index[0]), PlayerSort);

    // print information
    large[0] = 0;

    for (i = 0; i < count; i++) {
        Q_snprintf(small, sizeof(small), "%3i %s\n",
                   game.clients[index[i]].ps.stats[STAT_FRAGS],
                   game.clients[index[i]].pers.netname);
        if (strlen(small) + strlen(large) > sizeof(large) - 50) {
            // can't print all of them in one packet
            strcat(large, "...\n");
            break;
        }
        strcat(large, small);
    }

    gi.cprintf(ent, PRINT_HIGH | PRINT_NO_NOTIFY, "%s\n%i players\n", large, count);
}

bool CheckFlood(edict_t *ent)
{
    int        i;
    gclient_t *cl;

    if (flood_msgs->integer) {
        cl = ent->client;

        if (level.time < cl->flood_locktill) {
            gi.cprintf(ent, PRINT_HIGH, "You can't talk for %.f more seconds\n",
                       TO_SEC(cl->flood_locktill - level.time));
            return true;
        }
        i = cl->flood_whenhead - flood_msgs->integer + 1;
        if (i < 0)
            i = (sizeof(cl->flood_when) / sizeof(cl->flood_when[0])) + i;
        if (i >= q_countof(cl->flood_when))
            i = 0;
        if (cl->flood_when[i] && level.time - cl->flood_when[i] < SEC(flood_persecond->value)) {
            cl->flood_locktill = level.time + SEC(flood_waitdelay->value);
            gi.cprintf(ent, PRINT_CHAT, "You can't talk for %d more seconds\n",
                       flood_waitdelay->integer);
            return true;
        }
        cl->flood_whenhead = (cl->flood_whenhead + 1) % (sizeof(cl->flood_when) / sizeof(cl->flood_when[0]));
        cl->flood_when[cl->flood_whenhead] = level.time;
    }
    return false;
}

/*
=================
Cmd_Wave_f
=================
*/
static void Cmd_Wave_f(edict_t *ent)
{
    int cmd = Q_atoi(gi.argv(1));

    // no dead or noclip waving
    if (ent->deadflag || ent->movetype == MOVETYPE_NOCLIP)
        return;

    // can't wave when ducked
    bool do_animate = ent->client->anim_priority <= ANIM_WAVE && !(ent->client->ps.pmove.pm_flags & PMF_DUCKED);

    if (do_animate)
        ent->client->anim_priority = ANIM_WAVE;

    const char *other_notify_msg = NULL, *other_notify_none_msg = NULL;

    vec3_t start, dir;
    P_ProjectSource(ent, ent->client->v_angle, vec3_origin, start, dir);

    // see who we're aiming at
    edict_t *aiming_at = NULL;
    float best_dist = -9999;

    for (int i = 1; i <= game.maxclients; i++) {
        edict_t *player = &g_edicts[i];
        if (!player->inuse || player == ent)
            continue;

        VectorSubtract(player->s.origin, start, dir);
        float dist = VectorNormalize(dir);

        if (DotProduct(dir, ent->client->v_forward) < 0.97f)
            continue;
        if (dist < best_dist)
            continue;

        best_dist = dist;
        aiming_at = player;
    }

    switch (cmd) {
    case GESTURE_FLIP_OFF:
        other_notify_msg = "%s flipped the bird at %s.\n";
        other_notify_none_msg = "%s flipped the bird.\n";
        if (do_animate) {
            ent->s.frame = FRAME_flip01 - 1;
            ent->client->anim_end = FRAME_flip12;
        }
        break;
    case GESTURE_SALUTE:
        other_notify_msg = "%s salutes %s.\n";
        other_notify_none_msg = "%s salutes.\n";
        if (do_animate) {
            ent->s.frame = FRAME_salute01 - 1;
            ent->client->anim_end = FRAME_salute11;
        }
        break;
    case GESTURE_TAUNT:
        other_notify_msg = "%s taunts %s.\n";
        other_notify_none_msg = "%s taunts.\n";
        if (do_animate) {
            ent->s.frame = FRAME_taunt01 - 1;
            ent->client->anim_end = FRAME_taunt17;
        }
        break;
    case GESTURE_WAVE:
        other_notify_msg = "%s waves at %s.\n";
        other_notify_none_msg = "%s waves.\n";
        if (do_animate) {
            ent->s.frame = FRAME_wave01 - 1;
            ent->client->anim_end = FRAME_wave11;
        }
        break;
    case GESTURE_POINT:
    default:
        other_notify_msg = "%s points at %s.\n";
        other_notify_none_msg = "%s points.\n";
        if (do_animate) {
            ent->s.frame = FRAME_point01 - 1;
            ent->client->anim_end = FRAME_point12;
        }
        break;
    }

    if (CheckFlood(ent))
        return;

    bool has_a_target = false;

    if (cmd == GESTURE_POINT) {
        for (int i = 1; i <= game.maxclients; i++) {
            edict_t *player = &g_edicts[i];
            if (player->inuse && player != ent && OnSameTeam(ent, player)) {
                has_a_target = true;
                break;
            }
        }
    }

    if (cmd == GESTURE_POINT && has_a_target) {
        // don't do this stuff if we're flooding
        vec3_t end;
        VectorMA(start, 2048, ent->client->v_forward, end);
        trace_t tr = gi.trace(start, NULL, NULL, end, ent, MASK_SHOT & ~CONTENTS_WINDOW);
        other_notify_msg = "%s pinged a location.\n";

        if (tr.fraction != 1.0f) {
            // send to all teammates
            for (int i = 1; i <= game.maxclients; i++) {
                edict_t *player = &g_edicts[i];
                if (!player->inuse)
                    continue;
                if (player != ent && !OnSameTeam(ent, player))
                    continue;

                gix.local_sound(player, NULL, player, CHAN_AUTO, gi.soundindex("misc/help_marker.wav"), 1.0f, ATTN_NONE, 0.0f);
                gi.cprintf(player, PRINT_HIGH, other_notify_msg, ent->client->pers.netname);
            }
        }
    } else {
        edict_t *targ = NULL;
        while ((targ = findradius(targ, ent->s.origin, 1024)) != NULL) {
            if (ent == targ)
                continue;
            if (!targ->client)
                continue;
            if (!gix.inVIS(ent->s.origin, targ->s.origin, VIS_PVS | VIS_NOAREAS))
                continue;

            if (aiming_at && other_notify_msg)
                gi.cprintf(targ, PRINT_HIGH, other_notify_msg, ent->client->pers.netname, aiming_at->client->pers.netname);
            else if (other_notify_none_msg)
                gi.cprintf(targ, PRINT_HIGH, other_notify_none_msg, ent->client->pers.netname);
        }

        if (aiming_at && other_notify_msg)
            gi.cprintf(ent, PRINT_HIGH, other_notify_msg, ent->client->pers.netname, aiming_at->client->pers.netname);
        else if (other_notify_none_msg)
            gi.cprintf(ent, PRINT_HIGH, other_notify_none_msg, ent->client->pers.netname);
    }

    ent->client->anim_time = 0;
}

/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f(edict_t *ent, bool arg0)
{
    int     j;
    edict_t *other;
    char    text[2048];

    if (gi.argc() < 2 && !arg0)
        return;

    if (CheckFlood(ent))
        return;

    Q_snprintf(text, sizeof(text), "%s: ", ent->client->pers.netname);

    if (arg0) {
        Q_strlcat(text, gi.argv(0), sizeof(text));
        Q_strlcat(text, " ", sizeof(text));
        Q_strlcat(text, gi.args(), sizeof(text));
    } else {
        Q_strlcat(text, COM_StripQuotes(gi.args()), sizeof(text));
    }

    // don't let text be too long for malicious reasons
    if (strlen(text) > 150)
        text[150] = 0;

    Q_strlcat(text, "\n", sizeof(text));

    if (sv_dedicated->integer)
        gi.cprintf(NULL, PRINT_CHAT, "%s", text);

    for (j = 1; j <= game.maxclients; j++) {
        other = &g_edicts[j];
        if (!other->inuse)
            continue;
        if (!other->client)
            continue;
        gi.cprintf(other, PRINT_CHAT, "%s", text);
    }
}

static void Cmd_PlayerList_f(edict_t *ent)
{
    int i;
    char st[80];
    char text[MAX_IDEAL_PACKET_SIZE];
    edict_t *e2;

    // connect time, ping, score, name
    *text = 0;
    for (i = 0, e2 = g_edicts + 1; i < game.maxclients; i++, e2++) {
        if (!e2->inuse)
            continue;

        int sec = TO_SEC(level.time - e2->client->resp.entertime);
        Q_snprintf(st, sizeof(st), "%02d:%02d %4d %3d %s%s\n",
                   sec / 60, sec % 60,
                   e2->client->ping,
                   e2->client->resp.score,
                   e2->client->pers.netname,
                   e2->client->resp.spectator ? " (spectator)" : "");
        if (strlen(text) + strlen(st) > sizeof(text) - 50) {
            if (strlen(text) < sizeof(text) - 12)
                strcat(text, "And more...\n");
            gi.cprintf(ent, PRINT_HIGH, "%s", text);
            return;
        }
        strcat(text, st);
    }

    if (*text)
        gi.cprintf(ent, PRINT_HIGH, "%s", text);
}

static void Cmd_Switchteam_f(edict_t *ent)
{
    if (!G_TeamplayEnabled())
        return;

    // [Paril-KEX] in force-join, just do a regular team join.
    if (g_teamplay_force_join->integer) {
        // check if we should even switch teams
        edict_t *player;
        int team1count = 0, team2count = 0;
        ctfteam_t best_team;

        for (int i = 1; i <= game.maxclients; i++) {
            player = &g_edicts[i];

            // NB: we are counting ourselves in this one, unlike
            // the other assign team func
            if (!player->inuse)
                continue;

            switch (player->client->resp.ctf_team) {
            case CTF_TEAM1:
                team1count++;
                break;
            case CTF_TEAM2:
                team2count++;
                break;
            default:
                break;
            }
        }

        if (team1count < team2count)
            best_team = CTF_TEAM1;
        else
            best_team = CTF_TEAM2;

        if (ent->client->resp.ctf_team != best_team) {
            ////
            ent->svflags = SVF_NONE;
            ent->flags &= ~FL_GODMODE;
            ent->client->resp.ctf_team = best_team;
            ent->client->resp.ctf_state = 0;
            CTFAssignSkin(ent, Info_ValueForKey(ent->client->pers.userinfo, "skin"));

            // if anybody has a menu open, update it immediately
            CTFDirtyTeamMenu();

            if (ent->solid == SOLID_NOT) {
                // spectator
                PutClientInServer(ent);

                G_PostRespawn(ent);

                gi.bprintf(PRINT_HIGH, "%s joined the %s team.\n",
                           ent->client->pers.netname, CTFTeamName(best_team));
                return;
            }

            ent->health = 0;
            player_die(ent, ent, ent, 100000, vec3_origin, (mod_t) { .id = MOD_SUICIDE, .no_point_loss = true });

            // don't even bother waiting for death frames
            ent->deadflag = true;
            respawn(ent);

            ent->client->resp.score = 0;

            gi.bprintf(PRINT_HIGH, "%s changed to the %s team.\n",
                       ent->client->pers.netname, CTFTeamName(best_team));
        }

        return;
    }

    if (ent->client->resp.ctf_team != CTF_NOTEAM)
        CTFObserver(ent);

    if (!ent->client->menu)
        CTFOpenJoinMenu(ent);
}

static void Cmd_ListMonsters_f(edict_t *ent)
{
    if (!G_CheatCheck(ent))
        return;
    if (!g_debug_monster_kills->integer)
        return;

    for (int i = 0; i < level.total_monsters; i++) {
        edict_t *e = level.monsters_registered[i];

        if (!e || !e->inuse)
            continue;
        if (!(e->svflags & SVF_MONSTER) || (e->monsterinfo.aiflags & AI_DO_NOT_COUNT))
            continue;
        if (e->deadflag)
            continue;

        gi.dprintf("%s\n", etos(e));
    }
}

/*
=================
ClientCommand
=================
*/
void ClientCommand(edict_t *ent)
{
    const char *cmd;

    if (!ent->client)
        return; // not fully in game yet

    cmd = gi.argv(0);

    if (Q_strcasecmp(cmd, "players") == 0) {
        Cmd_Players_f(ent);
        return;
    }
    if (Q_strcasecmp(cmd, "say") == 0) {
        Cmd_Say_f(ent, false);
        return;
    }
    if (Q_strcasecmp(cmd, "say_team") == 0 || Q_strcasecmp(cmd, "steam") == 0) {
        if (G_TeamplayEnabled())
            CTFSay_Team(ent, gi.args());
        else
            Cmd_Say_f(ent, false);
        return;
    }
    if (Q_strcasecmp(cmd, "score") == 0) {
        Cmd_Score_f(ent);
        return;
    }
    if (Q_strcasecmp(cmd, "help") == 0) {
        Cmd_Help_f(ent);
        return;
    }
    if (Q_strcasecmp(cmd, "listmonsters") == 0) {
        Cmd_ListMonsters_f(ent);
        return;
    }

    if (level.intermissiontime)
        return;

    if (Q_strcasecmp(cmd, "target") == 0)
        Cmd_Target_f(ent);
    else if (Q_strcasecmp(cmd, "use") == 0 || Q_strcasecmp(cmd, "use_only") == 0 ||
             Q_strcasecmp(cmd, "use_index") == 0 || Q_strcasecmp(cmd, "use_index_only") == 0)
        Cmd_Use_f(ent);
    else if (Q_strcasecmp(cmd, "drop") == 0 ||
             Q_strcasecmp(cmd, "drop_index") == 0)
        Cmd_Drop_f(ent);
    else if (Q_strcasecmp(cmd, "give") == 0)
        Cmd_Give_f(ent);
    else if (Q_strcasecmp(cmd, "god") == 0)
        Cmd_God_f(ent);
    else if (Q_strcasecmp(cmd, "immortal") == 0)
        Cmd_Immortal_f(ent);
    // Paril: cheats to help with dev
    else if (Q_strcasecmp(cmd, "spawn") == 0)
        Cmd_Spawn_f(ent);
    else if (Q_strcasecmp(cmd, "teleport") == 0)
        Cmd_Teleport_f(ent);
    else if (Q_strcasecmp(cmd, "notarget") == 0)
        Cmd_Notarget_f(ent);
    else if (Q_strcasecmp(cmd, "novisible") == 0)
        Cmd_Novisible_f(ent);
    else if (Q_strcasecmp(cmd, "alertall") == 0)
        Cmd_AlertAll_f(ent);
    else if (Q_strcasecmp(cmd, "noclip") == 0)
        Cmd_Noclip_f(ent);
    else if (Q_strcasecmp(cmd, "inven") == 0)
        Cmd_Inven_f(ent);
    else if (Q_strcasecmp(cmd, "invnext") == 0)
        SelectNextItem(ent, IF_ANY, true);
    else if (Q_strcasecmp(cmd, "invprev") == 0)
        SelectPrevItem(ent, IF_ANY);
    else if (Q_strcasecmp(cmd, "invnextw") == 0)
        SelectNextItem(ent, IF_WEAPON, true);
    else if (Q_strcasecmp(cmd, "invprevw") == 0)
        SelectPrevItem(ent, IF_WEAPON);
    else if (Q_strcasecmp(cmd, "invnextp") == 0)
        SelectNextItem(ent, IF_POWERUP, true);
    else if (Q_strcasecmp(cmd, "invprevp") == 0)
        SelectPrevItem(ent, IF_POWERUP);
    else if (Q_strcasecmp(cmd, "invuse") == 0)
        Cmd_InvUse_f(ent);
    else if (Q_strcasecmp(cmd, "invdrop") == 0)
        Cmd_InvDrop_f(ent);
    else if (Q_strcasecmp(cmd, "weapprev") == 0)
        Cmd_WeapPrev_f(ent);
    else if (Q_strcasecmp(cmd, "weapnext") == 0)
        Cmd_WeapNext_f(ent);
    else if (Q_strcasecmp(cmd, "weaplast") == 0 || Q_strcasecmp(cmd, "lastweap") == 0)
        Cmd_WeapLast_f(ent);
    else if (Q_strcasecmp(cmd, "kill") == 0)
        Cmd_Kill_f(ent);
    else if (Q_strcasecmp(cmd, "kill_ai") == 0)
        Cmd_Kill_AI_f(ent);
    else if (Q_strcasecmp(cmd, "where") == 0)
        Cmd_Where_f(ent);
    else if (Q_strcasecmp(cmd, "clear_ai_enemy") == 0)
        Cmd_Clear_AI_Enemy_f(ent);
    else if (Q_strcasecmp(cmd, "putaway") == 0)
        Cmd_PutAway_f(ent);
    else if (Q_strcasecmp(cmd, "wave") == 0)
        Cmd_Wave_f(ent);
    else if (Q_strcasecmp(cmd, "playerlist") == 0)
        Cmd_PlayerList_f(ent);
    // ZOID
    else if (Q_strcasecmp(cmd, "team") == 0)
        CTFTeam_f(ent);
    else if (Q_strcasecmp(cmd, "id") == 0)
        CTFID_f(ent);
    else if (Q_strcasecmp(cmd, "yes") == 0)
        CTFVoteYes(ent);
    else if (Q_strcasecmp(cmd, "no") == 0)
        CTFVoteNo(ent);
    else if (Q_strcasecmp(cmd, "ready") == 0)
        CTFReady(ent);
    else if (Q_strcasecmp(cmd, "notready") == 0)
        CTFNotReady(ent);
    else if (Q_strcasecmp(cmd, "ghost") == 0)
        CTFGhost(ent);
    else if (Q_strcasecmp(cmd, "admin") == 0)
        CTFAdmin(ent);
    else if (Q_strcasecmp(cmd, "stats") == 0)
        CTFStats(ent);
    else if (Q_strcasecmp(cmd, "warp") == 0)
        CTFWarp(ent);
    else if (Q_strcasecmp(cmd, "boot") == 0)
        CTFBoot(ent);
    else if (Q_strcasecmp(cmd, "playerlist") == 0)
        CTFPlayerList(ent);
    else if (Q_strcasecmp(cmd, "observer") == 0)
        CTFObserver(ent);
    // ZOID
    else if (Q_strcasecmp(cmd, "switchteam") == 0)
        Cmd_Switchteam_f(ent);
    else // anything that doesn't match a command will be a chat
        Cmd_Say_f(ent, true);
}
