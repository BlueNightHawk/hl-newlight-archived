#ifndef CL_ANIMATING_H
#define CL_ANIMATING_H

void CL_Muzzleflash(const Vector org);

typedef struct animutils_s
{
	int LookupActivityWeight(cl_entity_s* ent, int activity, int weight);
	int LookupActivity(cl_entity_s* ent, int activity);
	float StudioEstimateFrame(cl_entity_s* e, mstudioseqdesc_t* pseqdesc);

	void DispatchAnimEvents(cl_entity_s* e, float flInterval);

	void StudioEvent(const struct mstudioevent_s* event, const struct cl_entity_s* entity);

	int LookupActivityHeaviest(cl_entity_s* ent, int activity);

	void SetBodygroup(cl_entity_t* ent, int iGroup, int iValue);

	void SendWeaponAnim(int iAnim, int iBody);

	int GetBodygroup(cl_entity_t* ent, int iGroup, int iValue);
} animutils_t;

extern animutils_s cl_animutils;

#endif