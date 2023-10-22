/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "hud.h"

extern "C"
{
#include "pm_math.h"
}

#include "parsemsg.h"
#include "cl_util.h"

#include "r_efx.h"
#include "event_api.h"
#include "com_model.h"
#include <string.h>

DECLARE_MESSAGE( m_Radio, SendAudio )
DECLARE_MESSAGE( m_Radio, ReloadSound )
DECLARE_MESSAGE( m_Radio, BotVoice )

int CHudRadio::Init( )
{
	HOOK_MESSAGE( SendAudio );
	HOOK_MESSAGE( ReloadSound );
	HOOK_MESSAGE( BotVoice );
	gHUD.AddHudElem( this );
	m_iFlags = 0;
	return 1;
}


void Broadcast( const char *msg, int pitch )
{
	if ( msg[0] == '%' && msg[1] == '!' )
		gEngfuncs.pfnPlaySoundVoiceByName( &const_cast<char *>(msg)[1], 1.0f, pitch );
	else
		gEngfuncs.pfnPlaySoundVoiceByName(const_cast<char *>(msg), 1.0f, pitch );
}

int CHudRadio::MsgFunc_SendAudio( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );

	int SenderID = reader.ReadByte( );
	char *sentence = reader.ReadString( );
	int pitch = reader.ReadShort( );

	Broadcast( sentence, pitch );

	if( SenderID <= MAX_PLAYERS )
	{
		g_PlayerExtraInfo[SenderID].radarflashes = 22;
		g_PlayerExtraInfo[SenderID].radarflashtime = gHUD.m_flTime;
		g_PlayerExtraInfo[SenderID].radarflashtimedelta = 0.5f;
	}
	return 1;
}

int CHudRadio::MsgFunc_ReloadSound( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );

	int vol = reader.ReadByte( );
	if ( reader.ReadByte( ) )
	{
		gEngfuncs.pfnPlaySoundByName( "weapon/generic_reload.wav", vol / 255.0f );
	}
	else
	{
		gEngfuncs.pfnPlaySoundByName( "weapon/generic_shot_reload.wav", vol / 255.0f );
	}
	return 1;
}


static void VoiceIconCallback(struct tempent_s *ent, float frametime, float currenttime)
{
	int entIndex = ent->clientIndex;
	if( !g_PlayerExtraInfo[entIndex].talking )
	{
		g_PlayerExtraInfo[entIndex].talking = false;
		ent->die = 0.0f;
	}
}

void CHudRadio::Voice(int entindex, bool bTalking)
{
	extra_player_info_t *pplayer;
	TEMPENTITY *temp;
	int spr;

	if( entindex < 0 || entindex > MAX_PLAYERS - 1) // bomb can't talk!
		return;

	pplayer = g_PlayerExtraInfo + entindex;

	if( bTalking == pplayer->talking )
		return; // user is talking already

	if( !bTalking && pplayer->talking )
	{
		pplayer->talking = false;
		return; // stop talking
	}

	spr = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/voiceicon.spr" );
	if( !spr ) return;

	temp = gEngfuncs.pEfxAPI->R_DefaultSprite( vec3_origin, spr, 0 );
	if( !temp ) return;

	pplayer->talking = true; // sprite is created

	temp->flags = FTENT_SPRANIMATELOOP | FTENT_CLIENTCUSTOM | FTENT_PLYRATTACHMENT;
	temp->tentOffset.z = 40;
	temp->clientIndex = entindex;
	temp->callback = VoiceIconCallback;
	temp->entity.curstate.scale = 0.60f;
	temp->entity.curstate.rendermode = kRenderTransAdd;
	temp->die = gHUD.m_flTime + 60.0f; // 60 seconds must be enough?
}

int CHudRadio::MsgFunc_BotVoice( const char *pszName, int iSize, void *buf )
{
	BufferReader reader( pszName, buf, iSize );

	int enable   = reader.ReadByte();
	int entIndex = reader.ReadByte();

	Voice( entIndex, enable );

	return 1;
}
