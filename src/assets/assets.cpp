#include "assets/assets.h"

#include "utils/string.h"
#include "utils/error_stream.h"
#include "utils/tokenizer.h"

inline void
SortTags(Game_Assets* assets, Asset_Tag* tags)
{
    for (U32 i = 0; i < ASSET_MAX_PER_ASSET_TAG_COUNT - 1; ++i)
    {
        for (U32 j = i + 1; j < ASSET_MAX_PER_ASSET_TAG_COUNT; ++j)
        {
            bool should_swap = false;
            
            if (tags[i].value == 0 && tags[j].value != 0)
            {
                should_swap = true;
            }
            
            else
            {
                U32 value_i = 0;
                U32 value_j = 0;
                U32 precedence_i = 0;
                U32 precedence_j = 0;
                
                if (tags[i].value)
                {
                    value_i = tags[i].value - 1;
                    precedence_i = assets->tag_table[tags[i].value - 1].precedence;
                }
                
                if (tags[j].value)
                {
                    value_j = tags[j].value - 1;
                    precedence_j = assets->tag_table[tags[j].value - 1].precedence;
                }
                
                should_swap = (precedence_i < precedence_j || (precedence_i == precedence_j && value_i < value_j));
            }
            
            if (should_swap)
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
                        break;
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

// TODO(soimn): Match by the tags along with the value
inline Asset_ID
GetBestMatchingAsset(Game_Assets* assets, Enum8(ASSET_TYPE) type, Asset_Tag* passed_tags)
{
    Asset_ID result = {};
    
    Asset* first_asset = 0;
    U32 count          = 0;
    switch (type)
    {
        case Asset_Mesh:
        first_asset = assets->meshes;
        count       = assets->mesh_count;
        break;
        
        case Asset_Texture:
        first_asset = assets->textures;
        count       = assets->texture_count;
        break;
        
        INVALID_DEFAULT_CASE;
    }
    
    // TODO(soimn): Should this be moved out to the callers side?
    Asset_Tag tags[ASSET_MAX_PER_ASSET_TAG_COUNT];
    CopyArray(passed_tags, tags, ASSET_MAX_PER_ASSET_TAG_COUNT);
    
    SortTags(assets, tags);
    
    
    Asset* first_matching_asset = SearchAssetsByTag(first_asset, count, tags[0].value, 0, false);
    if (first_matching_asset)
    {
        count = (U32)(first_matching_asset - first_asset + 1);
        
        for (U32 i = 0; i < ASSET_MAX_PER_ASSET_TAG_COUNT; ++i)
        {
            Asset* last_matching_asset = SearchAssetsByTag(first_matching_asset, count, tags[i].value, (U8) i, true);
            
            if (last_matching_asset == first_matching_asset)
            {
                result.value = (U32)(first_matching_asset - assets->assets);
                break;
            }
            
            else
            {
                count = (U32)(last_matching_asset - first_matching_asset + 1);
                
                if (i == ASSET_MAX_PER_ASSET_TAG_COUNT - 1 || !tags[i].value)
                {
                    result.value = (U32)(first_matching_asset - assets->assets);
                    break;
                }
                
                else
                {
                    first_matching_asset = SearchAssetsByTag(first_matching_asset, count, tags[i + 1].value, (U8) i + 1, false);
                }
            }
        }
    }
    
    else
    {
        switch (type)
        {
            case Asset_Mesh:
            result = assets->default_mesh;
            break;
            
            case Asset_Texture:
            result = assets->default_texture;
            break;
            
            INVALID_DEFAULT_CASE;
        }
    }
    
    return result;
}

inline Asset*
GetAsset(Game_Assets* assets, Asset_ID id)
{
    Assert(id.value < assets->asset_count);
    return assets->assets + id.value;
}

// NOTE(soimn): There are no overflow checks on data file, tag and asset count. Tag count is U16, others U32
inline void
ReloadAssets(Game_Assets* assets, Error_Stream* error_stream)
{
    ClearArena(&assets->arena);
    *assets = {};
    
    /// Open all asset reg files and validate the headers
    Platform_File_Group asset_reg_file_group = Platform->GetAllFilesOfTypeBegin(Platform_AssetFile);
    
    if (asset_reg_file_group.file_count)
    {
        assets->asset_files = PushArray(&assets->arena, Asset_File, asset_reg_file_group.file_count);
        Platform_File_Info* scan = asset_reg_file_group.first_file_info;
        
        for (U32 i = 0; i < asset_reg_file_group.file_count; ++i)
        {
            Platform_File_Handle file_handle = Platform->OpenFile(scan, Platform_OpenRead);
            
            if (file_handle.is_valid)
            {
                U8 header_buffer[512] = {};
                File_Error_Code read_error = Platform->ReadFromFile(file_handle, 0, ARRAY_COUNT(header_buffer), header_buffer);
                
                if (read_error > 0)
                {
                    String header     = {ARRAY_COUNT(header_buffer), (U8*) header_buffer};
                    String first_word = GetWord(&header);
                    
                    if (StringCompare(first_word, CONST_STRING("ARF")))
                    {
                        String version_tag = GetWord(&header);
                        
                        UMM divider = 0;
                        if (!FindChar(version_tag, '.', &divider))
                        {
                            divider = version_tag.size;
                        }
                        
                        I64 major_version = 0;
                        String major      = {divider, version_tag.data};
                        bool is_major_valid = ParseInt(major, 10, &major_version);
                        is_major_valid = is_major_valid && (major_version <= (I64) U16_MAX);
                        
                        I64 minor_version = 0;
                        String minor      = {version_tag.size - divider, version_tag.data + divider};
                        bool is_minor_valid = ParseInt(minor, 10, &minor_version) || divider == version_tag.size;
                        is_minor_valid = is_minor_valid && (0 <= minor_version && minor_version <= (I64) U16_MAX);
                        
                        if (is_major_valid && is_minor_valid)
                        {
                            CopyStruct(scan, &assets->asset_files[assets->asset_file_count].file_info);
                            assets->asset_files[assets->asset_file_count].version = ((U32) major_version << 15) & (U32) minor_version;
                            
                            ++assets->asset_file_count;
                        }
                        
                        else
                        {
                            //// Error: File version is invalid
                            PushError(error_stream, "The asset file \"%S\" has an invalid version tag. Expected \"MAJOR.MINOR\" got \"%S\"", scan->base_name, version_tag);
                        }
                    }
                    
                    else
                    {
                        //// Error: File is missing ARF tag
                        PushError(error_stream, "The asset file \"%S\" is missing an \"ARF\" tag.", scan->base_name);
                    }
                }
                
                else
                {
                    //// Error: Failed to read the first 512 bytes
                    PushError(error_stream, "Failed to read the first 512 bytes the asset file: \"%S\"", scan->base_name);
                }
            }
            
            else
            {
                //// Error: Failed to open the file
                PushError(error_stream, "Failed to open the asset file: \"%S\"", scan->base_name);
            }
            
            Platform->CloseFile(&file_handle);
            
            scan = scan->next;
        }
        
        Platform->GetAllFilesOfTypeEnd(&asset_reg_file_group);
        
        Memory_Arena temp_memory = {};
        temp_memory.block_size = KILOBYTES(4);
        
        void* file_buffer = PushSize(&temp_memory, MEGABYTES(512));
        
        struct Data_File_Entry
        {
            String path;
            Data_File_Entry* next;
        };
        
        struct Tag_Entry
        {
            String name;
            U32 precedence;
            Tag_Entry* next;
        };
        
        struct Asset_Entry
        {
            Asset asset;
            Asset_Entry* next;
        };
        
        Data_File_Entry* first_data_file_entry = 0;
        Tag_Entry* first_tag_entry             = 0;
        Asset_Entry* first_asset_entry         = 0;
        
        Data_File_Entry* current_data_file_entry = 0;
        Tag_Entry* current_tag_entry             = 0;
        Asset_Entry* current_asset_entry         = 0;
        
        U32 data_file_count = 0;
        U16 tag_count       = 0;
        U32 asset_count     = 0;
        
        { /// Adding default tags
            Tag_Entry* none  = PushStruct(&temp_memory, Tag_Entry);
            none->name       = CONST_STRING("none");
            none->precedence = 0;
            
            first_tag_entry   = none;
            current_tag_entry = none;
            ++tag_count;
        }
        
        { /// Adding default assets
            Asset_Entry* default_mesh    = PushStruct(&temp_memory, Asset_Entry);
            Asset_Entry* default_texture = PushStruct(&temp_memory, Asset_Entry);
            
            // TODO(soimn): Fill these
            *default_mesh    = {};
            *default_texture = {};
            
            first_asset_entry   = default_mesh;
            default_mesh->next  = default_texture;
            current_asset_entry = default_texture;
            asset_count += ASSET_TYPE_COUNT;
        }
        
        for (U32 i = 0; i < assets->asset_file_count; ++i)
        {
            bool encountered_errors = false;
            
            Data_File_Entry* first_temp_data_file_entry = 0;
            Tag_Entry* first_temp_tag_entry             = 0;
            Asset_Entry* first_temp_asset_entry         = 0;
            
            Data_File_Entry* current_temp_data_file_entry = 0;
            Tag_Entry* current_temp_tag_entry             = 0;
            Asset_Entry* current_temp_asset_entry         = 0;
            
            U32 temp_data_file_count = 0;
            U16 temp_tag_count       = 0;
            U32 temp_asset_count     = 0;
            
            Platform_File_Handle file_handle = Platform->OpenFile(&assets->asset_files[i].file_info, Platform_OpenRead);
            
            File_Error_Code read_error = Platform->ReadFromFile(file_handle, 0, MEGABYTES(512), file_buffer);
            
            if (read_error > 0 && read_error != MEGABYTES(512))
            {
                UMM file_size = read_error;
                String file_contents = {file_size, (U8*) file_buffer};
                
                // Skip header
                GetWord(&file_contents);
                GetWord(&file_contents);
                
                Tokenizer tokenizer = Tokenize(file_contents);
                
#define PushParsingError(message, ...) PushError(error_stream, "Encountered an error in the asset file: \"%S\", at line: %U. " message, assets->asset_files[i].file_info.base_name, tokenizer.line_nr, ##__VA_ARGS__)
                
                Token token = GetToken(&tokenizer);
                
                while (token.type != Token_EndOfStream)
                {
                    switch (token.type)
                    {
                        case Token_Identifier:
                        if (StringCompare(token.string, CONST_STRING("TAG")))
                        {
                            token = GetToken(&tokenizer);
                            
                            if (token.type == Token_Identifier)
                            {
                                String tag_name = token.string;
                                
                                EnforceCase(tag_name, 1);
                                
                                if (tag_name.size)
                                {
                                    if (RequireToken(&tokenizer, Token_Colon))
                                    {
                                        token = GetToken(&tokenizer);
                                        
                                        if (token.type == Token_Number && !token.is_float)
                                        {
                                            I32 tag_precedence = token.value_i32;
                                            
                                            if (RequireToken(&tokenizer, Token_Semicolon))
                                            {
                                                bool found_equal = false;
                                                Tag_Entry* tag_scan  = first_temp_tag_entry;
                                                
                                                while (tag_scan)
                                                {
                                                    if (StringCompare(tag_name, tag_scan->name))
                                                    {
                                                        found_equal = true;
                                                        break;
                                                    }
                                                    
                                                    tag_scan = tag_scan->next;
                                                }
                                                
                                                if (!found_equal)
                                                {
                                                    tag_scan = first_tag_entry;
                                                    
                                                    while (tag_scan)
                                                    {
                                                        if (StringCompare(tag_name, tag_scan->name))
                                                        {
                                                            found_equal = true;
                                                            break;
                                                        }
                                                        
                                                        tag_scan = tag_scan->next;
                                                    }
                                                }
                                                
                                                if (!found_equal)
                                                {
                                                    Tag_Entry* new_entry = PushStruct(&temp_memory, Tag_Entry);
                                                    
                                                    *new_entry = {};
                                                    new_entry->precedence = tag_precedence;
                                                    new_entry->name       = tag_name;
                                                    
                                                    if (first_temp_tag_entry)
                                                    {
                                                        current_temp_tag_entry->next = new_entry;
                                                    }
                                                    
                                                    else
                                                    {
                                                        first_temp_tag_entry = new_entry;
                                                    }
                                                    
                                                    current_temp_tag_entry = new_entry;
                                                    
                                                    ++temp_tag_count;
                                                }
                                                
                                                else
                                                {
                                                    //// ERROR
                                                    PushParsingError("The tag name \"%S\", in the asset file \"%S\", is already in use", tag_name, assets->asset_files[i].file_info.base_name);
                                                    encountered_errors = true;
                                                }
                                            }
                                            
                                            else
                                            {
                                                //// ERROR
                                                PushParsingError("Missing semicolon after tag definition");
                                                encountered_errors = true;
                                            }
                                        }
                                        
                                        else
                                        {
                                            //// ERROR
                                            PushParsingError("Invalid precedence in tag definition");
                                            encountered_errors = true;
                                        }
                                    }
                                    
                                    else
                                    {
                                        //// ERROR
                                        PushParsingError("Missing colon after tag name in tag definition");
                                        encountered_errors = true;
                                    }
                                }
                                
                                else
                                {
                                    //// ERROR
                                    PushParsingError("Empty name in tag definition");
                                    encountered_errors = true;
                                }
                            }
                            
                            else
                            {
                                //// ERROR
                                PushParsingError("Missing name after tag definition tag");
                                encountered_errors = true;
                            }
                        }
                        
                        else if (StringCompare(token.string, CONST_STRING("MESH"))
                                 || StringCompare(token.string, CONST_STRING("TEXTURE")))
                        {
                            Asset_Entry temp_entry = {};
                            U32 tag_index = 0;
                            
                            if (StringCompare(token.string, CONST_STRING("MESH")))
                            {
                                temp_entry.asset.type = Asset_Mesh;
                            }
                            
                            else
                            {
                                temp_entry.asset.type = Asset_Texture;
                            }
                            
                            if (RequireToken(&tokenizer, Token_Colon))
                            {
                                token = GetToken(&tokenizer);
                                
                                if (token.type == Token_String)
                                {
                                    String data_file_path = token.string;
                                    
                                    bool found_data_file_equal      = false;
                                    U32 data_file_index             = data_file_count;
                                    Data_File_Entry* data_file_scan = first_temp_data_file_entry;
                                    while (data_file_scan)
                                    {
                                        if (StringCompare(data_file_path, data_file_scan->path))
                                        {
                                            found_data_file_equal = true;
                                        }
                                        
                                        ++data_file_index;
                                        data_file_scan = data_file_scan->next;
                                    }
                                    
                                    if (!found_data_file_equal)
                                    {
                                        data_file_index = 0;
                                        data_file_scan  = first_data_file_entry;
                                        while (data_file_scan)
                                        {
                                            if (StringCompare(data_file_path, data_file_scan->path))
                                            {
                                                found_data_file_equal = true;
                                            }
                                            
                                            ++data_file_index;
                                            data_file_scan = data_file_scan->next;
                                        }
                                    }
                                    
                                    if (!found_data_file_equal)
                                    {
                                        Data_File_Entry* new_entry = PushStruct(&temp_memory, Data_File_Entry);
                                        
                                        *new_entry = {};
                                        new_entry->path = data_file_path;
                                        
                                        if (first_temp_data_file_entry)
                                        {
                                            current_temp_data_file_entry->next = new_entry;
                                        }
                                        
                                        else
                                        {
                                            first_temp_data_file_entry = new_entry;
                                        }
                                        
                                        current_temp_data_file_entry = new_entry;
                                        
                                        temp_entry.asset.source_file_id = temp_data_file_count++;
                                    }
                                    
                                    else
                                    {
                                        temp_entry.asset.source_file_id = data_file_index;
                                    }
                                    
                                    token = GetToken(&tokenizer);
                                    if (token.type == Token_Brace && token.value_c == '{')
                                    {
                                        token = GetToken(&tokenizer);
                                        
                                        while (!(token.type == Token_Brace && token.value_c == '}'))
                                        {
                                            if (token.type == Token_Identifier)
                                            {
                                                if (StringCompare(token.string, CONST_STRING("TAGS")))
                                                {
                                                    if (RequireToken(&tokenizer, Token_Colon))
                                                    {
                                                        while (token.type != Token_Semicolon)
                                                        {
                                                            token = GetToken(&tokenizer);
                                                            
                                                            if (token.type == Token_Identifier)
                                                            {
                                                                String tag_name = token.string;
                                                                
                                                                EnforceCase(tag_name, 1);
                                                                
                                                                token = GetToken(&tokenizer);
                                                                
                                                                if (token.type == Token_Equals || (token.type == Token_Operator && token.value_c == ',')
                                                                    || token.type == Token_Semicolon)
                                                                {
                                                                    U16 index        = (U16) tag_count;
                                                                    bool found_equal = false;
                                                                    Tag_Entry* tag_scan  = first_temp_tag_entry;
                                                                    while (tag_scan)
                                                                    {
                                                                        if (StringCompare(tag_scan->name, tag_name))
                                                                        {
                                                                            found_equal = true;
                                                                            break;
                                                                        }
                                                                        
                                                                        ++index;
                                                                        tag_scan = tag_scan->next;
                                                                    }
                                                                    
                                                                    if (!found_equal)
                                                                    {
                                                                        index = 0;
                                                                        tag_scan  = first_tag_entry;
                                                                        while (tag_scan)
                                                                        {
                                                                            if (StringCompare(tag_scan->name, tag_name))
                                                                            {
                                                                                found_equal = true;
                                                                                break;
                                                                            }
                                                                            
                                                                            ++index;
                                                                            tag_scan = tag_scan->next;
                                                                        }
                                                                    }
                                                                    
                                                                    if (found_equal)
                                                                    {
                                                                        if (token.type == Token_Equals)
                                                                        {
                                                                            token = GetToken(&tokenizer);
                                                                            
                                                                            if (token.type == Token_Number)
                                                                            {
                                                                                token = GetToken(&tokenizer);
                                                                                
                                                                                if ((token.type == Token_Operator && token.value_c == ',') || token.type == Token_Semicolon)
                                                                                {
                                                                                    temp_entry.asset.tags[tag_index++].value = index;
                                                                                    
                                                                                    // TODO(soimn): Handle tag values
                                                                                    
                                                                                    if (token.type == Token_Semicolon)
                                                                                    {
                                                                                        break;
                                                                                    }
                                                                                }
                                                                                
                                                                                else
                                                                                {
                                                                                    //// ERROR
                                                                                    PushParsingError("Missing list delimeter or semicolon");
                                                                                    encountered_errors = true;
                                                                                    break;
                                                                                }
                                                                            }
                                                                            
                                                                            else
                                                                            {
                                                                                //// ERROR
                                                                                PushParsingError("Expected a number after equal sign in tag list");
                                                                                encountered_errors = true;
                                                                                break;
                                                                            }
                                                                        }
                                                                        
                                                                        else
                                                                        {
                                                                            temp_entry.asset.tags[tag_index++].value = index;
                                                                            
                                                                            if (token.type == Token_Semicolon)
                                                                            {
                                                                                break;
                                                                            }
                                                                        }
                                                                    }
                                                                    
                                                                    else
                                                                    {
                                                                        //// ERROR
                                                                        PushParsingError("Unknown tag specified in asset tag list");
                                                                        encountered_errors = true;
                                                                        break;
                                                                    }
                                                                }
                                                                
                                                                else
                                                                {
                                                                    //// ERROR
                                                                    PushParsingError("Invalid token following tag name in asset tag list");
                                                                    encountered_errors = true;
                                                                    break;
                                                                }
                                                            }
                                                            
                                                            else
                                                            {
                                                                //// ERROR
                                                                PushParsingError("Expected tag name after TAGS tag");
                                                                encountered_errors = true;
                                                                break;
                                                            }
                                                        }
                                                    }
                                                    
                                                    else
                                                    {
                                                        //// ERROR
                                                        PushParsingError("Missing colon after TAGS tag");
                                                        encountered_errors = true;
                                                    }
                                                }
                                                
                                                else if (StringCompare(token.string, CONST_STRING("OFFSET"))
                                                         || StringCompare(token.string, CONST_STRING("SIZE")))
                                                {
                                                    bool is_offset = (token.string.data[0] == 'O');
                                                    
                                                    if (RequireToken(&tokenizer, Token_Colon))
                                                    {
                                                        token = GetToken(&tokenizer);
                                                        
                                                        if (token.type == Token_Number && !token.is_float && token.value_i32 >= 0)
                                                        {
                                                            U64 amount = (U64) token.value_i32;
                                                            
                                                            token = PeekToken(&tokenizer);
                                                            
                                                            if (token.type == Token_Semicolon)
                                                            {
                                                                if (is_offset)
                                                                {
                                                                    temp_entry.asset.offset = (U32)amount;
                                                                }
                                                                
                                                                else
                                                                {
                                                                    temp_entry.asset.size = (U32)amount;
                                                                }
                                                            }
                                                            
                                                            else if (token.type == Token_Identifier)
                                                            {
                                                                GetToken(&tokenizer);
                                                                
                                                                if (RequireToken(&tokenizer, Token_Semicolon))
                                                                {
                                                                    if (token.string.size == 2 && (token.string.data[0] == 'K' || token.string.data[0] == 'M' || token.string.data[0] == 'G') && token.string.data[1] == 'B')
                                                                    {
                                                                        char c = token.string.data[0];
                                                                        U64 multiplier = (c == 'K' ? KILOBYTES(1) : (c == 'M' ? MEGABYTES(1) : GIGABYTES(1)));
                                                                        
                                                                        U64 final_amount = amount * multiplier;
                                                                        
                                                                        if (final_amount <= U32_MAX)
                                                                        {
                                                                            if (is_offset)
                                                                            {
                                                                                temp_entry.asset.offset = (U32)final_amount;
                                                                            }
                                                                            
                                                                            else
                                                                            {
                                                                                temp_entry.asset.size = (U32)final_amount;
                                                                            }
                                                                        }
                                                                        
                                                                        else
                                                                        {
                                                                            //// ERROR
                                                                            PushParsingError("Specified %s is too large. Max value is 4GB.", (is_offset ? "offset" : "size"));
                                                                            encountered_errors = true;
                                                                        }
                                                                    }
                                                                    
                                                                    else
                                                                    {
                                                                        //// ERROR
                                                                        PushParsingError("Invalid unit");
                                                                        encountered_errors = true;
                                                                    }
                                                                }
                                                                
                                                                else
                                                                {
                                                                    //// ERROR
                                                                    PushParsingError("Expected a semicolon at the end of the line");
                                                                    encountered_errors = true;
                                                                }
                                                            }
                                                            
                                                            else
                                                            {
                                                                //// ERROR
                                                                PushParsingError("Expected a semicolon at the end of the line");
                                                                encountered_errors = true;
                                                            }
                                                        }
                                                        
                                                        else
                                                        {
                                                            //// ERROR
                                                            PushParsingError("Invalid token after \"%s\" in asset definition. Expected  a number.", (is_offset ? "OFFSET" : "SIZE"));
                                                            encountered_errors = true;
                                                        }
                                                    }
                                                    
                                                    else
                                                    {
                                                        //// ERROR
                                                        PushParsingError("Missing colon after \"%s\" in asset definition", (is_offset ? "OFFSET" : "SIZE"));
                                                        encountered_errors = true;
                                                    }
                                                }
                                                
                                                else
                                                {
                                                    //// ERROR
                                                    PushParsingError("Invalid identifier in asset definition");
                                                    encountered_errors = true;
                                                }
                                            }
                                            
                                            else
                                            {
                                                //// ERROR
                                                PushParsingError("Encountered an unknown token in asset defintion");
                                                encountered_errors = true;
                                            }
                                            
                                            if (!encountered_errors)
                                            {
                                                token = GetToken(&tokenizer);
                                            }
                                            
                                            else
                                            {
                                                break;
                                            }
                                        }
                                    }
                                    
                                    else
                                    {
                                        //// ERROR
                                        PushParsingError("Expected open brace after asset definition header");
                                        encountered_errors = true;
                                    }
                                }
                                
                                else
                                {
                                    //// ERROR
                                    PushParsingError("Missing path string after asset definition type tag");
                                    encountered_errors = true;
                                }
                            }
                            
                            else
                            {
                                //// ERROR
                                PushParsingError("Missing colon after asset definition type tag");
                                encountered_errors = true;
                            }
                            
                            if (!encountered_errors)
                            {
                                Asset_Entry* new_entry = PushStruct(&temp_memory, Asset_Entry);
                                
                                *new_entry = {};
                                new_entry->asset.reg_file_id = i;
                                new_entry->asset.type        = temp_entry.asset.type;
                                new_entry->asset.offset      = temp_entry.asset.offset;
                                new_entry->asset.size        = temp_entry.asset.size;
                                
                                CopyArray(temp_entry.asset.tags, new_entry->asset.tags, ASSET_MAX_PER_ASSET_TAG_COUNT);
                                
                                if (first_temp_asset_entry)
                                {
                                    current_temp_asset_entry->next = new_entry;
                                }
                                
                                else
                                {
                                    first_temp_asset_entry = new_entry;
                                }
                                
                                current_temp_asset_entry = new_entry;
                                
                                ++temp_asset_count;
                            }
                        }
                        
                        else
                        {
                            //// ERROR
                            PushParsingError("Encountered unkown identifier");
                            encountered_errors = true;
                        }
                        break;
                        
                        default:
                        //// ERROR
                        PushParsingError("Encountered an unkown token");
                        encountered_errors = true;
                        break;
                    }
                    
                    if (!encountered_errors)
                    {
                        token = GetToken(&tokenizer);
                    }
                    
                    else
                    {
                        break;
                    }
                }
            }
            
            else
            {
                //// Error: Failed to read contents of file
                PushError(error_stream, "Failed to read the contents of the asset file: \"%S\"", assets->asset_files[i].file_info.base_name);
                encountered_errors = true;
            }
            
            if (U16_MAX - temp_tag_count < tag_count)
            {
                //// Error: Too many tags
                PushError(error_stream, "Too many tags");
                encountered_errors = true;
            }
            
            if (!encountered_errors)
            {
                if (U32_MAX - temp_asset_count >= asset_count)
                {
                    if (first_data_file_entry)
                    {
                        current_data_file_entry->next = first_temp_data_file_entry;
                    }
                    
                    else
                    {
                        first_data_file_entry   = first_temp_data_file_entry;
                        current_data_file_entry = first_data_file_entry;
                    }
                    
                    if (current_temp_data_file_entry)
                    {
                        current_data_file_entry = current_temp_data_file_entry;
                    }
                    
                    current_tag_entry->next = first_temp_tag_entry;
                    
                    if (current_temp_tag_entry)
                    {
                        current_tag_entry = current_temp_tag_entry;
                    }
                    
                    if (first_asset_entry)
                    {
                        current_asset_entry->next = first_temp_asset_entry;
                    }
                    
                    else
                    {
                        first_asset_entry   = first_temp_asset_entry;
                        current_asset_entry = first_asset_entry;
                    }
                    
                    if (current_temp_asset_entry)
                    {
                        current_asset_entry = current_temp_asset_entry;
                    }
                    
                    data_file_count += temp_data_file_count;
                    tag_count       += temp_tag_count;
                    asset_count     += temp_asset_count;
                }
                
                else
                {
                    PushError(error_stream, "Too many assets");
                    encountered_errors = true;
                    break;
                }
            }
#undef PushParsingError
        }
        
        assets->data_file_count = data_file_count;
        assets->tag_count       = tag_count;
        assets->asset_count     = asset_count;
        
        if (data_file_count)
        {
            assets->data_files = PushArray(&assets->arena, String, data_file_count);
            
            Data_File_Entry* data_file_scan = first_data_file_entry;
            for (U32 i = 0; i < data_file_count; ++i)
            {
                assets->data_files[i].size = data_file_scan->path.size;
                assets->data_files[i].data = (U8*) PushSize(&assets->arena, data_file_scan->path.size);
                Copy(data_file_scan->path.data, assets->data_files[i].data, data_file_scan->path.size);
                
                data_file_scan = data_file_scan->next;
            }
        }
        
        U16* tag_diff_table = 0;
        
        if (tag_count)
        {
            assets->tag_table = PushArray(&assets->arena, Asset_Tag_Table_Entry, tag_count);
            
            Tag_Entry* tag_scan = first_tag_entry;
            for (U16 i = 0; i < tag_count; ++i)
            {
                assets->tag_table[i]           = {};
                assets->tag_table[i].name.size = tag_scan->name.size;
                assets->tag_table[i].name.data = (U8*) PushSize(&assets->arena, tag_scan->name.size);
                Copy(tag_scan->name.data, assets->tag_table[i].name.data, tag_scan->name.size);
                
                assets->tag_table[i].precedence = tag_scan->precedence;
                
                tag_scan = tag_scan->next;
            }
            
            for (U16 i = 1; i < tag_count - 1; ++i)
            {
                for (U16 j = i + 1; j < tag_count; ++j)
                {
                    if (assets->tag_table[i].name.data[0] > assets->tag_table[j].name.data[0])
                    {
                        Asset_Tag_Table_Entry temp = {};
                        CopyStruct(&assets->tag_table[i], &temp);
                        CopyStruct(&assets->tag_table[j], &assets->tag_table[i]);
                        CopyStruct(&temp, &assets->tag_table[j]);
                    }
                }
            }
            
            tag_diff_table = PushArray(&temp_memory, U16, tag_count);
            
            tag_scan = first_tag_entry;
            for (U16 i = 0; i < tag_count; ++i)
            {
                U16 new_index = 0;
                for (U16 j = 0; j < tag_count; ++j)
                {
                    if (StringCompare(tag_scan->name, assets->tag_table[j].name))
                    {
                        new_index = j;
                        break;
                    }
                }
                
                tag_diff_table[i] = new_index;
                
                tag_scan = tag_scan->next;
            }
        }
        
        if (asset_count)
        {
            assets->assets = PushArray(&assets->arena, Asset, asset_count);
            
            Asset_Entry* asset_scan = first_asset_entry;
            for (U32 i = 0; i < ASSET_TYPE_COUNT; ++i)
            {
                CopyStruct(&asset_scan->asset, &assets->assets[i]);
                asset_scan = asset_scan->next;
            }
            
            for (U32 i = ASSET_TYPE_COUNT; i < asset_count; ++i)
            {
                CopyStruct(&asset_scan->asset, &assets->assets[i]);
                
                for (U32 j = 0; j < ASSET_MAX_PER_ASSET_TAG_COUNT; ++j)
                {
                    assets->assets[i].tags[j].value = tag_diff_table[assets->assets[i].tags[j].value];
                }
                
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
                
                asset_scan = asset_scan->next;
            }
            
            for (U32 i = 0; i < assets->asset_count - (ASSET_TYPE_COUNT + 1); ++i)
            {
                for (U32 j = i + 1; j < assets->asset_count - ASSET_TYPE_COUNT; ++j)
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
            
            assets->meshes   = assets->assets + ASSET_TYPE_COUNT;
            assets->textures = assets->meshes + assets->mesh_count;
            
            SortAssetsByTags(assets, assets->meshes, assets->mesh_count);
            SortAssetsByTags(assets, assets->textures, assets->texture_count);
        }
        
        ClearArena(&temp_memory);
    }
}