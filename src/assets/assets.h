#pragma once

#include "ant_types.h"
#include "assets/asset_tags.h"
#include "utils/memory_utils.h"

enum AssetFormatTag
{
	Asset_Model,
	Asset_Bitmap,
	Asset_Sound,
	Asset_Font
};

enum AssetStateTag
{
	Asset_NotLoaded = 0x0,
	Asset_Queued    = 0x1,
	Asset_Loaded	= 0x2
};

alignas(8)
struct asset_name_tag
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
alignas(8)
struct asset_entry
{
	enum32(AssetFormatTag) type;
	enum32(AssetStateTag) state;
	asset_name_tag name;
	u32 asset_file_id;
	u64 offset;
	u32 size;

	union
	{
		u32 model_id;
		u32 bitmap_id;
		u32 sound_id;
		u32 MAX_UNION_SIZE[3];
	};
};

StaticAssert(sizeof(asset_entry) == 2 * sizeof(u32) + sizeof(asset_name_tag) + sizeof(u32) + sizeof(u64) + 3 * sizeof(u64));

#define AAF_CODE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))
#define AAF_MAGIC_VALUE AAF_CODE('a', 'a', 'f', 'c')
#define AAF_VERSION 1

struct asset_reg_header
{
	u32 magic_value;
	u32 version;

	u32 asset_count;
	b32 has_external_assets;

	u32 reserved_[12];
};

struct asset_header
{
	u32 magic_value;
	u32 version;

	u32 asset_count;

	u32 reserved_[13];
};
#pragma pack(pop)
