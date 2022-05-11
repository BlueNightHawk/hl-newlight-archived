#pragma once

void CacheGlowModels();
void ReCacheGlowModels();

int GetBoneIndexByName(char* name);
int GetAnimBoneFromFile(char* name);
int GetCamBoneIndex(cl_entity_s* view);
bool GetOffsetFromFile(char* name, float* offset, float* aoffset);
void StoreChapterNames();