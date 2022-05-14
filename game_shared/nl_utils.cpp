#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

#ifdef CLIENT_DLL
#include "hud.h"

#include "cl_animating.h"
#include "cl_filesystem.h"
#include "r_studioint.h"
#include "event_api.h"

extern engine_studio_api_s IEngineStudio;
#else
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "saverestore.h"
#include "client.h"
#include "decals.h"
#include "gamerules.h"
#include "game.h"

#include "nl_defs.h"
#endif
#include "studio.h"
#include "com_model.h"
#include "../cl_dll/in_defs.h"


nlutils_t nlutils;

using std::cin;
using std::endl;
using std::string;
using std::filesystem::directory_iterator;
#ifdef CLIENT_DLL
model_s* nlutils_s::GetModel(char* szname)
{
	char path[256];
	nlfs.GetFullPath(path);
	strcat(path, szname);

	if (!std::filesystem::exists(path))
	{
		nlfs.GetFullPath(path, "valve");
		strcat(path, szname);
		if (!std::filesystem::exists(path))
		{
			gEngfuncs.Con_Printf("Missing model : %s \n", szname);
			return IEngineStudio.Mod_ForName("models/testsphere.mdl", 0);
		}
	}

	for (int i = 0; i < 512; i++)
	{
		model_s* pMdl = gHUD.m_pModCache[i];
		if (pMdl && pMdl->name && !stricmp(pMdl->name, szname))
		{
			return pMdl;
		}
		else if (pMdl)
			continue;
		else
		{
			gHUD.m_pModCache[i] = IEngineStudio.Mod_ForName(szname, 0);
			return gHUD.m_pModCache[i];
		}
	}
	gEngfuncs.Con_Printf("Model cache overflow : %s \n", szname);
	return IEngineStudio.Mod_ForName("models/testsphere.mdl", 0);
}

int nlutils_s::PrecacheModel(const char* szname)
{
	return gEngfuncs.pEventAPI->EV_FindModelIndex(szname);
}

#else

void GetFullPath(char* path, char* mod = nullptr)
{
	char fullpath[256] = {""};
	char gamedir[64];
	GET_GAME_DIR(gamedir);
	string dir = std::filesystem::current_path().string();
	strcpy(fullpath, dir.c_str());
	strcat(fullpath, "\\");
	if (mod != nullptr)
		strcat(fullpath, mod);
	else
		strcat(fullpath, gamedir);
	strcat(fullpath, "\\");

	strcpy(path, fullpath);
}

int nlutils_s::PrecacheModel(const char* szname)
{
	char path[256];
	GetFullPath(path);
	strcat(path, szname);

	if (!std::filesystem::exists(path) && strlen(szname) > 5)
	{
		GetFullPath(path, "valve");
		strcat(path, szname);
		if (!std::filesystem::exists(path))
		{
			ALERT(at_console, "Missing model : %s \n", szname);
			return (*g_engfuncs.pfnPrecacheModel)("models/testsphere.mdl");
		}
	}

	return (*g_engfuncs.pfnPrecacheModel)(szname);
}
#endif
// SHADOWS START
void nlutils_s::Matrix3x4_VectorTransform(const float in[3][4], const float v[3], float out[3])
{
	out[0] = v[0] * in[0][0] + v[1] * in[0][1] + v[2] * in[0][2] + in[0][3];
	out[1] = v[0] * in[1][0] + v[1] * in[1][1] + v[2] * in[1][2] + in[1][3];
	out[2] = v[0] * in[2][0] + v[1] * in[2][1] + v[2] * in[2][2] + in[2][3];
}

void nlutils_s::Matrix3x4_VectorITransform(const float in[3][4], const float v[3], float out[3])
{
	Vector dir;

	dir[0] = v[0] - in[0][3];
	dir[1] = v[1] - in[1][3];
	dir[2] = v[2] - in[2][3];

	out[0] = dir[0] * in[0][0] + dir[1] * in[1][0] + dir[2] * in[2][0];
	out[1] = dir[0] * in[0][1] + dir[1] * in[1][1] + dir[2] * in[2][1];
	out[2] = dir[0] * in[0][2] + dir[1] * in[1][2] + dir[2] * in[2][2];
}
// SHADOWS END
float nlutils_s::lerp(float start, float end, float frac)
{
	// Exact, monotonic, bounded, determinate, and (for a=b=0) consistent:
	if (start <= 0 && end >= 0 || start >= 0 && end <= 0)
		return frac * end + (1 - frac) * start;

	if (frac == 1)
		return end; // exact
	// Exact at t=0, monotonic except near t=1,
	// bounded, determinate, and consistent:
	const float x = start + frac * (end - start);
	return frac > 1 == end > start ? V_max(end, x) : V_min(end, x); // monotonic near t=1
}

//-----------------------------------------------------------------------------
// Purpose: Generates Euler angles given a left-handed orientation matrix. The
//			columns of the matrix contain the forward, left, and up vectors.
// Input  : matrix - Left-handed orientation matrix.
//			angles[PITCH, YAW, ROLL]. Receives right-handed counterclockwise
//				rotations in degrees around Y, Z, and X respectively.
//-----------------------------------------------------------------------------

void nlutils_s::MatrixAngles(const float matrix[3][4], Vector& angles, Vector& position)
{
	MatrixGetColumn(matrix, 3, position);
	MatrixAngles(matrix, angles);
}


void nlutils_s::MatrixAngles(const float matrix[3][4], float* angles)
{
	float forward[3];
	float left[3];
	float up[3];

	//
	// Extract the basis vectors from the matrix. Since we only need the Z
	// component of the up vector, we don't get X and Y.
	//
	forward[0] = matrix[0][0];
	forward[1] = matrix[1][0];
	forward[2] = matrix[2][0];
	left[0] = matrix[0][1];
	left[1] = matrix[1][1];
	left[2] = matrix[2][1];
	up[2] = matrix[2][2];

	float xyDist = sqrtf(forward[0] * forward[0] + forward[1] * forward[1]);

	// enough here to get angles?
	if (xyDist > 0.001f)
	{
		// (yaw)	y = ATAN( forward.y, forward.x );		-- in our space, forward is the X axis
		angles[1] = RAD2DEG(atan2f(forward[1], forward[0]));

		// (pitch)	x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
		angles[0] = RAD2DEG(atan2f(-forward[2], xyDist));

		// (roll)	z = ATAN( left.z, up.z );
		angles[2] = RAD2DEG(atan2f(left[2], up[2]));
	}
	else // forward is mostly Z, gimbal lock-
	{
		// (yaw)	y = ATAN( -left.x, left.y );			-- forward is mostly z, so use right for yaw
		angles[1] = RAD2DEG(atan2f(-left[0], left[1]));

		// (pitch)	x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
		angles[0] = RAD2DEG(atan2f(-forward[2], xyDist));

		// Assume no roll in this case as one degree of freedom has been lost (i.e. yaw == roll)
		angles[2] = 0;
	}
}

void nlutils_s::MatrixGetColumn(const float in[3][4], int column, Vector& out)
{
	out.x = in[0][column];
	out.y = in[1][column];
	out.z = in[2][column];
}

void nlutils_s::MatrixSetColumn(const Vector& in, int column, float out[3][4])
{
	out[0][column] = in.x;
	out[1][column] = in.y;
	out[2][column] = in.z;
}

//-----------------------------------------------------------------------------
// Forward direction vector with a reference up vector -> Euler angles
//-----------------------------------------------------------------------------

void nlutils_s::VectorAngles(const Vector& forward, const Vector& pseudoup, Vector& angles)
{
	Vector left;

	CrossProduct(pseudoup, forward, left);
	VectorNormalizeFast(left);

	float xyDist = sqrtf(forward[0] * forward[0] + forward[1] * forward[1]);

	// enough here to get angles?
	if (xyDist > 0.001f)
	{
		// (yaw)	y = ATAN( forward.y, forward.x );		-- in our space, forward is the X axis
		angles[1] = RAD2DEG(atan2f(forward[1], forward[0]));

		// The engine does pitch inverted from this, but we always end up negating it in the DLL
		// UNDONE: Fix the engine to make it consistent
		// (pitch)	x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
		angles[0] = RAD2DEG(atan2f(-forward[2], xyDist));

		float up_z = (left[1] * forward[0]) - (left[0] * forward[1]);

		// (roll)	z = ATAN( left.z, up.z );
		angles[2] = RAD2DEG(atan2f(left[2], up_z));
	}
	else // forward is mostly Z, gimbal lock-
	{
		// (yaw)	y = ATAN( -left.x, left.y );			-- forward is mostly z, so use right for yaw
		angles[1] = RAD2DEG(atan2f(-left[0], left[1])); // This was originally copied from the "void MatrixAngles( const matrix3x4_t& matrix, float *angles )" code, and it's 180 degrees off, negated the values and it all works now (Dave Kircher)

		// The engine does pitch inverted from this, but we always end up negating it in the DLL
		// UNDONE: Fix the engine to make it consistent
		// (pitch)	x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
		angles[0] = RAD2DEG(atan2f(-forward[2], xyDist));

		// Assume no roll in this case as one degree of freedom has been lost (i.e. yaw == roll)
		angles[2] = 0;
	}
}

void nlutils_s::AngleMatrix(Vector const& angles, const Vector& position, float matrix[3][4])
{
	AngleMatrix(angles, matrix);
	MatrixSetColumn(position, 3, matrix);
}

void nlutils_s::AngleMatrix(const Vector& angles, float matrix[3][4])
{
	float sr, sp, sy, cr, cp, cy;

	SinCos(DEG2RAD(angles[YAW]), &sy, &cy);
	SinCos(DEG2RAD(angles[PITCH]), &sp, &cp);
	SinCos(DEG2RAD(angles[ROLL]), &sr, &cr);

	// matrix = (YAW * PITCH) * ROLL
	matrix[0][0] = cp * cy;
	matrix[1][0] = cp * sy;
	matrix[2][0] = -sp;

	float crcy = cr * cy;
	float crsy = cr * sy;
	float srcy = sr * cy;
	float srsy = sr * sy;
	matrix[0][1] = sp * srcy - crsy;
	matrix[1][1] = sp * srsy + crcy;
	matrix[2][1] = sr * cp;

	matrix[0][2] = (sp * crcy + srsy);
	matrix[1][2] = (sp * crsy - srcy);
	matrix[2][2] = cr * cp;

	matrix[0][3] = 0.0f;
	matrix[1][3] = 0.0f;
	matrix[2][3] = 0.0f;
}

void nlutils_s::VectorNormalizeFast(Vector& vec)
{
	// FLT_EPSILON is added to the radius to eliminate the possibility of divide by zero.
	float iradius = 1.f / (sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z) + FLT_EPSILON);

	vec.x *= iradius;
	vec.y *= iradius;
	vec.z *= iradius;
}