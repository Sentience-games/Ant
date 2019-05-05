#include "assets/assets.h"

inline U32
SortTags(Game_Assets* assets, Asset_Tag* tags)
{
    
    U32 erroneous_index = UINT32_MAX;
    for (U32 i = 0; i < ASSET_MAX_PER_ASSET_TAG_COUNT; ++i)
    {
        if (tags[i].value >= assets->tag_count)
        {
            erroneous_index = i;
            break;
        }
    }
    
    if (erroneous_index == UINT32_MAX)
    {
        for (U32 i = 0; i < ASSET_MAX_PER_ASSET_TAG_COUNT - 1; ++i)
        {
            for (U32 j = i + 1; j < ASSET_MAX_PER_ASSET_TAG_COUNT; ++j)
            {
                if (tags[i].value > tags[j].value || (tags[i].value == tags[j].value && assets->tag_table[tags[i].value].precedence > assets->tag_table[tags[j].value].precedence) || !tags[i].value && tags[j].value)
                {
                    Asset_Tag temp = tags[i];
                    tags[i] = tags[j];
                    tags[j] = temp;
                    j = i + 1;
                }
            }
        }
    }
    
    return erroneous_index;
}

inline bool
SortAssetsByTags(Game_Assets* assets, Asset* assets_to_sort, U32 count)
{
    bool encountered_errors = false;
    
    for (U32 i = 0; i < count; ++i)
    {
        encountered_errors = (SortTags(assets, assets_to_sort[i].tags) != UINT32_MAX);
    }
    
    if (!encountered_errors)
    {
        for (U32 i = 0; i < count - 1; ++i)
        {
            for (U32 j = i + 1; j < count; ++j)
            {
                bool should_swap = false;
                
                if (assets_to_sort[i].tags[0].value > assets_to_sort[j].tags[0].value)
                {
                    should_swap = true;
                }
                
                else if (assets_to_sort[i].tags[0].value == assets_to_sort[j].tags[0].value)
                {
                    for (U32 k = 1; k < ASSET_MAX_PER_ASSET_TAG_COUNT; ++k)
                    {
                        if (assets_to_sort[i].tags[k].value > assets_to_sort[j].tags[k].value)
                        {
                            should_swap = true;
                        }
                    }
                }
                
                if (should_swap)
                {
                    Asset temp = {};
                    CopyStruct(&assets_to_sort[i], &temp);
                    CopyStruct(&assets_to_sort[j], &assets_to_sort[i]);
                    CopyStruct(&assets_to_sort[i], &temp);
                    j = i + 1;
                }
            }
        }
    }
    
    return !encountered_errors;
}

inline bool
SortAssets(Game_Assets* assets)
{
    for (U32 i = 0; i < assets->asset_count - 1; ++i)
    {
        for (U32 j = i + 1; j < assets->asset_count; ++j)
        {
            if (assets->assets[i].type > assets->assets[j].type)
            {
                Asset temp = {};
                CopyStruct(&assets->assets[i], &temp);
                CopyStruct(&assets->assets[j], &assets->assets[i]);
                CopyStruct(&assets->assets[i], &temp);
                j = i + 1;
            }
        }
    }
    
    bool encountered_errors = false;
    for (U32 i = 0; i < assets->asset_count; ++i)
    {
        Asset* first = &assets->assets[i];
        while (assets->assets[i].type == first->type) ++i;
        
        U32 count = (U32)(&assets->assets[i] - first);
        
        switch(first->type)
        {
            case Asset_Mesh:
            assets->meshes     = first;
            assets->mesh_count = count;
            break;
            
            case Asset_Texture:
            assets->textures      = first;
            assets->texture_count = count;
            break;
            
            case Asset_Material:
            assets->materials      = first;
            assets->material_count = count;
            break;
            
            case Asset_Shader:
            assets->shaders      = first;
            assets->shader_count = count;
            break;
            
            INVALID_DEFAULT_CASE;
        }
        
        encountered_errors = (SortTags(assets, assets->assets[i].tags) != UINT32_MAX);
        
        if (encountered_errors) break;
    }
    
    if (!encountered_errors)
    {
        if (assets->meshes) SortAssetsByTags(assets, assets->meshes, assets->mesh_count);
        if (assets->textures) SortAssetsByTags(assets, assets->textures, assets->texture_count);
        if (assets->materials) SortAssetsByTags(assets, assets->materials, assets->material_count);
        if (assets->shaders) SortAssetsByTags(assets, assets->shaders, assets->shader_count);
    }
    
    return !encountered_errors;
}

inline Asset*
SearchAssetsByTag(Asset* first_asset, U32 count, U16 match_tag, U8 index, bool find_last)
{
    Asset* result = 0;
    
    if (count)
    {
        Asset* low  = first_asset;
        Asset* high = first_asset + count;
        
        while (high != low)
        {
            Asset* peek = (find_last ? (Memory_Index)(high - low + 1) / 2 + low : (Memory_Index)(high - low) / 2 + low);
            
            U16 peek_tag = peek->tags[0].value;
            
            if (peek_tag > match_tag)
            {
                high = peek - (Memory_Index) find_last;
            }
            
            else if (peek_tag == match_tag)
            {
                if (find_last) low = peek;
                else high = peek;
            }
            
            else
            {
                low = peek + (Memory_Index) !find_last;
            }
        }
        
        result = (low->tags[0].value == match_tag ? low : 0);
    }
    
    return result;
}

inline Asset_List
GetBestMatchingAssets(Game_Assets* assets, Enum8(ASSET_TYPE) type, const Asset_Tag (&tags) [ASSET_MAX_PER_ASSET_TAG_COUNT])
{
    Asset_List result = {};
    
    Asset_Tag copied_tags[ASSET_MAX_PER_ASSET_TAG_COUNT] = {};
    CopyArray(tags, ASSET_MAX_PER_ASSET_TAG_COUNT, copied_tags);
    bool successfully_sorted_tags = (SortTags(assets, copied_tags) == UINT32_MAX);
    
    if (successfully_sorted_tags)
    {
        Asset* first_asset = 0;
        U32 count = 0;
        
        switch (type)
        {
            case Asset_Mesh:
            first_asset = assets->meshes;
            count = assets->mesh_count;
            break;
            
            case Asset_Texture:
            first_asset = assets->textures;
            count = assets->texture_count;
            break;
            
            case Asset_Material:
            first_asset = assets->materials;
            count = assets->material_count;
            break;
            
            case Asset_Shader:
            first_asset = assets->shaders;
            count = assets->shader_count;
            break;
            
            INVALID_DEFAULT_CASE;
        }
        
        Asset* first_matching_asset = SearchAssetsByTag(first_asset, count, copied_tags[0].value, 0, false);
        
        if (first_matching_asset)
        {
            count = (U32)(first_matching_asset - first_asset);
            
            Asset* last_matching_asset = SearchAssetsByTag(first_matching_asset, count, copied_tags[0].value, 0, true);
            
            if (first_matching_asset == last_matching_asset)
            {
                result.first_asset = first_matching_asset;
                result.count = 1;
            }
            
            else
            {
                count = (U32)(last_matching_asset - first_matching_asset);
                first_asset = first_matching_asset;
                
                for (U8 i = 1; i < ASSET_MAX_PER_ASSET_TAG_COUNT; ++i)
                {
                    first_matching_asset = SearchAssetsByTag(first_asset, count, copied_tags[i].value, i, false);
                    
                    if (first_matching_asset)
                    {
                        count = (U32)(first_matching_asset - first_asset);
                        first_asset = first_matching_asset;
                        
                        last_matching_asset = SearchAssetsByTag(first_matching_asset, count, copied_tags[i].value, i, true);
                        
                        count = (U32)(last_matching_asset - first_matching_asset);
                    }
                    
                    else
                    {
                        result.first_asset = first_asset;
                        result.count = count;
                        break;
                    }
                }
            }
        }
        
        else
        {
            switch (type)
            {
                case Asset_Mesh:
                result.first_asset = assets->default_mesh;
                break;
                
                case Asset_Texture:
                result.first_asset = assets->default_texture;
                break;
                
                case Asset_Material:
                result.first_asset = assets->default_material;
                break;
                
                case Asset_Shader:
                result.first_asset = assets->default_shader;
                break;
                
                INVALID_DEFAULT_CASE;
            }
            
            result.count = 1;
        }
    }
    
    return result;
}