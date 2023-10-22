
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"
#include "game.h"
#include "sound.h"

#include "pm_shared.h"
#include "utllinkedlist.h"

// CSBOT and Nav
#include "game_shared/GameEvent.h"		// Game event enum used by career mode, tutor system, and bots
#include "game_shared/bot/bot_util.h"
#include "game_shared/bot/simple_state_machine.h"

#include "game_shared/steam_util.h"

#include "game_shared/bot/bot_manager.h"
#include "game_shared/bot/bot_constants.h"
#include "game_shared/bot/bot.h"

#include "game_shared/shared_util.h"
#include "game_shared/bot/bot_profile.h"

#include "game_shared/bot/nav.h"
#include "game_shared/bot/improv.h"
#include "game_shared/bot/nav_node.h"
#include "game_shared/bot/nav_area.h"
#include "game_shared/bot/nav_file.h"
#include "game_shared/bot/nav_path.h"

#include "airtank.h"
#include "h_ai.h"
#include "h_cycler.h"
#include "h_battery.h"

// Hostage
#include "hostage/hostage.h"
#include "hostage/hostage_localnav.h"

#include "bot/cs_bot.h"

// Tutor
#include "tutor.h"
#include "tutor_base_states.h"
#include "tutor_base_tutor.h"
#include "tutor_cs_states.h"
#include "tutor_cs_tutor.h"

#include "gamerules.h"
#include "career_tasks.h"
#include "maprules.h"

LINK_ENTITY_TO_CLASS(world_items, CWorldItem);

void CWorldItem::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "type"))
	{
		m_iType = Q_atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CWorldItem::Spawn()
{
	CBaseEntity *pEntity = NULL;

	switch (m_iType)
	{
	case 41: // ITEM_HEALTHKIT
		break;
	case 42: // ITEM_ANTIDOTE
		pEntity = CBaseEntity::Create("item_antidote", pev->origin, pev->angles);
		break;
	case 43: // ITEM_SECURITY
		pEntity = CBaseEntity::Create("item_security", pev->origin, pev->angles);
		break;
	case 44: // ITEM_BATTERY
		pEntity = CBaseEntity::Create("item_battery", pev->origin, pev->angles);
		break;
	case 45: // ITEM_SUIT
		pEntity = CBaseEntity::Create("item_suit", pev->origin, pev->angles);
		break;
	}

	if (pEntity != NULL)
	{
		pEntity->pev->target = pev->target;
		pEntity->pev->targetname = pev->targetname;
		pEntity->pev->spawnflags = pev->spawnflags;
	}

	REMOVE_ENTITY(edict());
}

void CItem::Spawn()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));

	SetTouch(&CItem::ItemTouch);

	if (!DROP_TO_FLOOR(ENT(pev)))
	{
		UTIL_Remove(this);
		return;
	}
}

void CItem::ItemTouch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer() || pOther->pev->deadflag != DEAD_NO)
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	if (!g_pGameRules->CanHaveItem(pPlayer, this))
		return;

	if (MyTouch(pPlayer))
	{
		SUB_UseTargets(pOther, USE_TOGGLE, 0);
		SetTouch(NULL);
		g_pGameRules->PlayerGotItem(pPlayer, this);

		if (g_pGameRules->ItemShouldRespawn(this) == GR_ITEM_RESPAWN_YES)
			Respawn();
		else
			UTIL_Remove(this);
	}
	else if (gEvilImpulse101)
		UTIL_Remove(this);
}

CBaseEntity *CItem::Respawn()
{
	SetTouch(NULL);

	pev->effects |= EF_NODRAW;

	// blip to whereever you should respawn.
	UTIL_SetOrigin(pev, g_pGameRules->VecItemRespawnSpot(this));

	SetThink(&CItem::Materialize);
	pev->nextthink = g_pGameRules->FlItemRespawnTime(this);

	return this;
}

void CItem::Materialize()
{
	if (pev->effects & EF_NODRAW)
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", VOL_NORM, ATTN_NORM, 0, 150);

		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch(&CItem::ItemTouch);
}

void CItemSuit::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_kevlar.mdl");
	CItem::Spawn();
}

void CItemSuit::Precache()
{
	PRECACHE_MODEL("models/w_kevlar.mdl");
	PRECACHE_SOUND("items/tr_kevlar.wav");
}

BOOL CItemSuit::MyTouch(CBasePlayer *pPlayer)
{
	if (pPlayer->pev->weapons & (1 << WEAPON_SUIT))
		return FALSE;

	EMIT_SOUND(pPlayer->edict(), CHAN_VOICE, "items/tr_kevlar.wav", VOL_NORM, ATTN_NORM);

	pPlayer->pev->weapons |= (1 << WEAPON_SUIT);
	pPlayer->m_iHideHUD &= ~(HIDEHUD_HEALTH | HIDEHUD_MONEY);

	return TRUE;
}

LINK_ENTITY_TO_CLASS(item_suit, CItemSuit);

void CItemBattery::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_battery.mdl");
	CItem::Spawn();
}

void CItemBattery::Precache()
{
	PRECACHE_MODEL("models/w_battery.mdl");
	PRECACHE_SOUND("items/gunpickup2.wav");
}

BOOL CItemBattery::MyTouch(CBasePlayer *pPlayer)
{
	if (pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY && (pPlayer->pev->weapons & (1 << WEAPON_SUIT)))
	{
		int pct;
		char szcharge[64];

		pPlayer->pev->armorvalue += gSkillData.batteryCapacity;
		pPlayer->pev->armorvalue = Q_min(pPlayer->pev->armorvalue, MAX_NORMAL_BATTERY);

		EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", VOL_NORM, ATTN_NORM);

		MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
			WRITE_STRING(STRING(pev->classname));
		MESSAGE_END();

		// Suit reports new power level
		// For some reason this wasn't working in release build -- round it.
		pct = (int)((float)(pPlayer->pev->armorvalue * 100.0f) * (1.0f / MAX_NORMAL_BATTERY) + 0.5f);
		pct = (pct / 5);

		if (pct > 0)
			pct--;

		Q_sprintf(szcharge,"!HEV_%1dP", pct);
		pPlayer->SetSuitUpdate(szcharge, FALSE, SUIT_NEXT_IN_30SEC);

		return TRUE;
	}
	return FALSE;
}

LINK_ENTITY_TO_CLASS(item_battery, CItemBattery);

void CItemAntidote::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_antidote.mdl");
	CItem::Spawn();
}

void CItemAntidote::Precache()
{
	PRECACHE_MODEL("models/w_antidote.mdl");
}

BOOL CItemAntidote::MyTouch(CBasePlayer *pPlayer)
{
	pPlayer->SetSuitUpdate("!HEV_DET4", FALSE, SUIT_NEXT_IN_1MIN);
	pPlayer->m_rgItems[ ITEM_ANTIDOTE ] += 1;

	return TRUE;
}

LINK_ENTITY_TO_CLASS(item_antidote, CItemAntidote);

void CItemSecurity::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_security.mdl");
	CItem::Spawn();
}

void CItemSecurity::Precache()
{
	PRECACHE_MODEL("models/w_security.mdl");
}

BOOL CItemSecurity::MyTouch(CBasePlayer *pPlayer)
{
	pPlayer->m_rgItems[ ITEM_SECURITY ] += 1;
	return TRUE;
}

LINK_ENTITY_TO_CLASS(item_security, CItemSecurity);

void CItemLongJump::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_longjump.mdl");
	CItem::Spawn();
}

void CItemLongJump::Precache()
{
	PRECACHE_MODEL("models/w_longjump.mdl");
}

BOOL CItemLongJump::MyTouch(CBasePlayer *pPlayer)
{
	if (pPlayer->m_fLongJump)
		return FALSE;

	if (pPlayer->pev->weapons & (1 << WEAPON_SUIT))
	{
		// player now has longjump module
		pPlayer->m_fLongJump = TRUE;
		SET_PHYSICS_KEY_VALUE(pPlayer->edict(), "slj", "1");

		MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
			WRITE_STRING(STRING(pev->classname));
		MESSAGE_END();

		// Play the longjump sound UNDONE: Kelly? correct sound?
		EMIT_SOUND_SUIT(pPlayer->edict(), "!HEV_A1");
		return TRUE;
	}

	return FALSE;
}

LINK_ENTITY_TO_CLASS(item_longjump, CItemLongJump);

void CItemKevlar::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_kevlar.mdl");
	CItem::Spawn();
}

void CItemKevlar::Precache()
{
	PRECACHE_MODEL("models/w_kevlar.mdl");
}

BOOL CItemKevlar::MyTouch(CBasePlayer *pPlayer)
{
	if (pPlayer->m_iKevlar == ARMOR_TYPE_EMPTY)
		pPlayer->m_iKevlar = ARMOR_TYPE_KEVLAR;

	pPlayer->pev->armorvalue = 100;
	EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "items/ammopickup2.wav", VOL_NORM, ATTN_NORM);

	MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
		WRITE_STRING(STRING(pev->classname));
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_ONE, gmsgArmorType, NULL, pPlayer->pev);
		WRITE_BYTE(0);
	MESSAGE_END();

	if (TheTutor != NULL)
	{
		TheTutor->OnEvent(EVENT_PLAYER_BOUGHT_SOMETHING, pPlayer);
	}

	return TRUE;
}

LINK_ENTITY_TO_CLASS(item_kevlar, CItemKevlar);

void CItemAssaultSuit::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_assault.mdl");
	CItem::Spawn();
}

void CItemAssaultSuit::Precache()
{
	PRECACHE_MODEL("models/w_assault.mdl");
}

BOOL CItemAssaultSuit::MyTouch(CBasePlayer *pPlayer)
{
	pPlayer->m_iKevlar = ARMOR_TYPE_HELMET;
	pPlayer->pev->armorvalue = 100;

	EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "items/ammopickup2.wav", VOL_NORM, ATTN_NORM);

	MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
		WRITE_STRING(STRING(pev->classname));
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_ONE, gmsgArmorType, NULL, pPlayer->pev);
		WRITE_BYTE(1);
	MESSAGE_END();

	if (TheTutor != NULL)
	{
		TheTutor->OnEvent(EVENT_PLAYER_BOUGHT_SOMETHING, pPlayer);
	}

	return TRUE;
}

LINK_ENTITY_TO_CLASS(item_assaultsuit, CItemAssaultSuit);

void CItemThighPack::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_thighpack.mdl");
	CItem::Spawn();
}

void CItemThighPack::Precache()
{
	PRECACHE_MODEL("models/w_thighpack.mdl");
}

BOOL CItemThighPack::MyTouch(CBasePlayer *pPlayer)
{
	if (pPlayer->m_iTeam != CT || pPlayer->m_bHasDefuser)
		return FALSE;

	pPlayer->m_bHasDefuser = true;
	pPlayer->pev->body = 1;

	ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Got_defuser");

	MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pPlayer->pev);
		WRITE_BYTE(STATUSICON_SHOW);
		WRITE_STRING("defuser");
		WRITE_BYTE(0);
		WRITE_BYTE(160);
		WRITE_BYTE(0);
	MESSAGE_END();

	SendItemStatus(pPlayer);
	EMIT_SOUND(pPlayer->edict(), CHAN_VOICE, "items/kevlar.wav", VOL_NORM, ATTN_NORM);

	if (TheTutor != NULL)
	{
		TheTutor->OnEvent(EVENT_PLAYER_BOUGHT_SOMETHING, pPlayer);
	}

	return TRUE;
}

LINK_ENTITY_TO_CLASS(item_thighpack, CItemThighPack);
