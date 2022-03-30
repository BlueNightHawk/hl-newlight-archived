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
		pParticle->SetLightFlag(LIGHT_INTENSITY);
		pParticle->SetRenderFlag(RENDER_FACEPLAYER_ROTATEZ);
		pParticle->m_vVelocity = pTrace->plane.normal * speed + vel;		
		pParticle->m_flDieTime = gEngfuncs.GetClientTime() + 5.5f;
		pParticle->m_flGravity = 1;
		pParticle->SetCollisionFlags(TRI_COLLIDEALL);

		return pParticle;
	}
	return NULL;
}

// Clusters
void WallPuffCluster(pmtrace_t* pTrace, char mat)
{
	int rendermode = kRenderTransAlpha;
	Vector color = Vector(255, 255, 255);
	float scale = gEngfuncs.pfnRandomFloat(1, 7);
	float life = 1.5;
	int lflag = LIGHT_INTENSITY;
	int cflag = TRI_COLLIDEALL; 
	float brightness = 255;

	CreateWallpuff(pTrace, "sprites/particles/pistol_smoke1.spr", 100, 68, 60, 50);

	for (int i = 0; i < gEngfuncs.pfnRandomLong(2, 10); i++)
	{
		CBaseParticle* pParticle = NULL;

		char* spr = NULL;

		switch (mat)
		{
			default:
			case CHAR_TEX_TILE:
			case CHAR_TEX_CONCRETE:
			{
				switch (gEngfuncs.pfnRandomLong(0, 3))
				{
				case 0:
					spr = "sprites/particles/debris_concrete.spr";
					break;
				case 1:
					spr = "sprites/particles/debris_concrete1.spr";
					break;
				case 2:
					spr = "sprites/particles/debris_concrete2.spr";
					break;
				case 3:
					spr = "sprites/particles/debris_concrete3.spr";
					break;
				}
			}
			break;
			case CHAR_TEX_SLOSH:
			case CHAR_TEX_DIRT:
				spr = "sprites/particles/debris_dirt.spr";
			break;
			case CHAR_TEX_GLASS:
				spr = "sprites/particles/debris_glass.spr";				
			break;
			case CHAR_TEX_GRATE:
			case CHAR_TEX_VENT:
			case CHAR_TEX_METAL:
			{
				spr = "sprites/particles/smallspark.spr";
				rendermode = kRenderTransAdd;
				scale = gEngfuncs.pfnRandomFloat(0.3, 0.6);
				lflag = LIGHT_NONE;
				cflag = TRI_COLLIDEKILL;
				if (i == 0)
				{
					gEngfuncs.pEfxAPI->R_SparkShower(pTrace->endpos);
				}
			}
			break;
			case CHAR_TEX_SNOW:
			{
				switch (gEngfuncs.pfnRandomLong(0, 3))
				{
				case 0:
					spr = "sprites/particles/debris_snow.spr";
					break;
				case 1:
					spr = "sprites/particles/debris_snow1.spr";
					break;
				case 2:
					spr = "sprites/particles/debris_snow2.spr";
					break;
				case 3:
					spr = "sprites/particles/debris_snow3.spr";
					break;
				}
			}
			break;
			case CHAR_TEX_WOOD:
			{
				switch (gEngfuncs.pfnRandomLong(0, 3))
				{
				case 0:
					spr = "sprites/particles/debris_wood.spr";
					break;
				case 1:
					spr = "sprites/particles/debris_wood1.spr";
					break;
				case 2:
					spr = "sprites/particles/debris_wood2.spr";
					break;
				case 3:
					spr = "sprites/particles/debris_wood3.spr";
					break;
				}
			}
			break;
		}
		pParticle = CreateCollideParticle(pTrace, spr, 0, 100, Vector(gEngfuncs.pfnRandomFloat(-30, 30), gEngfuncs.pfnRandomFloat(-30, 30), 85), scale, brightness);

		if (pParticle)
		{
			pParticle->m_iRendermode = rendermode;
			pParticle->m_vColor = color;
			pParticle->m_flDieTime = gEngfuncs.GetClientTime() + life;
			pParticle->SetLightFlag(lflag);
			pParticle->SetCollisionFlags(cflag);
		}
	}
}
