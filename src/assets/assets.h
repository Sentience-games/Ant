#pragma once

#include "ant_shared.h"
#include "renderer/renderer.h"

// NOTE(soimn): This limits an assset file to 16,384 assets.
#define ASSET_MAX_BLOCK_COUNT_PER_FILE 8
#define ASSET_MAX_ASSET_COUNT_PER_BLOCK 2048

struct AAF_Mesh_ID
{
    U32 value;
};

struct AAF_Texture_ID
{
    U32 value;
};

struct AAF_Material_ID
{
    U32 value;
};

struct AAF_Audio_ID
{
    U32 value;
};

enum ASSET_TYPE
{
    Asset_Mesh,
    Asset_Texture,
    Asset_Audio,
};

enum ASSET_STATE
{
    Asset_NotLoaded = 0,
    Asset_Queued,
    Asset_Loaded,
};

struct Asset
{
    // TODO(soimn): Add handle to file on disk or server
    
    // TODO(soimn):
    /*
    ** Check if UNC paths work with OpenFile and if the asset bank can be stored on a server which could be 
    ** accessed by such a path.
    */
    
    Memory_Index size;
    Memory_Index offset;
    Enum32(ASSET_TYPE) type;
    Enum32(ASSET_STATE) state;
    
    union
    {
        AAF_Mesh_ID mesh_id;
        AAF_Texture_ID texture_id;
        AAF_Material_ID material_id;
        AAF_Audio_ID audio_id;
    };
};

struct Asset_Block
{
    Asset* first;
    Asset* next;
};

struct Asset_File
{
    Asset_Block blocks[ASSET_MAX_BLOCK_COUNT_PER_FILE];
    Platform_File_Info file_info;
    U32 asset_count;
};

struct Asset_ID
{
    U32 index; // This refers to the index in the asset block
    U16 file;  // This file handle refers to the reg file, not the data file
    U16 block;
};

struct Game_Assets
{
    Platform_File_Group file_group;
    Asset_File* file_registry;
    U32 registry_size;
    
    Texture_Handle* textures;
    U32 texture_count;
    
    U32 material_count;
    Material* materials;
};