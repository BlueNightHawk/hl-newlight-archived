//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "Exports.h"

// buz start

#include "PlatformHeaders.h"
#include <gl/gl.h>
#include <gl/glext.h>

#include "r_studioint.h"
extern engine_studio_api_t IEngineStudio;

#include "com_model.h"
#include "studio.h"

#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"
extern CGameStudioModelRenderer g_StudioRenderer;

#define SURF_PLANEBACK 2
#define SURF_DRAWSKY 4
#define SURF_DRAWSPRITE 8
#define SURF_DRAWTURB 0x10
#define SURF_DRAWTILED 0x20
#define SURF_DRAWBACKGROUND 0x40
#define SURF_UNDERWATER 0x80
#define SURF_DONTWARP 0x100
#define BACKFACE_EPSILON 0.01

// 0-2 are axial planes
#define PLANE_X 0
#define PLANE_Y 1
#define PLANE_Z 2

// buz end

#include "particleman.h"
#include "tri.h"

extern IParticleMan* g_pParticleMan;

typedef struct
{
		short type;
		short texFormat;
		int width;
		int height;
}
msprite_t;

model_s* TRI_pModel;

// buz start

extern void UpdateLaserSpot(int index);

void ClearBuffer(void);
extern bool g_bShadows;

mleaf_t* Mod_PointInLeaf(Vector p, model_t* model) // quake's func
{
	mnode_t* node = model->nodes;
	while (1)
	{
		if (node->contents < 0)
			return (mleaf_t*)node;
		mplane_t* plane = node->plane;
		float d = DotProduct(p, plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return NULL; // never reached
}

model_t* g_pworld;
int g_visframe;
int g_framecount;
myvec3_t g_lightvec;

void RecursiveDrawWorld(mnode_t* node)
{
	if (node->contents == CONTENTS_SOLID)
		return;

	if (node->visframe != g_visframe)
		return;

	if (node->contents < 0)
		return; // faces already marked by engine

	// recurse down the children, Order doesn't matter
	RecursiveDrawWorld(node->children[0]);
	RecursiveDrawWorld(node->children[1]);

	// draw stuff
	int c = node->numsurfaces;
	if (c)
	{
		msurface_t* surf = g_pworld->surfaces + node->firstsurface;

		for (; c; c--, surf++)
		{
			if (surf->visframe != g_framecount)
				continue;

			if (surf->flags & (SURF_DRAWSKY | SURF_DRAWTURB | SURF_UNDERWATER))
				continue;

			// cull from light vector

			float dot;
			mplane_t* plane = surf->plane;

			switch (plane->type)
			{
			case PLANE_X:
				dot = g_lightvec[0];
				break;
			case PLANE_Y:
				dot = g_lightvec[1];
				break;
			case PLANE_Z:
				dot = g_lightvec[2];
				break;
			default:
				dot = DotProduct(g_lightvec, plane->normal);
				break;
			}

			if ((dot > 0) && (surf->flags & SURF_PLANEBACK))
				continue;

			if ((dot < 0) && !(surf->flags & SURF_PLANEBACK))
				continue;

			glpoly_t* p = surf->polys;
			float* v = p->verts[0];

			glBegin(GL_POLYGON);
			for (int i = 0; i < p->numverts; i++, v += VERTEXSIZE)
			{
				glTexCoord2f(v[3], v[4]);
				glVertex3fv(v);
			}
			glEnd();
		}
	}
}

// buz end
/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void DLLEXPORT HUD_DrawNormalTriangles()
{
	//	RecClDrawNormalTriangles();

	static	PFNGLACTIVETEXTUREARBPROC glActiveTextureARB = NULL;

	ClearBuffer(); // Buz

	gHUD.m_Spectator.DrawOverview();

	// Buz Start
	if (g_bShadows && IEngineStudio.IsHardware())
	{
		if (NULL == glActiveTextureARB)
			glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB");

		glPushAttrib(GL_ALL_ATTRIB_BITS);

		// BUzer: workaround half-life's bug, when multitexturing left enabled after
		// rendering brush entities
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);
		glActiveTextureARB(GL_TEXTURE0_ARB);

		glDepthMask(GL_FALSE);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_DST_COLOR, GL_ZERO);
		glColor4f(0.5, 0.5, 0.5, 1);

		glStencilFunc(GL_NOTEQUAL, 0, ~0);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glEnable(GL_STENCIL_TEST);

		// get current visframe number
		g_pworld = gEngfuncs.GetEntityByIndex(0)->model;
		mleaf_t* leaf = Mod_PointInLeaf(g_StudioRenderer.m_vRenderOrigin, g_pworld);
		g_visframe = leaf->visframe;

		// get current frame number
		g_framecount = g_StudioRenderer.m_nFrameCount;

		// get light vector
		g_StudioRenderer.GetShadowVector(g_lightvec);

		// draw world
		RecursiveDrawWorld(g_pworld->nodes);

		glPopAttrib();
	}

	g_bShadows = false;
	// Buz End
	// buz end
}


/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void DLLEXPORT HUD_DrawTransparentTriangles()
{
	//	RecClDrawTransparentTriangles();
	static float oldtime = 0;

	if (g_pParticleMan )
	{
		g_pParticleMan->Update();
//		g_pParticleMan->SetVariables()
	}

	oldtime = gEngfuncs.GetClientTime();
}

void TRI_GetSpriteParms(model_t* pSprite, int* frameWidth, int* frameHeight)
{
	if (!pSprite || pSprite->type != mod_sprite)
		return;

	msprite_t* pSpr = (msprite_t*)pSprite->cache.data;

	*frameWidth = pSpr->width;

	*frameHeight = pSpr->height;
}

void TRI_SprAdjustSize(int* x, int* y, int* w, int* h)
{
	float xscale,
		yscale;

	if (!x && !y && !w && !h)
		return;

	float yfactor = (float)ScreenWidth / (float)ScreenHeight;

	xscale = ((float)ScreenWidth / 1536.0f);
	yscale = ((float)ScreenHeight / 1536.0f) * yfactor;

	if (x)
	{
		bool swap_x = 0;
		if (*x > ScreenWidth / 2)
			swap_x = 1;
		if (swap_x)
			*x = (float)ScreenWidth - *x;
		*x *= xscale;
		if (swap_x)
			*x = (float)ScreenWidth - *x;
	}

	if (y)
	{
		bool swap_y = 0;

		if (*y > ScreenHeight / 2)
			swap_y = 1;
		if (swap_y)
			*y = (float)ScreenHeight - *y;
		*y *= yscale;
		if (swap_y)
			*y = (float)ScreenHeight - *y;
	}

	if (w)
		*w *= xscale;
	if (h)
		*h *= yscale;
}

extern float g_vLag[2];

void TRI_SprDrawStretchPic(model_t* pModel, int frame, float x, float y, float w, float h, float s1, float t1, float s2, float t2)
{
	if (nlvars.cl_hudlag->value)
	{
		x += g_vLag[0];
		y += g_vLag[1];
	}

	gEngfuncs.pTriAPI->SpriteTexture(pModel, frame);

	gEngfuncs.pTriAPI->Begin(TRI_QUADS);

	gEngfuncs.pTriAPI->TexCoord2f(s1, t1);
	gEngfuncs.pTriAPI->Vertex3f(x, y, 0);

	gEngfuncs.pTriAPI->TexCoord2f(s2, t1);
	gEngfuncs.pTriAPI->Vertex3f(x + w, y, 0);

	gEngfuncs.pTriAPI->TexCoord2f(s2, t2);
	gEngfuncs.pTriAPI->Vertex3f(x + w, y + h, 0);

	gEngfuncs.pTriAPI->TexCoord2f(s1, t2);
	gEngfuncs.pTriAPI->Vertex3f(x, y + h, 0);

	gEngfuncs.pTriAPI->End();

}

void TRI_SprDrawGeneric(int frame, int x, int y, Rect* prc)
{
	float s1,
		s2, t1, t2;

	int w,
		h;

	TRI_GetSpriteParms(TRI_pModel, &w, &h);

	if (prc)
	{
		Rect rc = *prc;
		if (rc.left <= 0 || rc.left >= w)
			rc.left = 0;
		if (rc.top <= 0 || rc.top >= h)
			rc.top = 0;
		if (rc.right <= 0 || rc.right > w)
			rc.right = w;
		if (rc.bottom <= 0 || rc.bottom > h)
			rc.bottom = h;

		s1 = (float)rc.left / w;
		t1 = (float)rc.top / h;
		s2 = (float)rc.right / w;
		t2 = (float)rc.bottom / h;
		w = rc.right - rc.left;
		h = rc.bottom - rc.top;
	}
	else
	{
		s1 = t1 = 0.0f;
		s2 = t2 = 1.0f;
	}

	TRI_SprAdjustSize(&x, &y, &w, &h);
	TRI_SprDrawStretchPic(TRI_pModel, frame, x, y, w, h, s1, t1, s2, t2);
}

void TRI_SprDrawAdditive(int frame, int x, int y, Rect* prc)
{
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);

	TRI_SprDrawGeneric(frame, x, y, prc);

	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}

void TRI_SprSet(int spr, int r, int g, int b)
{
	TRI_pModel = (model_s*)gEngfuncs.GetSpritePointer(spr);

	gEngfuncs.pTriAPI->Color4ub(r, g, b, 255);
}

void TRI_FillRGBA(int x, int y, int width, int height, int r, int g, int b, int a)
{
	if (nlvars.cl_hudlag->value)
	{
		x += g_vLag[0];
		y += g_vLag[1];
	}

	TRI_SprAdjustSize(&x, &y, &width, &height);

	gEngfuncs.pfnFillRGBA(x, y, width, height, r, g, b, a);
}