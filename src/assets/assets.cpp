#include "assets/assets.h"

#include "utils/string.h"

inline void
SortTags(Game_Assets* assets, Asset_Tag* tags)
{
    for (U32 i = 0; i < ASSET_MAX_PER_ASSET_TAG_COUNT - 1; ++i)
    {
        for (U32 j = i + 1; j < ASSET_MAX_PER_ASSET_TAG_COUNT; ++j)
        {
            Assert(tags[i].value < assets->tag_count && tags[j].value < assets->tag_count);
            
            bool less = tags[i].value > tags[j].value;
            bool precedence_less = assets->tag_table[tags[i].value].precedence > assets->tag_table[tags[j].value].precedence;
            
            if (less || (tags[i].value == tags[j].value && precedence_less) || !tags[i].value && tags[j].value)
            {
                Asset_Tag temp = tags[i];
                tags[i] = tags[j];
                tags[j] = temp;
                j = i + 1;
            }
        }
    }
}

inline void
SortAssetsByTags(Game_Assets* assets, Asset* assets_to_sort, U32 count)
{
    for (U32 i = 0; i < count; ++i)
    {
        SortTags(assets, assets_to_sort[i].tags);
    }
    
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
                CopyStruct(&temp, &assets_to_sort[j]);
                j = i + 1;
            }
        }
    }
}

inline void
SortAssets(Game_Assets* assets)
{
    for (U32 i = 1; i < assets->asset_count; ++i)
    {
        switch (assets->assets[i].type)
        {
            case Asset_Mesh:
            ++assets->mesh_count;
            break;
            
            case Asset_Texture:
            ++assets->texture_count;
            break;
            
            INVALID_DEFAULT_CASE;
        }
    }
    
    for (U32 i = 1; i < assets->asset_count - 1; ++i)
    {
        for (U32 j = i + 1; j < assets->asset_count; ++j)
        {
            if (assets->assets[i].type > assets->assets[j].type)
            {
                Asset temp = {};
                CopyStruct(&assets->assets[i], &temp);
                CopyStruct(&assets->assets[j], &assets->assets[i]);
                CopyStruct(&temp, &assets->assets[j]);
                j = i + 1;
            }
        }
    }
    
    SortAssetsByTags(assets, assets->meshes, assets->mesh_count);
    SortAssetsByTags(assets, assets->textures, assets->texture_count);
}

inline void
SortTagTable(Game_Assets* assets)
{
    for (U32 i = 1; i < assets->tag_count - 1; ++i)
    {
        for (U32 j = i + 1; j < assets->tag_count; ++j)
        {
            if (assets->tag_table[i].name.data[0] > assets->tag_table[j].name.data[0] && assets->tag_table[i].precedence > assets->tag_table[j].precedence)
            {
                Asset_Tag_Table_Entry temp = {};
                CopyStruct(&assets->tag_table[i], &temp);
                CopyStruct(&assets->tag_table[j], &assets->tag_table[i]);
            }
        }
    }
}

inline Asset*
SearchAssetsByTag(Asset* first_asset, U32 count, U16 match_tag, U8 index, bool find_last)
{
    Asset* result = 0;
    
    if (count)
    {
        Asset* low  = first_asset;
        Asset* high = first_asset + count - 1;
        
        while (high != low)
        {
            Asset* peek = (find_last ? (UMM)(high - low + 1) / 2 + low : (UMM)(high - low) / 2 + low);
            
            U16 peek_tag = peek->tags[index].value;
            
            if (peek_tag > match_tag)
            {
                high = peek - (UMM) find_last;
            }
            
            else if (peek_tag == match_tag)
            {
                if (find_last) low = peek;
                else high = peek;
            }
            
            else
            {
                low = peek + (UMM) !find_last;
            }
        }
        
        result = (low->tags[index].value == match_tag ? low : 0);
    }
    
    return result;
}

inline Asset_List
GetBestMatchingAssets(Game_Assets* assets, Enum8(ASSET_TYPE) type, const Asset_Tag (&tags) [ASSET_MAX_PER_ASSET_TAG_COUNT])
{
    Asset_List result = {};
    
    Asset_Tag copied_tags[ASSET_MAX_PER_ASSET_TAG_COUNT] = {};
    CopyArray(tags, copied_tags, ASSET_MAX_PER_ASSET_TAG_COUNT);
    SortTags(assets, copied_tags);
    
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
        
        INVALID_DEFAULT_CASE;
    }
    
    Asset* first_matching_asset = 0;
    if (copied_tags[0].value) first_matching_asset = SearchAssetsByTag(first_asset, count, copied_tags[0].value, 0, false);
    
    if (first_matching_asset)
    {
        count -= (U32)(first_matching_asset - first_asset);
        
        Asset* last_matching_asset = SearchAssetsByTag(first_matching_asset, count, copied_tags[0].value, 0, true);
        
        if (first_matching_asset == last_matching_asset)
        {
            result.first_asset = first_matching_asset;
            result.count = 1;
        }
        
        else
        {
            count = (U32)(last_matching_asset - first_matching_asset) + 1;
            first_asset = first_matching_asset;
            
            for (U8 i = 1; i < ASSET_MAX_PER_ASSET_TAG_COUNT; ++i)
            {
                first_matching_asset = 0;
                if (copied_tags[i].value) first_matching_asset = SearchAssetsByTag(first_asset, count, copied_tags[i].value, i, false);
                
                if (first_matching_asset)
                {
                    count -= (U32)(first_matching_asset - first_asset);
                    first_asset = first_matching_asset;
                    
                    last_matching_asset = SearchAssetsByTag(first_matching_asset, count, copied_tags[i].value, i, true);
                    
                    count = (U32)(last_matching_asset - first_matching_asset) + 1;
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
            
            INVALID_DEFAULT_CASE;
        }
        
        result.count = 1;
    }
    
    return result;
}

inline I32
ReloadAssets(Game_Assets* assets, Memory_Arena* asset_arena)
{
    I32 result = -1;
    
    Memory_Arena temp_memory;
    // block size
    
    { /// Open all asset reg files and validate the headers
        Platform_File_Group asset_reg_file_group = Platform->GetAllFilesOfTypeBegin(Platform_AssetFile);
        
        if (asset_reg_file_group.file_count)
        {
            Asset_File* temp_asset_reg_files = PushArray(&temp_memory, Asset_File, asset_reg_file_group.file_count);
            Platform_File_Info* scan = asset_reg_file_group.first_file_info;
            
            for (U32 i = 0; i < asset_reg_file_group.file_count; ++i)
            {
                Platform_File_Handle file_handle = Platform->OpenFile(scan, Platform_OpenRead);
                
                if (file_handle.is_valid)
                {
                    temp_asset_reg_files[i].file_info = scan;
                    
                    Asset_Reg_File_Header file_header = {};
                    
                    File_Error_Code read_error =Platform->ReadFromFile(file_handle, 0, sizeof(Asset_Reg_File_Header), &file_header);
                    
                    if (read_error == sizeof(Asset_Reg_File_Header))
                    {
                        if (file_header.magic_value == ASSET_REG_FILE_MAGIC_VALUE)
                        {
                            temp_asset_reg_files[i].data_file_count = file_header.data_file_count;
                            temp_asset_reg_files[i].tag_count       = file_header.tag_count;
                            temp_asset_reg_files[i].asset_count     = file_header.asset_count;
                            
                            temp_asset_reg_files[i].one_past_end_of_data_file_section = file_header.one_past_end_of_data_file_section;
                            temp_asset_reg_files[i].one_past_end_of_tag_table_section = file_header.one_past_end_of_tag_table_section;
                            
                            temp_asset_reg_files[i].is_valid = true;
                        }
                        
                        else if (file_header.magic_value == SwapEndianess(ASSET_REG_FILE_MAGIC_VALUE))
                        {
                            temp_asset_reg_files[i].data_file_count = SwapEndianess(file_header.data_file_count);
                            temp_asset_reg_files[i].tag_count       = SwapEndianess(file_header.tag_count);
                            temp_asset_reg_files[i].asset_count     = SwapEndianess(file_header.asset_count);
                            
                            temp_asset_reg_files[i].one_past_end_of_data_file_section = SwapEndianess(file_header.one_past_end_of_data_file_section);
                            temp_asset_reg_files[i].one_past_end_of_tag_table_section = SwapEndianess(file_header.one_past_end_of_tag_table_section);
                            
                            temp_asset_reg_files[i].wrong_endian = true;
                            temp_asset_reg_files[i].is_valid     = true;
                        }
                        
                        else
                        {
                            //// Error
                            temp_asset_reg_files[i].is_valid = false;
                        }
                    }
                    
                    else
                    {
                        //// Error
                        temp_asset_reg_files[i].is_valid = false;
                    }
                }
                
                else
                {
                    //// Error
                    temp_asset_reg_files[i].is_valid = false;
                }
                
                Platform->CloseFile(&file_handle);
                
                scan = scan->next;
            }
            
            ClearArena(asset_arena);
            *assets = {};
            
            assets->asset_file_count =  asset_reg_file_group.file_count;
            assets->asset_files      = PushArray(asset_arena, Asset_File, assets->asset_file_count);
            
            for (U32 i = 0; i < assets->asset_file_count; ++i)
            {
                Platform_File_Info* temp_file_info = assets->asset_files[i].file_info;
                assets->asset_files[i].file_info   = PushStruct(asset_arena, Platform_File_Info);
                
                CopyStruct(temp_file_info, assets->asset_files[i].file_info);
                
                UMM base_name_length = StrLength(assets->asset_files[i].file_info->base_name);
                char* temp_base_name = assets->asset_files[i].file_info->base_name;
                
                assets->asset_files[i].file_info->base_name = PushArray(asset_arena, char, base_name_length + 1);
                
                ZeroSize(assets->asset_files[i].file_info->base_name, base_name_length + 1);
                
                Copy(temp_base_name, assets->asset_files[i].file_info->base_name, base_name_length);
                
                Buffer temp_platform_data = assets->asset_files[i].file_info->platform_data;
                
                assets->asset_files[i].file_info->platform_data.data = (U8*) PushSize(asset_arena, temp_platform_data.size, MaxAlignOfPointer(temp_platform_data.data));
                
                ZeroSize(assets->asset_files[i].file_info->platform_data.data, temp_platform_data.size);
                
                Copy(temp_platform_data.data, assets->asset_files[i].file_info->platform_data.data, temp_platform_data.size);
            }
        }
        
        Platform->GetAllFilesOfTypeEnd(&asset_reg_file_group);
    }
    
    if (assets->asset_file_count)
    {
        ClearArena(&temp_memory);
        
        U32 max_data_file_count = 0;
        U32 max_tag_count       = 0;
        U32 max_asset_count     = 0;
        
        for (U32 i = 0; i < assets->asset_file_count; ++i)
        {
            if (assets->asset_files[i].is_valid)
            {
                max_data_file_count += assets->asset_files[i].data_file_count;
                max_tag_count       += assets->asset_files[i].tag_count;
                max_asset_count     += assets->asset_files[i].asset_count;
            }
        }
        
        Platform_File_Info* temp_data_files    = PushArray(&temp_memory, Platform_File_Info, max_data_file_count + 1);
        Asset_Tag_Table_Entry* temp_tag_table  = PushArray(&temp_memory, Asset_Tag_Table_Entry, max_tag_count + 1);
        Asset* temp_assets                     = PushArray(&temp_memory, Asset, max_asset_count + 1);
        
        // NOTE(soimn): The 0th member is reserved
        U32 real_data_file_count = 1;
        U32 real_tag_count       = 1;
        U32 real_asset_count     = 1; // TODO(soimn): Find out how to assign default assets to erroneous ones
        
        void* file_buffer = PushSize(&temp_memory, MEGABYTES(128));
        
        // NOTE(soimn): Up until this point, the allocations in the temp_memory arena had a block size equal to 
        //              the allocation size. This is set to a fixed value at this point in order to lower the heap 
        //              allocations during the following loop, as the following allocations are sometimes quite 
        //              small.
        temp_memory.block_size = 1024;
        
        for (U32 i = 0; i < assets->asset_file_count; ++i)
        {
            if (assets->asset_files[i].is_valid)
            {
                Platform_File_Handle file_handle = Platform->OpenFile(assets->asset_files[i].file_info, Platform_OpenRead);
                
                if (file_handle.is_valid)
                {
                    if (assets->asset_files[i].file_info->file_size <= MEGABYTES(128))
                    {
                        Asset_File* current_file = &assets->asset_files[i];
                        
                        File_Error_Code read_error = Platform->ReadFromFile(file_handle, sizeof(Asset_Reg_File_Header), MEGABYTES(128), file_buffer);
                        
                        if (read_error > 0)
                        {
                            String file_contents = {(UMM) read_error, (U8*) file_buffer};
                            
                            current_file->local_data_file_table = PushArray(asset_arena, U32, current_file->data_file_count);
                            current_file->local_tag_table = PushArray(asset_arena, U32, current_file->tag_count);
                            current_file->assets          = PushArray(asset_arena, U32, current_file->asset_count);
                            
                            ZeroSize(current_file->local_data_file_table, sizeof(U32) * current_file->data_file_count);
                            ZeroSize(current_file->local_tag_table, sizeof(U32) * current_file->tag_count);
                            ZeroSize(current_file->assets, sizeof(U32) * current_file->asset_count);
                            
                            U32 bytes_read = sizeof(Asset_Reg_File_Header);
                            
                            { /// Parse data file section
                                U32 index                   = 0;
                                U32 one_past_end_of_section = current_file->one_past_end_of_data_file_section;
                                
                                while (bytes_read < one_past_end_of_section)
                                {
                                    String path      = {};
                                    String base_name = {};
                                    
                                    U8* start = file_contents.data + bytes_read;
                                    U8* scan  = start;
                                    
                                    while (scan < file_contents.data + one_past_end_of_section && *scan != ';')
                                    {
                                        ++scan;
                                        ++bytes_read;
                                    }
                                    
                                    if (scan != file_contents.data + one_past_end_of_section)
                                    {
                                        String extracted_path = {(UMM)(start - scan), start};
                                        
                                        ++scan;
                                        ++bytes_read;
                                        
                                        Platform_File_Handle data_file_handle = Platform->OpenFileUTF8(extracted_path, Platform_OpenRead, &temp_memory, true);
                                        
                                        // TODO(soimn): Should this requirement be removed and a system for handling missing 
                                        //              files be impemented, such that files could be created during runtime 
                                        //              without the need for a full asset reload?
                                        if (data_file_handle.is_valid)
                                        {
                                            // NOTE(soimn): Do not swap this fetch with a manual path traversal and base name 
                                            //              extraction, as what is stored in the table are Platform_File_Handles, 
                                            //              and the content residing in the base_name and platform_data of those 
                                            //              structures are platform dependent.
                                            Platform_File_Info data_file_info = Platform->GetFileInfo(data_file_handle, &temp_memory);
                                            
                                            I64 found_equal = -1;
                                            for (U32 j = 0; j < real_data_file_count; ++j)
                                            {
                                                bool base_name_equal = StrCompare(data_file_info.base_name, temp_data_files[j].base_name);
                                                
                                                // NOTE(soimn): This StringCompare runs on a Buffer and therefore the comparison is 
                                                //              bytewise, eventhough the contained string may be UTF16.
                                                bool platform_data_equal = StringCompare(data_file_info.platform_data, temp_data_files[j].platform_data);
                                                
                                                if (base_name_equal && platform_data_equal)
                                                {
                                                    found_equal = j;
                                                    break;
                                                }
                                            }
                                            
                                            if (found_equal != -1)
                                            {
                                                current_file->local_data_file_table[index] = (U32) found_equal;
                                            }
                                            
                                            else
                                            {
                                                current_file->local_data_file_table[index] = real_data_file_count;
                                                temp_data_files[real_data_file_count++]    = data_file_info;
                                            }
                                        }
                                        
                                        Platform->CloseFile(&data_file_handle);
                                    }
                                    
                                    else
                                    {
                                        //// Error
                                    }
                                    
                                    ++index;
                                }
                            }
                            
                            { /// Parse tag table section
                                U32 index = 0;
                                U32 one_past_end_of_section = current_file->one_past_end_of_tag_table_section;
                                
                                while (bytes_read < one_past_end_of_section)
                                {
                                    U8* start = file_contents.data + bytes_read;
                                    U8* scan  = start;
                                    
                                    while (scan < file_contents.data + one_past_end_of_section && *scan != ';')
                                    {
                                        ++scan;
                                        ++bytes_read;
                                    }
                                    
                                    if (scan != file_contents.data + one_past_end_of_section)
                                    {
                                        String extracted_name = {(UMM)(scan - start), start};
                                        
                                        ++scan;
                                        ++bytes_read;
                                        
                                        start = scan;
                                        while (scan < file_contents.data + one_past_end_of_section && *scan != ';')
                                        {
                                            ++scan;
                                        }
                                        
                                        if (scan != file_contents.data + one_past_end_of_section && (UMM)(scan - start) == sizeof(U8))
                                        {
                                            bytes_read += (U32) (scan - start) + 1;
                                            
                                            U8 precedence   = *start;
                                            
                                            Asset_Tag_Table_Entry temp_entry = {};
                                            temp_entry.name        = extracted_name;
                                            temp_entry.precedence  = precedence;
                                            
                                            I64 found_equal = -1;
                                            for (U32 j = 0; j < real_tag_count; ++j)
                                            {
                                                if (StringCompare(temp_entry.name, temp_tag_table[i].name) && temp_entry.precedence == temp_tag_table[i].precedence)
                                                {
                                                    found_equal = j;
                                                }
                                            }
                                            
                                            if (found_equal != -1)
                                            {
                                                current_file->local_tag_table[index] = (U32) found_equal;
                                            }
                                            
                                            else
                                            {
                                                current_file->local_tag_table[index] = real_tag_count;
                                                temp_tag_table[real_tag_count++]     = temp_entry;
                                            }
                                        }
                                        
                                        else
                                        {
                                            //// Error
                                        }
                                    }
                                    
                                    else
                                    {
                                        //// Error
                                    }
                                    
                                    ++index;
                                }
                            }
                            
                            { /// Parse asset section
                                U32 index = 0;
                                
                                while (bytes_read < file_contents.size)
                                {
                                    U8* start = file_contents.data + bytes_read;
                                    U8* scan  = start;
                                    while (scan < file_contents.data + file_contents.size && *scan != ';')
                                    {
                                        ++scan;
                                        ++bytes_read;
                                    }
                                    
                                    ++bytes_read;
                                    
                                    if (scan < file_contents.data + file_contents.size)
                                    {
                                        Asset_Reg_File_Asset_Entry temp_entry = *(Asset_Reg_File_Asset_Entry*) start;
                                        U32 base_size = sizeof(U64) * 2 + sizeof(U16) * ASSET_MAX_PER_ASSET_TAG_COUNT;
                                        
                                        StaticAssert(sizeof(temp_entry) == sizeof(U64) * 2 + sizeof(U16) * ASSET_MAX_PER_ASSET_TAG_COUNT + MAX(sizeof(temp_entry.mesh_metadata), sizeof(temp_entry.texture_metadata)));
                                        
                                        U32 required_space = 0;
                                        switch (temp_entry.type)
                                        {
                                            case Asset_Mesh:
                                            required_space = base_size + sizeof(temp_entry.mesh_metadata);
                                            break;
                                            
                                            case Asset_Texture:
                                            required_space = base_size + sizeof(temp_entry.texture_metadata);
                                            break;
                                        }
                                        
                                        if ((U32)(scan - start) == required_space)
                                        {
                                            // NOTE(soimn): At this point the size of the terminated region is the same as the 
                                            //              required space for the type specified. This, and checking the existance 
                                            //              of the data file of each asset, is all the validity checks I can think 
                                            //              of at the moment (without changing the asset file format to a non-binary 
                                            //              format, akin to JSON). It would be nice if more checks could be done, 
                                            //              and weird errors averted, however the tell-tale signs of a parsing fault 
                                            //              is the lack of certain assets or default data assigned to said assets. 
                                            //              Therefore such a fault would hopefully not be very disruptive towards 
                                            //              development, and could most likely be resolved by making a asset reg 
                                            //              file visualizer.
                                            
                                            if (current_file->wrong_endian)
                                            {
                                                temp_entry.source_file_id = SwapEndianess(temp_entry.source_file_id);
                                                temp_entry.offset         = SwapEndianess(temp_entry.offset);
                                                temp_entry.size           = SwapEndianess(temp_entry.size);
                                                
                                                for (U32 j = 0; j < ASSET_MAX_PER_ASSET_TAG_COUNT; ++j)
                                                {
                                                    temp_entry.tags[j] = SwapEndianess(temp_entry.tags[j]);
                                                }
                                                
                                                switch (temp_entry.type)
                                                {
                                                    case Asset_Mesh:
                                                    temp_entry.mesh_metadata.triangle_list_count = SwapEndianess(temp_entry.mesh_metadata.triangle_list_count);
                                                    break;
                                                    
                                                    case Asset_Texture:
                                                    temp_entry.texture_metadata.width  = SwapEndianess(temp_entry.texture_metadata.width);
                                                    temp_entry.texture_metadata.height = SwapEndianess(temp_entry.texture_metadata.height);
                                                    break;
                                                    
                                                    INVALID_DEFAULT_CASE;
                                                }
                                            }
                                            
                                            if (temp_entry.source_file_id < current_file->data_file_count)
                                            {
                                                U32 source_file_id = current_file->local_data_file_table[temp_entry.source_file_id];
                                                
                                                if (!IsStructZero(&temp_data_files[source_file_id]))
                                                {
                                                    Asset asset = {};
                                                    
                                                    asset.reg_file_id    = i;
                                                    asset.source_file_id = temp_entry.source_file_id;
                                                    asset.offset         = temp_entry.offset;
                                                    asset.size           = temp_entry.size;
                                                    asset.type           = temp_entry.type;
                                                    asset.state          = Asset_Unloaded;
                                                    
                                                    switch (asset.type)
                                                    {
                                                        case Asset_Mesh:
                                                        asset.mesh.triangle_list_count = temp_entry.mesh_metadata.triangle_list_count;
                                                        break;
                                                        
                                                        case Asset_Texture:
                                                        asset.texture.type          = temp_entry.texture_metadata.type;
                                                        asset.texture.format        = temp_entry.texture_metadata.format;
                                                        asset.texture.u_wrapping    = temp_entry.texture_metadata.u_wrapping;
                                                        asset.texture.v_wrapping    = temp_entry.texture_metadata.v_wrapping;
                                                        asset.texture.min_filtering = temp_entry.texture_metadata.min_filtering;
                                                        asset.texture.mag_filtering = temp_entry.texture_metadata.mag_filtering;
                                                        asset.texture.width         = temp_entry.texture_metadata.width;
                                                        asset.texture.height        = temp_entry.texture_metadata.height;
                                                        asset.texture.mip_levels    = temp_entry.texture_metadata.mip_levels;
                                                        break;
                                                        
                                                        INVALID_DEFAULT_CASE;
                                                    }
                                                    
                                                    // NOTE(soimn): This translates the tags from the file local values, to the global 
                                                    //              values.
                                                    for (U32 j = 0; j < ASSET_MAX_PER_ASSET_TAG_COUNT; ++j)
                                                    {
                                                        asset.tags[j].value = (U16) current_file->local_tag_table[temp_entry.tags[j]];
                                                    }
                                                    
                                                    temp_assets[real_asset_count++] = asset;
                                                }
                                                
                                                else
                                                {
                                                    //// Error: The data file does not exist
                                                }
                                            }
                                            
                                            else
                                            {
                                                //// Error: The size of the terminated region does not equal the size required for the specified asset type
                                            }
                                        }
                                    }
                                    
                                    else
                                    {
                                        //// Error: The scan reached the end of the file without encountering a terminator
                                    }
                                    
                                    ++index;
                                }
                                
                            }
                        }
                        
                        else
                        {
                            //// Error: No data was read from the asset reg file
                            assets->asset_files[i].is_valid = false;
                        }
                    }
                    
                    else
                    {
                        //// Error: The asset reg file is too large
                        assets->asset_files[i].is_valid = false;
                    }
                }
                
                else
                {
                    //// Error: Failed to open file
                    assets->asset_files[i].is_valid = false;
                }
                
                Platform->CloseFile(&file_handle);
            }
        }
        
        assets->data_files      = PushArray(asset_arena, Platform_File_Info, real_data_file_count);
        assets->data_file_count = real_data_file_count;
        
        assets->tag_table = PushArray(asset_arena, Asset_Tag_Table_Entry, real_tag_count);
        assets->tag_count = real_tag_count;
        
        assets->assets      = PushArray(asset_arena, Asset, real_asset_count);
        assets->asset_count = real_asset_count;
        
        for (U32 i = 0; i < assets->data_file_count; ++i)
        {
            CopyStruct(&temp_data_files[i], &assets->data_files[i]);
            
            UMM base_name_length = StrLength(temp_data_files[i].base_name);
            assets->data_files[i].base_name = PushArray(asset_arena, char, base_name_length + 1);
            ZeroSize(assets->data_files[i].base_name, base_name_length + 1);
            Copy(temp_data_files[i].base_name, assets->data_files[i].base_name, base_name_length);
            
            assets->data_files[i].platform_data.size = temp_data_files[i].platform_data.size;
            assets->data_files[i].platform_data.data = (U8*) PushSize(asset_arena, temp_data_files[i].platform_data.size, MaxAlignOfPointer(temp_data_files[i].platform_data.data));
            ZeroSize(assets->data_files[i].platform_data.data, assets->data_files[i].platform_data.size);
            Copy(temp_data_files[i].platform_data.data, assets->data_files[i].platform_data.data, assets->data_files[i].platform_data.size);
            
        }
        
        for (U32 i = 0; i < assets->tag_count; ++i)
        {
            assets->tag_table[i].name.size = temp_tag_table[i].name.size;
            assets->tag_table[i].name.data = PushArray(asset_arena, U8, temp_tag_table[i].name.size);
            Copy(temp_tag_table[i].name.data, assets->tag_table[i].name.data, temp_tag_table[i].name.size);
        }
        
        for (U32 i = 0; i < assets->asset_count; ++i)
        {
            CopyStruct(&temp_assets[i], &assets->assets[i]);
        }
        
        SortTagTable(assets);
        SortAssets(assets);
    }
    
    else
    {
        //// Error: There were no asset reg files found
        result = -2;
    }
    
    
    return result;
}