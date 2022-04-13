#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "Exports.h"
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

#include "particleman.h"
#include "tri.h"

#include "pmtrace.h"
#include "pm_defs.h"

#include "Platform.h"

#include "pm_materials.h"

CBaseParticle* CreateWallpuff(pmtrace_t* pTrace, char* szModelName, float framerate, float speed, float scale, float brightness);
CBaseParticle* CreateCollideParticle(pmtrace_t* pTrace, char* szModelName, float speed, Vector vel, float scale, float brightness);
CBaseParticle* CreateGunSmoke(const Vector org, char* szModelName, float scale, float brightness);

void WallPuffCluster(pmtrace_t* pTrace, char mat);