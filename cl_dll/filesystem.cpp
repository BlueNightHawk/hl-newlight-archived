#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

#define MATHLIB_H
#include "hud.h"

// shitty code alert
// because i learnt C from an old book

using std::cin;
using std::endl;
using std::string;
using std::filesystem::directory_iterator;

void CacheGlowModels(void);


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
