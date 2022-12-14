// Client side entity management functions

#include <memory.h>

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_types.h"
#include "studio_event.h" // def. of mstudioevent_t
#include "r_efx.h"
#include "event_api.h"
#include "pm_defs.h"
#include "pmtrace.h"
#include "pm_shared.h"
#include "Exports.h"

#include "particleman.h"

// SHADOWS START
#include "PlatformHeaders.h"

#include <gl/GL.h>
#include <gl/GLU.h>
// SHADOWS END

#include "particle_presets.h"
#include "cl_animating.h"

#include "discord_integration.h"

extern IParticleMan* g_pParticleMan;

void Game_AddObjects();

extern Vector v_origin;

bool g_iAlive = true;

#define M4_CLIP 0
#define GL_SHELL 1
#define GLOCK_CLIP 2

void UpdateLaserSpot(int index);

/*
========================
HUD_AddEntity
	Return 0 to filter entity from visible list for rendering
========================
*/
int DLLEXPORT HUD_AddEntity(int type, struct cl_entity_s* ent, const char* modelname)
{
	//	RecClAddEntity(type, ent, modelname);

	switch (type)
	{
	case ET_NORMAL:
		if (ent->curstate.owner == gEngfuncs.GetLocalPlayer()->index)
		{
			if (ent->model && ent->model->type == mod_sprite)
			{
				if (stricmp(ent->model->name, "sprites/laserdot.spr") < 1)
				{
					gHUD.m_flSpotDieTime = gEngfuncs.GetClientTime();
					UpdateLaserSpot(ent->index);
					return 0;
				}
			}
		}
		break;
	case ET_PLAYER:
	{
		if ((ent && ent == gEngfuncs.GetLocalPlayer()))
		{
			if (g_iDrawLegs != 0)
				g_iDrawLegs = 0;

			if ((ent->curstate.effects & EF_DIMLIGHT) != 0 && nlvars.cl_clientflashlight->value != 0)
			{
				ent->curstate.effects &= ~EF_DIMLIGHT;
				gHUD.m_bFlashlight = true;
			}
			else
			{
				gHUD.m_bFlashlight = false;
			}
			if (nlvars.r_drawlegs->value == 0)
				return 0;
		}
	}
	case ET_BEAM:
	case ET_TEMPENTITY:
	case ET_FRAGMENTED:
	default:
		break;
	}
	// each frame every entity passes this function, so the overview hooks it to filter the overview entities
	// in spectator mode:
	// each frame every entity passes this function, so the overview hooks
	// it to filter the overview entities

	if (0 != g_iUser1)
	{
		gHUD.m_Spectator.AddOverviewEntity(type, ent, modelname);

		if ((g_iUser1 == OBS_IN_EYE || gHUD.m_Spectator.m_pip->value == INSET_IN_EYE) &&
			ent->index == g_iUser2)
			return 0; // don't draw the player we are following in eye
	}

	return 1;
}

/*
=========================
HUD_TxferLocalOverrides

The server sends us our origin with extra precision as part of the clientdata structure, not during the normal
playerstate update in entity_state_t.  In order for these overrides to eventually get to the appropriate playerstate
structure, we need to copy them into the state structure at this point.
=========================
*/
void DLLEXPORT HUD_TxferLocalOverrides(struct entity_state_s* state, const struct clientdata_s* client)
{
	//	RecClTxferLocalOverrides(state, client);

	VectorCopy(client->origin, state->origin);

	// Spectator
	state->iuser1 = client->iuser1;
	state->iuser2 = client->iuser2;

	// Duck prevention
	state->iuser3 = client->iuser3;

	// Fire prevention
	state->iuser4 = client->iuser4;
}

/*
=========================
HUD_ProcessPlayerState

We have received entity_state_t for this player over the network.  We need to copy appropriate fields to the
playerstate structure
=========================
*/
void DLLEXPORT HUD_ProcessPlayerState(struct entity_state_s* dst, const struct entity_state_s* src)
{
	//	RecClProcessPlayerState(dst, src);

	// Copy in network data
	VectorCopy(src->origin, dst->origin);
	VectorCopy(src->angles, dst->angles);

	VectorCopy(src->velocity, dst->velocity);

	dst->frame = src->frame;
	dst->modelindex = src->modelindex;
	dst->skin = src->skin;
	dst->effects = src->effects;
	dst->weaponmodel = src->weaponmodel;
	dst->movetype = src->movetype;
	dst->sequence = src->sequence;
	dst->animtime = src->animtime;

	dst->solid = src->solid;

	dst->rendermode = src->rendermode;
	dst->renderamt = src->renderamt;
	dst->rendercolor.r = src->rendercolor.r;
	dst->rendercolor.g = src->rendercolor.g;
	dst->rendercolor.b = src->rendercolor.b;
	dst->renderfx = src->renderfx;

	dst->framerate = src->framerate;
	dst->body = src->body;

	memcpy(&dst->controller[0], &src->controller[0], 4 * sizeof(byte));
	memcpy(&dst->blending[0], &src->blending[0], 2 * sizeof(byte));

	VectorCopy(src->basevelocity, dst->basevelocity);

	dst->friction = src->friction;
	dst->gravity = src->gravity;
	dst->gaitsequence = src->gaitsequence;
	dst->spectator = src->spectator;
	dst->usehull = src->usehull;
	dst->playerclass = src->playerclass;
	dst->team = src->team;
	dst->colormap = src->colormap;


	// Save off some data so other areas of the Client DLL can get to it
	cl_entity_t* player = gEngfuncs.GetLocalPlayer(); // Get the local player's index
	if (dst->number == player->index)
	{
		g_iPlayerClass = dst->playerclass;
		g_iTeamNumber = dst->team;

		if (src->iuser1 != 0)
			discord_integration::set_spectating(true);
		else if (g_iUser1 != 0)
			discord_integration::set_spectating(false);

		g_iUser1 = src->iuser1;
		g_iUser2 = src->iuser2;
		g_iUser3 = src->iuser3;
	}
}

/*
=========================
HUD_TxferPredictionData

Because we can predict an arbitrary number of frames before the server responds with an update, we need to be able to copy client side prediction data in
 from the state that the server ack'd receiving, which can be anywhere along the predicted frame path ( i.e., we could predict 20 frames into the future and the server ack's
 up through 10 of those frames, so we need to copy persistent client-side only state from the 10th predicted frame to the slot the server
 update is occupying.
=========================
*/
void DLLEXPORT HUD_TxferPredictionData(struct entity_state_s* ps, const struct entity_state_s* pps, struct clientdata_s* pcd, const struct clientdata_s* ppcd, struct weapon_data_s* wd, const struct weapon_data_s* pwd)
{
	//	RecClTxferPredictionData(ps, pps, pcd, ppcd, wd, pwd);

	ps->oldbuttons = pps->oldbuttons;
	ps->flFallVelocity = pps->flFallVelocity;
	ps->iStepLeft = pps->iStepLeft;
	ps->playerclass = pps->playerclass;

	pcd->viewmodel = ppcd->viewmodel;
	pcd->m_iId = ppcd->m_iId;
	pcd->ammo_shells = ppcd->ammo_shells;
	pcd->ammo_nails = ppcd->ammo_nails;
	pcd->ammo_cells = ppcd->ammo_cells;
	pcd->ammo_rockets = ppcd->ammo_rockets;
	pcd->m_flNextAttack = ppcd->m_flNextAttack;
	pcd->fov = ppcd->fov;
	pcd->weaponanim = ppcd->weaponanim;
	pcd->tfstate = ppcd->tfstate;
	pcd->maxspeed = ppcd->maxspeed;

	pcd->deadflag = ppcd->deadflag;

	// Spectating or not dead == get control over view angles.
	g_iAlive = (0 != ppcd->iuser1 || (pcd->deadflag == DEAD_NO));

	// Spectator
	pcd->iuser1 = ppcd->iuser1;
	pcd->iuser2 = ppcd->iuser2;

	// Duck prevention
	pcd->iuser3 = ppcd->iuser3;

	if (0 != gEngfuncs.IsSpectateOnly())
	{
		// in specator mode we tell the engine who we want to spectate and how
		// iuser3 is not used for duck prevention (since the spectator can't duck at all)
		pcd->iuser1 = g_iUser1; // observer mode
		pcd->iuser2 = g_iUser2; // first target
		pcd->iuser3 = g_iUser3; // second target
	}

	// Fire prevention
	pcd->iuser4 = ppcd->iuser4;

	pcd->fuser2 = ppcd->fuser2;
	pcd->fuser3 = ppcd->fuser3;

	VectorCopy(ppcd->vuser1, pcd->vuser1);
	VectorCopy(ppcd->vuser2, pcd->vuser2);
	VectorCopy(ppcd->vuser3, pcd->vuser3);
	VectorCopy(ppcd->vuser4, pcd->vuser4);

	memcpy(wd, pwd, MAX_WEAPONS * sizeof(weapon_data_t));
}

#if defined(BEAM_TEST)
// Note can't index beam[ 0 ] in Beam callback, so don't use that index
// Room for 1 beam ( 0 can't be used )
static cl_entity_t beams[2];

void BeamEndModel()
{
	cl_entity_t *player, *model;
	int modelindex;
	struct model_s* mod;

	// Load it up with some bogus data
	player = gEngfuncs.GetLocalPlayer();
	if (!player)
		return;

	mod = gEngfuncs.CL_LoadModel("models/sentry3.mdl", &modelindex);
	if (!mod)
		return;

	// Slot 1
	model = &beams[1];

	*model = *player;
	model->player = 0;
	model->model = mod;
	model->curstate.modelindex = modelindex;

	// Move it out a bit
	model->origin[0] = player->origin[0] - 100;
	model->origin[1] = player->origin[1];

	model->attachment[0] = model->origin;
	model->attachment[1] = model->origin;
	model->attachment[2] = model->origin;
	model->attachment[3] = model->origin;

	gEngfuncs.CL_CreateVisibleEntity(ET_NORMAL, model);
}

void Beams()
{
	static float lasttime;
	float curtime;
	struct model_s* mod;
	int index;

	BeamEndModel();

	curtime = gEngfuncs.GetClientTime();
	float end[3];

	if ((curtime - lasttime) < 10.0)
		return;

	mod = gEngfuncs.CL_LoadModel("sprites/laserbeam.spr", &index);
	if (!mod)
		return;

	lasttime = curtime;

	end[0] = v_origin.x + 100;
	end[1] = v_origin.y + 100;
	end[2] = v_origin.z;

	BEAM* p1;
	p1 = gEngfuncs.pEfxAPI->R_BeamEntPoint(-1, end, index,
		10.0, 2.0, 0.3, 1.0, 5.0, 0.0, 1.0, 1.0, 1.0, 1.0);
}
#endif

/*
=========================
HUD_CreateEntities
	
Gives us a chance to add additional entities to the render this frame
=========================
*/
void DLLEXPORT HUD_CreateEntities()
{
	//	RecClCreateEntities();

#if defined(BEAM_TEST)
	Beams();
#endif

	// buz: clear also stencil buffer
	int hasStencil;
	glGetIntegerv(GL_STENCIL_BITS, &hasStencil);
	if (hasStencil)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); 
		glClearStencil(0);
	}

	// Add in any game specific objects
	Game_AddObjects();

	GetClientVoiceMgr()->CreateEntities();
}

void CL_Muzzleflash(const Vector org)
{
	static dlight_t* dl = nullptr;
	dl = gEngfuncs.pEfxAPI->CL_AllocDlight(30);
	if (dl)
	{
		dl->origin = org;
		dl->color = {249, 207, 87};
		dl->radius = gEngfuncs.pfnRandomFloat(100, 135);
		dl->die = gEngfuncs.GetClientTime() + gHUD.m_flTimeDelta * 20;
		dl->decay = 300;
	}
}
/*
=========================
HUD_StudioEvent

The entity's studio model description indicated an event was
fired during this frame, handle the event by it's tag ( e.g., muzzleflash, sound )
=========================
*/
void DLLEXPORT HUD_StudioEvent(const struct mstudioevent_s* event, const struct cl_entity_s* entity)
{
	if (entity == gEngfuncs.GetViewModel())
		return;

	cl_animutils.StudioEvent(event, entity);
}
/*
=================
CL_UpdateTEnts

Simulation and cleanup of temporary entities
=================
*/
void DLLEXPORT HUD_TempEntUpdate(
	double frametime,			  // Simulation time
	double client_time,			  // Absolute time on client
	double cl_gravity,			  // True gravity on client
	TEMPENTITY** ppTempEntFree,	  // List of freed temporary ents
	TEMPENTITY** ppTempEntActive, // List
	int (*Callback_AddVisibleEntity)(cl_entity_t* pEntity),
	void (*Callback_TempEntPlaySound)(TEMPENTITY* pTemp, float damp))
{
	//	RecClTempEntUpdate(frametime, client_time, cl_gravity, ppTempEntFree, ppTempEntActive, Callback_AddVisibleEntity, Callback_TempEntPlaySound);

	static int gTempEntFrame = 0;
	int i;
	TEMPENTITY *pTemp, *pnext, *pprev;
	float freq, gravity, gravitySlow, life, fastFreq;

	Vector vAngles;

	gEngfuncs.GetViewAngles((float*)vAngles);

	if (g_pParticleMan)
		g_pParticleMan->SetVariables(cl_gravity, vAngles);

	// Nothing to simulate
	if (!*ppTempEntActive)
		return;

	// in order to have tents collide with players, we have to run the player prediction code so
	// that the client has the player list. We run this code once when we detect any COLLIDEALL
	// tent, then set this bool to true so the code doesn't get run again if there's more than
	// one COLLIDEALL ent for this update. (often are).
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(0, 1);

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);

	// !!!BUGBUG	-- This needs to be time based
	gTempEntFrame = (gTempEntFrame + 1) & 31;

	pTemp = *ppTempEntActive;

	// !!! Don't simulate while paused....  This is sort of a hack, revisit.
	if (frametime <= 0)
	{
		while (pTemp)
		{
			if ((pTemp->flags & FTENT_NOMODEL) == 0)
			{
				Callback_AddVisibleEntity(&pTemp->entity);
			}
			pTemp = pTemp->next;
		}
		goto finish;
	}

	pprev = NULL;
	freq = client_time * 0.01;
	fastFreq = client_time * 5.5;
	gravity = -frametime * cl_gravity;
	gravitySlow = gravity * 0.5;

	while (pTemp)
	{
		bool active;

		active = true;

		life = pTemp->die - client_time;
		pnext = pTemp->next;
		if (life < 0)
		{
			if ((pTemp->flags & FTENT_FADEOUT) != 0)
			{
				if (pTemp->entity.curstate.rendermode == kRenderNormal)
					pTemp->entity.curstate.rendermode = kRenderTransTexture;
				pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt * (1 + life * pTemp->fadeSpeed);
				if (pTemp->entity.curstate.renderamt <= 0)
					active = false;
			}
			else
				active = false;
		}
		if (!active) // Kill it
		{
			pTemp->next = *ppTempEntFree;
			*ppTempEntFree = pTemp;
			if (!pprev) // Deleting at head of list
				*ppTempEntActive = pnext;
			else
				pprev->next = pnext;
		}
		else
		{
			pprev = pTemp;

			VectorCopy(pTemp->entity.origin, pTemp->entity.prevstate.origin);

			if ((pTemp->flags & FTENT_SPARKSHOWER) != 0)
			{
				// Adjust speed if it's time
				// Scale is next think time
				if (client_time > pTemp->entity.baseline.scale)
				{
					// Show Sparks
					gEngfuncs.pEfxAPI->R_SparkEffect(pTemp->entity.origin, 8, -200, 200);

					// Reduce life
					pTemp->entity.baseline.framerate -= 0.1;

					if (pTemp->entity.baseline.framerate <= 0.0)
					{
						pTemp->die = client_time;
					}
					else
					{
						// So it will die no matter what
						pTemp->die = client_time + 0.5;

						// Next think
						pTemp->entity.baseline.scale = client_time + 0.1;
					}
				}
			}
			else if ((pTemp->flags & FTENT_PLYRATTACHMENT) != 0)
			{
				cl_entity_t* pClient;

				pClient = gEngfuncs.GetEntityByIndex(pTemp->clientIndex);

				VectorAdd(pClient->origin, pTemp->tentOffset, pTemp->entity.origin);
			}
			else if ((pTemp->flags & FTENT_SINEWAVE) != 0)
			{
				pTemp->x += pTemp->entity.baseline.origin[0] * frametime;
				pTemp->y += pTemp->entity.baseline.origin[1] * frametime;

				pTemp->entity.origin[0] = pTemp->x + sin(pTemp->entity.baseline.origin[2] + client_time * pTemp->entity.prevstate.frame) * (10 * pTemp->entity.curstate.framerate);
				pTemp->entity.origin[1] = pTemp->y + sin(pTemp->entity.baseline.origin[2] + fastFreq + 0.7) * (8 * pTemp->entity.curstate.framerate);
				pTemp->entity.origin[2] += pTemp->entity.baseline.origin[2] * frametime;
			}
			else if ((pTemp->flags & FTENT_SPIRAL) != 0)
			{
				float s, c;
				s = sin(pTemp->entity.baseline.origin[2] + fastFreq);
				c = cos(pTemp->entity.baseline.origin[2] + fastFreq);

				pTemp->entity.origin[0] += pTemp->entity.baseline.origin[0] * frametime + 8 * sin(client_time * 20 + (int)pTemp);
				pTemp->entity.origin[1] += pTemp->entity.baseline.origin[1] * frametime + 4 * sin(client_time * 30 + (int)pTemp);
				pTemp->entity.origin[2] += pTemp->entity.baseline.origin[2] * frametime;
			}

			else
			{
				for (i = 0; i < 3; i++)
					pTemp->entity.origin[i] += pTemp->entity.baseline.origin[i] * frametime;
			}

			if ((pTemp->flags & FTENT_SPRANIMATE) != 0)
			{
				pTemp->entity.curstate.frame += frametime * pTemp->entity.curstate.framerate;
				if (pTemp->entity.curstate.frame >= pTemp->frameMax)
				{
					pTemp->entity.curstate.frame = pTemp->entity.curstate.frame - (int)(pTemp->entity.curstate.frame);

					if ((pTemp->flags & FTENT_SPRANIMATELOOP) == 0)
					{
						// this animating sprite isn't set to loop, so destroy it.
						pTemp->die = client_time;
						pTemp = pnext;
						continue;
					}
				}
			}
			else if ((pTemp->flags & FTENT_SPRCYCLE) != 0)
			{
				pTemp->entity.curstate.frame += frametime * 10;
				if (pTemp->entity.curstate.frame >= pTemp->frameMax)
				{
					pTemp->entity.curstate.frame = pTemp->entity.curstate.frame - (int)(pTemp->entity.curstate.frame);
				}
			}
// Experiment
#if 0
			if ( pTemp->flags & FTENT_SCALE )
				pTemp->entity.curstate.framerate += 20.0 * (frametime / pTemp->entity.curstate.framerate);
#endif

			if ((pTemp->flags & FTENT_ROTATE) != 0)
			{
				pTemp->entity.angles[0] += pTemp->entity.baseline.angles[0] * frametime;
				pTemp->entity.angles[1] += pTemp->entity.baseline.angles[1] * frametime;
				pTemp->entity.angles[2] += pTemp->entity.baseline.angles[2] * frametime;

				VectorCopy(pTemp->entity.angles, pTemp->entity.latched.prevangles);
			}

			if ((pTemp->flags & (FTENT_COLLIDEALL | FTENT_COLLIDEWORLD)) != 0)
			{
				Vector traceNormal;
				float traceFraction = 1;

				if ((pTemp->flags & FTENT_COLLIDEALL) != 0)
				{
					pmtrace_t pmtrace;
					physent_t* pe;

					gEngfuncs.pEventAPI->EV_SetTraceHull(2);

					gEngfuncs.pEventAPI->EV_PlayerTrace(pTemp->entity.prevstate.origin, pTemp->entity.origin, PM_STUDIO_BOX, pTemp->clientIndex, &pmtrace);


					if (pmtrace.fraction != 1)
					{
						pe = gEngfuncs.pEventAPI->EV_GetPhysent(pmtrace.ent);

						if (0 == pmtrace.ent || (pe->info != pTemp->clientIndex))
						{
							traceFraction = pmtrace.fraction;
							VectorCopy(pmtrace.plane.normal, traceNormal);

							if (pTemp->hitcallback)
							{
								(*pTemp->hitcallback)(pTemp, &pmtrace);
							}
						}
					}
				}
				else if ((pTemp->flags & FTENT_COLLIDEWORLD) != 0)
				{
					pmtrace_t pmtrace;

					gEngfuncs.pEventAPI->EV_SetTraceHull(2);

					gEngfuncs.pEventAPI->EV_PlayerTrace(pTemp->entity.prevstate.origin, pTemp->entity.origin, PM_STUDIO_BOX | PM_WORLD_ONLY, pTemp->clientIndex, &pmtrace);

					if (pmtrace.fraction != 1)
					{
						traceFraction = pmtrace.fraction;
						VectorCopy(pmtrace.plane.normal, traceNormal);

						if ((pTemp->flags & FTENT_SPARKSHOWER) != 0)
						{
							// Chop spark speeds a bit more
							//
							VectorScale(pTemp->entity.baseline.origin, 0.6, pTemp->entity.baseline.origin);

							if (Length(pTemp->entity.baseline.origin) < 10)
							{
								pTemp->entity.baseline.framerate = 0.0;
							}
						}

						if (pTemp->hitcallback)
						{
							(*pTemp->hitcallback)(pTemp, &pmtrace);
						}
					}
				}

				if (traceFraction != 1) // Decent collision now, and damping works
				{
					float proj, damp;

					// Place at contact point
					VectorMA(pTemp->entity.prevstate.origin, traceFraction * frametime, pTemp->entity.baseline.origin, pTemp->entity.origin);
					// Damp velocity
					damp = pTemp->bounceFactor;
					if ((pTemp->flags & (FTENT_GRAVITY | FTENT_SLOWGRAVITY)) != 0)
					{
						damp *= 0.5;
						if (traceNormal[2] > 0.9) // Hit floor?
						{
							if (pTemp->entity.baseline.origin[2] <= 0 && pTemp->entity.baseline.origin[2] >= gravity * 3)
							{
								damp = 0; // Stop
								pTemp->flags &= ~(FTENT_ROTATE | FTENT_GRAVITY | FTENT_SLOWGRAVITY | FTENT_COLLIDEWORLD | FTENT_SMOKETRAIL);
								if ((pTemp->flags & FTENT_MODTRANSFORM) == 0)
								{
									pTemp->entity.angles[0] = 0;						
									pTemp->entity.angles[2] = 0;
								}
							}
						}
					}

					if ((pTemp->hitSound) != 0)
					{
						Callback_TempEntPlaySound(pTemp, damp);
					}

					if ((pTemp->flags & FTENT_COLLIDEKILL) != 0)
					{
						// die on impact
						pTemp->flags &= ~FTENT_FADEOUT;
						pTemp->die = client_time;
					}
					else
					{
						// Reflect velocity
						if (damp != 0)
						{
							proj = DotProduct(pTemp->entity.baseline.origin, traceNormal);
							VectorMA(pTemp->entity.baseline.origin, -proj * 2, traceNormal, pTemp->entity.baseline.origin);
							// Reflect rotation (fake)

						//	pTemp->entity.angles[1] = -pTemp->entity.angles[1];
						}

						if (damp != 1)
						{
							pTemp->entity.baseline.origin[2] *= damp;
							pTemp->entity.baseline.origin[0] *= damp * 1.5;
							pTemp->entity.baseline.origin[1] *= damp * 1.5;
							//VectorScale(pTemp->entity.baseline.origin, damp, pTemp->entity.baseline.origin);
							if ((pTemp->flags & FTENT_MODTRANSFORM) > 0)
							{
								for (int i = 0; i < 3; i++)
								{
									if (i == 1)
										continue;
									pTemp->entity.angles[i] = nlutils.lerp(pTemp->entity.angles[i], pTemp->entity.baseline.vuser1[i], frametime * 15.0f);
								}
							}
							else
							{
								VectorScale(pTemp->entity.angles, 0.9, pTemp->entity.angles);
							}
						}	
					}
				}

				if ((pTemp->flags & FTENT_MODTRANSFORM) > 0)
				{
					pmtrace_t pmtrace;

					gEngfuncs.pEventAPI->EV_SetTraceHull(2);

					gEngfuncs.pEventAPI->EV_PlayerTrace(pTemp->entity.origin, pTemp->entity.origin - Vector(0, 0, 2), PM_WORLD_ONLY, gEngfuncs.GetLocalPlayer()->index, &pmtrace);
					if (pmtrace.fraction < 1.0f)
					{
						for (int i = 0; i < 3; i++)
						{
							if (i == 1)
								continue;
							pTemp->entity.angles[i] = nlutils.lerp(pTemp->entity.angles[i], pTemp->entity.baseline.vuser1[i], frametime * 15.0f);
						}					
					}
				}

			}


			if ((pTemp->flags & FTENT_FLICKER) != 0 && gTempEntFrame == pTemp->entity.curstate.effects)
			{
				dlight_t* dl = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
				VectorCopy(pTemp->entity.origin, dl->origin);
				dl->radius = 60;
				dl->color.r = 255;
				dl->color.g = 120;
				dl->color.b = 0;
				dl->die = client_time + 0.01;
			}

			if ((pTemp->flags & FTENT_SMOKETRAIL) != 0)
			{
				gEngfuncs.pEfxAPI->R_RocketTrail(pTemp->entity.prevstate.origin, pTemp->entity.origin, 1);
			}

			if ((pTemp->flags & FTENT_GRAVITY) != 0)
				pTemp->entity.baseline.origin[2] += gravity;
			else if ((pTemp->flags & FTENT_SLOWGRAVITY) != 0)
				pTemp->entity.baseline.origin[2] += gravitySlow;

			if ((pTemp->flags & FTENT_CLIENTCUSTOM) != 0)
			{
				if (pTemp->callback)
				{
					(*pTemp->callback)(pTemp, frametime, client_time);
				}
			}

			// Cull to PVS (not frustum cull, just PVS)
			if ((pTemp->flags & FTENT_NOMODEL) == 0)
			{
				if (0 == Callback_AddVisibleEntity(&pTemp->entity))
				{
					if ((pTemp->flags & FTENT_PERSIST) == 0)
					{
						pTemp->die = client_time;		// If we can't draw it this frame, just dump it.
						pTemp->flags &= ~FTENT_FADEOUT; // Don't fade out, just die
					}
				}
			}
		}
		pTemp = pnext;
	}

finish:
	// Restore state info
	gEngfuncs.pEventAPI->EV_PopPMStates();
}

void MuzzleEvent(const struct cl_entity_s* entity, int i)
{
	CL_Muzzleflash((float*)&entity->attachment[i]);
	CreateGunSmoke((float*)&entity->attachment[i], "sprites/particles/pistol_smoke1.spr", gEngfuncs.pfnRandomFloat(25, 50), gEngfuncs.pfnRandomFloat(16, 32));
	CreateGunSmoke((float*)&entity->attachment[i], "sprites/particles/rifle_smoke1.spr", gEngfuncs.pfnRandomFloat(25, 50), gEngfuncs.pfnRandomFloat(16, 32));
}

void DropTempMags(struct cl_entity_s* entity, int type)
{
	extern ref_params_s g_pparams;
	extern Vector v_angles;
	Vector forward, right, up, dir;
	tempent_s* ptemp = nullptr;
	AngleVectors(v_angles, forward, right, up);

	switch (type)
	{
	default:
	case M4_CLIP:
	{
		dir = (g_viewinfo.bonepos[7] - g_viewinfo.prevbonepos[7]).Normalize();
		ptemp = gEngfuncs.pEfxAPI->R_TempModel(g_viewinfo.actualbonepos[7], dir * 75, g_viewinfo.actualboneangles[7], 25.0f, 1, 0);
		if (ptemp)
		{
			ptemp->entity.baseline.angles[1] = g_viewinfo.actualboneangles[7][1] * 1.3;
			ptemp->entity.baseline.angles[0] = g_viewinfo.actualboneangles[7][2] * 1.3;
			ptemp->entity.baseline.angles[2] = g_viewinfo.actualboneangles[7][0] * 1.3;
			NormalizeAngles(ptemp->entity.angles);
			ptemp->entity.model = nlutils.GetModel("models/v_m4clip.mdl");
			ptemp->flags |= FTENT_MODTRANSFORM | FTENT_ROTATE;
			ptemp->entity.baseline.effects = FTENT_MODTRANSFORM;
			ptemp->entity.baseline.entityType = 7;
			ptemp->entity.baseline.vuser1 = Vector(90, 0, 0);
			ptemp->clientIndex = entity->index;
		}
	}
	break;
	case GL_SHELL:
	{
		extern ref_params_s g_pparams;
		extern Vector v_angles;
		Vector forward, right, up;
		AngleVectors(v_angles, forward, right, up);
		dir = (g_viewinfo.bonepos[10] - g_viewinfo.prevbonepos[10]).Normalize();

		ptemp = gEngfuncs.pEfxAPI->R_TempModel(g_viewinfo.actualbonepos[10], dir * 35, g_viewinfo.actualboneangles[10], 25.0f, 1, 0);
		if (ptemp)
		{
			ptemp->entity.baseline.angles[0] = g_viewinfo.actualboneangles[10][0] * 1.3;
			ptemp->entity.model = nlutils.GetModel("models/v_glshell.mdl");
			ptemp->flags &= ~FTENT_COLLIDEALL;
			ptemp->flags |= FTENT_MODTRANSFORM | FTENT_ROTATE | FTENT_COLLIDEWORLD;
			ptemp->entity.baseline.effects = FTENT_MODTRANSFORM;
			ptemp->entity.baseline.entityType = 10;
			ptemp->entity.baseline.vuser1 = Vector(90, 0, 0);
			ptemp->clientIndex = entity->index;
		}
	}
	break;
	case GLOCK_CLIP:
	{
		extern ref_params_s g_pparams;
		extern Vector v_angles;
		Vector forward, right, up;
		AngleVectors(v_angles, forward, right, up);
		dir = (g_viewinfo.bonepos[46] - g_viewinfo.prevbonepos[46]).Normalize();

		ptemp = gEngfuncs.pEfxAPI->R_TempModel(g_viewinfo.actualbonepos[46], dir, Vector(0, 0, 0), 25.0f, 1, 0);
		if (ptemp)
		{
			ptemp->entity.model = nlutils.GetModel("models/v_glockclip.mdl");
			ptemp->flags |= FTENT_MODTRANSFORM | FTENT_ROTATE;
			ptemp->entity.angles = g_viewinfo.actualboneangles[46];
			ptemp->entity.baseline.effects = FTENT_MODTRANSFORM;
			ptemp->entity.baseline.entityType = 46;
			ptemp->bounceFactor = 0.9;
			ptemp->clientIndex = entity->index;
			ptemp->entity.baseline.vuser1 = Vector(0, 0, 0);
		}
	}
	break;
	}
}

/*
=================
HUD_GetUserEntity

If you specify negative numbers for beam start and end point entities, then
  the engine will call back into this function requesting a pointer to a cl_entity_t 
  object that describes the entity to attach the beam onto.

Indices must start at 1, not zero.
=================
*/
cl_entity_t DLLEXPORT* HUD_GetUserEntity(int index)
{
	//	RecClGetUserEntity(index);

#if defined(BEAM_TEST)
	// None by default, you would return a valic pointer if you create a client side
	//  beam and attach it to a client side entity.
	if (index > 0 && index <= 1)
	{
		return &beams[index];
	}
	else
	{
		return NULL;
	}
#else
	return NULL;
#endif
}

void UpdateFlashlight(ref_params_t* pparams)
{
	//	RecClCalcRefdef(pparams);
	static CBaseParticle* pParticle = nullptr;
	pmtrace_t tr;
	Vector forward;
	Vector vecSrc, vecEnd;
	dlight_t* dl = nullptr;
	int plindex = gEngfuncs.GetLocalPlayer()->index;

	float dist = 100;
	float invdist = 1;
	float color = 255;

	if (gHUD.m_bFlashlight)
	{
		AngleVectors(pparams->viewangles, forward, NULL, NULL);
		//AngleVectors(g_viewinfo.actualboneangles[0], forward, NULL, NULL);
		vecSrc = pparams->vieworg;
		vecEnd = vecSrc + forward * 8192;

		gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_GLASS_IGNORE | PM_STUDIO_BOX, plindex, &tr);

		if (nlvars.cl_fakeprojflashlight->value != 0)
		{
			dist = V_min((tr.endpos - vecSrc).Length() * 0.5, 300);
			invdist = 82 - V_min((tr.endpos - vecSrc).Length() * 0.15, 82);
			color = 255 - V_min((tr.endpos - vecSrc).Length() * 0.15, 255);

			dist = clamp(dist, 32, 200);
			invdist = clamp(invdist, 28, 82);
			color = clamp(color, 128, 255);
		}
		dl = gEngfuncs.pEfxAPI->CL_AllocDlight(plindex);
		if (dl)
		{
			dl->origin = tr.endpos;
			dl->color = {(byte)color, (byte)color, (byte)color};
			dl->radius = clamp(dist * 1.05, 64, 150);
			dl->decay = 512;
			dl->die = pparams->time + pparams->frametime * 20;
		}

		if (nlvars.cl_fakeprojflashlight->value != 0)
		{
			if (!pParticle)
			{
				pParticle = g_pParticleMan->CreateParticle(tr.endpos, tr.plane.normal, nlutils.GetModel("sprites/fl.spr"), dist * 1.01, invdist, "");
			}
			if (pParticle)
			{
				pParticle->m_vOrigin = tr.endpos;
				pParticle->SetLightFlag(LIGHT_NONE);
				pParticle->SetRenderFlag(RENDER_DEPTHRANGE | RENDER_FACEPLAYER_ROTATEZ);
				pParticle->m_vAngles.z = pparams->viewangles[2];
				pParticle->m_iRendermode = kRenderTransAdd;
				pParticle->m_flBrightness = nlutils.lerp(pParticle->m_flBrightness, invdist, pparams->frametime * 15.0f);
				pParticle->m_flSize = nlutils.lerp(pParticle->m_flSize, dist * 1.01, pparams->frametime * 15.0f);
			}
		}
		else
		{
			if (pParticle)
			{
				pParticle->m_flFadeSpeed = 64;
				pParticle = nullptr;
			}
		}
	}
	else
	{
		if (pParticle)
		{
			pParticle->m_flFadeSpeed = 64;
			pParticle = nullptr;
		}
	}
}

void UpdateLaserSpot(int index)
{
	int m_iBeam = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/smoke.spr");
	static BEAM* pBeam = nullptr;

	if (gHUD.m_flSpotDieTime < gEngfuncs.GetClientTime() || !gEngfuncs.GetViewModel()->model ||
		!gEngfuncs.GetViewModel()->model->name || stricmp(gEngfuncs.GetViewModel()->model->name, "models/v_rpg.mdl") > 0)
	{
		if (gHUD.m_pLaserSpot)
		{
			gHUD.m_pLaserSpot->m_flDieTime = 0.01;
			gHUD.m_pLaserSpot = nullptr;
		}
		if (pBeam)
		{
			pBeam->die = 0.0f;
			pBeam = nullptr;
		}
		gHUD.m_flSpotDieTime = 0.0f;
		return;
	}

	extern ref_params_t g_pparams;
	pmtrace_t tr;
	Vector forward;
	Vector angles;
	Vector vecSrc, vecEnd;
	VectorCopy(g_viewinfo.actualboneangles[47], angles);
	dlight_t* dl = nullptr;
	int plindex = gEngfuncs.GetLocalPlayer()->index;

	static int laserindex = index;
	if (index != -1)
		laserindex = index;

	AngleVectors(angles, forward, NULL, NULL);
	vecSrc = gEngfuncs.GetViewModel()->attachment[0];
	vecEnd = vecSrc + forward * 8192;

	gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_NORMAL | PM_GLASS_IGNORE, plindex, &tr);

	if (nlvars.r_dlightfx->value != 0)
		dl = gEngfuncs.pEfxAPI->CL_AllocElight(laserindex + 1);
	if (dl)
	{
		dl->origin = tr.endpos;
		dl->color = {(byte)255, (byte)0, (byte)0};
		dl->radius = 25;
		dl->decay = 512;
		dl->die = g_pparams.time + g_pparams.frametime * 20;
	}

	if (!gHUD.m_pLaserSpot)
	{
		gHUD.m_pLaserSpot = g_pParticleMan->CreateParticle(tr.endpos, tr.plane.normal, nlutils.GetModel("sprites/laserdot.spr"), 15, 255, "laserspot");
	}
	if (gHUD.m_pLaserSpot)
	{
		if (gHUD.m_pLaserSpot->m_flDieTime != 0 && gHUD.m_pLaserSpot->m_flDieTime < gEngfuncs.GetClientTime())
		{
			gHUD.m_pLaserSpot = nullptr;
			return;
		}
		gHUD.m_pLaserSpot->m_vOrigin = tr.endpos;
		gHUD.m_pLaserSpot->SetLightFlag(LIGHT_NONE);
		gHUD.m_pLaserSpot->SetRenderFlag(RENDER_DEPTHRANGE | RENDER_FACEPLAYER_ROTATEZ);
		gHUD.m_pLaserSpot->m_vAngles.z = g_viewinfo.actualboneangles[47][2];
		gHUD.m_pLaserSpot->m_iRendermode = kRenderTransAdd;
		gHUD.m_pLaserSpot->m_flDieTime = gEngfuncs.GetClientTime() + 0.1f;
	}

	gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecSrc + forward * 64, PM_GLASS_IGNORE, plindex, &tr);

	if (!pBeam)
	{
		pBeam = gEngfuncs.pEfxAPI->R_BeamEntPoint(
			gEngfuncs.GetViewModel()->index | 0x1000,
			tr.endpos,
			m_iBeam,
			9999,
			0.25,
			0.0,
			64,
			0,
			0,
			0,
			255,
			0,
			0);
	}
	if (pBeam)
	{
		pBeam->target = tr.endpos;
	}
}