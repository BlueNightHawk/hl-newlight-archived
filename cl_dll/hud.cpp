/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
//
// hud.cpp
//
// implementation of CHud class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"

#include "demo.h"
#include "demo_api.h"
#include "vgui_ScorePanel.h"

#include "particleman.h"

#include "discord_integration.h"

#include "cl_animating.h"

#include "cl_filesystem.h"

#include "PlatformHeaders.h"
#include <WinUser.h>

extern IParticleMan* g_pParticleMan;

nlvars_t nlvars;

hud_player_info_t g_PlayerInfoList[MAX_PLAYERS_HUD + 1];	// player info from the engine
extra_player_info_t g_PlayerExtraInfo[MAX_PLAYERS_HUD + 1]; // additional player info sent directly to the client dll

class CHLVoiceStatusHelper : public IVoiceStatusHelper
{
public:
	void GetPlayerTextColor(int entindex, int color[3]) override
	{
		color[0] = color[1] = color[2] = 255;

		if (entindex >= 0 && entindex < sizeof(g_PlayerExtraInfo) / sizeof(g_PlayerExtraInfo[0]))
		{
			int iTeam = g_PlayerExtraInfo[entindex].teamnumber;

			if (iTeam < 0)
			{
				iTeam = 0;
			}

			iTeam = iTeam % iNumberOfTeamColors;

			color[0] = iTeamColors[iTeam][0];
			color[1] = iTeamColors[iTeam][1];
			color[2] = iTeamColors[iTeam][2];
		}
	}

	void UpdateCursorState() override
	{
		gViewPort->UpdateCursorState();
	}

	int GetAckIconHeight() override
	{
		return ScreenHeight - gHUD.m_iFontHeight * 3 - 6;
	}

	bool CanShowSpeakerLabels() override
	{
		if (gViewPort && gViewPort->m_pScoreBoard)
			return !gViewPort->m_pScoreBoard->isVisible();
		else
			return false;
	}
};
static CHLVoiceStatusHelper g_VoiceStatusHelper;


extern client_sprite_t* GetSpriteList(client_sprite_t* pList, const char* psz, int iRes, int iCount);

extern cvar_t* sensitivity;
cvar_t* cl_lw = NULL;
cvar_t* cl_rollangle = nullptr;
cvar_t* cl_rollspeed = nullptr;

int Subtitles_SubtClear(const char* pszName, int iSize, void* pbuf);
int g_iRestoreViewent = 0;

void ShutdownInput();

//DECLARE_MESSAGE(m_Logo, Logo)
int __MsgFunc_Logo(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_Logo(pszName, iSize, pbuf));
}

//DECLARE_MESSAGE(m_Logo, Logo)
int __MsgFunc_ResetHUD(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_ResetHUD(pszName, iSize, pbuf));
}

int __MsgFunc_InitHUD(const char* pszName, int iSize, void* pbuf)
{
	gHUD.MsgFunc_InitHUD(pszName, iSize, pbuf);
	return 1;
}

int __MsgFunc_ViewMode(const char* pszName, int iSize, void* pbuf)
{
	gHUD.MsgFunc_ViewMode(pszName, iSize, pbuf);
	return 1;
}

int __MsgFunc_SetFOV(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_SetFOV(pszName, iSize, pbuf));
}

int __MsgFunc_Concuss(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_Concuss(pszName, iSize, pbuf));
}

int __MsgFunc_Weapons(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_Weapons(pszName, iSize, pbuf));
}

int __MsgFunc_GameMode(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_GameMode(pszName, iSize, pbuf));
}

int __MsgFunc_WAnim(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_WAnim(pszName, iSize, pbuf));
}

int __MsgFunc_Particles(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_Particles(pszName, iSize, pbuf));
}

int __MsgFunc_RenderInfo(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(gHUD.MsgFunc_RenderInfo(pszName, iSize, pbuf));
}


// TFFree Command Menu
void __CmdFunc_OpenCommandMenu()
{
	if (gViewPort)
	{
		gViewPort->ShowCommandMenu(gViewPort->m_StandardMenu);
	}
}

// TFC "special" command
void __CmdFunc_InputPlayerSpecial()
{
	if (gViewPort)
	{
		gViewPort->InputPlayerSpecial();
	}
}

void __CmdFunc_CloseCommandMenu()
{
	if (gViewPort)
	{
		gViewPort->InputSignalHideCommandMenu();
	}
}

void __CmdFunc_ForceCloseCommandMenu()
{
	if (gViewPort)
	{
		gViewPort->HideCommandMenu();
	}
}

// TFFree Command Menu Message Handlers
int __MsgFunc_ValClass(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_ValClass(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_TeamNames(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_TeamNames(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_Feign(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_Feign(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_Detpack(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_Detpack(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_VGUIMenu(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_VGUIMenu(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_MOTD(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_MOTD(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_BuildSt(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_BuildSt(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_RandomPC(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_RandomPC(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_ServerName(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_ServerName(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_ScoreInfo(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_ScoreInfo(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_TeamScore(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_TeamScore(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_TeamInfo(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_TeamInfo(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_Spectator(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_Spectator(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_SpecFade(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_SpecFade(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_ResetFade(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_ResetFade(pszName, iSize, pbuf));
	return 0;
}

int __MsgFunc_AllowSpec(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return static_cast<int>(gViewPort->MsgFunc_AllowSpec(pszName, iSize, pbuf));
	return 0;
}

void ResetCvars()
{
#define CVAR_SET (*gEngfuncs.Cvar_SetValue)

	CVAR_SET("nl_hudfade", 1);
	CVAR_SET("nl_toggleisight", 1);
	CVAR_SET("nl_autofov", 1);

	CVAR_SET("cl_rollspeed", 200);
	CVAR_SET("cl_rollangle", 2.0);
	CVAR_SET("cl_bobcycle", 1.05);
	CVAR_SET("cl_bob", 0.0075);
	CVAR_SET("cl_bobup", 0.5);

	CVAR_SET("nl_weaponlag", 2.0);
	CVAR_SET("nl_weaponlagscale", 1);
	CVAR_SET("nl_weaponlagspeed", 7.5);

	CVAR_SET("nl_fwdspeed", 200);
	CVAR_SET("nl_fwdangle", 2);

	CVAR_SET("nl_hudlag", 1);

	CVAR_SET("nl_animbone", 0);
	CVAR_SET("nl_guessanimbone", 0);

	CVAR_SET("nl_sprintanim", 1);
	CVAR_SET("nl_jumpanim", 1);
	CVAR_SET("nl_retractwpn", 1);
	CVAR_SET("nl_ironsight", 1);

	CVAR_SET("nl_r_shadows", 1);
	CVAR_SET("nl_r_shadow_height", 0);
	CVAR_SET("nl_r_shadow_x", 0);
	CVAR_SET("nl_r_shadow_y", 0);
	CVAR_SET("nl_r_shadow_alpha", 0);

	CVAR_SET("nl_glowmodels", 1);
	CVAR_SET("nl_camanims", 1);
	CVAR_SET("nl_dlightfx", 1);
	CVAR_SET("nl_drawlegs", 1);
}

void Precache()
{
	memset(gHUD.m_pModCache, (int)nullptr, 512);

	// Preload frequently used models
	// Might reduce stutters

	for (int i = 0; i < 512; i++)
	{
		if (strlen(gHUD.m_szGlowModels[i]) <= 1)
			break;

		char mdlname[64] = {"models/glowmodels/"};
		char tempstr[64];
		strcpy(tempstr, gHUD.m_szGlowModels[i]);

		strcat(mdlname, tempstr);

		nlutils.GetModel(mdlname);
	}
	nlutils.GetModel("models/v_m4clip.mdl");
	nlutils.GetModel("models/v_glockclip.mdl");
	nlutils.GetModel("models/v_glshell.mdl");

	nlutils.GetModel("sprites/particles/debris_concrete.spr");
	nlutils.GetModel("sprites/particles/debris_concrete1.spr");
	nlutils.GetModel("sprites/particles/debris_concrete2.spr");
	nlutils.GetModel("sprites/particles/debris_concrete3.spr");
	nlutils.GetModel("sprites/particles/debris_dirt.spr");
	nlutils.GetModel("sprites/particles/debris_glass.spr");
	nlutils.GetModel("sprites/particles/smallspark.spr");
	nlutils.GetModel("sprites/particles/debris_snow.spr");
	nlutils.GetModel("sprites/particles/debris_snow1.spr");
	nlutils.GetModel("sprites/particles/debris_snow2.spr");
	nlutils.GetModel("sprites/particles/debris_snow3.spr");
	nlutils.GetModel("sprites/particles/debris_wood.spr");
	nlutils.GetModel("sprites/particles/debris_wood1.spr");
	nlutils.GetModel("sprites/particles/debris_wood2.spr");
	nlutils.GetModel("sprites/particles/debris_wood3.spr");

	nlutils.GetModel("sprites/blood.spr");
	nlutils.GetModel("sprites/fire.spr");
	nlutils.GetModel("sprites/blooddrop.spr");
	nlutils.GetModel("sprites/steam1.spr");
	nlutils.GetModel("sprites/particles/smokepuff.spr");
	nlutils.GetModel("sprites/particles/pistol_smoke1.spr");
	nlutils.GetModel("sprites/hotglow.spr");
	nlutils.GetModel("models/player_legs.mdl");
	nlutils.GetModel("models/player_sci_legs.mdl");
	nlutils.GetModel("models/player_sci.mdl");

	nlutils.GetModel("sprites/fl.spr");
}

void nlvars_s::InitCvars()
{
	cl_toggleisight = CVAR_CREATE("nl_toggleisight", "1", FCVAR_ARCHIVE | FCVAR_USERINFO);

	hud_fade = CVAR_CREATE("nl_hudfade", "1", FCVAR_ARCHIVE);

	print_subtitles = CVAR_CREATE("nl_subtitles", "2", FCVAR_ARCHIVE);
	subtitles_font_scale = CVAR_CREATE("nl_subtitles_font_scale", "1", FCVAR_ARCHIVE);
	subtitles_language = CVAR_CREATE("nl_subtitles_language", "en", FCVAR_ARCHIVE);
	subtitles_log_candidates = CVAR_CREATE("nl_subtitles_log_candidates", "0", FCVAR_ARCHIVE);

	hud_crosshair = CVAR_CREATE("hud_crosshair", "1", FCVAR_ARCHIVE);
	hud_crosshair_speed = CVAR_CREATE("hud_crosshair_speed", "65", FCVAR_ARCHIVE); // speed of returning the crosshair to its original size

	cl_weaponlag = CVAR_CREATE("nl_weaponlag", "2.0", 0);
	cl_weaponlagscale = CVAR_CREATE("nl_weaponlagscale", "1", FCVAR_ARCHIVE);
	cl_weaponlagspeed = CVAR_CREATE("nl_weaponlagspeed", "7.5", FCVAR_ARCHIVE);

	cl_fwdspeed = CVAR_CREATE("nl_fwdspeed", "200", FCVAR_ARCHIVE);
	cl_fwdangle = CVAR_CREATE("nl_fwdangle", "2", FCVAR_ARCHIVE);

	cl_hudlag = CVAR_CREATE("nl_hudlag", "1", FCVAR_ARCHIVE);

	cl_animbone = CVAR_CREATE("nl_animbone", "0", FCVAR_ARCHIVE);
	cl_guessanimbone = CVAR_CREATE("nl_guessanimbone", "1", FCVAR_ARCHIVE);

	cl_sprintanim = CVAR_CREATE("nl_sprintanim", "1", FCVAR_ARCHIVE);
	cl_jumpanim = CVAR_CREATE("nl_jumpanim", "1", FCVAR_ARCHIVE);
	cl_retractwpn = CVAR_CREATE("nl_retractwpn", "1", FCVAR_ARCHIVE);
	cl_ironsight = CVAR_CREATE("nl_ironsight", "1", FCVAR_ARCHIVE);

	// SHADOWS START
	r_shadows = CVAR_CREATE("nl_r_shadows", "1", FCVAR_ARCHIVE);
	r_shadow_height = CVAR_CREATE("nl_r_shadow_height", "0", 0);
	r_shadow_x = CVAR_CREATE("nl_r_shadow_x", "0", 0);
	r_shadow_y = CVAR_CREATE("nl_r_shadow_y", "0", 0);
	r_shadow_alpha = CVAR_CREATE("nl_r_shadow_alpha", "0", FCVAR_ARCHIVE);
	// SHADOWS END

	r_glowmodels = CVAR_CREATE("nl_glowmodels", "1", FCVAR_ARCHIVE);
	r_camanims = CVAR_CREATE("nl_camanims", "1", FCVAR_ARCHIVE);
	r_dlightfx = CVAR_CREATE("nl_dlightfx", "1", FCVAR_ARCHIVE);

	r_drawlegs = CVAR_CREATE("nl_drawlegs", "1", FCVAR_ARCHIVE);

	cl_clientflashlight = CVAR_CREATE("nl_clientflashlight", "1", FCVAR_ARCHIVE);
	cl_fakeprojflashlight = CVAR_CREATE("nl_fakeprojflashlight", "1", FCVAR_ARCHIVE);

	chaptersunlocked = CVAR_CREATE("chaptersunlocked", "3", FCVAR_ARCHIVE);
}

extern bool g_iChapMenuOpen;

void ChapterSelect()
{
	g_iChapMenuOpen = !g_iChapMenuOpen;
}

void CloseChapterSelect()
{
	g_iChapMenuOpen = false;
}


// This is called every time the DLL is loaded
void CHud::Init()
{
	HOOK_MESSAGE(Logo);
	HOOK_MESSAGE(ResetHUD);
	HOOK_MESSAGE(GameMode);
	HOOK_MESSAGE(InitHUD);
	HOOK_MESSAGE(ViewMode);
	HOOK_MESSAGE(SetFOV);
	HOOK_MESSAGE(Concuss);
	HOOK_MESSAGE(Weapons);

	// TFFree CommandMenu
	HOOK_COMMAND("+commandmenu", OpenCommandMenu);
	HOOK_COMMAND("-commandmenu", CloseCommandMenu);
	HOOK_COMMAND("ForceCloseCommandMenu", ForceCloseCommandMenu);
	HOOK_COMMAND("special", InputPlayerSpecial);

	HOOK_MESSAGE(ValClass);
	HOOK_MESSAGE(TeamNames);
	HOOK_MESSAGE(Feign);
	HOOK_MESSAGE(Detpack);
	HOOK_MESSAGE(MOTD);
	HOOK_MESSAGE(BuildSt);
	HOOK_MESSAGE(RandomPC);
	HOOK_MESSAGE(ServerName);
	HOOK_MESSAGE(ScoreInfo);
	HOOK_MESSAGE(TeamScore);
	HOOK_MESSAGE(TeamInfo);

	HOOK_MESSAGE(Spectator);
	HOOK_MESSAGE(AllowSpec);

	HOOK_MESSAGE(SpecFade);
	HOOK_MESSAGE(ResetFade);

	// VGUI Menus
	HOOK_MESSAGE(VGUIMenu);

	HOOK_MESSAGE(WAnim);
	HOOK_MESSAGE(Particles);
	HOOK_MESSAGE(RenderInfo);

	g_viewinfo.m_iPrevSeq = -1;

	CVAR_CREATE("hud_classautokill", "1", FCVAR_ARCHIVE | FCVAR_USERINFO); // controls whether or not to suicide immediately on TF class switch
	CVAR_CREATE("hud_takesshots", "0", FCVAR_ARCHIVE);					   // controls whether or not to automatically take screenshots at the end of a round

	m_iLogo = 0;
	m_iFOV = 0;

	CVAR_CREATE("zoom_sensitivity_ratio", "1.2", 0);
	cl_autowepswitch = CVAR_CREATE("cl_autowepswitch", "1", FCVAR_ARCHIVE | FCVAR_USERINFO);

	default_fov = CVAR_CREATE("default_fov", "90", FCVAR_ARCHIVE);
	nlvars.r_autofov = CVAR_CREATE("nl_autofov", "1", FCVAR_ARCHIVE);
	m_pCvarStealMouse = CVAR_CREATE("hud_capturemouse", "1", FCVAR_ARCHIVE);
	m_pCvarDraw = CVAR_CREATE("hud_draw", "1", FCVAR_ARCHIVE);

	cl_lw = gEngfuncs.pfnGetCvarPointer("cl_lw");
	// Remove the cl_lw cvar
	if (cl_lw)
	{
		gEngfuncs.Cvar_SetValue("cl_lw", 1);
		cl_lw->name = "\0";
		cl_lw->value = 1;
		cl_lw->string = "1";
	}

	cl_rollangle = CVAR_CREATE("cl_rollangle", "2.0", FCVAR_ARCHIVE);
	cl_rollspeed = CVAR_CREATE("cl_rollspeed", "200", FCVAR_ARCHIVE);

	m_pSpriteList = NULL;

	// Clear any old HUD list
	if (m_pHudList)
	{
		HUDLIST* pList;
		while (m_pHudList)
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free(pList);
		}
		m_pHudList = NULL;
	}

	// In case we get messages before the first update -- time will be valid
	m_flTime = 1.0;

	m_Ammo.Init();
	m_Health.Init();
	m_SayText.Init();
	m_Spectator.Init();
	m_Geiger.Init();
	m_Train.Init();
	m_Battery.Init();
	m_Flash.Init();
	m_Message.Init();
	m_StatusBar.Init();
	m_DeathNotice.Init();
	m_AmmoSecondary.Init();
	m_TextMessage.Init();
	m_StatusIcons.Init();
	GetClientVoiceMgr()->Init(&g_VoiceStatusHelper, (vgui::Panel**)&gViewPort);

	m_Menu.Init();

	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo(&m_scrinfo);

	nlvars.InitCvars();
	CacheGlowModels();
	nlfs.StoreChapterNames();
	gEngfuncs.pfnAddCommand("reset_nl_cvars", ResetCvars);
	gEngfuncs.pfnAddCommand("recache_glowmodels", ReCacheGlowModels);
	gEngfuncs.pfnAddCommand("togglechapterselect", ChapterSelect);
	gEngfuncs.pfnAddCommand("closechapterselect", CloseChapterSelect);
	MsgFunc_ResetHUD(0, 0, NULL);
}

// CHud destructor
// cleans up memory allocated for m_rg* arrays
CHud::~CHud()
{
	delete[] m_rghSprites;
	delete[] m_rgrcRects;
	delete[] m_rgszSpriteNames;

	if (m_pHudList)
	{
		HUDLIST* pList;
		while (m_pHudList)
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free(pList);
		}
		m_pHudList = NULL;
	}
}

// GetSpriteIndex()
// searches through the sprite list loaded from hud.txt for a name matching SpriteName
// returns an index into the gHUD.m_rghSprites[] array
// returns 0 if sprite not found
int CHud::GetSpriteIndex(const char* SpriteName)
{
	// look through the loaded sprite name list for SpriteName
	for (int i = 0; i < m_iSpriteCount; i++)
	{
		if (strncmp(SpriteName, m_rgszSpriteNames + (i * MAX_SPRITE_NAME_LENGTH), MAX_SPRITE_NAME_LENGTH) == 0)
			return i;
	}

	return -1; // invalid sprite
}

extern bool g_bUpdateProgression;
void CHud::VidInit()
{
	cl_entity_s* view = gEngfuncs.GetViewModel();

	// TODO: needs to be called on every map change, not just when starting a new game
	if (g_pParticleMan)
		g_pParticleMan->ResetParticles();

	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo(&m_scrinfo);

	// ----------
	// Load Sprites
	// ---------
	//	m_hsprFont = LoadSprite("sprites/%d_font.spr");

	m_hsprLogo = 0;
	m_hsprCursor = 0;

	if (ScreenWidth < 640)
		m_iRes = 320;
	else
		m_iRes = 640;

	// Only load this once
	if (!m_pSpriteList)
	{
		// we need to load the hud.txt, and all sprites within
		m_pSpriteList = SPR_GetList("sprites/hud.txt", &m_iSpriteCountAllRes);

		if (m_pSpriteList)
		{
			// count the number of sprites of the appropriate res
			m_iSpriteCount = 0;
			client_sprite_t* p = m_pSpriteList;
			int j;
			for (j = 0; j < m_iSpriteCountAllRes; j++)
			{
				if (p->iRes == m_iRes)
					m_iSpriteCount++;
				p++;
			}

			// allocated memory for sprite handle arrays
			m_rghSprites = new HSPRITE[m_iSpriteCount];
			m_rgrcRects = new Rect[m_iSpriteCount];
			m_rgszSpriteNames = new char[m_iSpriteCount * MAX_SPRITE_NAME_LENGTH];

			p = m_pSpriteList;
			int index = 0;
			for (j = 0; j < m_iSpriteCountAllRes; j++)
			{
				if (p->iRes == m_iRes)
				{
					char sz[256];
					sprintf(sz, "sprites/%s.spr", p->szSprite);
					m_rghSprites[index] = SPR_Load(sz);
					m_rgrcRects[index] = p->rc;
					strncpy(&m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH], p->szName, MAX_SPRITE_NAME_LENGTH);

					index++;
				}

				p++;
			}
		}
	}
	else
	{
		// we have already have loaded the sprite reference from hud.txt, but
		// we need to make sure all the sprites have been loaded (we've gone through a transition, or loaded a save game)
		client_sprite_t* p = m_pSpriteList;
		int index = 0;
		for (int j = 0; j < m_iSpriteCountAllRes; j++)
		{
			if (p->iRes == m_iRes)
			{
				char sz[256];
				sprintf(sz, "sprites/%s.spr", p->szSprite);
				m_rghSprites[index] = SPR_Load(sz);
				index++;
			}

			p++;
		}
	}

	// assumption: number_1, number_2, etc, are all listed and loaded sequentially
	m_HUD_number_0 = GetSpriteIndex("number_0");

	m_iFontHeight = m_rgrcRects[m_HUD_number_0].bottom - m_rgrcRects[m_HUD_number_0].top;

	g_iRestoreViewent = 1;

	m_Ammo.VidInit();
	m_Health.VidInit();
	m_Spectator.VidInit();
	m_Geiger.VidInit();
	m_Train.VidInit();
	m_Battery.VidInit();
	m_Flash.VidInit();
	m_Message.VidInit();
	m_StatusBar.VidInit();
	m_DeathNotice.VidInit();
	m_SayText.VidInit();
	m_Menu.VidInit();
	m_AmmoSecondary.VidInit();
	m_TextMessage.VidInit();
	m_StatusIcons.VidInit();
	GetClientVoiceMgr()->VidInit();

	discord_integration::VidInit();
	Subtitles_SubtClear("ektum", sizeof("ektom"), (void*)"ektum");

	g_bUpdateProgression = true;

	Precache();
}

bool CHud::MsgFunc_Logo(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	// update Train data
	m_iLogo = READ_BYTE();

	return true;
}

float g_lastFOV = 0.0;

/*
============
COM_FileBase
============
*/
// Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
void COM_FileBase(const char* in, char* out)
{
	int len, start, end;

	len = strlen(in);

	// scan backward for '.'
	end = len - 1;
	while (0 != end && in[end] != '.' && in[end] != '/' && in[end] != '\\')
		end--;

	if (in[end] != '.') // no '.', copy to end
		end = len - 1;
	else
		end--; // Found ',', copy to left of '.'


	// Scan backward for '/'
	start = len - 1;
	while (start >= 0 && in[start] != '/' && in[start] != '\\')
		start--;

	if (in[start] != '/' && in[start] != '\\')
		start = 0;
	else
		start++;

	// Length of new sting
	len = end - start + 1;

	// Copy partial string
	strncpy(out, &in[start], len);
	// Terminate it
	out[len] = 0;
}

/*
=================
HUD_IsGame

=================
*/
bool HUD_IsGame(const char* game)
{
	const char* gamedir;
	char gd[1024];

	gamedir = gEngfuncs.pfnGetGameDirectory();
	if (gamedir && '\0' != gamedir[0])
	{
		COM_FileBase(gamedir, gd);
		if (!stricmp(gd, game))
			return true;
	}
	return false;
}

/*
=====================
HUD_GetFOV

Returns last FOV
=====================
*/
float HUD_GetFOV()
{
	if (0 != gEngfuncs.pDemoAPI->IsRecording())
	{
		// Write it
		int i = 0;
		unsigned char buf[100];

		// Active
		*(float*)&buf[i] = g_lastFOV;
		i += sizeof(float);

		Demo_WriteBuffer(TYPE_ZOOM, i, buf);
	}

	if (0 != gEngfuncs.pDemoAPI->IsPlayingback())
	{
		g_lastFOV = g_demozoom;
	}
	return g_lastFOV;
}

bool CHud::MsgFunc_SetFOV(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int newfov = READ_BYTE();
	int def_fov = CVAR_GET_FLOAT("default_fov");

	//Weapon prediction already takes care of changing the fog. ( g_lastFOV ).
	//But it doesn't restore correctly so this still needs to be used
	/*
	if ( cl_lw && cl_lw->value )
		return 1;
		*/

	g_lastFOV = newfov;

	if (newfov == 0)
	{
		m_flTargetFov = def_fov;
	}
	else
	{
		m_flTargetFov = newfov;
	}

	m_flTargetFov = clamp(m_flTargetFov, 60, 120);

	

	// the clients fov is actually set in the client data update section of the hud

	// Set a new sensitivity
	if (m_flTargetFov == def_fov)
	{
		// reset to saved sensitivity
		m_flMouseSensitivity = 0;
	}
	else
	{
		// set a new sensitivity that is proportional to the change from the FOV default
		m_flMouseSensitivity = sensitivity->value * ((float)newfov / (float)def_fov) * CVAR_GET_FLOAT("zoom_sensitivity_ratio");
	}

	return true;
}


void CHud::AddHudElem(CHudBase* phudelem)
{
	HUDLIST *pdl, *ptemp;

	//phudelem->Think();

	if (!phudelem)
		return;

	pdl = (HUDLIST*)malloc(sizeof(HUDLIST));
	if (!pdl)
		return;

	memset(pdl, 0, sizeof(HUDLIST));
	pdl->p = phudelem;

	if (!m_pHudList)
	{
		m_pHudList = pdl;
		return;
	}

	ptemp = m_pHudList;

	while (ptemp->pNext)
		ptemp = ptemp->pNext;

	ptemp->pNext = pdl;
}

float CHud::GetSensitivity()
{
	return m_flMouseSensitivity;
}
