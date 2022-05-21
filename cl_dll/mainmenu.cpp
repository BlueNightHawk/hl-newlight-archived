#include "mainmenu.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_resize.h"

#include "hud.h"



extern SDL_Window* window;

bool g_bUpdateProgression = false;
int g_iChapSelPage = 0;
bool g_bQuitMenuOpen = false;
bool g_bLeaveMenuOpen = false;
extern bool g_iChapMenuOpen;
int g_iNumChapters = 0;
int g_iNumPages = 0;

struct
{
	int width;
	int height;
	GLuint texture;
	char chaptername[64];
	char chapterpic[32];
	char chaptermap[32];
} g_textures[32];

struct
{
	int width;
	int height;
	int numframes;
	GLuint texture[128];
} g_logotex;

void DrawValve();

STBIDEF unsigned char* stbi_xload(stbi__context* s, int* x, int* y, int* frames, int** delays);
STBIDEF unsigned char* stbi_xload_mem(unsigned char* buffer, int len, int* x, int* y, int* frames, int** delays);
STBIDEF unsigned char* stbi_xload_file(char const* filename, int* x, int* y, int* frames, int** delays);

STBIDEF unsigned char* stbi_xload_mem(unsigned char* buffer, int len, int* x, int* y, int* frames, int** delays)
{
	stbi__context s;
	stbi__start_mem(&s, buffer, len);
	return stbi_xload(&s, x, y, frames, delays);
}

STBIDEF unsigned char* stbi_xload_file(char const* filename, int* x, int* y, int* frames, int** delays)
{
	FILE* f;
	stbi__context s;
	unsigned char* result = 0;

	if (!(f = stbi__fopen(filename, "rb")))
		return stbi__errpuc("can't fopen", "Unable to open file");

	stbi__start_file(&s, f);
	result = stbi_xload(&s, x, y, frames, delays);
	fclose(f);

	return result;
}

STBIDEF unsigned char* stbi_xload(stbi__context* s, int* x, int* y, int* frames, int** delays)
{
	int comp;
	unsigned char* result = 0;

	if (stbi__gif_test(s))
		return (unsigned char *)stbi__load_gif_main(s, delays, x, y, frames, &comp, 4);

	stbi__result_info ri;
	result = (unsigned char*)stbi__load_main(s, x, y, &comp, 4, &ri, 8);
	*frames = !!result;

	if (ri.bits_per_channel != 8)
	{
		STBI_ASSERT(ri.bits_per_channel == 16);
		result = stbi__convert_16_to_8((stbi__uint16*)result, *x, *y, 4);
		ri.bits_per_channel = 8;
	}

	return result;
}

void CreateSave()
{
	char mapname[64] = {};
	char cmd[128];

	nlfs.NextSaveFile("Half-Life-", mapname);
	
	sprintf(cmd, "save %s", mapname);
	gEngfuncs.Con_Printf("%s \n", cmd);
	gEngfuncs.pfnClientCmd(cmd);
}

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
	// Load from file
	int image_width = 0;
	int image_height = 0;

	int frame = 0;

	unsigned char* image_data = stbi_xload_file(filename, &image_width, &image_height, &frame, nullptr);
	if (image_data == NULL)
		return false;

	// Create a OpenGL texture identifier
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

	// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	stbi_image_free(image_data);

	*out_texture = image_texture;
	*out_width = image_width;
	*out_height = image_height;

	return true;
}


int nlfs_s::ParseChapters()
{
	char *pfile, *pfile2;
	pfile = pfile2 = (char*)gEngfuncs.COM_LoadFile("resource/chapters.txt", 5, NULL);
	char token[500];
	int i = -1;
	bool reading = false;

	if (pfile == nullptr)
	{
		return -1;
	}

	while (pfile = gEngfuncs.COM_ParseFile(pfile, token))
	{
		if (strlen(token) <= 0)
			break;

		if (!stricmp("title", token))
		{
			i++;
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			strcpy(g_textures[i].chaptername, token);
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			strcpy(g_textures[i].chapterpic, token);
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
			if (strlen(g_textures[i].chaptermap) == 0)
				strcpy(g_textures[i].chaptermap, token);

			reading = false;
		}
	}

	gEngfuncs.COM_FreeFile(pfile2);
	pfile = pfile2 = nullptr;

	return i + 1;
}

void MainMenuGUI_Init(int recache = 1)
{
	int num = 0;
	if (recache != 0)
	{
		num = g_iNumChapters = nlfs.ParseChapters();

		g_iNumPages = ceil(num / 3);
	}
	else
	{
		num = g_iNumChapters;
	}

	if (num < 0)
		return;
	for (int i = 0; i < num; i++)
	{
		char pic[128];

		sprintf(pic, ".\\%s\\chaptericons\\%s.png", gEngfuncs.pfnGetGameDirectory(), g_textures[i].chapterpic);
		if (LoadTextureFromFile(pic, &g_textures[i].texture, &g_textures[i].width, &g_textures[i].height))
		{
		}	
	}
	for (int i = 0; i < 46; i++)
	{
		char pic[128];

		if (i + 1 < 10)
		{
			sprintf(pic, ".\\%s\\media\\logo\\logo-0%i.png", gEngfuncs.pfnGetGameDirectory(), i+1);
		}
		else
		{
			sprintf(pic, ".\\%s\\media\\logo\\logo-%i.png", gEngfuncs.pfnGetGameDirectory(), i+1);
		}
		if (LoadTextureFromFile(pic, &g_logotex.texture[i], &g_logotex.width, &g_logotex.height))
		{
		}
	}
}

// prolly have to find a better alternative
void GetChapterNums(int &start, int &end)
{
	start = end * (g_iChapSelPage);
	end = start + 3;
	if (end > g_iNumChapters)
		end = g_iNumChapters;
}

void UpdateProgression()
{
	int maplen = strlen(gEngfuncs.pfnGetLevelName());
	if (maplen == 0)
		return;

	for (int i = 0; i < g_iNumChapters; i++)
	{
		if (strlen(g_textures[i].chaptermap) <= 0)
			break;

		if (!strncmp(gEngfuncs.pfnGetLevelName() + 5, g_textures[i].chaptermap, strlen(gEngfuncs.pfnGetLevelName() + 5) - 4))
		{
			gEngfuncs.Cvar_SetValue("chaptersunlocked", i);
			break;
		}
	}
	g_bUpdateProgression = false;
}

// i skipped the documentation :)

ImGuiStyle *UpdateStyle()
{
	ImGuiStyle* style = &ImGui::GetStyle();

	style->WindowRounding = 0.0f;
	style->WindowTitleAlign = ImVec2(0.001f, 1.0f);
	style->Colors[ImGuiCol_WindowBg] = ImVec4(0.2f, 0.2f, 0.2f, 0.95f);
	style->Colors[ImGuiCol_TitleBg] = ImVec4(0.2f, 0.2f, 0.2f, 0.95f);
	style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.2f, 0.2f, 0.2f, 0.95f);
	style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2f, 0.2f, 0.2f, 0.95f);
	style->Colors[ImGuiCol_Header] = ImVec4(0.2f, 0.2f, 0.2f, 0.95f);
	style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.2f, 0.2f, 0.2f, 0.95f);
	style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.2f, 0.2f, 0.2f, 0.95f);
	style->Colors[ImGuiCol_Button] = ImVec4(0.33f, 0.33f, 0.33f, 0.60f);
	style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.33f, 0.33f, 0.33f, 0.60f);
	style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.21f, 0.21f, 0.21f, 0.60f);
	style->Alpha = 1;
	return style;
}

void MainMenuGUI_DrawMainWindow()
{
	if (g_iNumChapters < 1)
		return;

	int maplen = strlen(gEngfuncs.pfnGetLevelName());

	const char* skill[3] = {"Easy", "Medium", "Hard"};
	static int selectedskill = 0;

	ImGuiStyle* style = UpdateStyle();
	ImVec4 tint = ImVec4(1, 1, 1, 1);

	const int GAME_MODE_WINDOW_WIDTH = 255 * 3.6;
	const int GAME_MODE_WINDOW_HEIGHT = 0;

	int width = 0;
	int startchp = 0;
	int endchp = 3;

	int x = 0, y = 0;

	GetChapterNums(startchp, endchp);

	if (g_bUpdateProgression)
		UpdateProgression();

	int RENDERED_WIDTH, RENDERED_HEIGHT;
	SDL_GetWindowSize(window, &RENDERED_WIDTH, &RENDERED_HEIGHT);

	x = (RENDERED_WIDTH / 2) - (GAME_MODE_WINDOW_WIDTH / 2);
	y = (RENDERED_HEIGHT / 2) - (180 / 1.5);

	ImGui::SetNextWindowPos(ImVec2(x,y), ImGuiSetCond_Once);
	ImGui::SetNextWindowSize(ImVec2(GAME_MODE_WINDOW_WIDTH, 0), ImGuiSetCond_Once);

	ImGui::Begin("Chapter Select", &g_bQuitMenuOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	ImGui::Spacing();
	ImGui::Separator();

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Columns(3, 0, false);

	for (int i = startchp; i < endchp; i++)
	{
		if (i > (int)nlvars.chaptersunlocked->value)
		{
			tint = ImVec4(1, 1, 1, 0.5);
			style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.33f, 0.33f, 0.33f, 0.60f);
			style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.33f, 0.33f, 0.33f, 0.60f);
		}
		else
		{
			tint = ImVec4(1, 1, 1, 1);
			style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.33f, 0.33f, 0.33f, 0.60f);
			style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.21f, 0.21f, 0.21f, 0.60f);
		}

		ImGui::BeginChild(i, ImVec2(GAME_MODE_WINDOW_WIDTH, 180), false, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Back            ").x) * 0.012f);
		ImGui::Text(g_textures[i].chaptername);
		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Back            ").x) * 0.012f);

		if (ImGui::ImageButton((void*)(intptr_t)g_textures[i].texture, ImVec2(g_textures[i].width, g_textures[i].height), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), tint))
		{
			if (i <= (int)nlvars.chaptersunlocked->value)
			{
				char szCmd[64];
				sprintf(szCmd, "map %s", g_textures[i].chaptermap);
				gEngfuncs.pfnClientCmd(szCmd);
				g_iChapMenuOpen = false;
			}
		}
		ImGui::EndChild();
		ImGui::NextColumn();
	}
	ImGui::Columns(1, 0, false);

	style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.33f, 0.33f, 0.33f, 0.60f);
	style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.21f, 0.21f, 0.21f, 0.60f);

	ImGui::Spacing();

	if (g_iChapSelPage > 0)
	{
		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Back            ").x) * 0.022f);
		if (ImGui::Button("Back            "))
		{
			g_iChapSelPage--;
		}
		ImGui::SameLine();
	}

	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Next            ").x) * 0.965f);
	if (g_iChapSelPage < g_iNumPages)
	{
		if (ImGui::Button("Next            "))
		{
			g_iChapSelPage++;
			if (g_iChapSelPage > g_iNumPages)
				g_iChapSelPage = g_iNumPages;
		}
	}
	else
	{
		ImGui::NewLine();
	}
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();

	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::Text("Difficulty:");
	ImGui::SameLine();
	ImGui::PushItemWidth(ImGui::CalcTextSize("Medium").x + 35);
	if (ImGui::Combo("", &selectedskill, skill, 3, -1))
	{
		gEngfuncs.Cvar_SetValue("skill", selectedskill + 1);
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Cancel          ").x) * 0.965f);
	if (ImGui::Button("Cancel          "))
	{
		g_iChapMenuOpen = false;
	}

	if (!g_iChapMenuOpen && maplen > 0)
		gEngfuncs.pfnClientCmd("unpause");

	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::End();
}

void MainMenuGUI_DrawQuitMenu()
{
	ImGuiStyle* style = UpdateStyle();
	ImVec4 tint = ImVec4(1, 1, 1, 1);

	const int GAME_MODE_WINDOW_WIDTH = 350;
	const int GAME_MODE_WINDOW_HEIGHT = 100;
	int maplen = strlen(gEngfuncs.pfnGetLevelName());

	int RENDERED_WIDTH, RENDERED_HEIGHT;
	SDL_GetWindowSize(window, &RENDERED_WIDTH, &RENDERED_HEIGHT);

	int x = (RENDERED_WIDTH / 2) - (GAME_MODE_WINDOW_WIDTH / 2);
	int y = (RENDERED_HEIGHT / 2) - (GAME_MODE_WINDOW_HEIGHT / 1.5);
	ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiSetCond_Once);
	ImGui::SetNextWindowSize(ImVec2(GAME_MODE_WINDOW_WIDTH, GAME_MODE_WINDOW_HEIGHT), ImGuiSetCond_Once);

	ImGui::Begin("Quit", &g_bQuitMenuOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	ImGui::Spacing();

	if (maplen == 0)
		ImGui::Text("Are you sure you want to quit?");
	else
		ImGui::Text("Do you wish to save the game before quitting?");

	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Cancel          ").x) * 0.9f);
	ImGui::SetCursorPosY((ImGui::GetWindowSize().y - ImGui::CalcTextSize("Cancel          ").y) * 0.75f);
	if (ImGui::Button("Cancel          "))
	{
		g_bQuitMenuOpen = false;
	}
	ImGui::SameLine();
	if (maplen > 0)
	{
		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Don't save      ").x) * 0.557f);
		ImGui::SetCursorPosY((ImGui::GetWindowSize().y - ImGui::CalcTextSize("Don't save      ").y) * 0.75f);
		if (ImGui::Button("Don't save      "))
		{
			gEngfuncs.pfnClientCmd("quit");
		}
		ImGui::SameLine();

		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Save            ").x) * 0.225f);
		ImGui::SetCursorPosY((ImGui::GetWindowSize().y - ImGui::CalcTextSize("Save            ").y) * 0.75f);
		if (ImGui::Button("Save            "))
		{
			CreateSave();
			gEngfuncs.pfnClientCmd("quit");
		}
		ImGui::SameLine();
	}
	else
	{
		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Quit            ").x) * 0.57f);
		ImGui::SetCursorPosY((ImGui::GetWindowSize().y - ImGui::CalcTextSize("Quit            ").y) * 0.75f);
		if (ImGui::Button("Quit            "))
		{
			gEngfuncs.pfnClientCmd("quit");
		}
		ImGui::SameLine();
	}

	if (!g_bQuitMenuOpen && maplen > 0)
		gEngfuncs.pfnClientCmd("unpause");
	ImGui::End();
}

void MainMenuGUI_DrawLeaveMenu()
{
	ImGuiStyle* style = UpdateStyle();
	ImVec4 tint = ImVec4(1, 1, 1, 1);

	const int GAME_MODE_WINDOW_WIDTH = 350;
	const int GAME_MODE_WINDOW_HEIGHT = 100;
	int maplen = strlen(gEngfuncs.pfnGetLevelName());

	int RENDERED_WIDTH, RENDERED_HEIGHT;
	SDL_GetWindowSize(window, &RENDERED_WIDTH, &RENDERED_HEIGHT);

	int x = (RENDERED_WIDTH / 2) - (GAME_MODE_WINDOW_WIDTH / 2);
	int y = (RENDERED_HEIGHT / 2) - (GAME_MODE_WINDOW_HEIGHT / 1.5);
	ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiSetCond_Once);
	ImGui::SetNextWindowSize(ImVec2(GAME_MODE_WINDOW_WIDTH, GAME_MODE_WINDOW_HEIGHT), ImGuiSetCond_Once);

	ImGui::Begin("Leave game", &g_bLeaveMenuOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	ImGui::Spacing();

	ImGui::Text("Do you wish to save the game before leaving?");

	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Cancel          ").x) * 0.9f);
	ImGui::SetCursorPosY((ImGui::GetWindowSize().y - ImGui::CalcTextSize("Cancel          ").y) * 0.75f);
	if (ImGui::Button("Cancel          "))
	{
		g_bLeaveMenuOpen = false;
	}
	ImGui::SameLine();

	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Don't save      ").x) * 0.557f);
	ImGui::SetCursorPosY((ImGui::GetWindowSize().y - ImGui::CalcTextSize("Don't save      ").y) * 0.75f);
	if (ImGui::Button("Don't save      "))
	{
		gEngfuncs.pfnClientCmd("disconnect");
		g_bLeaveMenuOpen = false;
	}
	ImGui::SameLine();

	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Save            ").x) * 0.225f);
	ImGui::SetCursorPosY((ImGui::GetWindowSize().y - ImGui::CalcTextSize("Save            ").y) * 0.75f);
	if (ImGui::Button("Save            "))
	{
		CreateSave();
		gEngfuncs.pfnClientCmd("disconnect");
		g_bLeaveMenuOpen = false;
	}
	ImGui::SameLine();

	if (!g_bLeaveMenuOpen && maplen > 0)
		gEngfuncs.pfnClientCmd("unpause");

	ImGui::End();
}
