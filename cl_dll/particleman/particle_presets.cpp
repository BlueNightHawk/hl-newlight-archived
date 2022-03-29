// Presets for particles

#include "particle_presets.h"

CBaseParticle* CreateWallpuff(pmtrace_t* pTrace, char* szModelName, float framerate, float speed, float scale, float brightness)
{
	model_s* spr = IEngineStudio.Mod_ForName(szModelName, 1);
	CBaseParticle* pParticle = g_pParticleMan->CreateParticle(pTrace->endpos + pTrace->plane.normal * 5, pTrace->plane.normal, spr, scale, brightness, "");

	if (pParticle)
	{
		pParticle->m_iRendermode = kRenderTransAdd;
		pParticle->m_iFramerate = framerate;
		pParticle->m_iFrame = 0;
		pParticle->m_iNumFrames = spr->numframes;
		pParticle->m_vColor = Vector(255, 255, 255);
		pParticle->SetLightFlag(LIGHT_COLOR);
		pParticle->SetCollisionFlags(TRI_ANIMATEDIE);
		pParticle->SetRenderFlag(RENDER_FACEPLAYER_ROTATEZ);
		pParticle->m_flScaleSpeed = 1.85;
		pParticle->m_vVelocity = pTrace->plane.normal * speed;

		pParticle->SetCullFlag(CULL_PVS);
		return pParticle;
	}
	return NULL;
}

CBaseParticle* CreateCollideParticle(pmtrace_t* pTrace, char* szModelName, float frame, float speed, Vector vel, float scale, float brightness)
{
	model_s* spr = IEngineStudio.Mod_ForName(szModelName, 1);
	CBaseParticle* pParticle = g_pParticleMan->CreateParticle(pTrace->endpos + pTrace->plane.normal * 5, pTrace->plane.normal, spr, scale, brightness, "");

	if (pParticle)
	{
		pParticle->m_iRendermode = kRenderTransAlpha;
	//	pParticle->m_iFrame = frame;
	//	pParticle->m_vColor = Vector(255, 255, 255);
		pParticle->SetLightFlag(LIGHT_INTENSITY);
		pParticle->SetRenderFlag(RENDER_FACEPLAYER_ROTATEZ);
		pParticle->m_vVelocity = pTrace->plane.normal * speed + vel;		
		pParticle->m_flDieTime = gEngfuncs.GetClientTime() + 5.5f;
		pParticle->m_flGravity = 1;
		pParticle->SetCollisionFlags(TRI_COLLIDEALL);

	//	pParticle->SetCullFlag(CULL_PVS);
		return pParticle;
	}
	return NULL;
}