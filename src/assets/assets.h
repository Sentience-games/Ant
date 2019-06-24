#pragma once

#include "ant_types.h"

// NOTE(soimn): the alignment of the members in the reg file structs are dependent on this, and must therefore be 
//              taken into acount if the max per asset tag count is to be altered.
// IMPORTANT(soimn): The asset count MUST be divisible by 4
#define ASSET_MAX_PER_ASSET_TAG_COUNT 8

#define ASSET_INTERNAL_MAX_ALLOWED_ASSETS_ADDED_AT_RUNTIME 100

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
    U32 size;
    
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
    
    I64* local_data_file_table;
    I64* local_tag_table;
    I64* assets;
    
    U32 data_file_count;
    U32 tag_count;
    U32 asset_count;
    
    U32 one_past_end_of_data_file_section;
    U32 one_past_end_of_tag_table_section;
    
    B16 wrong_endian;
    B16 is_valid;
};

struct Game_Assets
{
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

#define ASSET_REG_FILE_MAGIC_VALUE U32_FROM_BYTES('a', 'a', 'f', 'r')
#define ASSET_DATA_FILE_MAGIC_VALUE U32_FROM_BYTES('a', 'a', 'f', 'd')

#pragma pack(push, 1)
struct Asset_Reg_File_Header
{
    U32 magic_value;
    
    U32 version;
    
    U32 data_file_count;
    U32 tag_count;
    U32 asset_count;
    
    U32 one_past_end_of_data_file_section;
    U32 one_past_end_of_tag_table_section;
};

StaticAssert(ASSET_MAX_PER_ASSET_TAG_COUNT && ASSET_MAX_PER_ASSET_TAG_COUNT % 4 == 0);
struct Asset_Reg_File_Asset_Entry
{
    // IMPORTANT(soimn): Remember to update the asset parser when something in this struct is altered
    U32 source_file_id;
    U32 offset;
    U32 size;
    Enum8(ASSET_TYPE) type;
    U8 pad_[3];
    U16 tags[ASSET_MAX_PER_ASSET_TAG_COUNT];
    
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
VERSION
DATA_FILE_COUNT
TAG_COUNT
ASSET_COUNT
OFFSET_TO_ONE_PAST_THE_END_OF_DATA_FILE_SECTION
OFFSET_TO_ONE_PAST_THE_END_OF_TAG_TABLE_SECTION

#FILE_TABLE
BASE_PATH
-- RELATIVE_PATH

#TAG_TABLE
    -- TAG_NAME - PRECEDENCE
    
#ASSET_TABLE
-- FILE_ID - OFFSET - SIZE - TYPE - PAD - TAGS - TYPE_SPECIFIC_METADATA
*/