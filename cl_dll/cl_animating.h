#ifndef CL_ANIMATING_H
#define CL_ANIMATING_H

int LookupActivityWeight(cl_entity_s* ent, int activity, int weight);
int LookupActivity(cl_entity_s* ent, int activity);
float StudioEstimateFrame(cl_entity_s* e, mstudioseqdesc_t* pseqdesc);

void DispatchAnimEvents(cl_entity_s* e, float flInterval);

void StudioEvent(const struct mstudioevent_s* event, const struct cl_entity_s* entity);
void CL_Muzzleflash(const Vector org);

int LookupActivityHeaviest(cl_entity_s* ent, int activity);

#endif