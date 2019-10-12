#include "win32_ant.h"

#include "renderer/win32_renderer.cpp"

global bool QuitRequested;
global bool Pause;
global I64 GlobalPerformanceCounterFreq;

/// Logging

PLATFORM_LOG_FUNCTION(Win32Log)
{
    char buffer[1024] = {};
    
    char module[14] = {};
    
    if ((log_options & Log_Platform) && !(log_options & Log_Renderer))
    {
        String platform_string = CONST_STRING("[PLATFORM] ");
        Copy(platform_string.data, module, platform_string.size);
    }
    
    else if ((log_options & Log_Renderer) && !(log_options & Log_Platform))
    {
        String renderer_string = CONST_STRING("[RENDERER] ");
        Copy(renderer_string.data, module, renderer_string.size);
    }
    
    else
    {
        String game_string = CONST_STRING("[GAME] ");
        Copy(game_string.data, module, game_string.size);
    }
    
    va_list arg_list;
    va_start(arg_list, &message);
    
    FormatString(buffer, ARRAY_COUNT(buffer), message, arg_list);
    
    va_end(arg_list);
    
    if (log_options & Log_MessagePrompt)
    {
        U32 prompt = MB_ICONINFORMATION;
        
        if (log_options & Log_Fatal)
        {
            prompt = MB_ICONHAND;
        }
        
        else if (log_options & Log_Error)
        {
            prompt = MB_ICONEXCLAMATION;
        }
        
        else if (log_options & Log_Warning)
        {
            prompt = MB_ICONWARNING;
        }
        
        MessageBoxA(0, buffer, APPLICATION_NAME, prompt);
    }
    
    OutputDebugStringA(module);
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
}



/// Memory

PLATFORM_ALLOCATE_MEMORY_BLOCK_FUNCTION(Win32AllocateMemoryBlock)
{
    Memory_Block* new_block = 0;
    
    UMM total_size = (alignof(Memory_Block) - 1) + sizeof(Memory_Block) + block_size;
    
    void* memory = VirtualAlloc(0, total_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    Assert(memory);
    
    new_block = (Memory_Block*) Align(memory, alignof(Memory_Block));
    
    *new_block = {};
    new_block->push_ptr = Align(new_block + 1, 8);
    new_block->space    = total_size - (new_block->push_ptr - (U8*) new_block);
    
    return new_block;
}

PLATFORM_FREE_MEMORY_BLOCK_FUNCTION(Win32FreeMemoryBlock)
{
    VirtualFree((void*) block, block->space + (block->push_ptr - (U8*) block), MEM_RELEASE | MEM_DECOMMIT);
}



/// File System Interaction

#define FILE_HANDLE_IS_VALID(vfs, handle) (handle && handle  - 1 < vfs->file_count)

struct VFS_File_Name_List_Entry
{
    String base_name_with_ext;
    Buffer path;
    VFS_File_Name_List_Entry* next;
};

struct VFS_Directory_List_Entry
{
    String name;
    VFS_File_Name_List_Entry* first_file;
    VFS_Directory_List_Entry* first_subdir;
    U32 file_count;
    U32 subdir_count;
    VFS_Directory_List_Entry* next;
};

internal inline String
Win32GetNextPathPiece(String path, bool* found_slash)
{
    String piece = {};
    piece.data = (U8*)path.data;
    
    if (found_slash) *found_slash = false;
    for (U8* scan = path.data; (UMM)(scan - path.data) < path.size;
         ++scan, ++piece.size)
    {
        if (*scan == '/')
        {
            if (found_slash) *found_slash = true;
            break;
        }
    }
    
    return piece;
}

internal void
Win32AddFilesAndDirectoriesRecursively(HANDLE find_handle, WIN32_FIND_DATAW* find_data, Buffer file_path_buffer, 
                                       UMM file_path_buffer_capacity, VFS_Directory_List_Entry* parent, 
                                       Memory_Arena* memory, Memory_Arena* string_memory, U32* dir_count, U32* file_count)
{
    VFS_Directory_List_Entry* current_dir  = 0;
    VFS_File_Name_List_Entry* current_file = 0;
    
    do
    {
        if (find_data->dwFileAttributes == FILE_ATTRIBUTE_NORMAL ||
            find_data->dwFileAttributes == FILE_ATTRIBUTE_READONLY)
        {
            /// Add file as list entry
            VFS_File_Name_List_Entry* new_file = PushStruct(memory, VFS_File_Name_List_Entry);
            ZeroStruct(new_file);
            
            /// Set file path
            UMM file_name_length = 0;
            for (wchar_t* scan = find_data->cFileName; *scan; ++scan, file_name_length);
            
            UMM file_path_size = file_path_buffer.size + file_name_length - 2;
            new_file->path.data = (U8*)PushArray(string_memory, wchar_t, file_path_size + 1);
            
            Copy(file_path_buffer.data, new_file->path.data, (file_path_buffer.size - 2) * sizeof(wchar_t));
            Copy(find_data->cFileName, new_file->path.data + (file_path_buffer.size - 2), file_name_length);
            
            new_file->path.size = file_path_size;
            ((wchar_t*)new_file->path.data)[file_path_size] = 0;
            
            /// Set file name
            U32 required_name_size = WideCharToMultiByte(CP_UTF8, 0, find_data->cFileName,
                                                         -1, 0, 0, 0, 0);
            
            new_file->base_name_with_ext.data = (U8*)PushSize(string_memory, required_name_size + 1);
            new_file->base_name_with_ext.size = required_name_size;
            
            WideCharToMultiByte(CP_UTF8, 0, find_data->cFileName, -1,
                                (char*)new_file->base_name_with_ext.data, (int)new_file->base_name_with_ext.size, 0, 0);
            
            new_file->base_name_with_ext.data[new_file->base_name_with_ext.size] = 0;
            
            if (current_file)
            {
                current_file->next = new_file;
            }
            
            else
            {
                parent->first_file = new_file;
            }
            
            current_file = new_file;
        }
        
        else if (find_data->dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
        {
            UMM dir_name_length = 0;
            for (wchar_t* scan = find_data->cFileName; *scan; ++scan, dir_name_length);
            
            if (file_path_buffer.size + dir_name_length + 1 <= file_path_buffer_capacity)
            {
                /// Add directory as list entry
                VFS_Directory_List_Entry* new_dir = PushStruct(memory, VFS_Directory_List_Entry);
                ZeroStruct(new_dir);
                
                if (current_dir)
                {
                    current_dir->next = new_dir;
                }
                
                else
                {
                    parent->first_subdir = new_dir;
                }
                
                current_dir = new_dir;
                ++parent->subdir_count;
                ++*dir_count;
                
                /// Set name of directory
                U32 required_name_size = WideCharToMultiByte(CP_UTF8, 0, find_data->cFileName, -1, 0, 0, 0, 0);
                
                new_dir->name.data = (U8*)PushSize(string_memory, required_name_size + 1);
                new_dir->name.size = required_name_size;
                
                WideCharToMultiByte(CP_UTF8, 0, find_data->cFileName, -1, (char*)new_dir->name.data, (int)new_dir->name.size, 0, 0);
                
                /// Recursively add subdirectories and files
                Buffer recur_buffer = file_path_buffer;
                
                wchar_t* end_of_file_path = (wchar_t*)recur_buffer.data + recur_buffer.size - 2;
                Copy(find_data->cFileName, end_of_file_path, dir_name_length * sizeof(wchar_t));
                
                end_of_file_path[dir_name_length]     = L'\\';
                end_of_file_path[dir_name_length + 1] = L'*';
                end_of_file_path[dir_name_length + 2] = 0;
                
                WIN32_FIND_DATAW recur_find_data;
                HANDLE recur_find_handle = FindFirstFileW((wchar_t*)recur_buffer.data, &recur_find_data);
                
                if (recur_find_handle != INVALID_HANDLE_VALUE)
                {
                    Win32AddFilesAndDirectoriesRecursively(recur_find_handle, &recur_find_data, recur_buffer,
                                                           file_path_buffer_capacity, new_dir, memory,
                                                           string_memory, dir_count, file_count);
                    FindClose(recur_find_handle);
                }
                
                else if (GetLastError() != ERROR_FILE_NOT_FOUND)
                {
                    //// ERROR: Something went wrong when calling FindFirstFileW
                    INVALID_CODE_PATH;
                }
            }
            
            else
            {
                //// ERROR: Out of memory for path buffer
                INVALID_CODE_PATH;
            }
        }
    } while (FindNextFileW(find_handle, find_data));
}

internal void
Win32FlatPackDirectoryListRecursively(VFS_Directory_List_Entry* parent, VFS_Directory** push_ptr, VFS_File_Name** file_name_push_ptr, VFS_File** file_push_ptr, U32* file_index, Memory_Arena* name_arena, Memory_Arena* path_arena)
{
    VFS_Directory* first_dir = *push_ptr;
    
    VFS_Directory_List_Entry* current_list_entry = parent->first_subdir;
    
    /// Place all top level directories under parent and append all files
    for (U32 i = 0; i < parent->subdir_count; ++i)
    {
        VFS_Directory* directory = (*push_ptr)++;
        ZeroStruct(directory);
        
        VFS_File_Name_List_Entry* file_scan = current_list_entry->first_file;
        for (U32 j = 0; j < directory->file_count; ++j)
        {
            VFS_File_Name* file_name = (*file_name_push_ptr)++;
            VFS_File* file           = (*file_push_ptr)++;
            U32 current_file_index   = (*file_index)++;
            
            /// Set all values of VFS_File_Name
            file_name->base_name_with_ext.data = PushArray(name_arena, U8, file_scan->base_name_with_ext.size);
            file_name->base_name_with_ext.size = file_scan->base_name_with_ext.size;
            Copy(file_scan->base_name_with_ext.data, file_name->base_name_with_ext.data, file_scan->base_name_with_ext.size);
            
            file_name->first_two_letters[0] = file_name->base_name_with_ext.data[0];
            file_name->first_two_letters[0] = (file_name->base_name_with_ext.size
                                               ? file_name->base_name_with_ext.data[1] : 0);
            
            file_name->file_index = current_file_index;
            
            /// Set all available values of VFS_File
            file->path.data = (U8*)PushArray(path_arena, wchar_t, file_scan->path.size);
            file->path.size = file_scan->path.size;
            Copy(file_scan->path.data, file->path.data, file_scan->path.size * sizeof(wchar_t));
            
            
            file_scan = file_scan->next;
        }
        
        current_list_entry = current_list_entry->next;
    }
    
    
    /// Loop over all top level directories under parent and append subdirectories
    VFS_Directory* current_dir                   = first_dir;
    current_list_entry                           = parent->first_subdir;
    for (U32 i = 0; i < parent->subdir_count; ++i, ++current_dir)
    {
        current_dir->subdir_offset = (U16)(*push_ptr - current_dir);
        current_dir->subdir_count  = (U16)current_list_entry->subdir_count;
        
        Win32FlatPackDirectoryListRecursively(current_list_entry, push_ptr, file_name_push_ptr,
                                              file_push_ptr, file_index, name_arena, path_arena);
        
        current_list_entry = current_list_entry->next;
    }
}

PLATFORM_RELOAD_VFS_FUNCTION(Win32ReloadVFS)
{
    Assert(vfs->mounting_point_count);
    
    ClearArena(&vfs->arena);
    ClearArena(&vfs->name_arena);
    ClearArena(&vfs->path_arena);
    
    ZeroStruct(vfs);
    
    Memory_Arena tmp_memory = {};
    tmp_memory.block_size = KILOBYTES(4);
    
    Memory_Arena string_memory = {};
    string_memory.block_size = KILOBYTES(4);
    
    VFS_Directory_List_Entry* root = PushStruct(&tmp_memory, VFS_Directory_List_Entry);
    ZeroStruct(root);
    
    U32 total_dir_count  = 1;
    U32 total_file_count = 0;
    
    for (U32 i = 0; i < vfs->mounting_point_count; ++i)
    {
        VFS_Mounting_Point* mount = &vfs->mounting_points[i];
        Assert(PathIsSane(mount->mount, false));
        Assert(PathIsSane(mount->path, false));
        
        VFS_Directory_List_Entry* mounting_point = 0;
        
        /// 
        /// Adding missing directories to meet the specified mount path
        /// 
        
        if (mount->mount.size == 1)
        {
            Assert(mount->mount.data[0] == '.');
            mounting_point = root;
        }
        
        else
        {
            /// Looping over all subdirectories and checking if they 
            /// match the mount path
            VFS_Directory_List_Entry* current_directory = root;
            bool first_mismatched = false;
            
            U8* scan = mount->mount.data;
            for (; (UMM)(scan - mount->mount.data) < mount->mount.size;
                 ++scan)
            {
                String remaining_data = {};
                remaining_data.data = scan;
                remaining_data.size = mount->mount.size - (UMM)(scan - mount->mount.data); 
                
                String piece = Win32GetNextPathPiece(remaining_data, 0);
                scan += piece.size + 1;
                
                VFS_Directory_List_Entry* subdir_scan =
                    current_directory->first_subdir;
                bool found_dir = false;
                for (U32 j = 0; j < current_directory->subdir_count; ++j)
                {
                    if (StringCompare(piece, subdir_scan->name))
                    {
                        current_directory = subdir_scan;
                        found_dir = true;
                    }
                    
                    subdir_scan = subdir_scan->next;
                }
                
                if (!found_dir)
                {
                    subdir_scan =
                        current_directory->first_subdir;
                    while (subdir_scan->next) subdir_scan = subdir_scan->next;
                    
                    /// Add directory to list
                    subdir_scan->next = PushStruct(&tmp_memory, VFS_Directory_List_Entry);
                    ZeroStruct(subdir_scan->next);
                    
                    subdir_scan->next->name.data = (U8*)PushSize(&string_memory, piece.size);
                    subdir_scan->next->name.size = piece.size;
                    
                    Copy(piece.data, subdir_scan->next->name.data, piece.size);
                    
                    ++total_dir_count;
                    current_directory = subdir_scan;
                    
                    // NOTE(soimn): If the first directory in the path mismatches, then the control is transferred to another loop which adds 
                    //              the remaining directories without testing for existance.
                    if (scan == mount->mount.data)
                    {
                        first_mismatched = true;
                        
                        break;
                    }
                    
                }
            }
            
            if (first_mismatched)
            {
                for (; (UMM)(scan - mount->mount.data) < mount->mount.size;
                     ++scan)
                {
                    String remaining_data = {};
                    remaining_data.data = scan;
                    remaining_data.size = mount->mount.size - (UMM)(scan - mount->mount.data); 
                    
                    bool found_slash;
                    String piece = Win32GetNextPathPiece(remaining_data, &found_slash);
                    scan += piece.size + 1;
                    
                    /// Add directory to list
                    current_directory->next = PushStruct(&tmp_memory, VFS_Directory_List_Entry);
                    ZeroStruct(current_directory->next);
                    
                    current_directory->next->name.data = (U8*)PushSize(&string_memory, piece.size);
                    current_directory->next->name.size = piece.size;
                    
                    Copy(piece.data, current_directory->next->name.data, piece.size);
                    
                    ++total_dir_count;
                    current_directory = current_directory->next;
                    
                }
            }
            
            mounting_point = current_directory;
        }
        
        /// 
        /// Loop thorugh all subdirectories and files present in mount->path and add them to the list
        /// 
        
        /// Convert mount->path form UTF8 to UTF16
        U32 required_chars = MultiByteToWideChar(CP_UTF8, 0, (char*)mount->path.data, (int)mount->path.size, 0, 0);
        
        // TODO(soimn): Is it reasonable to assume this will be large enough?
        UMM wide_path_size = 4 * MAX_PATH;
        wchar_t* wide_path = PushArray(&string_memory, wchar_t, wide_path_size);
        
        if (MultiByteToWideChar(CP_UTF8, 0, (char*)mount->path.data, (int)mount->path.size, wide_path, required_chars))
        {
            /// Check if mount->path exists
            if (GetFileAttributesW(wide_path) == FILE_ATTRIBUTE_DIRECTORY)
            {
                /// Add all subdirectories and files
                
                WIN32_FIND_DATAW find_data;
                
                if (wide_path[required_chars - 1] == L'\\' || wide_path[required_chars - 1] == L'/')
                {
                    wide_path[required_chars]     = L'*';
                    wide_path[required_chars + 1] = 0;
                }
                
                else
                {
                    wide_path[required_chars]     = L'\\';
                    wide_path[required_chars + 1] = L'*';
                    wide_path[required_chars + 2] = 0;
                }
                
                Buffer file_path_buffer = {};
                file_path_buffer.data = (U8*)wide_path;
                file_path_buffer.size = wide_path_size;
                
                HANDLE find_handle = FindFirstFileW(wide_path, &find_data);
                
                if (find_handle != INVALID_HANDLE_VALUE)
                {
                    Win32AddFilesAndDirectoriesRecursively(find_handle, &find_data, file_path_buffer, 
                                                           wide_path_size, mounting_point, &tmp_memory, 
                                                           &string_memory, &total_dir_count, &total_file_count);
                    
                    FindClose(find_handle);
                }
                
                else if (GetLastError() != ERROR_FILE_NOT_FOUND)
                {
                    //// ERROR: FindFirstFile failed for some reason
                    INVALID_CODE_PATH;
                }
            }
            
            else
            {
                // TODO(soimn): What should be done if this is reached?
            }
        }
        
        else
        {
            //// ERROR: Failed to convert the path to UTF16
            INVALID_CODE_PATH;
        }
    }
    
    /// 
    /// Flat pack the linked list of directories
    /// 
    
    if (total_dir_count <= U16_MAX)
    {
        vfs->arena.block_size = KILOBYTES(4);
        VFS_Directory* directories = PushArray(&vfs->arena, VFS_Directory, total_dir_count);
        VFS_File_Name* file_names  = PushArray(&vfs->arena, VFS_File_Name, total_file_count);
        VFS_File* files            = PushArray(&vfs->arena, VFS_File, total_file_count);
        U32 file_index             = 0;
        
        
        /// Add root
        VFS_Directory* root_dir = directories++;
        root_dir->subdir_offset = 1;
        root_dir->subdir_count  = (U16)root->subdir_count;
        root_dir->first_file    = 0;
        root_dir->file_count    = root->file_count;
        
        /// Add files from root
        VFS_File_Name_List_Entry* file_scan = root->first_file;
        for (U32 i = 0; i < root->file_count; ++i)
        {
            VFS_File_Name* file_name = file_names++;
            VFS_File* file           = files++;
            U32 current_file_index   = file_index++;
            
            file_name->base_name_with_ext.data = PushArray(&vfs->name_arena, U8, file_scan->base_name_with_ext.size);
            file_name->base_name_with_ext.size = file_scan->base_name_with_ext.size;
            Copy(file_scan->base_name_with_ext.data, file_name->base_name_with_ext.data, file_scan->base_name_with_ext.size);
            
            file_name->first_two_letters[0] = file_name->base_name_with_ext.data[0];
            file_name->first_two_letters[0] = (file_name->base_name_with_ext.size
                                               ? file_name->base_name_with_ext.data[1] : 0);
            
            file_name->file_index = current_file_index;
            
            file->path.data = (U8*)PushArray(&vfs->path_arena, wchar_t, file_scan->path.size);
            file->path.size = file_scan->path.size;
            Copy(file_scan->path.data, file->path.data, file_scan->path.size * sizeof(wchar_t));
            
            file_scan = file_scan->next;
        }
        
        Win32FlatPackDirectoryListRecursively(root, &directories, &file_names, &files, 
                                              &file_index, &vfs->name_arena, &vfs->path_arena);
    }
    
    else
    {
        //// ERROR: Too many directories
        INVALID_CODE_PATH;
    }
    
    ClearArena(&tmp_memory);
    ClearArena(&string_memory);
}

PLATFORM_GET_FILE_FUNCTION(Win32GetFile)
{
    File_Handle result = {};
    
    if (PathIsSane(path, true) && path.size >= 3)
    {
        U8* scan = path.data;
        
        if (path.data[0] == '.' && path.data[1] == '/')
        {
            Assert(path.size >= 5);
            scan += 2;
        }
        
        
        VFS_Directory* current_directory = vfs->root;
        for (; (UMM)(scan - path.data) < path.size; ++scan)
        {
            String remaining_data = {};
            remaining_data.data = scan;
            remaining_data.size = path.size - (UMM)(scan - path.data); 
            
            bool found_slash;
            String piece = Win32GetNextPathPiece(remaining_data, &found_slash);
            
            if (found_slash)
            {
                /// Search for directory
                
                bool found_directory = false;
                for (U16 i = 0; i < current_directory->subdir_count; ++i)
                {
                    char second_char = (piece.size > 1 ? piece.data[1] : 0);
                    
                    VFS_Directory* dir_scan = current_directory + current_directory->subdir_offset + i;
                    
                    if (dir_scan->first_two_letters[0] == piece.data[0] && dir_scan->first_two_letters[1] == second_char && StringCompare(dir_scan->name, piece))
                    {
                        current_directory = dir_scan;
                        found_directory = true;
                        break;
                    }
                }
                
                if (!found_directory)
                {
                    break;
                }
                
                scan += piece.size + 1;
            }
            
            else
            {
                /// Search for file
                
                // TODO(soimn): Swap this raw loop for a binary search on the first two letters
                for (U32 i = current_directory->first_file; i < current_directory->first_file + current_directory->file_count;
                     ++i)
                {
                    VFS_File_Name* file_name = &vfs->file_names[i];
                    
                    char second_char = (piece.size > 1 ? piece.data[1] : 0);
                    
                    if (file_name->first_two_letters[0] == piece.data[0] && file_name->first_two_letters[1] == second_char && StringCompare(file_name->base_name_with_ext, piece))
                    {
                        result.value = file_name->file_index + 1;
                    }
                }
                
                break;
            }
        }
    }
    
    return result;
}

PLATFORM_OPEN_FILE_FUNCTION(Win32OpenFile)
{
    bool succeeded = false;
    
    U32 access_permissions = 0;
    U32 creation_mode      = 0;
    
    if (flags & File_OpenRead)
    {
        access_permissions |= GENERIC_READ;
        creation_mode	   = OPEN_EXISTING;
    }
    
    if (flags & File_OpenWrite)
    {
        access_permissions |= GENERIC_WRITE;
        creation_mode	   = OPEN_ALWAYS;
    }
    
    Assert(FILE_HANDLE_IS_VALID(vfs, file_handle.value));
    VFS_File* file = &vfs->files[file_handle.value - 1];
    
    if (!file->is_open)
    {
        wchar_t* file_path = (wchar_t*) file->path.data;
        
        HANDLE win32_handle = CreateFileW(file_path, access_permissions,
                                          FILE_SHARE_READ, 0,
                                          creation_mode, 0, 0);
        
        if (win32_handle != INVALID_HANDLE_VALUE)
        {
            file->platform_data = (void*) win32_handle;
            
            file->is_open = true;
            file->flags   = flags;
            succeeded     = true;
        }
    }
    
    return succeeded;
}

PLATFORM_CLOSE_FILE_FUNCTION(Win32CloseFile)
{
    Assert(FILE_HANDLE_IS_VALID(vfs, file_handle.value));
    VFS_File* file = &vfs->files[file_handle.value - 1];
    
    if (file->is_open)
    {
        CloseHandle((HANDLE) file->platform_data);
        
        file->flags   = 0;
        file->is_open = false;
    }
}

// TODO(soimn): Handle reading from BLOB / archive
PLATFORM_READ_FROM_FILE_FUNCTION(Win32ReadFromFile)
{
    bool succeeded = false;
    
    Assert(FILE_HANDLE_IS_VALID(vfs, file_handle.value));
    VFS_File* file = &vfs->files[file_handle.value - 1];
    
    Assert(size);
    
    if (file->is_open && (file->flags & File_OpenRead))
    {
        OVERLAPPED overlapped = {};
        overlapped.Offset	 = (file->offset + offset) & U32_MAX;
        overlapped.OffsetHigh = ((file->offset + offset) & ~((U64)U32_MAX)) >> 32;
        
        U32 capped_size = Max(size, file->size);
        
        U32 tmp_bytes_read = 0;
        if (ReadFile((HANDLE) file->platform_data, memory, size, (LPDWORD) &tmp_bytes_read, &overlapped) && tmp_bytes_read == capped_size)
        {
            if (bytes_read) *bytes_read = tmp_bytes_read;
            succeeded = true;
        }
    }
    
    return succeeded;
}

// TODO(soimn): Handle writing to BLOB / archive
PLATFORM_WRITE_TO_FILE_FUNCTION(Win32WriteToFile)
{
    bool succeeded = false;
    
    Assert(FILE_HANDLE_IS_VALID(vfs, file_handle.value));
    VFS_File* file = &vfs->files[file_handle.value - 1];
    
    Assert(size);
    
    if (file->is_open && (file->flags & File_OpenWrite))
    {
        OVERLAPPED overlapped = {};
        overlapped.Offset	 = (file->offset + offset) & U32_MAX;
        overlapped.OffsetHigh = ((file->offset + offset) & ~((U64)U32_MAX)) >> 32;
        
        U32 tmp_bytes_written = 0;
        if (WriteFile((HANDLE) file->platform_data, memory, size, (LPDWORD) &tmp_bytes_written, &overlapped))
        {
            if (bytes_written) *bytes_written = tmp_bytes_written;
            succeeded = true;
        }
    }
    
    return succeeded;
}

PLATFORM_GET_FILE_SIZE_FUNCTION(Win32GetFileSize)
{
    U32 result = 0;
    
    Assert(FILE_HANDLE_IS_VALID(vfs, file_handle.value));
    VFS_File* file = &vfs->files[file_handle.value - 1];
    
    result = file->size;
    
    return result;
}



/// Game Loading

internal bool
Win32ReloadGameCode(Win32_Game_Code* game_code)
{
    const wchar_t* game_code_path        = L"..\\" CONCAT(L, APPLICATION_NAME) L".dll";
    const wchar_t* loaded_game_code_path = L"..\\" CONCAT(L, APPLICATION_NAME) L"_loaded.dll";
    
    bool succeeded = false;
    
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesExW(game_code_path, GetFileExInfoStandard, &data))
    {
        FILETIME new_timestamp = data.ftLastWriteTime;
        if (!game_code->is_valid || CompareFileTime(&new_timestamp, &game_code->timestamp) == 1)
        {
            if (game_code->is_valid)
            {
                FreeLibrary(game_code->module);
                game_code->module              = 0;
                game_code->GameUpdateAndRender = 0;
            }
            
            CopyFileW(game_code_path, loaded_game_code_path, FALSE);
            HMODULE module_handle = LoadLibraryExW(loaded_game_code_path, 0, DONT_RESOLVE_DLL_REFERENCES);
            
            if (module_handle)
            {
                game_update_and_render_function* GameUpdateAndRender = (game_update_and_render_function*) GetProcAddress(module_handle, "GameUpdateAndRender");
                
                if (GameUpdateAndRender)
                {
                    game_code->timestamp           = new_timestamp;
                    game_code->module              = module_handle;
                    game_code->GameUpdateAndRender = GameUpdateAndRender;
                    
                    game_code->is_valid = true;
                    succeeded           = true;
                    
                    Win32Log(Log_Platform | Log_Info, "Successfully loaded new game code");
                }
                
                else
                {
                    Win32Log(Log_Platform | Log_Fatal | Log_MessagePrompt, "Failed to load game code entry points. Win32 error code %u", GetLastError());
                }
            }
            
            else
            {
                Win32Log(Log_Platform | Log_Fatal | Log_MessagePrompt, "Failed to load game code dll. Win32 error code: %u", GetLastError());
            }
        }
        
        else
        {
            // NOTE(soimn): Reload not necessary
            succeeded = true;
        }
    }
    
    return succeeded;
}



/// Timing

internal LARGE_INTEGER
Win32GetTimestamp()
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

internal F32
Win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    return ((F32) end.QuadPart - start.QuadPart) / ((F32) GlobalPerformanceCounterFreq);
}



/// Input Processing

internal void
Win32PrepareButtonForProcessing(Game_Button_State* old_button, Game_Button_State* new_button, F32 frame_delta)
{
    /// HACK IMPORTANT TODO(soimn): Figure out a stable way to provide this time
    F32 time_since_old = frame_delta;
    
    F32 duration_to_add = 0.0f;
    
    if (old_button->hold_duration > 0.0f)
    {
        duration_to_add = time_since_old;
    }
    
    else if (old_button->hold_duration <= 0.0f && old_button->ended_down)
    {
        duration_to_add = time_since_old / (old_button->transition_count + 1);
    }
    
    new_button->hold_duration = Max(old_button->hold_duration, 0.0f) + duration_to_add;
    
    new_button->did_cross_hold_threshold = (old_button->hold_duration < GAME_BUTTON_HOLD_THRESHOLD && new_button->hold_duration >= GAME_BUTTON_HOLD_THRESHOLD);
    new_button->ended_down = old_button->ended_down;
}

internal void
Win32ProcessDigitalButtonPress(Game_Button_State* button, bool is_down)
{
    if ((B16) is_down != button->ended_down)
    {
        if (button->hold_duration > 0.0f)
        {
            button->hold_duration *= -1.0f;
        }
        
        ++button->transition_count;
        button->ended_down = is_down;
        
        // TODO(soimn): Find out what the actuation amount should be. Average? last?
        button->actuation_amount = (F32) is_down;
    }
}

// TODO(soimn): Investigate the performance expense of this function
// NOTE(soimn): Based on https://blog.molecular-matters.com/2011/09/05/properly-handling-keyboard-input/
internal U64
Win32ProcessVirtualKeycodeAndScancode(U32 keycode, U32 scancode, U32 flags)
{
    U64 result = Key_Invalid;
    
    // NOTE(soimn): Some keys send fake VK_SHIFT codes
    U32 mapped_keycode = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
    if (keycode == VK_SHIFT && !(mapped_keycode == VK_LSHIFT || mapped_keycode == VK_RSHIFT))
    {
        keycode = 255;
    }
    
    if (keycode != 255)
    {
        // NOTE(soimn): Right and left shift
        if (keycode == VK_SHIFT)
        {
            keycode = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
        }
        
        // NOTE(soimn): Correcting esacped sequences
        else if (keycode == VK_NUMLOCK)
        {
            scancode = (MapVirtualKey(keycode, MAPVK_VK_TO_VSC) | 0x100);
        }
        
        bool e0 = ((flags & RI_KEY_E0) != 0);
        bool e1 = ((flags & RI_KEY_E1) != 0);
        
        if (e1)
        {
            // NOTE(soimn): MapVirtualKey does not function correctly when passed VK_PAUSE
            if (keycode == VK_PAUSE)
            {
                scancode = 0x45;
            }
            
            else
            {
                scancode = MapVirtualKey(keycode, MAPVK_VK_TO_VSC);
            }
        }
        
        // NOTE(soimn): Alphanumerical characters are mapped to their ASCII equivalents
        if (keycode >= 'A' && keycode <= 'Z')
        {
            result = Key_A + (keycode - 'A');
        }
        
        else if (keycode >= '0' && keycode <= '9')
        {
            result = Key_0 + (keycode - '9');
        }
        
        else
        {
            switch (keycode)
            {
                case VK_CANCEL:    result = Key_Break;       break;
                case VK_BACK:      result = Key_Backspace;   break;
                case VK_TAB:       result = Key_Tab;         break;
                case VK_PAUSE:     result = Key_Pause;       break;
                case VK_CAPITAL:   result = Key_CapsLock;    break;
                case VK_ESCAPE:    result = Key_Escape;      break;
                case VK_SPACE:     result = Key_Space;       break;
                case VK_SNAPSHOT:  result = Key_PrintScreen; break;
                case VK_LWIN:      result = Key_LSuper;      break;
                case VK_RWIN:      result = Key_RSuper;      break;
                case VK_NUMPAD0:   result = Key_KP0;         break;
                case VK_NUMPAD1:   result = Key_KP1;         break;
                case VK_NUMPAD2:   result = Key_KP2;         break;
                case VK_NUMPAD3:   result = Key_KP3;         break;
                case VK_NUMPAD4:   result = Key_KP4;         break;
                case VK_NUMPAD5:   result = Key_KP5;         break;
                case VK_NUMPAD6:   result = Key_KP6;         break;
                case VK_NUMPAD7:   result = Key_KP7;         break;
                case VK_NUMPAD8:   result = Key_KP8;         break;
                case VK_NUMPAD9:   result = Key_KP9;         break;
                case VK_MULTIPLY:  result = Key_KPMult;      break;
                case VK_ADD:       result = Key_KPPlus;      break;
                case VK_SUBTRACT:  result = Key_KPMinus;     break;
                case VK_SEPARATOR: result = Key_KPEnter;     break;
                case VK_DECIMAL:   result = Key_KPDecimal;   break;
                case VK_DIVIDE:    result = Key_KPDiv;       break;
                case VK_F1:        result = Key_F1;          break;
                case VK_F2:        result = Key_F2;          break;
                case VK_F3:        result = Key_F3;          break;
                case VK_F4:        result = Key_F4;          break;
                case VK_F5:        result = Key_F5;          break;
                case VK_F6:        result = Key_F6;          break;
                case VK_F7:        result = Key_F7;          break;
                case VK_F8:        result = Key_F8;          break;
                case VK_F9:        result = Key_F9;          break;
                case VK_F10:       result = Key_F10;         break;
                case VK_F11:       result = Key_F11;         break;
                case VK_F12:       result = Key_F12;         break;
                case VK_NUMLOCK:   result = Key_NumLock;     break;
                case VK_SCROLL:    result = Key_ScrollLock;  break;
                
                case VK_CONTROL:
                result = (e0 ? Key_RCtrl : Key_LCtrl);
                break;
                
                case VK_MENU:
                result = (e0 ? Key_RAlt : Key_LAlt);
                break;
                
                case VK_RETURN:
                result = (e0 ? Key_KPEnter : Key_Enter);
                break;
                
                case VK_INSERT:
                result = (e0 ? Key_Insert : Key_KP0);
                break;
                
                case VK_DELETE:
                result = (e0 ? Key_Delete : Key_KPDecimal);
                break;
                
                case VK_HOME:
                result = (e0 ? Key_Home : Key_KP7);
                break;
                
                case VK_END:
                result = (e0 ? Key_End : Key_KP1);
                break;
                
                case VK_PRIOR:
                result = (e0 ? Key_PgUp : Key_KP9);
                break;
                
                case VK_NEXT:
                result = (e0 ? Key_PgDn : Key_KP3);
                break;
                
                case VK_LEFT:
                result = (e0 ? Key_Left : Key_KP4);
                break;
                
                case VK_RIGHT:
                result = (e0 ? Key_Right : Key_KP6);
                break;
                
                case VK_UP:
                result = (e0 ? Key_Up : Key_KP8);
                break;
                
                case VK_DOWN:
                result = (e0 ? Key_Down : Key_KP2);
                break;
                
                case VK_CLEAR:
                result = (e0 ? Key_Invalid : Key_KP5);
                break;
                
                default:
                result = Key_Invalid;
                break;
            }
        }
    }
    
    return result;
}

/// Win32 Messaging

LRESULT CALLBACK
Win32MainWindowProc(HWND window_handle, UINT msg_code,
                    WPARAM w_param, LPARAM l_param)
{
    LRESULT result = 0;
    
    switch (msg_code)
    {
        // TODO(soimn): call games' exit handler instead of closing immediately
        case WM_CLOSE:
        QuitRequested = true;
        break;
        
        case WM_QUIT:
        QuitRequested = true;
        break;
        
        case WM_INPUT:
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        // IMPORTANT TODO(soimn): Find out why Win32MainWindowProc recieves WM_INPUT messages
#if 0
#ifdef ANT_DEBUG
        INVALID_CODE_PATH;
#else
        Win32Log(Log_Platform | Log_Fatal | Log_MessagePrompt, "Input reached Win32MainWindowProc!");
#endif
#endif
        break;
        
        default:
        result = DefWindowProcW(window_handle, msg_code, w_param, l_param);
        break;
    }
    
    return result;
}

internal void
Win32ProcessPendingMessages(HWND window_handle, Platform_Game_Input* new_input, Enum32(KEYCODE)* game_keymap, Enum32(KEYCODE)* editor_keymap, Memory_Arena* temp_memory)
{
    MSG message = {};
    
    U8* raw_input_buffer      = 0;
    U32 raw_input_buffer_size = 0;
    
    U8 keyboard_state[256] = {};
    bool keyboard_state_valid = GetKeyboardState((PBYTE) keyboard_state);
    
    bool alt_down   = (keyboard_state_valid && (keyboard_state[VK_MENU]    & 0x80) != 0);
    bool shift_down = (keyboard_state_valid && (keyboard_state[VK_SHIFT]   & 0x80) != 0);
    bool ctrl_down  = (keyboard_state_valid && (keyboard_state[VK_CONTROL] & 0x80) != 0);
    
    while (PeekMessageA(&message, window_handle, 0, 0, PM_REMOVE))
    {
        switch(message.message)
        {
            case WM_QUIT:
            QuitRequested = true;
            break;
            
            case WM_INPUT:
            {
                if (GET_RAWINPUT_CODE_WPARAM(message.wParam) == RIM_INPUT)
                {
                    U32 data_block_size = 0;
                    GetRawInputData((HRAWINPUT) message.lParam, RID_INPUT, 0, &data_block_size, sizeof(RAWINPUTHEADER));
                    
                    if (data_block_size)
                    {
                        if (data_block_size > raw_input_buffer_size)
                        {
                            raw_input_buffer = (U8*) PushSize(temp_memory, 2 * data_block_size, alignof(U64));
                        }
                        
                        GetRawInputData((HRAWINPUT) message.lParam, RID_INPUT, raw_input_buffer, &data_block_size, sizeof(RAWINPUTHEADER));
                        
                        RAWINPUT* input = (RAWINPUT*) raw_input_buffer;
                        U64 platform_keycode = Key_Invalid;
                        bool is_down         = false;
                        bool is_mouse_wheel  = false;
                        
                        if (input->header.dwType == RIM_TYPEMOUSE)
                        {
                            RAWMOUSE mouse = input->data.mouse;
                            Assert(!(mouse.usFlags & MOUSE_MOVE_ABSOLUTE && mouse.usFlags & MOUSE_VIRTUAL_DESKTOP));
                            
                            U32 flags = mouse.usButtonFlags;
                            
                            new_input->mouse_delta += Vec2((F32) mouse.lLastX, (F32) mouse.lLastY);
                            
                            switch (flags)
                            {
                                case RI_MOUSE_BUTTON_1_DOWN: platform_keycode = Key_Mouse1; is_down = true;  break;
                                case RI_MOUSE_BUTTON_1_UP:   platform_keycode = Key_Mouse1; is_down = false; break;
                                case RI_MOUSE_BUTTON_3_DOWN: platform_keycode = Key_Mouse3; is_down = true;  break;
                                case RI_MOUSE_BUTTON_3_UP:   platform_keycode = Key_Mouse3; is_down = false; break;
                                case RI_MOUSE_BUTTON_2_DOWN: platform_keycode = Key_Mouse2; is_down = true;  break;
                                case RI_MOUSE_BUTTON_2_UP:   platform_keycode = Key_Mouse2; is_down = false; break;
                                case RI_MOUSE_BUTTON_4_DOWN: platform_keycode = Key_Mouse4; is_down = true;  break;
                                case RI_MOUSE_BUTTON_4_UP:   platform_keycode = Key_Mouse4; is_down = false; break;
                                case RI_MOUSE_BUTTON_5_DOWN: platform_keycode = Key_Mouse5; is_down = true;  break;
                                case RI_MOUSE_BUTTON_5_UP:   platform_keycode = Key_Mouse5; is_down = false; break;
                                
                                case RI_MOUSE_WHEEL:
                                {
                                    is_mouse_wheel = true;
                                    
                                    if (mouse.usButtonData > 0)
                                    {
                                        platform_keycode = Key_MouseWheelUp;
                                    }
                                    
                                    else
                                    {
                                        platform_keycode = Key_MouseWheelDn;
                                    }
                                    
                                    new_input->wheel_delta += mouse.usButtonData;
                                } break;
                                
                                default:
                                platform_keycode = Key_Invalid;
                                break;
                            }
                        }
                        
                        else if (input->header.dwType == RIM_TYPEKEYBOARD)
                        {
                            RAWKEYBOARD keyboard = input->data.keyboard;
                            
                            U32 keycode  = keyboard.VKey;
                            U32 scancode = keyboard.MakeCode;
                            U32 flags    = keyboard.Flags;
                            
                            platform_keycode = Win32ProcessVirtualKeycodeAndScancode(keycode, scancode, flags);
                            is_down = !(flags & RI_KEY_BREAK);
                            
                            alt_down   = (platform_keycode == Key_LAlt   || platform_keycode == Key_RAlt   ? is_down : alt_down);
                            shift_down = (platform_keycode == Key_LShift || platform_keycode == Key_RShift ? is_down : shift_down);
                            ctrl_down  = (platform_keycode == Key_LCtrl  || platform_keycode == Key_RCtrl  ? is_down : ctrl_down);
                        }
                        
                        if (platform_keycode != Key_Invalid)
                        {
                            if (alt_down && !(platform_keycode == Key_LAlt || platform_keycode == Key_RAlt) && platform_keycode == Key_F4)
                            {
                                new_input->quit_requested = true;
                            }
                            
                            else if (new_input->editor_mode)
                            {
                                if (alt_down && platform_keycode == Key_E && !is_down)
                                {
                                    Win32Log(Log_Platform | Log_Info | Log_Verbose, "Exited editor");
                                    new_input->editor_mode = false;
                                }
                                
                                else
                                {
                                    for (U32 i = 0; i < EDITOR_BUTTON_COUNT; ++i)
                                    {
                                        if (platform_keycode == editor_keymap[i])
                                        {
                                            if (is_mouse_wheel)
                                            {
                                                // TODO(soimn): Should this be calculated from the delta?
                                                new_input->editor_buttons[i].transition_count += 2;
                                            }
                                            
                                            else
                                            {
                                                Win32ProcessDigitalButtonPress(&new_input->editor_buttons[i], is_down);
                                            }
                                        }
                                    }
                                }
                            }
                            
                            else if (alt_down && !(platform_keycode == Key_LAlt || platform_keycode == Key_RAlt))
                            {
                                if (platform_keycode == Key_P && !is_down)
                                {
                                    Win32Log(Log_Platform | Log_Info | Log_Verbose, "The game loop was %spaused", (Pause ? "un" : ""));
                                    Pause = !Pause;
                                }
                                
                                else if (platform_keycode >= Key_F1 && platform_keycode <= Key_F12)
                                {
                                    
                                }
                                
                                else if (platform_keycode == Key_E && !is_down)
                                {
                                    Win32Log(Log_Platform | Log_Info | Log_Verbose, "Entered editor");
                                    new_input->editor_mode = true;
                                }
                            }
                            
                            else
                            {
                                for (U32 i = 0; i < GAME_BUTTON_COUNT; ++i)
                                {
                                    if (platform_keycode == game_keymap[i])
                                    {
                                        if (is_mouse_wheel)
                                        {
                                            // TODO(soimn): Should this be calculated from the delta?
                                            new_input->active_controllers[0].buttons[i].transition_count += 2;
                                        }
                                        
                                        else
                                        {
                                            Win32ProcessDigitalButtonPress(&new_input->active_controllers[0].buttons[i], is_down);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                
                // NOTE(soimn): GET_RAWINPUT_CODE_WPARAM docs:
                //              "The application must call DefWindowProc so the system can perform cleanup."
                DefWindowProcW(window_handle, message.message, message.wParam, message.lParam);
            } break;
            
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            {
                // TODO(soimn): Implement fallback for RawInput
            } break;
            
            default:
            Win32MainWindowProc(window_handle, message.message, message.wParam, message.lParam);
            break;
        }
    }
}

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev_instance,
        LPSTR command_line, int window_show_mode)
{
    UNUSED_PARAMETER(prev_instance);
    UNUSED_PARAMETER(command_line);
    UNUSED_PARAMETER(window_show_mode);
    
    HWND window_handle;
    WNDCLASSEXW window_class = {};
    
    window_class.cbSize        = sizeof(WNDCLASSEX);
    window_class.style         = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    window_class.lpfnWndProc   = &Win32MainWindowProc;
    window_class.hInstance     = instance;
    window_class.hbrBackground = 0;
    window_class.lpszClassName = CONCAT(L, APPLICATION_NAME);
    
    if (RegisterClassExW(&window_class))
    {
        window_handle =
            CreateWindowExW(window_class.style,
                            window_class.lpszClassName,
                            CONCAT(L, APPLICATION_NAME),
                            WS_OVERLAPPEDWINDOW | WS_SYSMENU,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            NULL, NULL,
                            instance, NULL);
        
        if (window_handle)
        {
            Memory_Arena persistent_memory  = {};
            persistent_memory.block_size    = KILOBYTES(4);
            
            Memory_Arena renderer_memory = {};
            renderer_memory.block_size   = KILOBYTES(4);
            
            Memory_Arena frame_memory = {};
            frame_memory.block_size   = MEGABYTES(4);
            
            Game_Memory game_memory = {};
            
            game_memory.persistent_arena = &persistent_memory;
            game_memory.renderer_arena   = &renderer_memory;
            game_memory.frame_arena      = &frame_memory;
            
            game_memory.platform_api.Log                 = &Win32Log;
            game_memory.platform_api.AllocateMemoryBlock = &Win32AllocateMemoryBlock;
            game_memory.platform_api.FreeMemoryBlock     = &Win32FreeMemoryBlock;
            game_memory.platform_api.ReloadVFS           = &Win32ReloadVFS;
            game_memory.platform_api.GetFile             = &Win32GetFile;
            game_memory.platform_api.OpenFile            = &Win32OpenFile;
            game_memory.platform_api.CloseFile           = &Win32CloseFile;
            game_memory.platform_api.ReadFromFile        = &Win32ReadFromFile;
            game_memory.platform_api.WriteToFile         = &Win32WriteToFile;
            game_memory.platform_api.GetFileSize         = &Win32GetFileSize;
            
            Platform = &game_memory.platform_api;
            
            game_memory.state = PushStruct(&persistent_memory, Game_State);
            
            Win32_Game_Code game_code = {};
            
            /// Timing
            LARGE_INTEGER performance_counter_freq;
            QueryPerformanceFrequency(&performance_counter_freq);
            GlobalPerformanceCounterFreq = performance_counter_freq.QuadPart;
            
            bool encountered_errors = false;
            
            
            //// SETUP
            do
            {
                /// Set current working directory to /data
                wchar_t* buffer = (wchar_t*) PushSize(&frame_memory, MEGABYTES(4), alignof(wchar_t));
                U32 buffer_size = MEGABYTES(4) / sizeof(wchar_t);
                
                U32 result = GetModuleFileNameW(0, buffer, buffer_size - 5);
                
                encountered_errors = !result;
                if (result)
                {
                    wchar_t* last_slash = buffer;
                    for (wchar_t* scan = buffer; *scan; ++scan)
                    {
                        if (*scan == '\\' || *scan == '/')
                        {
                            last_slash = scan;
                        }
                    }
                    
                    Copy(L"data", last_slash + 1, sizeof(L"data"));
                    
                    encountered_errors = !SetCurrentDirectoryW(buffer);
                }
                
                if (encountered_errors)
                {
                    Win32Log(Log_Platform | Log_Fatal | Log_MessagePrompt, "Failed to set current working directory. Win32 error code: %u", GetLastError());
                    break;
                }
                
                /// Load Game Code
                if (!Win32ReloadGameCode(&game_code))
                {
                    encountered_errors = true;
                    break;
                }
                
                /// Register RawInput Devices
                RAWINPUTDEVICE device[2] = {};
                
                // TODO(soimn): Find out if RIDEV_NOLEGACY is worth specifying in release mode
                // Keyboard
                device[0].usUsagePage = 1;
                device[0].usUsage     = 6;
                device[0].dwFlags     = /*RIDEV_NOLEGACY | */RIDEV_NOHOTKEYS | RIDEV_DEVNOTIFY;
                
                // Mouse
                device[1].usUsagePage = 1;
                device[1].usUsage     = 2;
                device[1].dwFlags     = RIDEV_DEVNOTIFY;
                
                // TODO(soimn): Win32 message loop WM_*KEY* fallback
                if (RegisterRawInputDevices(device, 2, sizeof(RAWINPUTDEVICE)) == FALSE)
                {
                    Win32Log(Log_Platform | Log_Fatal | Log_MessagePrompt, "Failed to register RawInput devices. Win32 error code: %u", GetLastError());
                    encountered_errors = true;
                    break;
                }
                
                /// Initialize renderer
                
                Win32InitRenderer(&game_memory.platform_api, RendererAPI_None, game_memory.renderer_arena, game_memory.frame_arena);
            } while (0);
            
            
            //// GAME LOOP
            if (!encountered_errors)
            {
                bool running = true;
                Pause        = false;
                ShowWindow(window_handle, SW_SHOW);
                
                Platform_Game_Input input_buffer[2] = {};
                Platform_Game_Input* new_input      = &input_buffer[0];
                Platform_Game_Input* old_input      = &input_buffer[1];
                
                F32 frame_delta = 0.0f;
                LARGE_INTEGER last_counter = Win32GetTimestamp();
                
                while (running)
                {
                    ResetArena(&frame_memory);
                    
#ifdef ANT_ENABLE_HOT_RELOAD
                    while (!Win32ReloadGameCode(&game_code));
#endif
                    
                    { /// Prepare for input processing
                        for (U32 i = 0; i < GAME_MAX_ACTIVE_CONTROLLER_COUNT + 1; ++i)
                        {
                            Game_Controller_Input* new_controller = &new_input->active_controllers[i];
                            Game_Controller_Input* old_controller = &old_input->active_controllers[i];
                            
                            for (U32 j = 0; j < GAME_BUTTON_COUNT; ++j)
                            {
                                Game_Button_State* new_button = &new_controller->buttons[j];
                                Game_Button_State* old_button = &old_controller->buttons[j];
                                
                                Win32PrepareButtonForProcessing(old_button, new_button, frame_delta);
                            }
                        }
                        
                        for (U32 i = 0; i < EDITOR_BUTTON_COUNT; ++i)
                        {
                            Game_Button_State* new_button = &new_input->editor_buttons[i];
                            Game_Button_State* old_button = &old_input->editor_buttons[i];
                            
                            Win32PrepareButtonForProcessing(old_button, new_button, frame_delta);
                        }
                        
                        new_input->editor_mode = old_input->editor_mode;
                    }
                    
                    Win32ProcessPendingMessages(window_handle, new_input, game_memory.keyboard_game_keymap, game_memory.keyboard_editor_keymap, &frame_memory);
                    
                    if (!Pause)
                    {
                        new_input->quit_requested = new_input->quit_requested || QuitRequested;
                        game_code.GameUpdateAndRender(&game_memory, new_input);
                        
                        if (new_input->quit_requested)
                        {
                            running = false;
                        }
                    }
                    
                    
                    ResetArena(&frame_memory);
                    
                    CopyStruct(new_input, old_input);
                    ZeroStruct(new_input);
                    
                    LARGE_INTEGER new_counter = Win32GetTimestamp();
                    frame_delta = Win32GetSecondsElapsed(last_counter, new_counter);
                    last_counter = new_counter;
                }
            }
        }
        
        else
        {
            Win32Log(Log_Platform | Log_Fatal | Log_MessagePrompt, "Failed to create a window. Win32 error code: %u", GetLastError());
        }
    }
    
    else
    {
        Win32Log(Log_Platform | Log_Fatal | Log_MessagePrompt, "Failed to register window class. Win32 error code: %u", GetLastError());
    }
}