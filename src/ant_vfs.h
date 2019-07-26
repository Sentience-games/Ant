#pragma once

#include "ant_shared.h"

struct VFS_File
{
    String base_name;
    String base_name_with_ext;
    
    void* platform_data;
    VFS_File* next;
    
    U32 offset;
    U32 size;
    Buffer path;
    bool is_open;
    Enum8(FILE_OPEN_FLAGS) flags;
    char first_two_letters[2];
};

struct VFS_Directory
{
    VFS_Directory* next_dir;
    VFS_Directory* first_subdir;
    VFS_File* files;
    U32 file_count;
    U32 subdir_count;
    String name;
    char first_two_letters[2];
};

struct VFS_Mounting_Point
{
    String path;
    String alias;
};

struct VFS
{
    struct Memory_Arena* arena;
    
    VFS_Mounting_Point* mounting_points;
    U32 mounting_point_count;
    
    U32 essential_file_count;
    String* essential_files;
    
    VFS_Directory* directory_table;
    
    VFS_File* file_table;
    U32 file_count;
};

struct File_Handle
{
    U32 value;
};

inline bool
PathIsSane(String path, bool err_on_slash_at_end = false)
{
    bool encountered_illegal_chars = false;
    
    char illegal_chars[] = {'\\', '%', '*', '?', ':', '#', '|', '"', '<', '>', ';', ',', '(', ')', '&'};
    
    if (path.size)
    {
        if (!(err_on_slash_at_end && path.data[path.size - 1] == '/'))
        {
            for (UMM i = 0; i < path.size; ++i)
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
    }
    
    return !encountered_illegal_chars;
}