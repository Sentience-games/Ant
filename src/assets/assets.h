#pragma once

#include "ant_types.h"
#include "assets/asset_tags.h"
#include "utils/memory_utils.h"

#include "assets/asset_tags.h"

struct font_glyph_info
{
	f32 ascent;
	f32 advance;
	f32 bearing_x;
	v4 dimensions;
};

struct aaf_mesh
{
    
};

struct aaf_texture
{
    
};

struct aaf_sound
{
    
};

struct aaf_font
{
	f32 max_ascent;
	f32 max_descent;
    
	aaf_texture atlas;
    
	font_glyph_info* glyph_infos;
};

enum ASSET_TYPE
{
	Asset_NOYPE,
    
	Asset_Model,
	Asset_Texture,	
	Asset_Sound,
	Asset_Font,
    
	ASSET_TYPE_COUNT
};

enum ASSET_STATE
{
	Asset_NotLoaded = 0x0,
	Asset_Queued    = 0x1,
	Asset_Loaded	= 0x2
};

struct alignas(MEMORY_64BIT_ALIGNED) asset_name_tag
{
	string name;
	enum32(AssetNameTag) tag;
};

// NOTE(soimn):
// This is a table which serves as a mapping between ids
// and qualified names at runtime. The qualified name is the
// one to be used in asset files.
global_variable asset_name_tag AssetNameTable[] =
{
	{CONST_STRING("DefaultFont"), Asset_DefaultFont}
};

// NOTE(soimn):
// Assets are contained in separate files when running in debug mode,
// and in a BLOB format when running in release mode.
// The asset files are queried at startup, and a table containing
// all asset files are created. This table contains the identifier of
// the asset file and the relative pathname from the base asset directories
// point of reference. This identifier is used when loading assets and is stored
// in each asset as an asset_file_id. This asset_file_id is generated at startup from
// a lookup in the asset file table, with the filename extracted from the
// reg-file as the search key.

#pragma pack(push, 1)
struct alignas(MEMORY_64BIT_ALIGNED) aaf_asset
{
	enum32(ASSET_TYPE) type;
	enum32(ASSET_STATE) state;
	asset_name_tag name;
	u32 asset_file_id;
	u64 offset;
	u32 size;
    
	union
	{
		aaf_mesh    mesh;
		aaf_texture texture;
		aaf_sound   sound;
		aaf_font    font;
	};
};
#pragma pack(pop)
