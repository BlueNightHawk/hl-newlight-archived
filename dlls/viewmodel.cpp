/***
 *
 *	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"

LINK_ENTITY_TO_CLASS(sv_viewmodel, CViewModel);

void CViewModel::Spawn()
{
	pev->classname = MAKE_STRING("sv_viewmodel"); // hack to allow for old names
	Precache();
	SET_MODEL(ENT(pev), "models/v_hands.mdl");

	pev->effects |= EF_NODRAW;

	SetThink(&CViewModel::UpdateThink);
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CViewModel::Precache()
{
	PRECACHE_MODEL("models/v_hands.mdl");
	PRECACHE_MODEL("models/v_fall.mdl");
}

void CViewModel::UpdateThink()
{
	pev->nextthink = gpGlobals->time - gpGlobals->frametime;

	if (!m_pPlayer)
		return;

	if (stricmp(STRING(pev->model), STRING(m_pPlayer->pev->viewmodel)))
		SET_MODEL(ENT(pev), STRING(m_pPlayer->pev->viewmodel));

	pev->origin = m_pPlayer->GetGunPosition();
	UTIL_SetOrigin(pev, pev->origin);
	pev->angles = INVPITCH(m_pPlayer->pev->v_angle);
}