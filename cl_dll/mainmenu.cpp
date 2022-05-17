#include "mainmenu.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "hud.h"

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
	// Load from file
	int image_width = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
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

extern SDL_Window* window;

int g_iChapSelPage = 0;
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

enum texlist_e
{
	ICON_BMI = 0,
	ICON_AM,
	ICON_UC,
	ICON_OC,
	ICON_WGH,
	ICON_BP,
};

void Centered(char *text)
{
	auto windowWidth = ImGui::GetWindowSize().x;
	auto textWidth = ImGui::CalcTextSize(text).x;

	ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
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
			strcpy(g_textures[i].chaptermap, token);
		}
	}

	gEngfuncs.COM_FreeFile(pfile2);
	pfile = pfile2 = nullptr;

	return i + 1;
}

void MainMenuGUI_Init()
{
	int num = g_iNumChapters = nlfs.ParseChapters();

	g_iNumPages = ceil(num / 3);

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
}

// prolly have to find a better alternative
void GetChapterNums(int &start, int &end)
{
	start = end * (g_iChapSelPage);
	end = start + 3;
	if (end > g_iNumChapters)
		end = g_iNumChapters;
}

// i skipped the documentation :)

void MainMenuGUI_DrawMainWindow()
{
	if (g_iNumChapters < 1)
		return;

	ImGuiStyle* style = &ImGui::GetStyle();

	const int GAME_MODE_WINDOW_WIDTH = 255 * 3.4;
	const int GAME_MODE_WINDOW_HEIGHT = 150 + 20;

	int width = 0;

	int RENDERED_WIDTH;
	SDL_GetWindowSize(window, &RENDERED_WIDTH, NULL);
	ImGui::SetNextWindowPos(ImVec2(RENDERED_WIDTH - GAME_MODE_WINDOW_WIDTH, 0), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(GAME_MODE_WINDOW_WIDTH, GAME_MODE_WINDOW_HEIGHT), ImGuiSetCond_FirstUseEver);

	ImGui::Begin("Chapter Select", &g_iChapMenuOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	
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

	ImGui::Spacing();
	ImGui::Separator();

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Columns(3, 0, false);

	int startchp = 0;
	int endchp = 3;

	GetChapterNums(startchp, endchp);

	for (int i = startchp; i < endchp; i++)
	{
		ImGui::BeginChild(i, ImVec2(GAME_MODE_WINDOW_WIDTH, GAME_MODE_WINDOW_HEIGHT), false);
		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Back            ").x) * 0.012f);
		ImGui::Text(g_textures[i].chaptername);
		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Back            ").x) * 0.012f);
		if (ImGui::ImageButton((void*)(intptr_t)g_textures[i].texture, ImVec2(g_textures[i].width, g_textures[i].height), ImVec2(0, 0), ImVec2(1, 1), -1))
		{
			char szCmd[64];
			sprintf(szCmd, "map %s", g_textures[i].chaptermap);
			gEngfuncs.pfnClientCmd(szCmd);
			g_iChapMenuOpen = false;
		}
		ImGui::EndChild();
		ImGui::NextColumn();
	}
	ImGui::Columns(1, 0, false);

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

	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Cancel          ").x) * 0.965f);
	if (ImGui::Button("Cancel          "))
	{
		g_iChapMenuOpen = false;
	}

	ImGui::Spacing();

	ImGui::End();
}
