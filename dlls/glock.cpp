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

LINK_ENTITY_TO_CLASS(weapon_glock, CGlock);
LINK_ENTITY_TO_CLASS(weapon_9mmhandgun, CGlock);

void CGlock::Spawn()
{
	pev->classname = MAKE_STRING("weapon_9mmhandgun"); // hack to allow for old names
	Precache();
	m_iId = WEAPON_GLOCK;
	SET_MODEL(ENT(pev), "models/w_9mmhandgun.mdl");

	m_iDefaultAmmo = GLOCK_DEFAULT_GIVE;

	pev->body = 0;

	FallInit(); // get ready to fall down.
}


void CGlock::Precache()
{
	PRECACHE_MODEL("models/v_9mmhandgun.mdl");
	PRECACHE_MODEL("models/w_9mmhandgun.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl"); // brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	PRECACHE_SOUND("weapons/pl_gun1.wav"); //silenced handgun
	PRECACHE_SOUND("weapons/pl_gun2.wav"); //silenced handgun
	PRECACHE_SOUND("weapons/pl_gun3.wav"); //handgun

	m_usFireGlock1 = PRECACHE_EVENT(1, "events/glock1.sc");
	m_usFireGlock2 = PRECACHE_EVENT(1, "events/glock2.sc");
}

bool CGlock::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GLOCK_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK;
	p->iWeight = GLOCK_WEIGHT;

	return true;
}

bool CGlock::Deploy()
{
	// pev->body = 1;
	bool bDeploy = DefaultDeploy("models/v_9mmhandgun.mdl", "models/p_9mmhandgun.mdl", 0, "onehanded");

	if (m_bSilencerOn)
	{
		SetBody(2, 1);
	}
	else
	{
		SetBody(2, 0);
	}

	if (m_pPlayer->HasWeaponBit(WEAPON_SILENCER))
	{
		if (!m_pPlayer->m_bNotFirstDraw[WEAPON_SILENCER] && !m_bSilencerOn)
		{
			m_flSilencerTime = UTIL_WeaponTimeBase() + 0.5;
		}
	}

	return bDeploy;
}

void CGlock::SecondaryAttack()
{
	if (!m_pPlayer->HasWeaponBit(WEAPON_SILENCER))
	{
		return;
	}

	if ((m_pPlayer->m_iBtnAttackBits & IN_ATTACK2) != 0)
		return;

	int iAnim = 0;

	if (pev->body != GetBody(2, 0))
	{
		iAnim = SendWeaponAnim(ACT_SPECIAL_ATTACK1, -1, 2);
		m_flBodySwitchTime = gpGlobals->time + 1.7;
		m_flSilencerTime = gpGlobals->time + 1.8;
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(2.35);
	}
	else
	{
		iAnim = SendWeaponAnim(ACT_SPECIAL_ATTACK1, -1, 1);
		m_flBodySwitchTime = gpGlobals->time + 0.5;
		m_flSilencerTime = gpGlobals->time + 2.95;
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(2.95);
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + GetSeqLength(iAnim);
	m_bSilencerState = true;
	m_pPlayer->m_iBtnAttackBits |= IN_ATTACK2;
}

void CGlock::Holster()
{
	m_fInReload = false; // cancel any reload in progress.

	if (m_pPlayer->m_iFOV != 0)
	{
		ThirdAttack();
	}

	if (m_pPlayer->HasWeaponBit(WEAPON_SILENCER))
	{
		m_pPlayer->m_bNotFirstDraw[WEAPON_SILENCER] = true;
	}

	m_bSilencerState = false;
	m_flSilencerTime = gpGlobals->time;
	m_flBodySwitchTime = gpGlobals->time;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = 0.0f;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(ACT_DISARM);
}

void CGlock::PrimaryAttack()
{
	if ((m_pPlayer->m_iBtnAttackBits & IN_ATTACK) != 0)
		return;
	GlockFire(0.01, 0.15, true);
}

void CGlock::GlockFire(float flSpread, float flCycleTime, bool fUseAutoAim)
{
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay(0.2);
		}

		Reload();
		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags = 0;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	// silenced
	if (m_bSilencerOn)
	{
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	}
	else
	{
		// non-silenced
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	}

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming;

	if (fUseAutoAim)
	{
		vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);
	}
	else
	{
		UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->m_vecRecoil);
		vecAiming = gpGlobals->v_forward;
	}

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireGlock1 : m_usFireGlock2, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, m_pPlayer->m_vecRecoil[0] * 1000, 0, (m_iClip == 0) ? 1 : 0, m_bSilencerOn);

	m_pPlayer->m_iBtnAttackBits |= IN_ATTACK;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(flCycleTime);

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}


void CGlock::Reload()
{
	if (m_bSilencerState)
		return;

	if (m_pPlayer->ammo_9mm <= 0)
	{
		if (LookupActivity(ACT_USE) < 0)
		{
			SendWeaponAnim(ACT_IDLE, -1, 2);
		}
		else
		{
			SendWeaponAnim(ACT_IDLE, -1, 1);
		}
		return;
	}
	bool iResult;

	if (m_iClip == 0)
		iResult = DefaultReload(17, ACT_RELOAD, 1.65, -1, 2);
	else
		iResult = DefaultReload(17, ACT_RELOAD, 1.65, -1, 1);

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	}
	else
	{
		if (LookupActivity(ACT_USE) < 0)
		{
			SendWeaponAnim(ACT_IDLE, -1, 2);
		}
		else
		{
			SendWeaponAnim(ACT_IDLE, -1, 1);
		}
	}
}

void CGlock::ItemPostFrame()
{
	if (m_bSilencerState)
	{
		if (m_flBodySwitchTime < gpGlobals->time)
		{
			if (m_bSilencerOn)
			{
				SetBody(2, 0);
			}
			else
			{
				SetBody(2, 1);
			}
			m_flBodySwitchTime = 0;
		}
		if (m_flSilencerTime < gpGlobals->time)
		{
			m_bSilencerState = false;
			m_bSilencerOn = !m_bSilencerOn;
			m_flSilencerTime = 0;
		}
	}
	else	
	{
		if (m_bSilencerOn)
		{
			SetBody(2, 1);
		}
		else
		{
			SetBody(2, 0);
		}

		if (m_pPlayer->HasWeaponBit(WEAPON_SILENCER))
		{
			if (!m_pPlayer->m_bNotFirstDraw[WEAPON_SILENCER] && !m_bSilencerOn && m_flSilencerTime < UTIL_WeaponTimeBase())
			{
				SecondaryAttack();
				m_pPlayer->m_bNotFirstDraw[WEAPON_SILENCER] = true;
			}
		}
	}

	CBasePlayerWeapon::ItemPostFrame();
}



void CGlock::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim = 0;
		if (m_pPlayer->m_iFOV == 0)
		{
			iAnim = SendWeaponAnim(ACT_IDLE);
			if (iAnim != -1)
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + GetSeqLength(iAnim) * RANDOM_FLOAT(1.2,3.0);
		}
	}
}








class CGlockAmmo : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_9mmclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		if (pOther->GiveAmmo(AMMO_GLOCKCLIP_GIVE, "9mm", _9MM_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(ammo_glockclip, CGlockAmmo);
LINK_ENTITY_TO_CLASS(ammo_9mmclip, CGlockAmmo);
