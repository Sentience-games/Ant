#pragma once

#include "ant_types.h"

// NOTE(soimn): the alignment of the members in the reg file structs are dependent on this, and must therefore be 
//              taken into acount if the max per asset tag count is to be altered.
#define ASSET_MAX_PER_ASSET_TAG_COUNT 8

#define ASSET_INTERNAL_MAX_ALLOWED_ASSETS_ADDED_AT_RUNTIME 100

enum ASSET_TYPE
{
    Asset_NoType = 0,
    
    Asset_Mesh,
    Asset_Material,
    Asset_Texture,
    Asset_Shader,
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
    
    U32 source_file;
    U32 offset;
    U32 size;
    
    Enum8(ASSET_TYPE) type;
    Enum8(ASSET_STATE) state;
    
    union
    {
        Triangle_Mesh mesh;
        Material material;
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
    Platform_File_Handle file_handle;
    
    // NOTE(soimn): this will be used for "by file" searching in the editor, but is currently not implemented.
    Asset** assets;
    U32 asset_count;
};

struct Game_Assets
{
    Asset_Tag_Table_Entry* tag_table;
    U32 tag_count;
    
    U32 asset_file_count;
    Asset_File* asset_files;
    
    // NOTE(soimn): it may be smart to split the assets into several blocks in debug / internal mode to allow the 
    //              appending of assets at runtime without much hassle, currently there is a cap to how many 
    //              assets the user is allowed to add without reloading all assets.
    Asset* assets;
    U32 asset_count;
    
    U32 mesh_count;
    U32 texture_count;
    U32 material_count;
    U32 shader_count;
    Asset* meshes;
    Asset* textures;
    Asset* materials;
    Asset* shaders;
    
    Asset* default_mesh;
    Asset* default_texture;
    Asset* default_material;
    Asset* default_shader;
};

#define ASSET_REG_FILE_MAGIC_VALUE U32_FROM_BYTES('a', 'a', 'f', 'r')
#define ASSET_DATA_FILE_MAGIC_VALUE U32_FROM_BYTES('a', 'a', 'f', 'd')


#pragma pack(push, 1)
struct Asset_Reg_File_Header
{
    U32 magic_value;
    
    U32 data_file_count;
    U32 tag_count;
    U32 asset_count;
};

struct Asset_Reg_File_Asset_Entry
{
    U32 source_file_id;
    U32 offset;
    U32 size;
    U16 tags[ASSET_MAX_PER_ASSET_TAG_COUNT];
    Enum8(ASSET_TYPE) type;
    U8 pad_[3];
    
    union
    {
        struct
        {
            U32 triangle_list_count;
        } mesh_metadata;
        
        struct
        {
            Enum8(TEXTURE_TYPE) type;
            Enum8(TEXTURE_FORMAT) format;
            Enum8(TEXTURE_WRAPPING) u_wrapping;
            Enum8(TEXTURE_WRAPPING) v_wrapping;
            Enum8(TEXTURE_FILTERING) min_filtering;
            Enum8(TEXTURE_FILTERING) mag_filtering;
            U16 width;
            U16 height;
            U8 mip_levels;
        } texture_metadata;
    };
};
#pragma pack(pop)

// NOTE(soimn): The structure of an asset reg file
/*
MAGIC_VALUE
DATA_FILE_COUNT
TAG_COUNT
ASSET_COUNT

#FILE_TABLE
BASE_PATH
-- FILE_ID - RELATIVE_PATH

#TAG_TABLE
    -- TAG_NAME - ASSET_COUNT
    
#ASSET_TABLE
-- TAGS - FILE_ID - OFFSET - SIZE - TYPE - TYPE_SPECIFIC_METADATA
*/