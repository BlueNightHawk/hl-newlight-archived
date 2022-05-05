//=========================================================
//
//				Functions ported from server
//					
//
//=========================================================

#include "hud.h"

#include "com_model.h"
#include "r_studioint.h"
#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"
#include "activity.h"

#include "cl_animating.h"
#include "particle_presets.h"

#include <gl/GL.h>
#include "gl/glext.h"

extern engine_studio_api_s IEngineStudio;

int LookupActivityWeight(cl_entity_s *ent, int activity, int weight)
{
	if (ent == nullptr)
		return 0;
	
	int seqarray[64] = {};
	int numseqs = 0;

	studiohdr_t* pstudiohdr;

	pstudiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(ent->model);
	if (!pstudiohdr)
		return 0;

	mstudioseqdesc_t* pseqdesc;

	pseqdesc = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex);

	int weighttotal = 0;

	pseqdesc = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex);

	int seq = -1;
	for (int i = 0; i < pstudiohdr->numseq; i++)
	{
		if (pseqdesc[i].activity == activity)
		{
			if (pseqdesc[i].actweight == weight)
			{
				weighttotal += pseqdesc[i].actweight;
				if (0 == weighttotal || gEngfuncs.pfnRandomLong(0, weighttotal - 1) < pseqdesc[i].actweight)
					seq = i;
			}
		}
	}

	return seq;
}

int LookupActivity(cl_entity_s *ent, int activity)
{
	if (ent == nullptr)
		return 0;

	studiohdr_t* pstudiohdr;

	pstudiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(ent->model);
	if (!pstudiohdr)
		return 0;

	mstudioseqdesc_t* pseqdesc;

	pseqdesc = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex);

	int weighttotal = 0;
	int seq = -1;
	for (int i = 0; i < pstudiohdr->numseq; i++)
	{
		if (pseqdesc[i].activity == activity)
		{
			weighttotal += pseqdesc[i].actweight;
			if (0 == weighttotal || gEngfuncs.pfnRandomLong(0, weighttotal - 1) < pseqdesc[i].actweight)
				seq = i;
		}
	}

	return seq;
}

float StudioEstimateFrame(cl_entity_s* e, mstudioseqdesc_t* pseqdesc)
{
	double dfdt, f = 0;

	if (gEngfuncs.GetAbsoluteTime() < gHUD.m_flAnimTime)
	{
		dfdt = 0;
	}
	else
	{
		dfdt = (gEngfuncs.GetAbsoluteTime() - gHUD.m_flAnimTime) * e->curstate.framerate * pseqdesc->fps;
		//		gEngfuncs.Con_Printf("%f %f %f\n", (m_clTime - m_pCurrentEntity->curstate.animtime), m_clTime, m_pCurrentEntity->curstate.animtime);
	}

	f += dfdt;

	if ((pseqdesc->flags & STUDIO_LOOPING) != 0)
	{
		if (pseqdesc->numframes > 1)
		{
			f -= (int)(f / (pseqdesc->numframes - 1)) * (pseqdesc->numframes - 1);
		}
		if (f < 0)
		{
			f += (pseqdesc->numframes - 1);
		}
	}
	else
	{
		if (f >= pseqdesc->numframes - 1.001)
		{
			f = pseqdesc->numframes - 1.001;
		}
		if (f < 0.0)
		{
			f = 0.0;
		}
	}

	return f;
}

void StudioEvent(const struct mstudioevent_s* event, const struct cl_entity_s* entity)
{
	int iShouldDrawLegs = (g_iDrawLegs <= 0 && entity == gEngfuncs.GetLocalPlayer()) ? 1 : 0;

	if (iShouldDrawLegs != 0 && !cam_thirdperson)
		return;

	switch (event->event)
	{
	case 5001:
		gEngfuncs.pEfxAPI->R_MuzzleFlash((float*)&entity->attachment[0], atoi(event->options));
		CL_Muzzleflash((float*)&entity->attachment[0]);
		CreateGunSmoke((float*)&entity->attachment[0], "sprites/particles/pistol_smoke1.spr", gEngfuncs.pfnRandomFloat(25, 50), gEngfuncs.pfnRandomFloat(16, 32));
		CreateGunSmoke((float*)&entity->attachment[0], "sprites/particles/rifle_smoke1.spr", gEngfuncs.pfnRandomFloat(25, 50), gEngfuncs.pfnRandomFloat(16, 32));
		break;
	case 5011:
		gEngfuncs.pEfxAPI->R_MuzzleFlash((float*)&entity->attachment[1], atoi(event->options));
		CL_Muzzleflash((float*)&entity->attachment[1]);
		CreateGunSmoke((float*)&entity->attachment[1], "sprites/particles/pistol_smoke1.spr", gEngfuncs.pfnRandomFloat(25, 50), gEngfuncs.pfnRandomFloat(16, 32));
		CreateGunSmoke((float*)&entity->attachment[1], "sprites/particles/rifle_smoke1.spr", gEngfuncs.pfnRandomFloat(25, 50), gEngfuncs.pfnRandomFloat(16, 32));
		break;
	case 5021:
		gEngfuncs.pEfxAPI->R_MuzzleFlash((float*)&entity->attachment[2], atoi(event->options));
		CL_Muzzleflash((float*)&entity->attachment[2]);
		CreateGunSmoke((float*)&entity->attachment[2], "sprites/particles/pistol_smoke1.spr", gEngfuncs.pfnRandomFloat(25, 50), gEngfuncs.pfnRandomFloat(16, 32));
		CreateGunSmoke((float*)&entity->attachment[2], "sprites/particles/rifle_smoke1.spr", gEngfuncs.pfnRandomFloat(25, 50), gEngfuncs.pfnRandomFloat(16, 32));
		break;
	case 5031:
		gEngfuncs.pEfxAPI->R_MuzzleFlash((float*)&entity->attachment[3], atoi(event->options));
		CL_Muzzleflash((float*)&entity->attachment[3]);
		CreateGunSmoke((float*)&entity->attachment[3], "sprites/particles/pistol_smoke1.spr", gEngfuncs.pfnRandomFloat(25, 50), gEngfuncs.pfnRandomFloat(16, 32));
		CreateGunSmoke((float*)&entity->attachment[3], "sprites/particles/rifle_smoke1.spr", gEngfuncs.pfnRandomFloat(25, 50), gEngfuncs.pfnRandomFloat(16, 32));
		break;
	case 5002:
		gEngfuncs.pEfxAPI->R_SparkEffect((float*)&entity->attachment[0], atoi(event->options), -100, 100);
		break;
	// Client side sound
	case 6001:
	{	
		extern ref_params_s g_pparams;
		extern Vector v_angles;
		Vector forward, right, up;
		AngleVectors(v_angles, forward, right, up);
		tempent_s* ptemp = gEngfuncs.pEfxAPI->R_TempModel(g_viewinfo.actualbonepos[7], Vector(g_pparams.simvel) + (right * -80), g_viewinfo.actualboneangles[7], 25.0f, 1, 0);
		if (ptemp)
		{
			ptemp->entity.baseline.angles[1] = g_viewinfo.actualboneangles[7][1] * 1.3;
			ptemp->entity.baseline.angles[0] = g_viewinfo.actualboneangles[7][2] * 1.3;
			ptemp->entity.baseline.angles[2] = g_viewinfo.actualboneangles[7][0] * 1.3;
			NormalizeAngles(ptemp->entity.angles);
			ptemp->entity.model = IEngineStudio.Mod_ForName("models/v_m4clip.mdl", 0);
			//ptemp->flags &= ~FTENT_GRAVITY;
			ptemp->flags |= FTENT_MODTRANSFORM | FTENT_ROTATE;
			ptemp->entity.baseline.effects = FTENT_MODTRANSFORM; 
			ptemp->entity.baseline.entityType = 7;
			ptemp->entity.baseline.vuser1 = Vector(90, 0, 0);
			gEngfuncs.GetViewModel()->curstate.body = 999;
		}	
	}
		break;
	case 6003:
	{
		extern ref_params_s g_pparams;
		extern Vector v_angles;
		Vector forward, right, up;
		AngleVectors(v_angles, forward, right, up);
		tempent_s* ptemp = gEngfuncs.pEfxAPI->R_TempModel(g_viewinfo.actualbonepos[46], Vector(g_pparams.simvel) + (up * -5) + (right * -10), g_viewinfo.actualboneangles[46], 25.0f, 1, 0);
		if (ptemp)
		{
			ptemp->entity.model = IEngineStudio.Mod_ForName("models/v_glockclip.mdl", 0);
			ptemp->flags |= FTENT_MODTRANSFORM | FTENT_ROTATE;
			ptemp->entity.baseline.effects = FTENT_MODTRANSFORM;
			ptemp->entity.baseline.entityType = 46;
			ptemp->entity.baseline.vuser1 = Vector(0, 0, 0);
			gEngfuncs.GetViewModel()->curstate.body = 1;
		}
	}
	break;
	case 6002:
		gEngfuncs.GetViewModel()->curstate.body = 0;
		break;
	case 5004:
		gEngfuncs.pfnPlaySoundByNameAtLocation((char*)event->options, 1.0, (float*)&entity->attachment[0]);
		break;
	default:
		break;
	}
}

int LookupActivityHeaviest(cl_entity_s* ent, int activity)
{
	studiohdr_t* pstudiohdr;
	if (!ent->model)
		return 0;

	pstudiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(ent->model);
	if (!pstudiohdr)
		return 0;

	mstudioseqdesc_t* pseqdesc;

	pseqdesc = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex);

	int weight = 0;
	int seq = -1;
	for (int i = 0; i < pstudiohdr->numseq; i++)
	{
		if (pseqdesc[i].activity == activity)
		{
			if (pseqdesc[i].actweight > weight)
			{
				weight = pseqdesc[i].actweight;
				seq = i;
			}
		}
	}

	return seq;
}

// ripped from xash
// required for events to play correctly after changlevel animation fix
void DispatchAnimEvents(cl_entity_s* e, float flInterval)
{
	mstudioseqdesc_t* pseqdesc;
	mstudioevent_t* pevent;
	studiohdr_t* pstudiohdr = nullptr;

	if (!e || !e->model)
		return;

	pstudiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(e->model);
	if (!pstudiohdr)
		return;

	int i, sequence;
	float end, start;

	if (pstudiohdr->numattachments < 1)
	{
		for (int i = 0; i < 4; i++)
			e->attachment[i] = e->origin;
	}
	
	if ((e->curstate.effects & EF_MUZZLEFLASH) == 1)
	{
		dlight_t* el = gEngfuncs.pEfxAPI->CL_AllocElight(0);

		e->curstate.effects &= ~EF_MUZZLEFLASH;
		VectorCopy(e->attachment[0], el->origin);
		el->die = gEngfuncs.GetClientTime() + 0.05f;
		el->color.r = 255;
		el->color.g = 192;
		el->color.b = 64;
		el->decay = 320;
		el->radius = 24;
	}

	sequence = clamp(0, e->curstate.sequence, pstudiohdr->numseq - 1);
	pseqdesc = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex) + sequence;

	// no events for this animation
	if (pseqdesc->numevents == 0)
		return;

	end = StudioEstimateFrame(e, pseqdesc);
	start = end - e->curstate.framerate * flInterval * pseqdesc->fps;
	pevent = (mstudioevent_t*)((byte*)pstudiohdr + pseqdesc->eventindex);

	if (e->latched.sequencetime == e->curstate.animtime)
	{
		if ((pseqdesc->flags & STUDIO_LOOPING) == 1)
			start = -0.01f;
	}

	for (i = 0; i < pseqdesc->numevents; i++)
	{
		// ignore all non-client-side events
		if (pevent[i].event < 5000)
			continue;

		bool muzzleflash = (pevent[i].event == 5001 || pevent[i].event == 5011 || pevent[i].event == 6002 || pevent[i].event == 6001 || pevent[i].event == 5021 || pevent[i].event == 5031) ? 1 : 0;

		if (e == gEngfuncs.GetViewModel() && muzzleflash)
			continue;

		if ((float)pevent[i].frame > start && pevent[i].frame <= end)
		{
			StudioEvent(&pevent[i], e);	
		}
	}
}

//
// sprite representation in memory
//
typedef enum
{
	SPR_SINGLE = 0,
	SPR_GROUP,
	SPR_ANGLED
} spriteframetype_t;

typedef struct mspriteframe_s
{
	int width;
	int height;
	float up, down, left, right;
	int gl_texturenum;
} mspriteframe_t;

typedef struct
{
	int numframes;
	float* intervals;
	mspriteframe_t* frames[1];
} mspritegroup_t;

typedef struct
{
	spriteframetype_t type;
	mspriteframe_t* frameptr;
} mspriteframedesc_t;

typedef struct
{
	short type;
	short texFormat;
	int maxwidth;
	int maxheight;
	int numframes;
	int radius;
	int facecull;
	int synctype;
	mspriteframedesc_t frames[1];
} msprite_t;

#define Q_rint(x) ((x) < 0 ? ((int)((x)-0.5f)) : ((int)((x) + 0.5f)))

/*
================
R_GetSpriteFrame

assume pModel is valid
================
*/
mspriteframe_t* R_GetSpriteFrame(const model_t* pModel, int frame, float yaw)
{
	extern Vector v_angles;

	msprite_t* psprite;
	mspritegroup_t* pspritegroup;
	mspriteframe_t* pspriteframe = NULL;
	float *pintervals, fullinterval;
	int i, numframes;
	float targettime;

	float time = gEngfuncs.GetClientTime();

	psprite = (msprite_t *)pModel->cache.data;

	if (frame < 0)
	{
		frame = 0;
	}
	else if (frame >= psprite->numframes)
	{
		if (frame > psprite->numframes)
			gEngfuncs.Con_Printf("R_GetSpriteFrame: no such frame %d (%s)\n", frame, pModel->name);
		frame = psprite->numframes - 1;
	}

	if (psprite->frames[frame].type == SPR_SINGLE)
	{
		pspriteframe = psprite->frames[frame].frameptr;
	}
	else if (psprite->frames[frame].type == SPR_GROUP)
	{
		pspritegroup = (mspritegroup_t*)psprite->frames[frame].frameptr;
		pintervals = pspritegroup->intervals;
		numframes = pspritegroup->numframes;
		fullinterval = pintervals[numframes - 1];

		// when loading in Mod_LoadSpriteGroup, we guaranteed all interval values
		// are positive, so we don't have to worry about division by zero
		targettime = time - ((int)(time / fullinterval)) * fullinterval;

		for (i = 0; i < (numframes - 1); i++)
		{
			if (pintervals[i] > targettime)
				break;
		}
		pspriteframe = pspritegroup->frames[i];
	}
	else if (psprite->frames[frame].type == 2)
	{
		int angleframe = (int)(Q_rint((v_angles[1] - yaw + 45.0f) / 360 * 8) - 4) & 7;

		// e.g. doom-style sprite monsters
		pspritegroup = (mspritegroup_t*)psprite->frames[frame].frameptr;
		pspriteframe = pspritegroup->frames[angleframe];
	}

	return pspriteframe;
}

int R_GetSpriteTexture(const model_t* m_pSpriteModel, int frame)
{
	if (!m_pSpriteModel || m_pSpriteModel->type != mod_sprite || !m_pSpriteModel->cache.data)
		return 0;

	return R_GetSpriteFrame(m_pSpriteModel, frame, 0.0f)->gl_texturenum;
}