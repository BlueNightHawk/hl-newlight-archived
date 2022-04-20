//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#include <assert.h>
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "dlight.h"
#include "triangleapi.h"

#include <stdio.h>
#include <string.h>
#include <memory.h>

#include "studio_util.h"
#include "r_studioint.h"

#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"
#include "Exports.h"

//
// Override the StudioModelRender virtual member functions here to implement custom bone
// setup, blending, etc.
//

// Global engine <-> studio model rendering code interface
extern engine_studio_api_t IEngineStudio;

// The renderer object, created on the stack.
CGameStudioModelRenderer g_StudioRenderer;
/*
====================
CGameStudioModelRenderer

====================
*/
CGameStudioModelRenderer::CGameStudioModelRenderer()
{
}

////////////////////////////////////
// Hooks to class implementation
////////////////////////////////////

/*
====================
R_StudioDrawPlayer

====================
*/
int R_StudioDrawPlayer(int flags, entity_state_t* pplayer)
{
	return static_cast<int>(g_StudioRenderer.StudioDrawPlayer(flags, pplayer));
}

/*
====================
R_StudioDrawModel

====================
*/
int R_StudioDrawModel(int flags)
{
	return static_cast<int>(g_StudioRenderer.StudioDrawModel(flags));
}

/*
====================
R_StudioInit

====================
*/
void R_StudioInit()
{
	g_StudioRenderer.Init();
}

// The simple drawing interface we'll pass back to the engine
r_studio_interface_t studio =
	{
		STUDIO_INTERFACE_VERSION,
		R_StudioDrawModel,
		R_StudioDrawPlayer,
};

/*
====================
HUD_GetStudioModelInterface

Export this function for the engine to use the studio renderer class to render objects.
====================
*/
int DLLEXPORT HUD_GetStudioModelInterface(int version, struct r_studio_interface_s** ppinterface, struct engine_studio_api_s* pstudio)
{
	//	RecClStudioInterface(version, ppinterface, pstudio);

	if (version != STUDIO_INTERFACE_VERSION)
		return 0;

	// Point the engine to our callbacks
	*ppinterface = &studio;

	// Copy in engine helper functions
	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));

	// Initialize local variables, etc.
	R_StudioInit();

	// Success
	return 1;
}

void StudioLightAtPos(const float* pos, float* color, int& amblight, float* dir)
{
	g_StudioRenderer.StudioLightAtPos(pos, color, amblight, dir);
}

//=========================================================
//
//		Functions ported from server for viewmodel
//					related stuff
//
//=========================================================

int LookupActivityWeight(int activity, int weight)
{
	cl_entity_s* view = gEngfuncs.GetViewModel();

	if (view == nullptr)
		return 0;

	studiohdr_t* pstudiohdr;

	pstudiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(view->model);
	if (!pstudiohdr)
		return 0;

	mstudioseqdesc_t* pseqdesc;

	pseqdesc = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex);

	int seq = -1;
	for (int i = 0; i < pstudiohdr->numseq; i++)
	{
		if (pseqdesc[i].activity == activity)
		{
			if (pseqdesc[i].actweight == weight)
			{
				seq = i;
			}
		}
	}

	return seq;
}

int LookupActivity(int activity)
{
	cl_entity_s* view = gEngfuncs.GetViewModel();

	if (view == nullptr)
		return 0;

	studiohdr_t* pstudiohdr;

	pstudiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(view->model);
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

	if (gHUD.m_flCurTime < gHUD.m_flAnimTime)
	{
		dfdt = 0;
	}
	else
	{
		dfdt = (gHUD.m_flCurTime - gHUD.m_flAnimTime) * e->curstate.framerate * pseqdesc->fps;
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

// ripped from xash
// required for events to play correctly after changlevel animation fix
void HandleAnimEvent(mstudioevent_s* event, cl_entity_s* entity);
void DispatchAnimEvents(float flInterval)
{
	mstudioseqdesc_t* pseqdesc;
	mstudioevent_t* pevent;
	cl_entity_t* e = gEngfuncs.GetViewModel();
	studiohdr_t* pstudiohdr = nullptr;

	if (!e || !e->model)
		return;

	pstudiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(e->model);
	if (!pstudiohdr)
		return;

	int i, sequence;
	float end, start;

	// fill attachments with interpolated origin
	/*
	if (m_pStudioHeader->numattachments <= 0)
	{
		Matrix3x4_OriginFromMatrix(g_studio.rotationmatrix, e->attachment[0]);
		Matrix3x4_OriginFromMatrix(g_studio.rotationmatrix, e->attachment[1]);
		Matrix3x4_OriginFromMatrix(g_studio.rotationmatrix, e->attachment[2]);
		Matrix3x4_OriginFromMatrix(g_studio.rotationmatrix, e->attachment[3]);
	}
	*/
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

		if ((float)pevent[i].frame > start && pevent[i].frame <= end)
			HandleAnimEvent(&pevent[i], e);
	}
}