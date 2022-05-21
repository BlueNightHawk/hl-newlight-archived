#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

#define MATHLIB_H
#include "hud.h"

#include "studio.h"
#include "com_model.h"
#include "cl_animating.h"
#include "cl_filesystem.h"
#include "r_studioint.h"

extern engine_studio_api_s IEngineStudio;

nlfs_s nlfs;

using std::cin;
using std::endl;
using std::string;
using std::filesystem::directory_iterator;

int nlfs_s::NextSaveFile(char* savename, char *out_savename)
{
	char fullpath[256] = {""};
	char savenum[4] = {"\0"};
	string dir = std::filesystem::current_path().string();
	strcpy(fullpath, dir.c_str());
	strcat(fullpath, "\\");
	strcat(fullpath, gEngfuncs.pfnGetGameDirectory());
	strcat(fullpath, "\\SAVE");

	string path = fullpath;

	int i = 0;

	for (const auto& file : directory_iterator(path))
	{
		if (file.path().extension().string().c_str() == nullptr || stricmp(file.path().extension().string().c_str(), ".sav"))
			continue;
	
		if (!strncmp(savename, (file.path().string().c_str() + strlen(dir.c_str()) + strlen(gEngfuncs.pfnGetGameDirectory()) + strlen("\\SAVE") + 2), strlen(savename)))
		{
			i++;
		}
	}
	strcpy(out_savename, savename);
	if (i < 10)
	{
		sprintf(savenum, "00%i", i);
	}
	else if (i < 100)
	{
		sprintf(savenum, "0%i", i);
	}
	else if (i < 1000)
	{
		sprintf(savenum, "%i", i);
	}
	strcat(out_savename, savenum);
	return i;
}

void ReCacheGlowModels(void)
{
	for (int i = 0; i < 512; i++)
	{
		memset(gHUD.m_szGlowModels[i], '\0', 64);
	}

	CacheGlowModels();
}

void CacheGlowModels(void)
{
	// shitty code alert!!!!!!
	// i learnt C from an old book pls dont shame me

	char fullpath[256] = {""};
	string dir = std::filesystem::current_path().string();
	strcpy(fullpath, dir.c_str());
	strcat(fullpath, "\\");
	strcat(fullpath, gEngfuncs.pfnGetGameDirectory());
	strcat(fullpath, "\\models\\glowmodels");	

	string path = fullpath;

	int i = 0;

	for (const auto& file : directory_iterator(path))
	{
		if (file.path().extension().string().c_str() == nullptr  || stricmp(file.path().extension().string().c_str(), ".mdl"))
			continue;

		strcpy(gHUD.m_szGlowModels[i], file.path().string().c_str() + strlen(dir.c_str()) + strlen(gEngfuncs.pfnGetGameDirectory()) + strlen("\\models\\glowmodels") + 2);
		i++;	
	}
}

// for viewmodel only
int nlfs_s::GetBoneIndexByName(char* name)
{
	mstudiobone_t* pbone = nullptr;
	int index = -1;

	for (int i = 0; i < g_viewinfo.phdr->numbones; i++)
	{
		pbone = (mstudiobone_t*)((byte*)g_viewinfo.phdr + g_viewinfo.phdr->boneindex);
		if (!stricmp(name, pbone[i].name))
		{
			index = i;
			break;
		}
	}
	return index;
}

// get camanim bone index from file
int nlfs_s::GetAnimBoneFromFile(char* name)
{
	static int prevboneindex = -1;
	static char pszPrevName[100] = {"\0"};

	if (strlen(pszPrevName) > 1 && !stricmp(pszPrevName, name))
		return prevboneindex;

	char *pfile, *pfile2;
	pfile = pfile2 = (char*)gEngfuncs.COM_LoadFile("models/animbonelist.txt", 5, NULL);
	char token[500];
	int index = -1;

	if (pfile == nullptr)
	{
		return -1;
	}

	while (pfile = gEngfuncs.COM_ParseFile(pfile, token))
	{
		if (strlen(token) <= 0)
			break;

		if (!stricmp(token, name))
		{
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			if (!stricmp("bone", token))
			{
				pfile = gEngfuncs.COM_ParseFile(pfile, token);
				index = GetBoneIndexByName(token);
			}
			else
				index = atoi(token);
			break;
		}
	}

	gEngfuncs.COM_FreeFile(pfile2);
	pfile = pfile2 = nullptr;

	strcpy(pszPrevName, name);

	prevboneindex = index;
	return index;
}

int nlfs_s::GetCamBoneIndex(cl_entity_s* view)
{
	int index = -1;

	// if it finds the bone index in the text file, bone guessing will not be done
	index = GetAnimBoneFromFile(view->model->name + 7);

	mstudiobone_t* pbone = nullptr;

	for (int i = 0; i < g_viewinfo.phdr->numbones; i++)
	{
		pbone = (mstudiobone_t*)((byte*)g_viewinfo.phdr + g_viewinfo.phdr->boneindex);

		if (pbone == nullptr || pbone[i].name == nullptr)
			break;

		if (strlen(pbone[i].name) > 1 && !stricmp(pbone[i].name, "camera"))
		{
			index = i;
			break;
		}
		// try using common gun bone names to get bone index
		else if (nlvars.cl_guessanimbone->value != 0 && (index) == -1 && strlen(pbone[i].name) > 1)
		{
			// add checks for more names if needed
			if (!stricmp(pbone[i].name, "gun"))
			{
				index = i;
				break;
			}
			else if (!stricmp(pbone[i].name, "weapon"))
			{
				index = i;
				break;
			}
			else if (!stricmp(pbone[i].name, "glock"))
			{
				index = i;
				break;
			}
			else if (!stricmp(pbone[i].name, "Bip01 R Wrist") || !stricmp(pbone[i].name, "Bip01 R Hand"))
			{
				index = i;
				break;
			}
		}
	}

	if ((int)nlvars.cl_animbone->value > 0)
		index = (int)nlvars.cl_animbone->value - 1;

	return index;
}

// get offsets from text file
bool nlfs_s::GetOffsetFromFile(char* name, float* offset, float* aoffset)
{
	static char pszPrevName[100] = {"\0"};

	if (strlen(pszPrevName) > 1 && !stricmp(pszPrevName, name))
		return true;

	char *pfile, *pfile2;
	pfile = pfile2 = (char*)gEngfuncs.COM_LoadFile("models/isight_offsets.txt", 5, NULL);
	char token[500];
	bool found = false;

	if (pfile == nullptr)
	{
		return false;
	}

	while (pfile = gEngfuncs.COM_ParseFile(pfile, token))
	{
		if (strlen(token) <= 0)
			break;
		if (!stricmp(token, name))
		{
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			offset[0] = atof(token);
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			offset[1] = -atof(token);
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			offset[2] = atof(token);
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			aoffset[0] = atof(token);
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			aoffset[1] = atof(token);
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			aoffset[2] = atof(token);
			found = true;
			break;
		}
	}

	gEngfuncs.COM_FreeFile(pfile2);
	pfile = pfile2 = nullptr;

	strcpy(pszPrevName, name);

	return found;
}

void nlfs_s::StoreChapterNames()
{
	char *pfile, *pfile2;
	pfile = pfile2 = (char*)gEngfuncs.COM_LoadFile("maps/chapterlist.txt", 5, NULL);
	char token[500];
	int i = -1;
	int j = 0;
	bool reading = false;

	if (pfile == nullptr)
	{
		return;
	}

	while (pfile = gEngfuncs.COM_ParseFile(pfile, token))
	{
		if (strlen(token) <= 0)
			break;

		if (!stricmp("title", token))
		{
			i++;
			j = 1;
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			strcpy(chapterdata[i][0], token);
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			strcpy(chapterdata[i][1], token);
		}
		else if (!stricmp("{", token))
		{
			reading = true;
		}
		else if (!stricmp("}", token))
		{
			reading = false;
		}
		else if (reading)
		{
			j++;
			strcpy(chapterdata[i][j], token);
		}
	}

	gEngfuncs.COM_FreeFile(pfile2);
	pfile = pfile2 = nullptr;
}

void nlfs_s::GetFullPath(char* path, char* mod)
{
	char fullpath[256] = {""};
	string dir = std::filesystem::current_path().string();
	strcpy(fullpath, dir.c_str());
	strcat(fullpath, "\\");
	if (mod != nullptr)
		strcat(fullpath, mod);
	else
		strcat(fullpath, gEngfuncs.pfnGetGameDirectory());
	strcat(fullpath, "\\");

	strcpy(path, fullpath);
}
