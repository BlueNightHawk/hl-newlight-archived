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
} animutils_t;

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

extern viewinfo_s g_viewinfo;
extern animutils_s cl_animutils;

#endif