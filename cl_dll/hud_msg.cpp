/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"

#include "particleman.h"
#include "cl_animating.h"

#include "particle_presets.h"

#include "particle_presets.h"

extern IParticleMan* g_pParticleMan;

extern BEAM* pBeam;
extern BEAM* pBeam2;
extern TEMPENTITY* pFlare; // Vit_amiN


/// USER-DEFINED SERVER MESSAGE HANDLERS

bool CHud::MsgFunc_ResetHUD(const char* pszName, int iSize, void* pbuf)
{
	ASSERT(iSize == 0);

	// clear all hud data
	HUDLIST* pList = m_pHudList;

	while (pList)
	{
		if (pList->p)
			pList->p->Reset();
		pList = pList->pNext;
	}

	//Reset weapon bits.
	m_iWeaponBits = 0ULL;

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	return true;
}

void CAM_ToFirstPerson();

void CHud::MsgFunc_ViewMode(const char* pszName, int iSize, void* pbuf)
{
	CAM_ToFirstPerson();
}

void CHud::MsgFunc_InitHUD(const char* pszName, int iSize, void* pbuf)
{
	// prepare all hud data
	HUDLIST* pList = m_pHudList;

	while (pList)
	{
		if (pList->p)
			pList->p->InitHUDData();
		pList = pList->pNext;
	}

	//Probably not a good place to put this.
	pBeam = pBeam2 = NULL;
	pFlare = NULL; // Vit_amiN: clear egon's beam flare
}


bool CHud::MsgFunc_GameMode(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	//Note: this user message could be updated to include multiple gamemodes, so make sure this checks for game mode 1
	//See CHalfLifeTeamplay::UpdateGameMode
	//TODO: define game mode constants
	m_Teamplay = READ_BYTE() == 1;

	return true;
}


bool CHud::MsgFunc_Damage(const char* pszName, int iSize, void* pbuf)
{
	int armor, blood;
	Vector from;
	int i;
	float count;

	BEGIN_READ(pbuf, iSize);
	armor = READ_BYTE();
	blood = READ_BYTE();

	for (i = 0; i < 3; i++)
		from[i] = READ_COORD();

	count = (blood * 0.5) + (armor * 0.5);

	if (count < 10)
		count = 10;

	// TODO: kick viewangles,  show damage visually

	return true;
}

bool CHud::MsgFunc_Concuss(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iConcussionEffect = READ_BYTE();
	if (0 != m_iConcussionEffect)
	{
		int r, g, b;
		UnpackRGB(r, g, b, RGB_YELLOWISH);
		this->m_StatusIcons.EnableIcon("dmg_concuss", r, g, b);
	}
	else
		this->m_StatusIcons.DisableIcon("dmg_concuss");
	return true;
}

bool CHud::MsgFunc_Weapons(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	const std::uint64_t lowerBits = READ_LONG();
	const std::uint64_t upperBits = READ_LONG();

	m_iWeaponBits = lowerBits | (upperBits << 32ULL);

	return true;
}

extern int EV_SendWeaponAnim(int iAnim, int iBody, int iWeight);
bool CHud::MsgFunc_WAnim(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	const int iAnim = READ_SHORT();
	int iBody = READ_SHORT();
	const int iWeight = READ_SHORT();
	const char* model = READ_STRING();
	int iSeq = -1;

	char modname[64] = {"null"};

	cl_entity_s* view = gEngfuncs.GetViewModel();

	if (view->model && view->model->name)
		strcpy(modname, view->model->name);

	if (model && !stricmp(model, "none"))
	{
		view->model = nullptr;
		view->curstate.frame = view->curstate.framerate = view->curstate.sequence = 0;
	}
	else if (model && stricmp(modname, model))
		view->model = gEngfuncs.CL_LoadModel(model, &view->index);

	EV_SendWeaponAnim(iAnim, view->curstate.body, iWeight);
	return true;
}

bool CHud::MsgFunc_Particles(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int m_iPreset = READ_BYTE();

	switch (m_iPreset)
	{
	case PARTICLE_EXPLOSMOKE:
	{
		Vector org;
		org.x = READ_COORD();
		org.y = READ_COORD();
		org.z = READ_COORD();
		ExplosionSmoke(org);
	}
	break;
	case PARTICLE_EXPLOSION:
	{
		Vector org;
		org.x = READ_COORD();
		org.y = READ_COORD();
		org.z = READ_COORD();
		ExplosionCluster(org);
	}
	break;
	case PARTICLE_BLOODDRIP:
	{
		int index = READ_SHORT();
		int bloodcol = READ_BYTE();
		BloodDroplets(index, bloodcol);
	}
	break;
	}	
	
	return true;
}

bool CHud::MsgFunc_RenderInfo(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	cl_entity_s *view = gEngfuncs.GetViewModel();

	color24 colRenderColor;
	int iBody = READ_SHORT();
	int iSkin = READ_SHORT();
	int iRenderMode = READ_BYTE();
	int iRenderFx = READ_BYTE();
	int iRenderAmt = READ_BYTE();
	colRenderColor.r = (byte)READ_BYTE();
	colRenderColor.g = (byte)READ_BYTE();
	colRenderColor.b = (byte)READ_BYTE();

	if (!view)
		return false;

	view->curstate.body = iBody;
	view->curstate.skin = iSkin;
	view->curstate.rendermode = iRenderMode;
	view->curstate.renderfx = iRenderFx;
	view->curstate.renderamt = iRenderAmt;
	view->curstate.rendercolor = colRenderColor;

	return true;
}