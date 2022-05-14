#define NOCLAMP
#include "hud.h"
#include "cl_dll.h"
#include "parsemsg.h"
#include "cvardef.h"

#include "PlatformHeaders.h"

#include "net_api.h"

#include <stdio.h>	// for safe_sprintf()
#include <stdarg.h> // "
#include <string.h> // for strncpy()

#include <iostream> 
#include <string> // the C++ Standard String Class

#include "subtitles.h"
#include "cpp_aux.h"
#include "fs_aux.h"
#include <map>
#include <fstream>
#include <regex>



extern SDL_Window* window;

using namespace aux;

std::map<std::string, SubtitleOutput> subtitlesToDraw;
std::map<std::string, SubtitleColor> colors;
std::map<std::string, std::vector<Subtitle>> subtitleMap;

void Subtitles_Init()
{
	gEngfuncs.pfnHookUserMsg("OnSound", Subtitles_OnSound);
	gEngfuncs.pfnHookUserMsg("SubtClear", Subtitles_SubtClear);
	gEngfuncs.pfnHookUserMsg("SubtRemove", Subtitles_SubtRemove);

	auto resourceDirectory = FS_ResolveModPath("resource");
	auto subtitleFiles = FS_GetAllFileNamesByWildcard("resource\\subtitles_*.txt");

	std::regex rgx("subtitles_(\\w+)\\.txt");
	for (auto& sub : subtitleFiles)
	{
		std::smatch match;
		std::regex_search(sub, match, rgx);

		if (match.size() > 1)
		{
			Subtitles_ParseSubtitles(
				FS_ResolveModPath(std::string("resource\\") + sub),
				match.str(1));
		}
	}
}

void Subtitles_ParseSubtitles(const std::string& filePath, const std::string& language)
{

	std::ifstream inp(filePath);
	if (!inp.is_open())
	{
		gEngfuncs.Con_DPrintf("%s SUBTITLE PARSER ERROR: failed to read file\n", filePath.c_str());
		return;
	}

	std::string line;
	int lineCount = 0;
	while (std::getline(inp, line))
	{
		lineCount++;

		line = str::trim(line);
		if (line.size() == 0)
		{
			continue;
		}

		if (line.find("SUBTITLE") != 0 && line.find("COLOR") != 0)
		{
			continue;
		}

		std::vector<std::string> parts = str::split(line, '|');

		if (parts.at(0) == "SUBTITLE")
		{
			if (parts.size() < 6)
			{
				gEngfuncs.Con_DPrintf("%s SUBTITLE PARSER ERROR ON Line %d: insufficient subtitle data\n", filePath.c_str(), lineCount);
				continue;
			}
			std::string subtitleKey = str::toUppercase(language + "_" + parts.at(1));
			std::string colorKey = parts.at(2);
			std::string text = parts.at(5);
			float delay, duration;
			try
			{
				delay = std::stof(parts.at(3));
				duration = std::stof(parts.at(4));

				subtitleMap[subtitleKey].push_back({delay,
					duration,
					colorKey,
					text});
			}
			catch (std::invalid_argument e)
			{
				gEngfuncs.Con_DPrintf("%s SUBTITLE PARSER ERROR ON Line %d: delay or duration is not a number\n", filePath.c_str(), lineCount);
			}
		}
		else if (parts.at(0) == "COLOR")
		{
			if (parts.size() < 5)
			{
				gEngfuncs.Con_DPrintf("%s SUBTITLE PARSER ERROR ON Line %d: insufficient color data\n", filePath.c_str(), lineCount);
				continue;
			}
			std::string colorKey = str::toUppercase(parts.at(1));
			float r, g, b;
			try
			{
				r = std::stof(parts.at(2));
				g = std::stof(parts.at(3));
				b = std::stof(parts.at(4));

				colors[colorKey] = {r, g, b};
			}
			catch (std::invalid_argument e)
			{
				gEngfuncs.Con_DPrintf("%s SUBTITLE PARSER ERROR ON Line %d: some of color data is not a number\n", filePath.c_str(), lineCount);
			}
		}
	}
}

std::string GetSubtitleKeyWithLanguage(const std::string& key)
{
	std::string language_to_use = std::string(nlvars.subtitles_language->string);
	auto subtitleKey = str::toUppercase(language_to_use + "_" + key);

	if (subtitleMap.count(subtitleKey))
	{
		return subtitleKey;
	}
	else
	{
		return "EN_" + key;
	}
}

bool Subtitle_IsFarAwayFromPlayer(const SubtitleOutput& subtitle)
{
	if (subtitle.ignoreLongDistances)
	{
		return false;
	}

	Vector playerOrigin = gEngfuncs.GetLocalPlayer()->origin;
	float distance = (subtitle.pos - playerOrigin).Length();

	return distance >= 768;
}

void Subtitles_Draw()
{
	ImGuiStyle* style = &ImGui::GetStyle();

	static float l_FrameWidth = 0;
	static float l_FrameHeight = 0;

	if (subtitlesToDraw.size() == 0)
	{
		l_FrameWidth = 0;
		l_FrameHeight = 0;
		return;
	}

	extern ref_params_s g_pparams;

	int ScreenWidth, ScreenHeight;
	SDL_GetWindowSize(window, &ScreenWidth, &ScreenHeight);

	float FrameWidth = 0;
	float FrameHeight = 0;
	float LowestWidth = 0;
	float RealWidth = 0;
	int ldindex = 0;
	int farindex = -1;
	int numsubstodraw = 0;
	int numsubstodraw2 = 0;
	float prevduration = 0;

	bool willDrawAtLeastOneSubtitle = false;

	float time = gEngfuncs.GetClientTime();
	float fontScale = nlvars.subtitles_font_scale->value;
	if (fontScale < 1.0f)
	{
		fontScale = 1.0f;
	}
	if (fontScale > 2.0f)
	{
		fontScale = 2.0f;
	}

	auto i = subtitlesToDraw.begin();
	while (i != subtitlesToDraw.end())
	{
		auto& subtitle = i->second;

		if (time >= subtitle.duration)
		{
			i = subtitlesToDraw.erase(i);
			continue;
		}

		if (time < subtitle.delay)
		{
			i++;
			continue;
		}

		ImVec2 textSize = ImGui::CalcTextSize(subtitle.text.c_str());

		FrameWidth = textSize.x * fontScale;

		if (!Subtitle_IsFarAwayFromPlayer(subtitle))
		{
			FrameHeight += textSize.y * fontScale;

			FrameWidth += 20;

			willDrawAtLeastOneSubtitle = true;

			if (RealWidth < FrameWidth)
				RealWidth = FrameWidth;

			if (LowestWidth == 0 || FrameWidth < LowestWidth)
				LowestWidth = FrameWidth;
			numsubstodraw2++;
		}
		else
		{
			subtitle.alpha = nlutils.lerp(subtitle.alpha, 0.0f, g_pparams.frametime * 6.5f);
			willDrawAtLeastOneSubtitle = true;
		}
		i++;
	}

	if (!willDrawAtLeastOneSubtitle)
	{
		return;
	}

	if (l_FrameWidth == 0)
		l_FrameWidth = LowestWidth;

	l_FrameHeight = nlutils.lerp(l_FrameHeight, FrameHeight + 7.5 * numsubstodraw2, g_pparams.frametime * 13.5f);
	l_FrameWidth = nlutils.lerp(l_FrameWidth, RealWidth, g_pparams.frametime * 15.5f);

	ImGui::SetNextWindowPos(ImVec2(ScreenWidth / 2.0f - l_FrameWidth / 2.0f, ScreenHeight / 1.3f), 0);
	ImGui::SetNextWindowSize(ImVec2(l_FrameWidth, l_FrameHeight));

	ImGui::Begin("Subtitles", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	style->WindowRounding = 5.0f;
	style->WindowTitleAlign = ImVec2(0.5, 0.5);

	for (auto& pair : subtitlesToDraw)
	{

		bool isfar = false;
		auto& subtitle = pair.second;
		if (time < subtitle.delay)
		{
			continue;
		}


		if (Subtitle_IsFarAwayFromPlayer(subtitle))
		{
			continue;
		}

		ImVec2 textSize = ImGui::CalcTextSize(subtitle.text.c_str());
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2.0f - textSize.x / 2.0f);
		ImGui::TextColored(ImVec4(subtitle.color[0], subtitle.color[1], subtitle.color[2], subtitle.alpha), "%s", subtitle.text.c_str());

		if ((subtitle.duration - 0.25f) <= time)
		{
			subtitle.alpha = nlutils.lerp(subtitle.alpha, 0, g_pparams.frametime * 8.5f);
		}
		else
			subtitle.alpha = nlutils.lerp(subtitle.alpha, 1, g_pparams.frametime * 8.5f);

		if (prevduration < subtitle.duration)
		{
			prevduration = subtitle.duration;
			ldindex = numsubstodraw;
		}
		numsubstodraw++;
	}

	auto& pair = subtitlesToDraw.begin();
	if (ldindex > 0)
	{
		for (int j = 0; j < (ldindex); j++)
		{
			pair++;
		}
	}

	auto& subtitle = pair->second;
	if ((subtitle.duration - 0.6) <= time || numsubstodraw == 0 || numsubstodraw2 == 0)
	{
		style->Alpha = nlutils.lerp(style->Alpha, 0, g_pparams.frametime * 6.5f);
	}
	else
		style->Alpha = nlutils.lerp(style->Alpha, 1, g_pparams.frametime * 6.5f);
	
	ImGui::SetWindowFontScale(fontScale);
	ImGui::End();
}

void Subtitles_Push(const std::string& key, const std::string& text, float duration, const Vector& color, const Vector& pos, float delay, int ignoreLongDistances)
{
	if (subtitlesToDraw.count(key))
	{
		return;
	}

	int ScreenWidth;
	SDL_GetWindowSize(window, &ScreenWidth, NULL);

	std::vector<std::string> lines = str::wordWrap(text.c_str(), ScreenWidth / 2.0f, [](const std::string& str) -> float
		{ return ImGui::CalcTextSize(str.c_str()).x; });

	for (size_t i = 0; i < lines.size(); i++)
	{
		std::string actualKey = key + "_" + std::to_string(i);

		float startTime = gEngfuncs.GetClientTime() + delay;

		actualKey = GetSubtitleKeyWithLanguage(actualKey);

		subtitlesToDraw[actualKey] = {
			startTime,
			startTime + ( duration + 0.5f ),
			ignoreLongDistances,
			color,
			pos,
			lines.at(i)};
	}
}

const std::vector<Subtitle> Subtitles_GetByKey(const std::string& key)
{
	std::string actualKey = str::toUppercase(key);

	return subtitleMap.count(actualKey) ? subtitleMap[actualKey] : std::vector<Subtitle>();
}

const SubtitleColor Subtitles_GetSubtitleColorByKey(const std::string& key)
{
	SubtitleColor defaultColor = {
		1.0,
		1.0,
		1.0};
	return colors.count(key) ? colors[key] : defaultColor;
}

void Subtitles_Push(const std::string& key, int ignoreLongDistances, const Vector& pos)
{
	float print_subtitles_cvar = nlvars.print_subtitles->value;
	if (print_subtitles_cvar <= 0.0f)
	{
		return;
	}

	std::string actualKey = GetSubtitleKeyWithLanguage(key);

	auto subtitles = Subtitles_GetByKey(actualKey);

	for (size_t i = 0; i < subtitles.size(); i++)
	{
		auto& subtitle = subtitles.at(i);
		auto color = Subtitles_GetSubtitleColorByKey(subtitle.colorKey);

		Subtitles_Push(
			actualKey + std::to_string(i),
			subtitle.text,
			subtitle.duration,
			Vector(color.r, color.g, color.b),
			pos,
			subtitle.delay,
			ignoreLongDistances);
	}
}

int Subtitles_OnSound(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	std::string key = READ_STRING();
	bool ignoreLongDistances = READ_BYTE();
	float x = READ_COORD();
	float y = READ_COORD();
	float z = READ_COORD();
	
	if (nlvars.subtitles_log_candidates->value >= 1.0f)
	{
		gEngfuncs.Con_DPrintf("RECEIVED SUBTITLE CANDIDATE: %s\n", key.c_str());
	}

	ignoreLongDistances = false;

	// Dumb exception for Grunt cutscene, because player is actually far away from sound event
	if (key.find("!HG_DRAG") == 0)
	{
		ignoreLongDistances = true;
	}

	Subtitles_Push(key, ignoreLongDistances, Vector(x, y, z));

	return 1;
}

int Subtitles_SubtClear(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	subtitlesToDraw.clear();

	return 1;
}

int Subtitles_SubtRemove(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	std::string key = READ_STRING();

	auto i = subtitlesToDraw.begin();
	while (i != subtitlesToDraw.end())
	{
		auto subtitleKey = GetSubtitleKeyWithLanguage(key);

		if (subtitleKey.find(key) == 0)
		{
			i = subtitlesToDraw.erase(i);
			continue;
		}

		i++;
	}

	return 1;
}