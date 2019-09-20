#pragma once

#include "ant_shared.h"

struct VFS_File
{
    void* platform_data;
    Buffer path;
    
    U32 offset;
    U32 size;
    
    bool is_open;
    Enum8(FILE_OPEN_FLAGS) flags;
};

struct VFS_File_Name
{
    String base_name_with_ext;
    U32 file_index;
    char first_two_letters[2];
};

struct VFS_Directory
{
    String name;
    
    U16 subdir_offset;
    U16 subdir_count;
    char first_two_letters[2];
    
    U32 file_count;
    U32 first_file;
};

struct VFS_Mounting_Point
{
    String mount;
    String path;
};

struct File_Handle
{
    U32 value;
};

inline bool
PathIsSane(String path, bool err_on_slash_at_end = false)
{
    bool succeeded = false;
    
    char illegal_chars[] = {'\\', '%', '*', '?', ':', '#', '|', '"', '<', '>', ';', ',', '(', ')', '&'};
    
    if (path.size)
    {
        if (!(err_on_slash_at_end && path.data[path.size - 1] == '/'))
        {
            bool encountered_illegal_chars = false;
            
            bool has_prefix = (path.size > 1 && path.data[1] == '/');
            has_prefix = (has_prefix && 
                          (path.data[0] == '.' || path.data[0] == '/'));
            
            for (U32 i = has_prefix; i < path.size; ++i)
            {
                if (path.data[i] == '/' && path.size > i + 1 && path.data[i + 1] == '/')
                {
                    encountered_illegal_chars = true;
                    break;
                }
                
                if (!((path.data[i] >= 'A' && path.data[i] <= 'Z') || (path.data[i] >= 'a' && path.data[i] <= 'z') ||
                      (path.data[i] >= '0' && path.data[i] <= '9')))
                {
                    for (U32 j = 0; j < ARRAY_COUNT(illegal_chars); ++j)
                    {
                        if (path.data[i] == illegal_chars[j])
                        {
                            encountered_illegal_chars = true;
                            break;
                        }
                    }
                    
                    if (encountered_illegal_chars)
                    {
                        break;
                    }
                }
            }
            
            succeeded = !encountered_illegal_chars;
        }
    }
    
    return succeeded;
}