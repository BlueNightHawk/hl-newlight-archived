#pragma once

#if (defined(CLIENT_DLL) && !defined(PM_SHARED))
#include "cl_entity.h"
#include "com_model.h"
#include "studio.h"

typedef struct viewinfo_s
{
	Vector attachment_forward[4];
	Vector attachment_right[4];
	Vector attachment_up[4];

	Vector actualbonepos[MAXSTUDIOBONES];
	Vector actualboneangles[MAXSTUDIOBONES];

	Vector bonepos[MAXSTUDIOBONES];
	Vector boneangles[MAXSTUDIOBONES];

	Vector prevbonepos[MAXSTUDIOBONES];
	Vector prevboneangles[MAXSTUDIOBONES];

	studiohdr_t* phdr;

	alight_s vmodel_lighting;

	// stuff specifically for viewmodel
	double m_flAnimTime;
	double m_flCurFrame;
	double m_flCurTime;

	int m_iPrevSeq;
} vminfo_t;

typedef struct nlvars_t
{
	struct cvar_s* r_autofov;
	struct cvar_s* hud_crosshair;
	struct cvar_s* hud_crosshair_speed;

	struct cvar_s* cl_toggleisight;
	struct cvar_s* hud_fade;

	struct cvar_s* print_subtitles;
	struct cvar_s* subtitles_font_scale;
	struct cvar_s* subtitles_language;
	struct cvar_s* subtitles_log_candidates;

	struct cvar_s* cl_weaponlag;
	struct cvar_s* cl_weaponlagscale;
	struct cvar_s* cl_weaponlagspeed;

	struct cvar_s* cl_fwdangle;
	struct cvar_s* cl_fwdspeed;

	struct cvar_s* cl_hudlag;

	struct cvar_s* cl_animbone;

	struct cvar_s* cl_guessanimbone;

	struct cvar_s* cl_sprintanim;
	struct cvar_s* cl_jumpanim;
	struct cvar_s* cl_retractwpn;
	struct cvar_s* cl_ironsight;

	struct cvar_s* r_shadows;
	struct cvar_s* r_shadow_height;
	struct cvar_s* r_shadow_x;
	struct cvar_s* r_shadow_y;
	struct cvar_s* r_shadow_alpha;

	struct cvar_s* r_glowmodels;
	struct cvar_s* r_camanims;
	struct cvar_s* r_dlightfx;
	struct cvar_s* r_drawlegs;

	struct cvar_s* cl_clientflashlight;
	struct cvar_s* cl_fakeprojflashlight;

	void InitCvars();
} nlvars_s;

typedef struct nlfs_t
{
	char chapterdata[64][32][64];

	int GetBoneIndexByName(char* name);
	int GetAnimBoneFromFile(char* name);
	int GetCamBoneIndex(cl_entity_s* view);

	bool GetOffsetFromFile(char* name, float* offset, float* aoffset);

	void StoreChapterNames();

	void GetFullPath(char* path, char* mod = nullptr);
	int ParseChapters();
} nlfs_s;

#endif
typedef struct nlutils_t
{
#if (defined(CLIENT_DLL) && !defined(PM_SHARED))
	model_s* GetModel(char* szname);
	#endif
	// SHADOWS START
	void Matrix3x4_VectorTransform(const float in[3][4], const float v[3], float out[3]);
	void Matrix3x4_VectorITransform(const float in[3][4], const float v[3], float out[3]);
	// SHADOWS END
	float lerp(float start, float end, float frac);

	void MatrixAngles(const float matrix[3][4], Vector& angles, Vector& position);
	void MatrixAngles(const float matrix[3][4], float* angles);

	void MatrixGetColumn(const float in[3][4], int column, Vector& out);
	void MatrixSetColumn(const Vector& in, int column, float out[3][4]);

	void VectorAngles(const Vector& forward, const Vector& pseudoup, Vector& angles);
	void AngleMatrix(Vector const& angles, const Vector& position, float matrix[3][4]);
	void AngleMatrix(const Vector& angles, float matrix[3][4]);

	void VectorNormalizeFast(Vector& vec);

	// Math routines done in optimized assembly math package routines
	void SinCos(float radians, float* sine, float* cosine)
	{
		*sine = sin(radians);
		*cosine = cos(radians);
	}

	Vector LerpVector(const float* start, const float* end, float frac)
	{
		Vector out;
		for (int i = 0; i < 3; i++)
			out[i] = lerp(start[i], end[i], frac);

		return out;
	}

	int PrecacheModel(const char* szname);
} nlutils_s;

#if (defined(CLIENT_DLL) && !defined(PM_SHARED))
extern viewinfo_s g_viewinfo;
extern nlvars_s nlvars;
extern nlfs_s nlfs;
#endif
extern nlutils_s nlutils;
