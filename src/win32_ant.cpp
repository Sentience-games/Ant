#include "win32_ant.h"

global bool Running;

/// Logging

PLATFORM_LOG_FUNCTION(Win32Log)
{
    char buffer[1024] = {};
    
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
    
    else
    {
        OutputDebugStringA(buffer);
        OutputDebugStringA("\n");
    }
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

// TODO(soimn): Handle file and directory names with 0 size
internal void
Win32AddAllFilesAndDirectories(HANDLE search_handle, WIN32_FIND_DATAW* find_data, wchar_t* current_dir_path, VFS_Directory* current_dir, Memory_Arena* temp_memory, U32* total_dir_count, U32* total_file_count)
{
    VFS_Directory* previous_dir = 0;
    
    while (search_handle != INVALID_HANDLE_VALUE)
    {
        // TODO(soimn): Widen file attribute support and handling
        if (find_data->dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
        {
            if (find_data->cFileName[0] == '.' && (find_data->cFileName[1] == 0 || (find_data->cFileName[1] == '.' && find_data->cFileName[2] == 0)))
            {
                // continue
            }
            
            else
            {
                ++*total_dir_count;
                
                // TODO(soimn): Deal with duplication
                
                VFS_Directory* new_dir = PushStruct(temp_memory, VFS_Directory);
                *new_dir = {};
                
                { /// Add directory to search tree
                    if (previous_dir)
                    {
                        previous_dir->next_dir = new_dir;
                    }
                    
                    else
                    {
                        current_dir->first_subdir = new_dir;
                    }
                    
                    previous_dir = new_dir;
                    ++current_dir->subdir_count;
                }
                
                U32 name_length = 0;
                for (wchar_t* scan = find_data->cFileName; *scan; ++scan, ++name_length);
                
                { /// Set directory name
                    UMM required_size = (UMM) WideCharToMultiByte(CP_UTF8, 0, find_data->cFileName, name_length, (char*) new_dir->name.data, 0, 0, 0);
                    
                    new_dir->name.data = PushArray(temp_memory, U8, required_size);
                    new_dir->name.size = required_size;
                    
                    WideCharToMultiByte(CP_UTF8, 0, find_data->cFileName, name_length, (char*) new_dir->name.data, (U32) new_dir->name.size, 0, 0);
                    
                    new_dir->first_two_letters[0] = new_dir->name.data[0];
                    
                    if (new_dir->name.size > 1)
                        new_dir->first_two_letters[1] = new_dir->name.data[1];
                }
                
                { /// Look into directory
                    U32 dir_length = 0;
                    for (wchar_t* scan = current_dir_path; *scan; ++scan, ++dir_length);
                    
                    wchar_t* wildcard = L"\\*";
                    CopyArray((wchar_t*) find_data->cFileName, current_dir_path + dir_length, name_length);
                    Copy((wchar_t*) wildcard, (wchar_t*) current_dir_path + dir_length + name_length, sizeof(L"\\*"));
                    
                    WIN32_FIND_DATAW rec_find_data;
                    HANDLE rec_search_handle = FindFirstFileW(current_dir_path, &rec_find_data);
                    
                    current_dir_path[dir_length + name_length + 1] = 0;
                    
                    Win32AddAllFilesAndDirectories(rec_search_handle, &rec_find_data, current_dir_path, new_dir, temp_memory, total_dir_count, total_file_count);
                    
                    current_dir_path[dir_length] = 0;
                    
                    FindClose(rec_search_handle);
                }
            }
        }
        
        else if (find_data->dwFileAttributes == FILE_ATTRIBUTE_NORMAL || find_data->dwFileAttributes == FILE_ATTRIBUTE_ARCHIVE || 
                 find_data->dwFileAttributes == (FILE_ATTRIBUTE_ARCHIVE & FILE_ATTRIBUTE_READONLY))
        {
            ++*total_file_count;
            
            // TODO(soimn): Deal with duplication
            
            VFS_File* new_file = PushStruct(temp_memory, VFS_File);
            
            *new_file = {};
            
            U32 name_length = 0;
            for (wchar_t* scan = find_data->cFileName; *scan; ++scan, ++name_length);
            
            { /// Set file name
                UMM required_size = (UMM) WideCharToMultiByte(CP_UTF8, 0, find_data->cFileName, name_length, (char*) new_file->base_name_with_ext.data, 0, 0, 0);
                
                new_file->base_name_with_ext.data = PushArray(temp_memory, U8, required_size);
                new_file->base_name_with_ext.size = required_size;
                
                WideCharToMultiByte(CP_UTF8, 0, find_data->cFileName, name_length, (char*) new_file->base_name_with_ext.data, (U32) new_file->base_name_with_ext.size, 0, 0);
                
                UMM base_name_size = new_file->base_name_with_ext.size;
                for (UMM i = 0; i < new_file->base_name_with_ext.size; ++i)
                {
                    if (new_file->base_name_with_ext.data[i] == '.')
                    {
                        base_name_size = i;
                    }
                }
                
                new_file->base_name.data = new_file->base_name_with_ext.data;
                new_file->base_name.size = base_name_size;
                
                new_file->first_two_letters[0] = new_file->base_name.data[0];
                
                if (new_file->base_name.size > 1)
                    new_file->first_two_letters[1] = new_file->base_name.data[1];
            }
            
            { /// Set path
                U32 dir_length = 0;
                for (wchar_t* scan = current_dir_path; *scan; ++scan, ++dir_length);
                
                new_file->path.size = dir_length + name_length;
                new_file->path.data = (U8*) PushArray(temp_memory, wchar_t, new_file->path.size + 1);
                
                CopyArray(current_dir_path, new_file->path.data, dir_length);
                CopyArray((wchar_t*) find_data->cFileName, (wchar_t*) new_file->path.data + dir_length, name_length);
                
                ((wchar_t*)new_file->path.data)[new_file->path.size] = 0;
            }
            
            // TODO(soimn): Deal with size and offset
            new_file->size = find_data->nFileSizeLow;
            
            if (current_dir->files)
            {
                VFS_File* last_file = current_dir->files;
                while (last_file->next) last_file = last_file->next;
                
                last_file->next = new_file;
            }
            
            else
            {
                current_dir->files = new_file;
            }
            
            ++current_dir->file_count;
        }
        
        if (!FindNextFileW(search_handle, find_data))
        {
            break;
        }
    }
}

internal void
Win32FlatPackDirectories(VFS* vfs, VFS_Directory* start_dir, U32* dir_index)
{
    VFS_Directory* dir_scan = start_dir;
    VFS_Directory* prev_dir = 0;
    
    while (dir_scan)
    {
        ++*dir_index;
        
        VFS_Directory* dir = &vfs->directory_table[*dir_index - 1];
        
        *dir = *dir_scan;
        
        dir->name.data = PushArray(vfs->arena, U8, dir_scan->name.size);
        CopyArray(dir_scan->name.data, dir->name.data, dir_scan->name.size);
        
        if (prev_dir)
        {
            prev_dir->next_dir = dir;
        }
        
        prev_dir = dir;
        
        if (dir->subdir_count)
        {
            dir->first_subdir = dir + 1;
            
            Win32FlatPackDirectories(vfs, dir_scan->first_subdir, dir_index);
        }
        
        dir_scan = dir_scan->next_dir;
    }
}

// TODO(soimn): Compressed / uncompressed archives
PLATFORM_RELOAD_VFS_FUNCTION(Win32ReloadVFS)
{
    ClearArena(vfs->arena);
    
    Memory_Arena temp_memory = {};
    temp_memory.block_size   = KILOBYTES(4);
    
    wchar_t path_buffer[2 * MAX_PATH] = {};
    
    VFS_Directory* root_dir = PushStruct(&temp_memory, VFS_Directory);
    *root_dir = {};
    root_dir->name = CONST_STRING(".");
    root_dir->first_two_letters[0] = '.';
    
    U32 total_dir_count  = 0;
    U32 total_file_count = 0;
    
    B8* should_check_before_adding = PushArray(&temp_memory, B8, vfs->mounting_point_count);
    ZeroArray(should_check_before_adding, vfs->mounting_point_count);
    
    for (U32 i = 0; i < vfs->mounting_point_count; ++i)
    {
        // NOTE(soimn): Ensure that all aliases start from the root directory
        Assert(vfs->mounting_points[i].alias.size >= 1 && vfs->mounting_points[i].alias.data[0] == '.'
               && (vfs->mounting_points[i].alias.data[1] == '/' || vfs->mounting_points[i].alias.size == 1));
        
        // NOTE(soimn): Ensure that all paths are either relative or marked as absolute
        Assert(vfs->mounting_points[i].path.size >= 1 && (vfs->mounting_points[i].path.data[0] == '.' || vfs->mounting_points[i].path.data[0] == '/')
               && vfs->mounting_points[i].path.data[1] == '/');
        
        Assert(PathIsSane(vfs->mounting_points[i].alias, true) && PathIsSane(vfs->mounting_points[i].path, true));
        
        for (U32 j = i + 1; j < vfs->mounting_point_count; ++j)
        {
            String s0 = vfs->mounting_points[i].alias;
            String s1 = vfs->mounting_points[j].alias;
            
            while (s0.size && s1.size && s0.data[0] == s1.data[0])
            {
                --s0.size;
                ++s0.data;
                --s1.size;
                ++s1.data;
            }
            
            if (!s0.size || !s1.size)
            {
                should_check_before_adding[j] = true;
            }
        }
    }
    
    for (U32 i = 0; i < vfs->mounting_point_count; ++i)
    {
        U32 chars_written = MultiByteToWideChar(CP_UTF8, 0, (char*) vfs->mounting_points[i].path.data, (int) vfs->mounting_points[i].path.size, path_buffer, ARRAY_COUNT(path_buffer));
        
        U32 length = 0;
        for (wchar_t* scan = path_buffer; *scan; ++scan, ++length);
        
        wchar_t* wildcard = L"\\*";
        CopyArray((wchar_t*) wildcard, (wchar_t*) path_buffer + length, sizeof(L"\\*"));
        
        if (chars_written && chars_written == vfs->mounting_points[i].path.size)
        {
            WIN32_FIND_DATAW find_data;
            HANDLE search_handle = FindFirstFileW(path_buffer, &find_data);
            
            if (search_handle != INVALID_HANDLE_VALUE)
            {
                path_buffer[length + 1] = 0;
                
                VFS_Directory* current_dir = 0;
                
                if (StringCompare(vfs->mounting_points[i].alias, CONST_STRING(".")))
                {
                    current_dir = root_dir;
                }
                
                else
                {
                    // TODO(soimn): add directories
                }
                
                Win32AddAllFilesAndDirectories(search_handle, &find_data, path_buffer, current_dir, &temp_memory, &total_dir_count, &total_file_count);
            }
            
            else
            {
                //// ERROR
            }
            
            FindClose(search_handle);
        }
        
        else
        {
            //// ERROR
        }
    }
    
    // TODO(soimn): Loop over all essential files and add them at the appropriate places
    
    vfs->directory_table = PushArray(vfs->arena, VFS_Directory, total_dir_count);
    vfs->file_table      = PushArray(vfs->arena, VFS_File, total_file_count);
    
    vfs->file_count = total_file_count;
    
    U32 dir_index  = 0;
    U32 file_index = 0;
    
    Win32FlatPackDirectories(vfs, root_dir, &dir_index);
    
    for (U32 i = 0; i < dir_index; ++i)
    {
        VFS_File* first_file = (vfs->directory_table[i].file_count ? vfs->file_table + file_index : 0);
        
        for (VFS_File* file_iter = vfs->directory_table[i].files; file_iter; file_iter = file_iter->next)
        {
            VFS_File* file = &vfs->file_table[file_index++];
            *file = *file_iter;
            
            file->base_name_with_ext.data = PushArray(vfs->arena, U8, file_iter->base_name_with_ext.size);
            CopyArray(file_iter->base_name_with_ext.data, file->base_name_with_ext.data, file_iter->base_name_with_ext.size);
            
            file->base_name.data = file->base_name_with_ext.data;
            
            file->path.data = (U8*) PushArray(vfs->arena, wchar_t, file_iter->path.size + 1);
            CopyArray((wchar_t*) file_iter->path.data, file->path.data, file_iter->path.size);
            
            ((wchar_t*) file->path.data)[file->path.size] = 0;
            
            file->next = (file_iter->next ? file + 1 : 0);
        }
        
        vfs->directory_table[i].files = first_file;
    }
}

// TODO(soimn): Error messages, feedback and robustness
PLATFORM_GET_FILE_FUNCTION(Win32GetFile)
{
    File_Handle result = {0};
    
    if (path.size > 3) // NOTE(soimn): 3 is the minimum number of chars in a meaningful path (A.X)
    {
        if (PathIsSane(path, true))
        {
            UMM skip_count = 0;
            
            if ((path.data[0] == '.' && path.data[1] == '/'))
            {
                skip_count = 2;
            }
            
            String search_path = {path.size - skip_count, path.data + skip_count};
            VFS_Directory* current_dir = &vfs->directory_table[0];
            
            for (;;)
            {
                UMM dir_name_length = 0;
                while (search_path.size - dir_name_length && search_path.data[dir_name_length] != '/') ++dir_name_length;
                
                if (dir_name_length == search_path.size)
                {
                    String file_name = search_path;
                    
                    for (VFS_File* file_iter = current_dir->files; file_iter; file_iter = file_iter->next)
                    {
                        if (StringCompare(file_name, file_iter->base_name_with_ext))
                        {
                            result.value = (U32)(file_iter - &vfs->file_table[0]) + 1;
                            break;
                        }
                    }
                    
                    break;
                }
                
                else
                {
                    String dir_name = {dir_name_length, search_path.data};
                    
                    VFS_Directory* next_dir = 0;
                    for (VFS_Directory* dir_iter = current_dir->first_subdir; dir_iter; dir_iter = dir_iter->next_dir)
                    {
                        if (StringCompare(dir_name, dir_iter->name))
                        {
                            next_dir = dir_iter;
                            break;
                        }
                    }
                    
                    if (next_dir)
                    {
                        search_path.size -= dir_name_length + 1;
                        search_path.data += dir_name_length + 1;
                        
                        current_dir = next_dir;
                    }
                    
                    else
                    {
                        break;
                    }
                }
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
    
    Assert(file_handle.value && file_handle.value < vfs->file_count);
    VFS_File* file = &vfs->file_table[file_handle.value - 1];
    
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
    Assert(file_handle.value && file_handle.value < vfs->file_count);
    VFS_File* file = &vfs->file_table[file_handle.value - 1];
    
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
    
    Assert(file_handle.value && file_handle.value < vfs->file_count);
    VFS_File* file = &vfs->file_table[file_handle.value - 1];
    
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
    
    Assert(file_handle.value && file_handle.value < vfs->file_count);
    VFS_File* file = &vfs->file_table[file_handle.value - 1];
    
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
    
    Assert(file_handle.value && file_handle.value < vfs->file_count);
    VFS_File* file = &vfs->file_table[file_handle.value - 1];
    
    result = file->size;
    
    return result;
}

/// Game Loading

internal bool
Win32ReloadGameCodeIfNecessary(Win32_Game_Code* game_code)
{
    const wchar_t* game_code_path        = L".\\" CONCAT(L, APPLICATION_NAME) L".dll";
    const wchar_t* loaded_game_code_path = L".\\" CONCAT(L, APPLICATION_NAME) L"_loaded.dll";
    
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
                    
                    Win32Log(Log_Info, "Successfully loaded new game code");
                }
                
                else
                {
                    Win32Log(Log_Fatal | Log_MessagePrompt, "Failed to load game code entry point. Win32 error code %u", GetLastError());
                }
            }
            
            else
            {
                Win32Log(Log_Fatal | Log_MessagePrompt, "Failed to load game code dll. Win32 error code: %u", GetLastError());
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
        Running = false;
        break;
        
        case WM_QUIT:
        Running = false;
        break;
        
        default:
        result = DefWindowProcW(window_handle, msg_code, w_param, l_param);
        break;
    }
    
    return result;
}
internal void
Win32ProcessPendingMessages(HWND window_handle)
{
    MSG message = {};
    
    while (PeekMessageA(&message, window_handle, 0, 0, PM_REMOVE))
    {
        switch(message.message)
        {
            case WM_QUIT:
            Running = false;
            break;
            
            default:
            Win32MainWindowProc(window_handle, message.message, message.wParam, message.lParam);
            break;
        }
    }
}

int CALLBACK WinMain(HINSTANCE instance,
					 HINSTANCE prev_instance,
					 LPSTR command_line,
					 int window_show_mode)
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
            bool encountered_errors = false;
            
            Memory_Arena persistent_memory  = {};
            persistent_memory.block_size    = KILOBYTES(4);
            
            Memory_Arena frame_local_memory = {};
            frame_local_memory.block_size   = MEGABYTES(4);
            
            Game_Memory game_memory = {};
            
            game_memory.persistent_arena = &persistent_memory;
            game_memory.frame_arena      = &frame_local_memory;
            
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
            
            { /// Set current working directory to /data
                wchar_t* buffer = (wchar_t*) PushSize(&frame_local_memory, MEGABYTES(4), alignof(wchar_t));
                U32 buffer_size = MEGABYTES(4) / sizeof(wchar_t);
                
                U32 result = GetModuleFileNameW(0, buffer, buffer_size);
                
                U32 last_slash = 0;
                for (wchar_t* scan = buffer; *scan; ++scan)
                {
                    if (*scan == '\\' || *scan == '/')
                    {
                        last_slash = (U32)(scan - buffer);
                    }
                }
                
                buffer[last_slash] = 0;
                
                U32 error_code = GetLastError();
                if (!(result && error_code != ERROR_INSUFFICIENT_BUFFER && SetCurrentDirectoryW(buffer)))
                {
                    encountered_errors = true;
                    Win32Log(Log_Fatal | Log_MessagePrompt, "Failed to set current working directory");
                }
            }
            
            if (!encountered_errors)
            {
                Running = true;
                ShowWindow(window_handle, SW_SHOW);
                
                while (Running)
                {
                    ResetArena(&frame_local_memory);
                    Win32ProcessPendingMessages(window_handle);
                    
                    if (Win32ReloadGameCodeIfNecessary(&game_code))
                    {
                        game_code.GameUpdateAndRender(&game_memory);
                    }
                }
            }
        }
        
        else
        {
            Win32Log(Log_Fatal | Log_MessagePrompt, "Failed to create a window. Win32 error code: %u", GetLastError());
        }
    }
    
    else
    {
        Win32Log(Log_Fatal | Log_MessagePrompt, "Failed to register window class. Win32 error code: %u", GetLastError());
    }
}