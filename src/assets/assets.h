#pragma once

#include "ant_types.h"

#define ASSET_MAX_PER_ASSET_TAG_COUNT 8

enum ASSET_TYPE
{
    Asset_NoType = 0,
    
    Asset_Mesh,
    Asset_Texture,
};

enum ASSET_STATE
{
    Asset_Unloaded = 0,
    
    Asset_Queued,
    Asset_Loaded,
};

struct Asset_Tag
{
    U16 value;
};

struct Asset_Tag_Table_Entry
{
    String name;
    
    // NOTE(soimn): this will be used for tag searching in the editor, but is currently not implemented.
    struct Asset** assets;
    U32 asset_count;
    
    U32 precedence;
};

struct Asset
{
    Asset_Tag tags[ASSET_MAX_PER_ASSET_TAG_COUNT];
    
    I64 source_file_id;
    U32 reg_file_id;
    U32 offset;
    U32 size; // NOTE(soimn): If this is zero, the size is interpreted as the remainder of the file
    
    Enum8(ASSET_TYPE) type;
    Enum8(ASSET_STATE) state;
    
    union
    {
        Triangle_Mesh mesh;
        Texture texture;
    };
};

struct Asset_List
{
    Asset* first_asset;
    U32 count;
};

struct Asset_File
{
    Platform_File_Info file_info;
    
    U32 version; // TODO(soimn): Should this even be stored?
    
    I64* assets;
    U32 asset_count;
};

struct Game_Assets
{
    Memory_Arena arena;
    
    Asset_Tag_Table_Entry* tag_table;
    U32 tag_count;
    
    U32 asset_file_count;
    Asset_File* asset_files;
    
    String* data_files;
    U32 data_file_count;
    
    U32 asset_count;
    Asset* assets;
    
    U32 mesh_count;
    U32 texture_count;
    Asset* meshes;
    Asset* textures;
    
    Asset* default_mesh;
    Asset* default_texture;
};