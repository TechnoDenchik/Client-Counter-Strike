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
#include "cl_util.h"
#include "parsemsg.h"
#include "draw_util.h"
#include "triangleapi.h"
#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

DECLARE_COMMAND( m_Radar, ShowRadar )
DECLARE_COMMAND( m_Radar, HideRadar )

DECLARE_MESSAGE( m_Radar, Radar )
DECLARE_MESSAGE( m_Radar, HostageK )
DECLARE_MESSAGE( m_Radar, HostagePos )
DECLARE_MESSAGE( m_Radar, BombDrop )
DECLARE_MESSAGE( m_Radar, BombPickup )


static byte	r_RadarCross[8][8] =
{
{1,1,0,0,0,0,1,1},
{1,1,1,0,0,1,1,1},
{0,1,1,1,1,1,1,0},
{0,0,1,1,1,1,0,0},
{0,0,1,1,1,1,0,0},
{0,1,1,1,1,1,1,0},
{1,1,1,0,0,1,1,1},
{1,1,0,0,0,0,1,1}
};

static byte	r_RadarT[8][8] =
{
{1,1,1,1,1,1,1,1},
{1,1,1,1,1,1,1,1},
{0,0,0,1,1,0,0,0},
{0,0,0,1,1,0,0,0},
{0,0,0,1,1,0,0,0},
{0,0,0,1,1,0,0,0},
{0,0,0,1,1,0,0,0},
{0,0,0,1,1,0,0,0}
};

static byte	r_RadarFlippedT[8][8] =
{
{0,0,0,1,1,0,0,0},
{0,0,0,1,1,0,0,0},
{0,0,0,1,1,0,0,0},
{0,0,0,1,1,0,0,0},
{0,0,0,1,1,0,0,0},
{0,0,0,1,1,0,0,0},
{1,1,1,1,1,1,1,1},
{1,1,1,1,1,1,1,1}
};

#define BLOCK_SIZE_MAX 1024

static byte	data2D[BLOCK_SIZE_MAX*4];	// intermediate texbuffer

int CHudRadar::Init()
{
	HOOK_MESSAGE( Radar );
	HOOK_COMMAND( "drawradar", ShowRadar );
	HOOK_COMMAND( "hideradar", HideRadar );
	HOOK_MESSAGE( HostageK );
	HOOK_MESSAGE( HostagePos );
	HOOK_MESSAGE( BombDrop );
	HOOK_MESSAGE( BombPickup );

	m_iFlags = HUD_DRAW;

	cl_radartype = CVAR_CREATE( "cl_radartype", "0", FCVAR_ARCHIVE );

	bTexturesInitialized = bUseRenderAPI = false;

	gHUD.AddHudElem( this );
	return 1;
}

void CHudRadar::Reset()
{
	// make radar don't draw old players after new map
	for( int i = 0; i < 34; i++ )
	{
		g_PlayerExtraInfo[i].radarflashes = 0;

		if( i <= MAX_HOSTAGES ) g_HostageInfo[i].radarflashes = 0;
	}
}

static void Radar_InitBitmap( int w, int h, byte *buf )
{
	for( int x = 0; x < w; x++ )
	{
		for( int y = 0; y < h; y++ )
		{
			data2D[(y * 8 + x) * 4 + 0] = 255;
			data2D[(y * 8 + x) * 4 + 1] = 255;
			data2D[(y * 8 + x) * 4 + 2] = 255;
			data2D[(y * 8 + x) * 4 + 3] = buf[y*h + x]  * 255;
		}
	}
}

int CHudRadar::InitBuiltinTextures( void )
{
	texFlags_t defFlags = (texFlags_t)(TF_NOMIPMAP | TF_NOPICMIP | TF_NEAREST | TF_CLAMP | TF_HAS_ALPHA);

	if( bTexturesInitialized )
		return 1;

	const struct
	{
		const char	*name;
		byte	*buf;
		int		*texnum;
		int		w, h;
		void	(*init)( int w, int h, byte *buf );
		int	texType;
	}
	textures[] =
	{
	{ "radarT",		   (byte*)r_RadarT,      &hT,		 8, 8, Radar_InitBitmap, TEX_CUSTOM },
	{ "radarcross",    (byte*)r_RadarCross,    &hCross,    8, 8, Radar_InitBitmap, TEX_CUSTOM },
	{ "radarflippedT", (byte*)r_RadarFlippedT, &hFlippedT, 8, 8, Radar_InitBitmap, TEX_CUSTOM }
	};
	size_t	i, num_builtin_textures = sizeof( textures ) / sizeof( textures[0] );

	for( i = 0; i < num_builtin_textures; i++ )
	{
		textures[i].init( textures[i].w, textures[i].h, textures[i].buf );
		*textures[i].texnum = gRenderAPI.GL_CreateTexture( textures[i].name, textures[i].w, textures[i].h, data2D, defFlags );
		if( *textures[i].texnum == 0 )
		{
			for( size_t j = 0; j < i; i++ )
			{
				gRenderAPI.GL_FreeTexture( *textures[i].texnum );
			}
			return 0;
		}

		gRenderAPI.GL_SetTextureType( *textures[i].texnum, textures[i].texType );
	}

	hDot = gRenderAPI.GL_LoadTexture( "*white", NULL, 0, 0 );

	bTexturesInitialized = true;

	return 1;
}

void CHudRadar::Shutdown( void )
{
	// GL_FreeTexture( hDot ); engine inner texture
	if( bTexturesInitialized )
	{
		gRenderAPI.GL_FreeTexture( hT );
		gRenderAPI.GL_FreeTexture( hFlippedT );
		gRenderAPI.GL_FreeTexture( hCross );
	}
}

int CHudRadar::VidInit(void)
{
	bUseRenderAPI = g_iXash && InitBuiltinTextures();

	m_hRadar.SetSpriteByName( "radar" );
	m_hRadarOpaque.SetSpriteByName( "radaropaque" );
	m_hRadarBombTarget[0].SetSpriteByName("radar_a");
	m_hRadarBombTarget[1].SetSpriteByName("radar_b");
	iMaxRadius = (m_hRadar.rect.right - m_hRadar.rect.left) / 2.0f;
	return 1;
}

void CHudRadar::UserCmd_HideRadar()
{
	m_iFlags &= ~HUD_DRAW;
}

void CHudRadar::UserCmd_ShowRadar()
{
	m_iFlags |= HUD_DRAW;
}

int CHudRadar::MsgFunc_Radar(const char *pszName,  int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );

	int index = reader.ReadByte();
	g_PlayerExtraInfo[index].origin.x = reader.ReadCoord();
	g_PlayerExtraInfo[index].origin.y = reader.ReadCoord();
	g_PlayerExtraInfo[index].origin.z = reader.ReadCoord();
	return 1;
}

bool CHudRadar::FlashTime( float flTime, extra_player_info_t *pplayer )
{
	// radar flashing
	if( pplayer->radarflashes )
	{
		if( flTime > pplayer->radarflashtime )
		{
			pplayer->nextflash = !pplayer->nextflash;
			pplayer->radarflashtime += pplayer->radarflashtimedelta;
			pplayer->radarflashes--;
		}
	}
	else
	{
		return true;
	}

	return pplayer->nextflash;
}

bool CHudRadar::HostageFlashTime( float flTime, hostage_info_t *pplayer )
{
	// radar flashing
	if( pplayer->radarflashes )
	{
		if( flTime > pplayer->radarflashtime )
		{
			pplayer->nextflash = !pplayer->nextflash;
			pplayer->radarflashtime += pplayer->radarflashtimedelta;
			pplayer->radarflashes--;
		}
	}
	else
	{
		return false; // non-flashing hostage must be never drawn on radar!
	}

	return pplayer->nextflash;
}

void CHudRadar::DrawZAxis( Vector pos, int r, int g, int b, int a )
{
	const float diff = 128;

	if( pos.z > -diff && pos.z < diff )
	{
		DrawRadarDot( pos.x, pos.y, r, g, b, a );
	}
	else if( pos.z <= -diff )
	{
		// higher than player
		DrawT( pos.x, pos.y, r, g, b, a );
	}
	else
	{
		// lower than player
		DrawFlippedT( pos.x, pos.y, r, g, b, a );
	}
}

int CHudRadar::Draw(float flTime)
{
	if ( (gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH) ||
		 gEngfuncs.IsSpectateOnly() ||
		 !(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT))) ||
		 gHUD.m_fPlayerDead )
		return 1;

	int iTeamNumber = g_PlayerExtraInfo[ gHUD.m_Scoreboard.m_iPlayerNum ].teamnumber;
	int r, g, b;

	if( cl_radartype->value )
	{
		SPR_Set(m_hRadarOpaque.spr, 200, 200, 200);
		SPR_DrawHoles(0, 0, 0, &m_hRadarOpaque.rect);
	}
	else
	{
		SPR_Set( m_hRadar.spr, 25, 75, 25 );
		SPR_DrawAdditive( 0, 0, 0, &m_hRadarOpaque.rect );
	}

	if( bUseRenderAPI )
	{
		gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
		gEngfuncs.pTriAPI->CullFace( TRI_NONE );
		gEngfuncs.pTriAPI->Brightness( 1 );
		gRenderAPI.GL_SelectTexture( 0 );
	}

	for(int i = 0; i < 33; i++)
	{
		// skip local player and dead players
		if( i == gHUD.m_Scoreboard.m_iPlayerNum || g_PlayerExtraInfo[i].dead )
			continue;

		// skip non-teammates
		if( g_PlayerExtraInfo[i].teamnumber != iTeamNumber )
			continue;

		// decide should player draw at this time. For flashing.
		// Always true for non-flashing players
		if( !FlashTime( flTime, &g_PlayerExtraInfo[i]) )
			continue;

		// player with C4 must be red
		if( g_PlayerExtraInfo[i].has_c4 )
		{
			DrawUtils::UnpackRGB( r, g, b, RGB_REDISH );
		}
		else
		{
			// white
			DrawUtils::UnpackRGB( r, g, b, RGB_WHITE );
		}

		// calc radar position
		Vector pos = WorldToRadar(gHUD.m_vecOrigin, g_PlayerExtraInfo[i].origin, gHUD.m_vecAngles);

		DrawZAxis( pos, r, g, b, 255 );
	}

	// Terrorist specific code( C4 Bomb )
	if( g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber == TEAM_TERRORIST )
	{
		if ( !g_PlayerExtraInfo[33].dead &&
			 g_PlayerExtraInfo[33].radarflashes &&
			 FlashTime( flTime, &g_PlayerExtraInfo[33] ))
		{
			Vector pos = WorldToRadar(gHUD.m_vecOrigin, g_PlayerExtraInfo[33].origin, gHUD.m_vecAngles);
			if( g_PlayerExtraInfo[33].playerclass ) // bomb planted
			{
				DrawCross( pos.x, pos.y, 255, 0, 0, 255 );
			}
			else
			{
				DrawZAxis( pos, 255, 0, 0, 255 );
			}
		}
	}
	// Counter-Terrorist specific code( hostages )
	else if( g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber == TEAM_CT )
	{
		// draw hostages for CT
		for( int i = 0; i < MAX_HOSTAGES; i++ )
		{
			if( !HostageFlashTime( flTime, g_HostageInfo + i ) )
			{
				continue;
			}

			Vector pos = WorldToRadar(gHUD.m_vecOrigin, g_HostageInfo[i].origin, gHUD.m_vecAngles);
			if( g_HostageInfo[i].dead )
			{
				DrawZAxis( pos, 255, 0, 0, 255 );
			}
			else
			{
				DrawZAxis( pos, 4, 25, 110, 255 );
			}
		}
	}

	return 0;
}

void CHudRadar::DrawPlayerLocation()
{
	DrawUtils::DrawConsoleString( 30, 30, g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].location );
}

inline void CHudRadar::DrawColoredTexture( int x, int y, int size, byte r, byte g, byte b, byte a, int texHandle )
{
	gRenderAPI.GL_Bind( 0, texHandle );
	gEngfuncs.pTriAPI->Color4ub( r, g, b, a );
	DrawUtils::Draw2DQuad( (iMaxRadius + x - size * 2) * gHUD.m_flScale,
						   (iMaxRadius + y - size * 2) * gHUD.m_flScale,
						   (iMaxRadius + x + size * 2) * gHUD.m_flScale,
						   (iMaxRadius + y + size * 2) * gHUD.m_flScale);
}


void CHudRadar::DrawRadarDot( int x, int y, int r, int g, int b, int a )
{
	const int size = 1;
	if( bUseRenderAPI )
	{
		DrawColoredTexture( x, y, size, r, g, b, a, hDot );
	}
	else
	{
		FillRGBA(iMaxRadius + x - size*2, iMaxRadius + y - size*2, size*4, size*4, r, g, b, a);
	}
}


void CHudRadar::DrawCross( int x, int y, int r, int g, int b, int a )
{
	const int size = 2;
	if( bUseRenderAPI )
	{
		DrawColoredTexture( x, y, size, r, g, b, a, hCross );
	}
	else
	{
		FillRGBA(iMaxRadius + x, iMaxRadius + y, size, size, r, g, b, a);
		FillRGBA(iMaxRadius + x - size, iMaxRadius + y - size, size, size, r, g, b, a);
		FillRGBA(iMaxRadius + x - size, iMaxRadius + y + size, size, size, r, g, b, a);
		FillRGBA(iMaxRadius + x + size, iMaxRadius + y - size, size, size, r, g, b, a);
		FillRGBA(iMaxRadius + x + size, iMaxRadius + y + size, size, size, r, g, b, a);
	}
}

void CHudRadar::DrawT( int x, int y, int r, int g, int b, int a )
{
	const int size = 2;

	if( bUseRenderAPI )
	{
		DrawColoredTexture( x, y, size, r, g, b, a, hT );
	}
	else
	{
		FillRGBA( iMaxRadius + x - size, iMaxRadius + y - size, size * 3, size, r, g, b, a);
		FillRGBA( iMaxRadius + x, iMaxRadius + y, size, size * 2, r, g, b, a);
	}
}

void CHudRadar::DrawFlippedT( int x, int y, int r, int g, int b, int a )
{
	const int size = 2;
	if( bUseRenderAPI )
	{
		DrawColoredTexture( x, y, size, r, g, b, a, hFlippedT );
	}
	else
	{
		FillRGBA( iMaxRadius + x, iMaxRadius + y - size, size, size*2, r, g, b, a);
		FillRGBA( iMaxRadius + x - size, iMaxRadius + y + size, size*3, size, r, g, b, a);
	}
}


Vector CHudRadar::WorldToRadar(const Vector vPlayerOrigin, const Vector vObjectOrigin, const Vector vAngles  )
{
	Vector2D diff = vObjectOrigin.Make2D() - vPlayerOrigin.Make2D();
	const float RADAR_SCALE = 32.0f;

	// Supply epsilon values to avoid divide-by-zero
	if( diff.x == 0 )
		diff.x = 0.00001f;
	if( diff.y == 0 )
		diff.y = 0.00001f;

	float flOffset = DEG2RAD( vAngles.y - RAD2DEG( atan2( diff.y, diff.x ) ) );

	// this magic 32.0f just scales position on radar
	float iRadius = min( diff.Length() / RADAR_SCALE, (float)iMaxRadius );

	// transform origin difference to radar source
	Vector ret( (float)(iRadius * sin(flOffset)),
				(float)(iRadius * -cos(flOffset)),
				(float)(vPlayerOrigin.z - vObjectOrigin.z) );

	return ret;
}

int CHudRadar::MsgFunc_BombDrop(const char *pszName, int iSize, void *pbuf)
{
	BufferReader reader( pszName, pbuf, iSize );

	g_PlayerExtraInfo[33].origin.x = reader.ReadCoord();
	g_PlayerExtraInfo[33].origin.y = reader.ReadCoord();
	g_PlayerExtraInfo[33].origin.z = reader.ReadCoord();

	g_PlayerExtraInfo[33].radarflashes = 99999;
	g_PlayerExtraInfo[33].radarflashtime = gHUD.m_flTime;
	g_PlayerExtraInfo[33].radarflashtimedelta = 0.5f;
	strncpy(g_PlayerExtraInfo[33].teamname, "TERRORIST", MAX_TEAM_NAME);
	g_PlayerExtraInfo[33].dead = false;
	g_PlayerExtraInfo[33].nextflash = true;

	int Flag = reader.ReadByte();
	g_PlayerExtraInfo[33].playerclass = Flag;

	if( Flag ) // bomb planted
	{
		gHUD.m_SpectatorGui.m_bBombPlanted = 0;
		gHUD.m_Timer.m_iFlags = 0;
	}
	return 1;
}

int CHudRadar::MsgFunc_BombPickup(const char *pszName, int iSize, void *pbuf)
{
	g_PlayerExtraInfo[33].radarflashes = false;
	g_PlayerExtraInfo[33].dead = true;

	return 1;
}

int CHudRadar::MsgFunc_HostagePos(const char *pszName, int iSize, void *pbuf)
{

	BufferReader reader( pszName, pbuf, iSize );
	int Flag = reader.ReadByte();
	int idx = reader.ReadByte();
	if( idx <= MAX_HOSTAGES )
	{
		g_HostageInfo[idx].origin.x = reader.ReadCoord();
		g_HostageInfo[idx].origin.y = reader.ReadCoord();
		g_HostageInfo[idx].origin.z = reader.ReadCoord();
		g_HostageInfo[idx].dead = false;

		if( Flag == 1 ) // first message about this hostage, start flashing
		{
			g_HostageInfo[idx].radarflashes = 99999;
			g_HostageInfo[idx].radarflashtime = gHUD.m_flTime;
			g_HostageInfo[idx].radarflashtimedelta = 0.5f;
		}
	}

	return 1;
}

int CHudRadar::MsgFunc_HostageK(const char *pszName, int iSize, void *pbuf)
{
	BufferReader reader( pszName, pbuf, iSize );
	int idx = reader.ReadByte();
	if ( idx <= MAX_HOSTAGES )
	{
		g_HostageInfo[idx].dead = true;
		g_HostageInfo[idx].radarflashtime = gHUD.m_flTime;
		g_HostageInfo[idx].radarflashes = 15;
		g_HostageInfo[idx].radarflashtimedelta = 0.1f;
	}

	return 1;
}
