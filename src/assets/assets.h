#pragma once

#include "ant_shared.h"

typedef U32 Asset_File_ID;

enum ASSET_TYPE
{
    Asset_Mesh,
    Asset_Texture,
    Asset_Sound, // Split this into SFX and music?
    // Asset_LevelDescription?
};

enum ASSET_STATE
{
    Asset_NotLoaded = 0,
    Asset_Queued,
    Asset_Loaded,
};

#pragma pack(push, 1)
struct Asset
{
    Memory_Index offset;
    Memory_Index size;
    Asset_File_ID source_file;
    Enum32(ASSET_TYPE) type;
    Enum32(ASSET_STATE) state;
    U32 id; // index into local buffers
};

typedef U32 Asset_ID;
#pragma pack(pop)