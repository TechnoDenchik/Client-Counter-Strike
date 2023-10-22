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
#include "triangleapi.h"
#include "r_efx.h"
#include "cl_util.h"

#include "draw_util.h"

int CHudSniperScope::Init()
{
	gHUD.AddHudElem(this);
	m_iFlags = HUD_DRAW;
	m_iScopeArc[0] = m_iScopeArc[1] =m_iScopeArc[2] = m_iScopeArc[3]  = 0;
	return 1;
}

int CHudSniperScope::VidInit()
{
	if( g_iXash == 0 )
	{
		ConsolePrint("^3No Xash Found Warning^7: CHudSniperScope is disabled!\n");
		m_iFlags = 0;
		return 0;
	}

	m_iScopeArc[0] = gRenderAPI.GL_LoadTexture("sprites/scope_arc_nw.tga", NULL, 0, TF_NEAREST |TF_NOPICMIP|TF_NOMIPMAP|TF_CLAMP);
	m_iScopeArc[1] = gRenderAPI.GL_LoadTexture("sprites/scope_arc_ne.tga", NULL, 0, TF_NEAREST |TF_NOPICMIP|TF_NOMIPMAP|TF_CLAMP);
	m_iScopeArc[2] = gRenderAPI.GL_LoadTexture("sprites/scope_arc.tga",    NULL, 0, TF_NEAREST |TF_NOPICMIP|TF_NOMIPMAP|TF_CLAMP);
	m_iScopeArc[3] = gRenderAPI.GL_LoadTexture("sprites/scope_arc_sw.tga", NULL, 0, TF_NEAREST |TF_NOPICMIP|TF_NOMIPMAP|TF_CLAMP);

	if( !m_iScopeArc[0] || !m_iScopeArc[1] || !m_iScopeArc[2] || !m_iScopeArc[3] )
	{
		gRenderAPI.Host_Error( "^3Error^7: Cannot load Sniper Scope arcs. Check sprites/scope_arc*.tga files\n" );
	}
	left = (TrueWidth - TrueHeight)/2;
	right = left + TrueHeight;
	centerx = TrueWidth/2;
	centery = TrueHeight/2;
	return 1;
}

int CHudSniperScope::Draw(float flTime)
{
	if(gHUD.m_iFOV > 40)
		return 1;
	gEngfuncs.pTriAPI->RenderMode(kRenderTransColor);
	gEngfuncs.pTriAPI->Brightness(1.0);
	gEngfuncs.pTriAPI->Color4ub(0, 0, 0, 255);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);

	gRenderAPI.GL_SelectTexture(0);

	gRenderAPI.GL_Bind(0, m_iScopeArc[0]);
	DrawUtils::Draw2DQuad(left, 0, centerx, centery);

	gRenderAPI.GL_Bind(0, m_iScopeArc[1]);
	DrawUtils::Draw2DQuad(centerx, 0, right, centery);

	gRenderAPI.GL_Bind(0, m_iScopeArc[2]);
	DrawUtils::Draw2DQuad(centerx, centery, right, TrueHeight);

	gRenderAPI.GL_Bind(0, m_iScopeArc[3]);
	DrawUtils::Draw2DQuad(left, centery, centerx, TrueHeight);

	FillRGBABlend( 0, 0, (ScreenWidth - ScreenHeight) / 2 + 2, ScreenHeight, 0, 0, 0, 255 );
	FillRGBABlend( (ScreenWidth - ScreenHeight) / 2 - 2 + ScreenHeight, 0, (ScreenWidth - ScreenHeight) / 2 + 2, ScreenHeight, 0, 0, 0, 255 );

	FillRGBABlend(0,                  ScreenHeight/2, ScreenWidth/2 - 20, 1,  0, 0, 0, 255);
	FillRGBABlend(ScreenWidth/2 + 20, ScreenHeight/2, ScreenWidth       , 1,  0, 0, 0, 255);

	FillRGBABlend(ScreenWidth/2, 0                  , 1, ScreenHeight/2 - 20, 0, 0, 0, 255);
	FillRGBABlend(ScreenWidth/2, ScreenHeight/2 + 20, 1, ScreenHeight       , 0, 0, 0, 255);

	return 0;
}

void CHudSniperScope::Shutdown()
{
	for( int i = 0; i < 4; i++ )
		gRenderAPI.GL_FreeTexture( m_iScopeArc[i] );
}
