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
//
// status_icons.cpp
//
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "event_api.h"
#include "com_weapons.h"

#include "draw_util.h"
#include "triangleapi.h"

DECLARE_MESSAGE( m_StatusIcons, StatusIcon )

int CHudStatusIcons::Init( void )
{
	HOOK_MESSAGE( StatusIcon );

	gHUD.AddHudElem( this );

	Reset();

	m_tgaC4[0] = m_tgaC4[1] = 0;

	return 1;
}

int CHudStatusIcons::VidInit( void )
{
	m_tgaC4[0] = gRenderAPI.GL_LoadTexture("resource/helperhud/c4_left_default", NULL, 0, TF_NEAREST | TF_NOPICMIP | TF_NOMIPMAP | TF_CLAMP);
	m_tgaC4[1] = gRenderAPI.GL_LoadTexture("resource/helperhud/c4_left_install", NULL, 0, TF_NEAREST | TF_NOPICMIP | TF_NOMIPMAP | TF_CLAMP);
	return 1;
}

void CHudStatusIcons::Reset( void )
{
	memset( m_IconList, 0, sizeof m_IconList );
	m_iFlags &= ~HUD_DRAW;
}

void CHudStatusIcons::Shutdown(void)
{
	for (int iTexture : m_tgaC4)
		gRenderAPI.GL_FreeTexture(iTexture);
}

// Draw status icons along the left-hand side of the screen
int CHudStatusIcons::Draw( float flTime )
{
	if (gEngfuncs.IsSpectateOnly())
		return 1;
	// find starting position to draw from, along right-hand side of screen
	int x = 5;
	int y = ScreenHeight / 2;

	// loop through icon list, and draw any valid icons drawing up from the middle of screen
	for ( int i = 0; i < MAX_ICONSPRITES; i++ )
	{
		if ( m_IconList[i].spr )
		{
			y -= ( m_IconList[i].rc.bottom - m_IconList[i].rc.top ) + 5;
			
			/*if( g_bInBombZone && !strcmp(m_IconList[i].szSpriteName, "c4") && ((int)(flTime * 10) % 2))
				SPR_Set( m_IconList[i].spr, 255, 16, 16 );
			else SPR_Set( m_IconList[i].spr, m_IconList[i].r, m_IconList[i].g, m_IconList[i].b );*/
			if (!strcmp(m_IconList[i].szSpriteName, "c4"))
			{
				gEngfuncs.pTriAPI->RenderMode(kRenderTransTexture);
				gEngfuncs.pTriAPI->Color4ub(255, 255, 255, 255);
				gRenderAPI.GL_SelectTexture(0);
				if(g_bInBombZone && ((int)(flTime * 10) % 2))
					gRenderAPI.GL_Bind(0, m_tgaC4[1]);
				else
					gRenderAPI.GL_Bind(0, m_tgaC4[0]);

				DrawUtils::Draw2DQuad(x, y, x + 58, y + 55);
			}
			else
			{
				SPR_Set(m_IconList[i].spr, m_IconList[i].r, m_IconList[i].g, m_IconList[i].b);
				SPR_DrawAdditive(0, x, y, &m_IconList[i].rc);
			}
			
		}
	}
	
	return 1;
}

// Message handler for StatusIcon message
// accepts five values:
//		byte   : TRUE = ENABLE icon, FALSE = DISABLE icon
//		string : the sprite name to display
//		byte   : red
//		byte   : green
//		byte   : blue
int CHudStatusIcons::MsgFunc_StatusIcon( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );

	int ShouldEnable = reader.ReadByte();
	char *pszIconName = reader.ReadString();

	if ( ShouldEnable )
	{
		int r = reader.ReadByte();
		int g = reader.ReadByte();
		int b = reader.ReadByte();
		EnableIcon( pszIconName, r, g, b );
		m_iFlags |= HUD_DRAW;
	}
	else
	{
		DisableIcon( pszIconName );
	}

	return 1;
}

// add the icon to the icon list, and set it's drawing color
void CHudStatusIcons::EnableIcon( const char *pszIconName, unsigned char red, unsigned char green, unsigned char blue )
{
	// check to see if the sprite is in the current list
	int i;
	for ( i = 0; i < MAX_ICONSPRITES; i++ )
	{
		if ( !stricmp( m_IconList[i].szSpriteName, pszIconName ) )
			break;
	}

	if ( i == MAX_ICONSPRITES )
	{
		// icon not in list, so find an empty slot to add to
		for ( i = 0; i < MAX_ICONSPRITES; i++ )
		{
			if ( !m_IconList[i].spr )
				break;
		}
	}

	// if we've run out of space in the list, overwrite the first icon
	if ( i == MAX_ICONSPRITES )
	{
		i = 0;
	}

	// Load the sprite and add it to the list
	// the sprite must be listed in hud.txt
	int spr_index = gHUD.GetSpriteIndex( pszIconName );
	m_IconList[i].spr = gHUD.GetSprite( spr_index );
	m_IconList[i].rc = gHUD.GetSpriteRect( spr_index );
	m_IconList[i].r = red;
	m_IconList[i].g = green;
	m_IconList[i].b = blue;
	strncpy( m_IconList[i].szSpriteName, pszIconName, MAX_ICONSPRITENAME_LENGTH );

	// Hack: Play Timer sound when a grenade icon is played (in 0.8 seconds)
	if ( strstr(m_IconList[i].szSpriteName, "grenade") )
	{
		cl_entity_t *pthisplayer = gEngfuncs.GetLocalPlayer();
		gEngfuncs.pEventAPI->EV_PlaySound( pthisplayer->index, pthisplayer->origin, CHAN_STATIC, "weapons/timer.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
	}
}

void CHudStatusIcons::DisableIcon( const char *pszIconName )
{
	// find the sprite is in the current list
	for ( int i = 0; i < MAX_ICONSPRITES; i++ )
	{
		if ( !stricmp( m_IconList[i].szSpriteName, pszIconName ) )
		{
			// clear the item from the list
			memset( &m_IconList[i], 0, sizeof( icon_sprite_t ) );
			return;
		}
	}
}
