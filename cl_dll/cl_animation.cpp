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

animutils_s cl_animutils;

int animutils_s::LookupActivityWeight(cl_entity_s *ent, int activity, int weight)
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

int animutils_s::LookupActivity(cl_entity_s* ent, int activity)
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

float animutils_s::StudioEstimateFrame(cl_entity_s* e, mstudioseqdesc_t* pseqdesc)
{
	double dfdt, f = 0;

	if (g_viewinfo.m_flCurTime < g_viewinfo.m_flAnimTime)
	{
		dfdt = 0;
	}
	else
	{
		dfdt = (g_viewinfo.m_flCurTime - g_viewinfo.m_flAnimTime) * e->curstate.framerate * pseqdesc->fps;
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

void animutils_s::SetBodygroup(cl_entity_t* ent, int iGroup, int iValue)
{
	studiohdr_t* pstudiohdr;

	pstudiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(ent->model);
	if (!pstudiohdr)
		return;

	if (iGroup > pstudiohdr->numbodyparts)
		return;

	mstudiobodyparts_t* pbodypart = (mstudiobodyparts_t*)((byte*)pstudiohdr + pstudiohdr->bodypartindex) + iGroup;

	if (iValue >= pbodypart->nummodels)
		return;

	int iCurrent = (ent->curstate.body / pbodypart->base) % pbodypart->nummodels;

	ent->curstate.body = (ent->curstate.body - (iCurrent * pbodypart->base) + (iValue * pbodypart->base));
}

void MuzzleEvent(const struct cl_entity_s* entity, int i)
{
	CL_Muzzleflash((float*)&entity->attachment[i]);
	CreateGunSmoke((float*)&entity->attachment[i], "sprites/particles/pistol_smoke1.spr", gEngfuncs.pfnRandomFloat(25, 50), gEngfuncs.pfnRandomFloat(16, 32));
	CreateGunSmoke((float*)&entity->attachment[i], "sprites/particles/rifle_smoke1.spr", gEngfuncs.pfnRandomFloat(25, 50), gEngfuncs.pfnRandomFloat(16, 32));
}

void animutils_s::StudioEvent(const struct mstudioevent_s* event, const struct cl_entity_s* entity)
{
	int iShouldDrawLegs = (g_iDrawLegs <= 0 && entity == gEngfuncs.GetLocalPlayer()) ? 1 : 0;

	if (iShouldDrawLegs != 0 && !cam_thirdperson)
		return;

	switch (event->event)
	{
	case 5001:
		gEngfuncs.pEfxAPI->R_MuzzleFlash((float*)&entity->attachment[0], atoi(event->options));
		MuzzleEvent(entity, 0);
		break;
	case 5011:
		gEngfuncs.pEfxAPI->R_MuzzleFlash((float*)&entity->attachment[1], atoi(event->options));
		MuzzleEvent(entity, 1);
		break;
	case 5021:
		gEngfuncs.pEfxAPI->R_MuzzleFlash((float*)&entity->attachment[2], atoi(event->options));
		MuzzleEvent(entity, 2);
		break;
	case 5031:
		gEngfuncs.pEfxAPI->R_MuzzleFlash((float*)&entity->attachment[3], atoi(event->options));
		MuzzleEvent(entity, 3);
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
			ptemp->entity.model = GetModel("models/v_m4clip.mdl");
			ptemp->flags |= FTENT_MODTRANSFORM | FTENT_ROTATE;
			ptemp->entity.baseline.effects = FTENT_MODTRANSFORM; 
			ptemp->entity.baseline.entityType = 7;
			ptemp->entity.baseline.vuser1 = Vector(90, 0, 0);
			ptemp->clientIndex = entity->index;
			SetBodygroup(gEngfuncs.GetViewModel(), 2, 1);
		}	
	}
		break;
	case 6003:
	{
		extern ref_params_s g_pparams;
		extern Vector v_angles;
		Vector forward, right, up;
		AngleVectors(v_angles, forward, right, up);
		tempent_s* ptemp = gEngfuncs.pEfxAPI->R_TempModel(g_viewinfo.actualbonepos[46], Vector(g_pparams.simvel) + (right * -10), g_viewinfo.actualboneangles[46], 25.0f, 1, 0);
		if (ptemp)
		{
			ptemp->entity.model = GetModel("models/v_glockclip.mdl");
			ptemp->flags |= FTENT_MODTRANSFORM | FTENT_ROTATE;
			ptemp->entity.baseline.effects = FTENT_MODTRANSFORM;
			ptemp->entity.baseline.entityType = 46;
			ptemp->bounceFactor = 0.9;
			ptemp->clientIndex = entity->index;
			ptemp->entity.baseline.vuser1 = Vector(0, 0, 0);
			gEngfuncs.GetViewModel()->curstate.body = 1;
		}
	}
	break;
	case 6004:
	{
		extern ref_params_s g_pparams;
		extern Vector v_angles;
		gEngfuncs.Con_Printf("a");
		Vector forward, right, up;
		AngleVectors(v_angles, forward, right, up);
		tempent_s* ptemp = gEngfuncs.pEfxAPI->R_TempModel(g_viewinfo.actualbonepos[10], Vector(0,0,0), g_viewinfo.actualboneangles[10], 25.0f, 1, 0);
		if (ptemp)
		{
			ptemp->entity.baseline.angles[0] = g_viewinfo.actualboneangles[10][0] * 1.3;
			ptemp->entity.model = GetModel("models/v_glshell.mdl");
			ptemp->flags |= FTENT_MODTRANSFORM | FTENT_ROTATE;
			ptemp->entity.baseline.effects = FTENT_MODTRANSFORM;
			ptemp->entity.baseline.entityType = 10;
			ptemp->entity.baseline.vuser1 = Vector(90, 0, 0);
			ptemp->clientIndex = entity->index;
			SetBodygroup(gEngfuncs.GetViewModel(), 3, 1);
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

int animutils_s::LookupActivityHeaviest(cl_entity_s* ent, int activity)
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
// required for event sounds to play correctly after changlevel animation fix
void animutils_s::DispatchAnimEvents(cl_entity_s* e, float flInterval)
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

		if (e == gEngfuncs.GetViewModel() && pevent->event != 5004)
			continue;

		if ((float)pevent[i].frame > start && pevent[i].frame <= end)
		{
			StudioEvent(&pevent[i], e);	
		}
	}
}

// fuck you valv
void animutils_s::SendWeaponAnim(int iAnim, int iBody)
{
	g_viewinfo.m_flAnimTime = g_viewinfo.m_flCurTime;
	gEngfuncs.pfnWeaponAnim(iAnim, iBody);
}