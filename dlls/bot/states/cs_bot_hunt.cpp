
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "training_gamerules.h"
#include "hltv.h"
#include "game.h"
#include "trains.h"
#include "vehicle.h"
#include "globals.h"

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

// Begin the hunt

void HuntState::OnEnter(CCSBot *me)
{
	// lurking death
	if (me->IsUsingKnife() && me->IsWellPastSafe() && !me->IsHurrying())
		me->Walk();
	else
		me->Run();

	me->StandUp();
	me->SetDisposition(CCSBot::ENGAGE_AND_INVESTIGATE);
	me->SetTask(CCSBot::SEEK_AND_DESTROY);
	me->DestroyPath();
}

// Hunt down our enemies

void HuntState::OnUpdate(CCSBot *me)
{
	// if we've been hunting for a long time, drop into Idle for a moment to
	// select something else to do
	const float huntingTooLongTime = 30.0f;
	if (gpGlobals->time - me->GetStateTimestamp() > huntingTooLongTime)
	{
		// stop being a rogue and do the scenario, since there must not be many enemies left to hunt
		me->PrintIfWatched("Giving up hunting, and being a rogue\n");
		me->SetRogue(false);
		me->Idle();
		return;
	}

	CCSBotManager *ctrl = TheCSBots();

	// scenario logic
	if (ctrl->GetScenario() == CCSBotManager::SCENARIO_DEFUSE_BOMB)
	{
		if (me->m_iTeam == TERRORIST)
		{
			// if we have the bomb and it's time to plant, or we happen to be in a bombsite and it seems safe, do it
			if (me->IsCarryingBomb())
			{
				const float safeTime = 3.0f;
				if (ctrl->IsTimeToPlantBomb() || (me->IsAtBombsite() && gpGlobals->time - me->GetLastSawEnemyTimestamp() > safeTime))
				{
					me->Idle();
					return;
				}
			}

			// if we notice the bomb lying on the ground, go get it
			if (me->NoticeLooseBomb())
			{
				me->FetchBomb();
				return;
			}

			// if bomb has been planted, and we hear it, move to a hiding spot near the bomb and watch it
			const Vector *bombPos = me->GetGameState()->GetBombPosition();
			if (!me->IsRogue() && me->GetGameState()->IsBombPlanted() && bombPos != NULL)
			{
				me->SetTask(CCSBot::GUARD_TICKING_BOMB);
				me->Hide(TheNavAreaGrid.GetNavArea(bombPos));
				return;
			}
		}
		// CT
		else
		{
			if (!me->IsRogue() && me->CanSeeLooseBomb())
			{
				// if we are near the loose bomb and can see it, hide nearby and guard it
				me->SetTask(CCSBot::GUARD_LOOSE_BOMB);
				me->Hide(ctrl->GetLooseBombArea());
				me->GetChatter()->AnnouncePlan("GoingToGuardLooseBomb", ctrl->GetLooseBombArea()->GetPlace());
				return;
			}
			else if (ctrl->IsBombPlanted())
			{
				// rogues will defuse a bomb, but not guard the defuser
				if (!me->IsRogue() || !ctrl->GetBombDefuser())
				{
					// search for the planted bomb to defuse
					me->Idle();
					return;
				}
			}
		}
	}
	else if (ctrl->GetScenario() == CCSBotManager::SCENARIO_RESCUE_HOSTAGES)
	{
		if (me->m_iTeam == TERRORIST)
		{
			if (me->GetGameState()->AreAllHostagesBeingRescued())
			{
				// all hostages are being rescued, head them off at the escape zones
				if (me->GuardRandomZone())
				{
					me->SetTask(CCSBot::GUARD_HOSTAGE_RESCUE_ZONE);
					me->PrintIfWatched("Trying to beat them to an escape zone!\n");
					me->SetDisposition(CCSBot::OPPORTUNITY_FIRE);
					me->GetChatter()->GuardingHostageEscapeZone(IS_PLAN);
					return;
				}
			}

			// if safe time is up, and we stumble across a hostage, guard it
			if (!me->IsRogue() && !me->IsSafe())
			{
				CHostage *hostage = me->GetGameState()->GetNearestVisibleFreeHostage();
				if (hostage != NULL)
				{
					CNavArea *area = TheNavAreaGrid.GetNearestNavArea(&hostage->pev->origin);
					if (area != NULL)
					{
						// we see a free hostage, guard it
						me->SetTask(CCSBot::GUARD_HOSTAGES);
						me->Hide(area);
						me->PrintIfWatched("I'm guarding hostages\n");
						me->GetChatter()->GuardingHostages(area->GetPlace(), IS_PLAN);
						return;
					}
				}
			}
		}
	}

	// listen for enemy noises
	if (me->ShouldInvestigateNoise())
	{
		me->InvestigateNoise();
		return;
	}

	// look around
	me->UpdateLookAround();

	// if we have reached our destination area, pick a new one
	// if our path fails, pick a new one
	if (me->GetLastKnownArea() == m_huntArea || me->UpdatePathMovement() != CCSBot::PROGRESSING)
	{
		m_huntArea = NULL;
		float oldest = 0.0f;

		int areaCount = 0;
		const float minSize = 150.0f;

		FOR_EACH_LL (TheNavAreaList, it)
		{
			CNavArea *area = TheNavAreaList[it];

			++areaCount;

			// skip the small areas
			const Extent *extent = area->GetExtent();
			if (extent->hi.x - extent->lo.x < minSize || extent->hi.y - extent->lo.y < minSize)
				continue;

			// keep track of the least recently cleared area
			float age = gpGlobals->time - area->GetClearedTimestamp(me->m_iTeam - 1);
			if (age > oldest)
			{
				oldest = age;
				m_huntArea = area;
			}
		}

		// if all the areas were too small, pick one at random
		int which = RANDOM_LONG(0, areaCount - 1);

		areaCount = 0;
		FOR_EACH_LL (TheNavAreaList, it2)
		{
			m_huntArea = TheNavAreaList[it2];

			if (which == areaCount)
				break;

			--which;
		}

		if (m_huntArea != NULL)
		{
			// create a new path to a far away area of the map
			me->ComputePath(m_huntArea, NULL, SAFEST_ROUTE);
		}
	}
}

// Done hunting

void HuntState::OnExit(CCSBot *me)
{
	;
}
