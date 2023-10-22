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

#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "vgui_parser.h"
#include <string.h>
#include "draw_util.h"

DECLARE_MESSAGE( m_Timer, RoundTime )
DECLARE_MESSAGE( m_Timer, ShowTimer )

int CHudTimer::Init()
{
	HOOK_MESSAGE( RoundTime );
	HOOK_MESSAGE( ShowTimer );
	m_iFlags = 0;
	m_bPanicColorChange = false;
	gHUD.AddHudElem(this);
	return 1;
}

int CHudTimer::VidInit()
{
	m_HUD_timer = gHUD.GetSpriteIndex( "stopwatch" );
	return 1;
}

int CHudTimer::Draw( float fTime )
{
	if ( ( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH ) )
        return 1;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;
	int r, g, b;
	// time must be positive
	int minutes = max( 0, (int)( m_iTime + m_fStartTime - gHUD.m_flTime ) / 60);
	int seconds = max( 0, (int)( m_iTime + m_fStartTime - gHUD.m_flTime ) - (minutes * 60));

	if( minutes * 60 + seconds > 20 )
	{
		DrawUtils::UnpackRGB(r,g,b, RGB_YELLOWISH );
	}
	else
	{
		m_flPanicTime += gHUD.m_flTimeDelta;
		// add 0.1 sec, so it's not flicker fast
		if( m_flPanicTime > ((float)seconds / 40.0f) + 0.1f)
		{
			m_flPanicTime = 0;
			m_bPanicColorChange = !m_bPanicColorChange;
		}
		DrawUtils::UnpackRGB( r, g, b, m_bPanicColorChange ? RGB_YELLOWISH : RGB_REDISH );
	}

	DrawUtils::ScaleColors( r, g, b, MIN_ALPHA );

    
    int iWatchWidth = gHUD.GetSpriteRect(m_HUD_timer).right - gHUD.GetSpriteRect(m_HUD_timer).left;
    
	int x = ScreenWidth/2;
	int y = ScreenHeight - 1.5 * gHUD.m_iFontHeight ;
    
    SPR_Set(gHUD.GetSprite(m_HUD_timer), r, g, b);
    SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_timer));
        
	x = DrawUtils::DrawHudNumber2( x + iWatchWidth / 4, y, false, 2, minutes, r, g, b );
	// draw :
	FillRGBA(x + iWatchWidth / 4, y + gHUD.m_iFontHeight / 4, 2, 2, r, g, b, 100);
	FillRGBA(x + iWatchWidth / 4, y + gHUD.m_iFontHeight - gHUD.m_iFontHeight / 4, 2, 2, r, g, b, 100);

	DrawUtils::DrawHudNumber2( x + iWatchWidth / 2, y, true, 2, seconds, r, g, b );
	return 1;
}

int CHudTimer::MsgFunc_RoundTime(const char *pszName, int iSize, void *pbuf)
{
	BufferReader reader( pszName, pbuf, iSize );
	m_iTime = reader.ReadShort();
	m_fStartTime = gHUD.m_flTime;
	m_iFlags = HUD_DRAW;
	return 1;
}

int CHudTimer::MsgFunc_ShowTimer(const char *pszName, int iSize, void *pbuf)
{
	m_iFlags = HUD_DRAW;
	return 1;
}

#define UPDATE_BOTPROGRESS 0
#define CREATE_BOTPROGRESS 1
#define REMOVE_BOTPROGRESS 2

DECLARE_MESSAGE( m_ProgressBar, BarTime )
DECLARE_MESSAGE( m_ProgressBar, BarTime2 )
DECLARE_MESSAGE( m_ProgressBar, BotProgress )

int CHudProgressBar::Init()
{
	HOOK_MESSAGE( BarTime );
	HOOK_MESSAGE( BarTime2 );
	HOOK_MESSAGE( BotProgress );
	m_iFlags = 0;
	m_szLocalizedHeader = NULL;
	m_szHeader[0] = '\0';
	m_fStartTime = m_fPercent = 0.0f;

	gHUD.AddHudElem(this);
	return 1;
}

int CHudProgressBar::VidInit()
{
	return 1;
}

int CHudProgressBar::Draw( float flTime )
{
	// allow only 0.0..1.0
	if( (m_fPercent < 0.0f) || (m_fPercent > 1.0f) )
	{
		m_iFlags = 0;
		m_fPercent = 0.0f;
		return 1;
	}

	if( m_szLocalizedHeader && m_szLocalizedHeader[0] )
	{
		int r, g, b;
		DrawUtils::UnpackRGB( r, g, b, RGB_YELLOWISH );
		DrawUtils::DrawHudString( ScreenWidth / 4, ScreenHeight / 2, ScreenWidth, (char*)m_szLocalizedHeader, r, g, b );

		DrawUtils::DrawRectangle( ScreenWidth/ 4, ScreenHeight / 2 + gHUD.GetCharHeight(), ScreenWidth/2, ScreenHeight/30 );
		FillRGBA( ScreenWidth/4+2, ScreenHeight/2 + gHUD.GetCharHeight() + 2, m_fPercent * (ScreenWidth/2-4), ScreenHeight/30-4, 255, 140, 0, 255 );
		return 1;
	}

	// prevent SIGFPE
	if( m_iDuration != 0.0f )
	{
		m_fPercent = ((flTime - m_fStartTime) / m_iDuration);
	}
	else
	{
		m_fPercent = 0.0f;
		m_iFlags = 0;
		return 1;
	}

	DrawUtils::DrawRectangle( ScreenWidth/4, ScreenHeight*2/3, ScreenWidth/2, 10 );
	FillRGBA( ScreenWidth/4+2, ScreenHeight*2/3+2, m_fPercent * (ScreenWidth/2-4), 6, 255, 140, 0, 255 );

	return 1;
}

int CHudProgressBar::MsgFunc_BarTime(const char *pszName, int iSize, void *pbuf)
{
	BufferReader reader( pszName, pbuf, iSize );

	m_iDuration = reader.ReadShort();
	m_fPercent = 0.0f;

	m_fStartTime = gHUD.m_flTime;

	m_iFlags = HUD_DRAW;
	return 1;
}

int CHudProgressBar::MsgFunc_BarTime2(const char *pszName, int iSize, void *pbuf)
{
	BufferReader reader( pszName, pbuf, iSize );

	m_iDuration = reader.ReadShort();
	m_fPercent = m_iDuration * (float)reader.ReadShort() / 100.0f;

	m_fStartTime = gHUD.m_flTime;

	m_iFlags = HUD_DRAW;
	return 1;
}

int CHudProgressBar::MsgFunc_BotProgress(const char *pszName, int iSize, void *pbuf)
{
	BufferReader reader( pszName, pbuf, iSize );
	m_iDuration = 0.0f; // don't update our progress bar
	m_iFlags = HUD_DRAW;

	float fNewPercent;
	int flag = reader.ReadByte();
	switch( flag )
	{
	case UPDATE_BOTPROGRESS:
	case CREATE_BOTPROGRESS:
		fNewPercent = (float)reader.ReadByte() / 100.0f;
		// cs behavior:
		// just don't decrease percent values
		if( m_fPercent < fNewPercent )
		{
			m_fPercent = fNewPercent;
		}
		strncpy(m_szHeader, reader.ReadString(), sizeof(m_szHeader));
		if( m_szHeader[0] == '#' )
			m_szLocalizedHeader = Localize(m_szHeader + 1);
		else
			m_szLocalizedHeader = m_szHeader;
		break;
	case REMOVE_BOTPROGRESS:
	default:
		m_fPercent = 0.0f;
		m_szHeader[0] = '\0';
		m_iFlags = 0;
		m_szLocalizedHeader = NULL;
		break;
	}

	return 1;
}
