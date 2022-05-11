// Presets for particles

#include "particle_presets.h"
#include "event_api.h"

#include "decals.h"
#include "ev_hldm.h"

#define DONT_BLEED -1
#define BLOOD_COLOR_RED (byte)247
#define BLOOD_COLOR_YELLOW (byte)195
#define BLOOD_COLOR_GREEN BLOOD_COLOR_YELLOW

//
// This must match the list in util.h
//
DLL_DECALLIST gDecals[] = {
	{"{shot1", 0},	   // DECAL_GUNSHOT1
	{"{shot2", 0},	   // DECAL_GUNSHOT2
	{"{shot3", 0},	   // DECAL_GUNSHOT3
	{"{shot4", 0},	   // DECAL_GUNSHOT4
	{"{shot5", 0},	   // DECAL_GUNSHOT5
	{"{lambda01", 0},  // DECAL_LAMBDA1
	{"{lambda02", 0},  // DECAL_LAMBDA2
	{"{lambda03", 0},  // DECAL_LAMBDA3
	{"{lambda04", 0},  // DECAL_LAMBDA4
	{"{lambda05", 0},  // DECAL_LAMBDA5
	{"{lambda06", 0},  // DECAL_LAMBDA6
	{"{scorch1", 0},   // DECAL_SCORCH1
	{"{scorch2", 0},   // DECAL_SCORCH2
	{"{blood1", 0},	   // DECAL_BLOOD1
	{"{blood2", 0},	   // DECAL_BLOOD2
	{"{blood3", 0},	   // DECAL_BLOOD3
	{"{blood4", 0},	   // DECAL_BLOOD4
	{"{blood5", 0},	   // DECAL_BLOOD5
	{"{blood6", 0},	   // DECAL_BLOOD6
	{"{yblood1", 0},   // DECAL_YBLOOD1
	{"{yblood2", 0},   // DECAL_YBLOOD2
	{"{yblood3", 0},   // DECAL_YBLOOD3
	{"{yblood4", 0},   // DECAL_YBLOOD4
	{"{yblood5", 0},   // DECAL_YBLOOD5
	{"{yblood6", 0},   // DECAL_YBLOOD6
	{"{break1", 0},	   // DECAL_GLASSBREAK1
	{"{break2", 0},	   // DECAL_GLASSBREAK2
	{"{break3", 0},	   // DECAL_GLASSBREAK3
	{"{bigshot1", 0},  // DECAL_BIGSHOT1
	{"{bigshot2", 0},  // DECAL_BIGSHOT2
	{"{bigshot3", 0},  // DECAL_BIGSHOT3
	{"{bigshot4", 0},  // DECAL_BIGSHOT4
	{"{bigshot5", 0},  // DECAL_BIGSHOT5
	{"{spit1", 0},	   // DECAL_SPIT1
	{"{spit2", 0},	   // DECAL_SPIT2
	{"{bproof1", 0},   // DECAL_BPROOF1
	{"{gargstomp", 0}, // DECAL_GARGSTOMP1,	// Gargantua stomp crack
	{"{smscorch1", 0}, // DECAL_SMALLSCORCH1,	// Small scorch mark
	{"{smscorch2", 0}, // DECAL_SMALLSCORCH2,	// Small scorch mark
	{"{smscorch3", 0}, // DECAL_SMALLSCORCH3,	// Small scorch mark
	{"{mommablob", 0}, // DECAL_MOMMABIRTH		// BM Birth spray
	{"{mommablob", 0}, // DECAL_MOMMASPLAT		// BM Mortar spray?? need decal
};

char GetTexType(int idx, pmtrace_t* ptr, float* vecSrc, float* vecEnd);

CBaseParticle* CreateWallpuff(pmtrace_t* pTrace, char* szModelName, float framerate, float speed, float scale, float brightness)
{
	model_s* spr = GetModel(szModelName);
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

CBaseParticle* CreateCollideParticle(pmtrace_t* pTrace, char* szModelName, float speed, Vector vel, float scale, float brightness)
{
	model_s* spr = GetModel(szModelName);
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

CBaseParticle* CreateGunSmoke(const Vector org, char* szModelName, float scale, float brightness)
{
	model_s* spr = GetModel(szModelName);
	CBaseParticle* pParticle = g_pParticleMan->CreateParticle(org, Vector(0,0,0), spr, scale, brightness, "");

	if (pParticle)
	{
		pParticle->m_iRendermode = kRenderTransAdd;
		pParticle->m_iFramerate = 50;
		pParticle->SetLightFlag(LIGHT_INTENSITY);
		pParticle->SetRenderFlag(RENDER_FACEPLAYER_ROTATEZ);
		pParticle->SetCollisionFlags(TRI_ANIMATEDIE);
		pParticle->m_flFadeSpeed = 1.274;
		pParticle->m_flDieTime = gEngfuncs.GetClientTime() + 5.5f;
		pParticle->m_iNumFrames = spr->numframes;

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
		pParticle = CreateCollideParticle(pTrace, spr, 100, Vector(gEngfuncs.pfnRandomFloat(-30, 30), gEngfuncs.pfnRandomFloat(-30, 30), 85), scale, brightness);

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

CBaseParticle* CreateSmoke(Vector origin, char *szName, float scale, float brightness, float life, float framerate)
{
	model_s* spr = GetModel(szName);
	CBaseParticle* pParticle = g_pParticleMan->CreateParticle(origin, Vector(0, 0, 0), spr, scale, brightness, "");

	if (pParticle)
	{
		pParticle->m_iRendermode = kRenderTransColor;
		pParticle->m_vColor = Vector(100,100, 100);
		pParticle->m_flDieTime = gEngfuncs.GetClientTime() + life;
		pParticle->SetLightFlag(LIGHT_INTENSITY);
		pParticle->m_iFramerate = framerate;
		pParticle->m_iFrame = 0;
		pParticle->m_iNumFrames = spr->numframes;
		pParticle->SetCollisionFlags(TRI_ANIMATEDIE);
		pParticle->SetRenderFlag(RENDER_FACEPLAYER);
	}
	return pParticle;
}

void SmokeCallback(CBaseParticle* ent)
{
	pmtrace_t tr = *gEngfuncs.PM_TraceLine(ent->m_vOrigin, ent->m_vOrigin - Vector(0, 0, 4), PM_TRACELINE_ANYVISIBLE, 2, -1);

	model_s* spr = GetModel("sprites/blood.spr");

	if (ent->m_flNextCallback > gEngfuncs.GetClientTime())
		return;

	if (tr.fraction == 1.0)
	{
		CBaseParticle* pPrtcl = CreateGunSmoke(ent->m_vOrigin + tr.plane.normal * 20, "sprites/particles/black_smoke1.spr", gEngfuncs.pfnRandomFloat(60, 80), gEngfuncs.pfnRandomFloat(200, 255));

		pPrtcl->m_iFramerate = 42;
		pPrtcl->SetLightFlag(LIGHT_INTENSITY);
		pPrtcl->m_iRendermode = kRenderTransColor;

		if (ent->m_iuser1 == 1)
			pPrtcl->m_vColor = Vector(255, 0, 0);
		else
			pPrtcl->m_vColor = Vector(255, 255, 255);

		pPrtcl->m_iFrame = 15;

		ent->m_flNextCallback = gEngfuncs.GetClientTime() + 0.05f;
	}
}

void FireCallback(CBaseParticle* ent)
{
	if (ent->m_flNextCallback > gEngfuncs.GetClientTime())
		return;

	if (ent->m_flBrightness <= 1)
	{
		ent->m_flDieTime = 0.1;
		return;
	}

	dlight_t *d = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
	d->origin = ent->m_vOrigin;
	d->color = {255, 255, 0};
	d->radius = gEngfuncs.pfnRandomFloat(30, 50);
	d->die = gEngfuncs.GetClientTime() + 0.1;
	d->decay = 150;

	ent->m_flNextCallback = gEngfuncs.GetClientTime() + 0.1;
}

void FireballTouch(CBaseParticle* ent, pmtrace_s* pTrace)
{
	Vector angles;
	VectorAngles(pTrace->plane.normal, angles);
	if (angles[0] != 90 || (angles.y + angles.z) != 0)
		return;

	model_s* spr = GetModel("sprites/fire.spr");

	CBaseParticle* pParticle = g_pParticleMan->CreateParticle(ent->m_vOrigin + pTrace->plane.normal * 5, pTrace->plane.normal, spr, gEngfuncs.pfnRandomFloat(10, 20), 255, "");

	if (pParticle)
	{
		pParticle->m_iRendermode = kRenderTransAdd;
		pParticle->m_iFramerate = 25;
		pParticle->SetLightFlag(LIGHT_NONE);
		pParticle->m_flFadeSpeed = 4;
		pParticle->SetRenderFlag(RENDER_FACEPLAYER_ROTATEZ);
		pParticle->m_flDieTime = gEngfuncs.GetClientTime() + 5.5f;
		pParticle->m_iNumFrames = spr->numframes;
		pParticle->Callback = FireCallback;
	}

	ent->m_flDieTime = 0.1;
}

void ExplosionCluster(Vector origin)
{
	int rendermode = kRenderTransAlpha;
	Vector color = Vector(255, 255, 255);
	float scale = gEngfuncs.pfnRandomFloat(1, 7);
	float life = 1.5;
	int lflag = LIGHT_INTENSITY;
	int cflag = TRI_COLLIDEALL;
	float brightness = 255;

	pmtrace_t tr = *gEngfuncs.PM_TraceLine(origin, origin - Vector(0, 0, 50), PM_TRACELINE_ANYVISIBLE, 2, -1);

	char mat = GetTexType(-1, &tr, origin, origin - Vector(0, 0, 50));

	for (int i = 0; i < gEngfuncs.pfnRandomLong(4, 26); i++)
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
				gEngfuncs.pEfxAPI->R_SparkShower(tr.endpos);
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
		pParticle = CreateCollideParticle(&tr, spr, 100, Vector(gEngfuncs.pfnRandomFloat(-250, 250), gEngfuncs.pfnRandomFloat(-250, 250), 500), scale, brightness);

		if (pParticle)
		{
			pParticle->m_iRendermode = rendermode;
			pParticle->m_vColor = color;
			pParticle->m_flDieTime = gEngfuncs.GetClientTime() + life;
			pParticle->SetLightFlag(lflag);

			if ((i % 2) == 0)
				pParticle->Callback = SmokeCallback;
			else
				pParticle->m_flDieTime = gEngfuncs.GetClientTime() + life * 50;
	
			pParticle->SetCollisionFlags(cflag);
		}
	}


	for (int i = 0; i < gEngfuncs.pfnRandomLong(4, 6); i++)
	{
		CBaseParticle* pParticle = NULL;

		pParticle = CreateCollideParticle(&tr, "sprites/hotglow.spr", 100, Vector(gEngfuncs.pfnRandomFloat(-250, 250), gEngfuncs.pfnRandomFloat(-250, 250), 500), scale, brightness);

		if (pParticle)
		{
			pParticle->m_iRendermode = kRenderTransAdd;
			pParticle->m_vColor = Vector(255,255,255);
			pParticle->m_flDieTime = gEngfuncs.GetClientTime() + life;
			pParticle->SetLightFlag(LIGHT_NONE);
			pParticle->TouchCallback = FireballTouch;

			pParticle->SetCollisionFlags(cflag);
		}
	}
}

void BloodTouch(CBaseParticle* ent, pmtrace_s *pTrace)
{
	ent->SetCollisionFlags(TRI_COLLIDEKILL);

	physent_t* pe;

	pe = gEngfuncs.pEventAPI->EV_GetPhysent(pTrace->ent);

	if (ent->m_iuser1 == 1)
		EV_HLDM_GunshotDecalTrace(pTrace, (char*)gDecals[DECAL_BLOOD1 + gEngfuncs.pfnRandomLong(0, 5)].name, false);
	else
		EV_HLDM_GunshotDecalTrace(pTrace, (char*)gDecals[DECAL_YBLOOD1 + gEngfuncs.pfnRandomLong(0, 5)].name, false);

	ent->m_flDieTime = 0.1;
}

void BloodDropCallback(CBaseParticle *ent)
{
	cl_entity_s* owner = ent->owner;

	if (!owner || (owner->curstate.effects & EF_NODRAW) != 0 || (owner->curstate.renderamt < 10)) 
	{
		ent->m_flDieTime = 0.1;
		return;
	}

	pmtrace_t tr;
	model_s* spr = GetModel("sprites/blood.spr");

	ent->m_vOrigin = owner->origin;

	gEngfuncs.pEventAPI->EV_PushPMStates();

	gEngfuncs.pEventAPI->EV_SetTraceHull(2);

	gEngfuncs.pEventAPI->EV_PlayerTrace(ent->m_vOrigin, ent->m_vOrigin - Vector(0,0,2), PM_WORLD_ONLY, owner->index, &tr);

	gEngfuncs.pEventAPI->EV_PopPMStates();

	float vel = (ent->m_vOrigin - ent->m_vuser1).Length();

	if (tr.fraction == 1.0 && ent->m_flNextCallback < gEngfuncs.GetClientTime())
	{
		CBaseParticle* pParticle = g_pParticleMan->CreateParticle(ent->m_vOrigin, ent->m_vAngles, spr, gEngfuncs.pfnRandomFloat(8, 15), 255, "");

		if (pParticle)
		{
			pParticle->m_iRendermode = kRenderTransAdd;

			if (ent->m_iuser1 == 1)
				pParticle->m_vColor = Vector(255, 0, 0);
			else
				pParticle->m_vColor = Vector(255, 229, 180);

			pParticle->m_flBrightness = 255;
			pParticle->m_flDieTime = gEngfuncs.GetClientTime() + 100.0f;
			pParticle->SetLightFlag(LIGHT_INTENSITY);
			pParticle->SetCollisionFlags(TRI_COLLIDEALL);
			pParticle->SetRenderFlag(RENDER_FACEPLAYER);
			pParticle->m_vVelocity = Vector(0, 0, -10);
			pParticle->m_flGravity = 1;
			pParticle->TouchCallback = BloodTouch;
			pParticle->m_iuser1 = ent->m_iuser1;
			ent->m_flNextCallback = gEngfuncs.GetClientTime() + gEngfuncs.pfnRandomFloat(0.2, 0.7);
		}
	}
	else if (tr.fraction != 1.0 && vel > 0.5 && ent->m_fuser1 < gEngfuncs.GetClientTime())
	{
		CBaseParticle *pPrtcl = CreateGunSmoke(ent->m_vOrigin + tr.plane.normal * 14, "sprites/particles/smokepuff.spr", gEngfuncs.pfnRandomFloat(35, 50), gEngfuncs.pfnRandomFloat(80, 125));
		pPrtcl->m_iFramerate = 20;
		pPrtcl->m_vVelocity = tr.plane.normal * 12;

		pPrtcl->SetLightFlag(LIGHT_INTENSITY);

		if (ent->m_iuser1 == 1)
			pPrtcl->m_vColor = Vector(255, 0, 0);
		else
			pPrtcl->m_vColor = Vector(255, 229, 180);

		ent->m_fuser1 = gEngfuncs.GetClientTime() + gEngfuncs.pfnRandomFloat(0.05, 0.15);
	}

	ent->m_vuser1 = ent->m_vOrigin;
}

void BloodDroplets(int index, int color)
{
	cl_entity_s *ent = gEngfuncs.GetEntityByIndex(index);

	if (!ent)
		return;

	model_s* spr = GetModel("sprites/blooddrop.spr");

	CBaseParticle* pParticle = g_pParticleMan->CreateParticle(ent->origin, ent->angles, spr, 0.1, 0, "");

	if (pParticle)
	{
		pParticle->m_iRendermode = kRenderTransAdd;
		pParticle->owner = ent;
		pParticle->m_vColor = Vector(0,0,0);
		pParticle->Callback = BloodDropCallback;

		if (color == BLOOD_COLOR_RED)
			pParticle->m_iuser1 = 1;
		else
			pParticle->m_iuser1 = 0;
	}
	
}

void ExplosionSmoke(Vector origin)
{
	CBaseParticle* pSmoke = CreateSmoke(origin, "sprites/steam1.spr", 200, 50, 4, 25);
}