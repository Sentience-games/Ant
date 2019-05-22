#include "win32_ant.h"

// TODO(soimn): Implement soft quit with a hook for messages.
global bool Running = false;

///
/// Memory Allocation
///

PLATFORM_ALLOCATE_MEMORY_BLOCK_FUNCTION(Win32AllocateMemoryBlock)
{
	Assert(size);
    
	UMM total_size = (alignof(Memory_Block) - 1) + sizeof(Memory_Block) + size;
    
	void* memory = VirtualAlloc(NULL, total_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Assert(memory);
    
    Memory_Block* block = (Memory_Block*) Align(memory, alignof(Memory_Block));
    block->prev = 0;
    block->next = 0;
	block->push_ptr = (U8*) Align((void*) (block + 1), 8);
	block->space    = total_size - (block->push_ptr - (U8*) block);
    
	return block;
}

PLATFORM_FREE_MEMORY_BLOCK_FUNCTION(Win32FreeMemoryBlock)
{
	Assert(block);
	VirtualFree((void*) block, block->space + (block->push_ptr - (U8*) block), MEM_RELEASE | MEM_DECOMMIT);
}


///
/// Logging
///

PLATFORM_LOG_INFO_FUNCTION(Win32LogInfo)
{
    char formatted_message[1024] = {};
    
    UMM chars_written = FormatString((char*) &formatted_message, ARRAY_COUNT(formatted_message), "[%s] [%s] %s @ %U: ", module, (is_debug ? "DEBUG" : "INFO"), function_name, line_nr);
    
    va_list arg_list;
    va_start(arg_list, format);
    
    FormatString((char*) &formatted_message + chars_written, ARRAY_COUNT(formatted_message) - chars_written, format, arg_list);
    
    va_end(arg_list);
    
	OutputDebugStringA(formatted_message);
    OutputDebugStringA("\n");
}

PLATFORM_LOG_ERROR_FUNCTION(Win32LogError)
{
	char formatted_message[1024] = {};
    
	UMM chars_written = FormatString((char*) &formatted_message, ARRAY_COUNT(formatted_message), "An error has occurred in the %s module.\nFunction: %s, line: %U\n\n", module, function_name, line_nr);
    
    va_list arg_list;
    va_start(arg_list, format);
    
    FormatString((char*) &formatted_message + chars_written, ARRAY_COUNT(formatted_message) - chars_written, format, arg_list);
    
    va_end(arg_list);
    
	MessageBoxA(NULL, formatted_message, APPLICATION_NAME, MB_ICONERROR | MB_OK);
    
	if (is_fatal)
		Running = false;
}


///
/// Raw Input
///

internal inline bool
Win32RegisterRawInputDevices(HWND window_handle)
{
	bool result = false;
    
	RAWINPUTDEVICE devices[2];
    
	devices[0].usUsagePage = 0x1;
	devices[0].usUsage	   = 0x6;
	devices[0].dwFlags	   = RIDEV_DEVNOTIFY
        | RIDEV_NOHOTKEYS
        | RIDEV_NOLEGACY;
	devices[0].hwndTarget  = window_handle;
    
	devices[1].usUsagePage = 0x1;
	devices[1].usUsage	   = 0x2;
	devices[1].dwFlags	   = RIDEV_DEVNOTIFY;
	devices[1].hwndTarget  = window_handle;
    
	if (RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE)) == FALSE)
	{
		WIN32LOG_FATAL("Failed to register raw input devices");
	}
    
	else
	{
		result = true;
	}
    
	return result;
}


///
/// File System Interaction
///

// TODO(soimn): see if this breaks when the path is long enough
internal wchar_t*
Win32GetCWD(Memory_Arena* persistent_memory, U32* result_length)
{
	wchar_t* result = NULL;
    
	// FIXME(soimn): this could span multiple memory blocks, which will fail.
	struct temporary_memory { Memory_Arena arena; };
	temporary_memory* temp_memory = BootstrapPushSize(temporary_memory, arena, KILOBYTES(32));
    
	U32 file_path_growth_amount = 256 * sizeof(wchar_t);
	U32 file_path_memory_size = file_path_growth_amount;
    
	wchar_t* file_path = (wchar_t*) PushSize(&temp_memory->arena, file_path_growth_amount);
	U32 path_length;
    
	for (;;)
	{
		path_length = GetModuleFileNameW(NULL, file_path, file_path_memory_size);
		
        // TODO(soimn): this might not be a good idea, as this function is also run in release mode
        Assert(path_length);
        
		if (path_length != file_path_memory_size)
		{
			break;
		}
        
		else
		{
			if (file_path_growth_amount >= temp_memory->arena.current_block->space)
			{
				WIN32LOG_FATAL("Failed to get the current working directory of the game, out of memory");
				return NULL;
			}
            
			else
			{
				file_path_memory_size += file_path_growth_amount;
                
				PushSize(&temp_memory->arena, file_path_memory_size);
				continue;
			}
		}
	}
    
	U32 length = 0;
	for (wchar_t* scan = file_path; *scan; ++scan)
	{	
		if (*scan == L'\\')
		{
			length = (U32)(scan - file_path);
		}
	}
    
	if (length == 0)
	{
		WIN32LOG_FATAL("Failed to get the current working directory of the game, no path separator found");
		return NULL;
	}
    
	++length;
    
	result = (wchar_t*) PushSize(persistent_memory, sizeof(wchar_t) * (length + 1));
	CopyArray(file_path, result, length);
	result[length] = (wchar_t)(NULL);
    
	*result_length = length;
    
	return result;
}


internal void
Win32BuildFullyQualifiedPath(Win32_Game_Info* game_info, const wchar_t* appendage,
							 wchar_t* buffer, U32 buffer_length)
{
	U32 length_of_cwd = 0;
	for (wchar_t* scan = (wchar_t*) game_info->cwd; *scan; ++scan)
	{
		++length_of_cwd;
	}
    
	I32 length_of_appendage = WStrLength(appendage, buffer_length);
    
	Assert(length_of_appendage != -1);
	Assert(length_of_cwd && buffer_length > (length_of_cwd + length_of_appendage));
    
	CopyArray(game_info->cwd, buffer, length_of_cwd);
	CopyArray(appendage, buffer + length_of_cwd, length_of_appendage);
};

B32
Win32GetFileLastWriteTime(const wchar_t* file_path, FILETIME* filetime)
{
    B32 succeeded = false;
    
	WIN32_FILE_ATTRIBUTE_DATA data;
	succeeded = GetFileAttributesExW(file_path, GetFileExInfoStandard, &data);
    
    if (succeeded)
	{
        *filetime = data.ftLastWriteTime;
	}
    
    return succeeded;
}

PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN_FUNCTION(Win32GetAllFilesOfTypeBegin)
{
    Platform_File_Group result = {};
    
	Win32_File_Group* win32_file_group = BootstrapPushSize(Win32_File_Group, memory, KILOBYTES(1));
	result.platform_data = win32_file_group;
    
	wchar_t* directory = L"";
	wchar_t* wildcard  = L"*.*";
    
	switch(file_type)
	{
		case PlatformFileType_AssetFile:
        directory = L"data\\";
        wildcard  = L"data\\*.aaf";
		break;
        
		INVALID_DEFAULT_CASE;
	}
    
	U32 directory_length = 0;
    
	for (wchar_t* it = directory; *it; ++it)
	{
		++directory_length;
	}
    
	WIN32_FIND_DATAW find_data;
	HANDLE find_handle = FindFirstFileW(wildcard, &find_data);
	while(find_handle != INVALID_HANDLE_VALUE)
	{
		++result.file_count;
		Platform_File_Info* info = PushStruct(&win32_file_group->memory, Platform_File_Info);
		info->next = result.first_file_info;
        
		FILETIME timestamp = find_data.ftLastWriteTime;
		info->timestamp = ASSEMBLE_LARGE_INT(timestamp.dwHighDateTime, timestamp.dwLowDateTime);
		info->file_size = ASSEMBLE_LARGE_INT(find_data.nFileSizeHigh, find_data.nFileSizeLow);
        
		wchar_t* base_name_begin = find_data.cFileName;
		wchar_t* base_name_end = NULL;
		wchar_t* scan = base_name_begin;
        
		while(*scan)
		{
			if (scan[0] == L'.')
			{
				base_name_end = scan;
			}
            
			++scan;
		}
        
		if (!base_name_end)
		{
			base_name_end = scan;
		}
        
		U32 base_name_length = (U32)(base_name_end - base_name_begin);
		U32 required_base_name_storage = WideCharToMultiByte(CP_UTF8, 0,
															 base_name_begin, base_name_length,
															 info->base_name, 0,
															 0, 0);
        
		info->base_name = (char*) PushSize(&win32_file_group->memory, required_base_name_storage + 1);
        
		required_base_name_storage = WideCharToMultiByte(CP_UTF8, 0, base_name_begin, base_name_length,
														 info->base_name, required_base_name_storage, 0, 0);
        
		info->base_name[required_base_name_storage] = 0;
        
		U32 file_name_size = (U32)(scan - find_data.cFileName) + 1;
		info->platform_data = PushArray(&win32_file_group->memory, wchar_t, directory_length + file_name_size);
		CopyArray(directory, info->platform_data, directory_length);
		CopyArray(find_data.cFileName, (wchar_t*)info->platform_data + directory_length, file_name_size);
        
		result.first_file_info = info;
		
		if (!FindNextFileW(find_handle, &find_data))
		{
			break;
		}
	}
    
	FindClose(find_handle);
    
	return result;
}

PLATFORM_GET_ALL_FILES_OF_TYPE_END_FUNCTION(Win32GetAllFilesOfTypeEnd)
{
	Win32_File_Group* win32_file_group = (Win32_File_Group*)file_group->platform_data;
    
	if (win32_file_group)
	{
		ClearArena(&win32_file_group->memory);
	}
}

PLATFORM_OPEN_FILE_UTF8_FUNCTION(Win32OpenFileDirect)
{
    Assert(file_path.data);
    
    Platform_File_Handle result = {};
    StaticAssert(sizeof(HANDLE) <= sizeof(result.platform_data));
    
    U32 access_permissions = 0;
	U32 creation_mode      = 0;
    
	if (open_flags & OpenFile_Read)
	{
		access_permissions |= GENERIC_READ;
		creation_mode	   = OPEN_EXISTING;
	}
    
	if (open_flags & OpenFile_Write)
	{
		access_permissions |= GENERIC_WRITE;
		creation_mode	   = OPEN_ALWAYS;
	}
    
    wchar_t* wide_file_path = 0;
    UMM required_size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (const char*) file_path.data, (int) file_path.size, wide_file_path, 0);
    
    wide_file_path = (wchar_t*) PushSize(temporary_memory, sizeof(wchar_t) * required_size);
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (const char*) file_path.data, (int) file_path.size, wide_file_path, (int) file_path.size);
    
    HANDLE win32_handle = CreateFileW(wide_file_path, access_permissions,
									  FILE_SHARE_READ, 0,
									  creation_mode, 0, 0);
    
	result.is_valid = (win32_handle != INVALID_HANDLE_VALUE);
	*((HANDLE*) &result.platform_data) = win32_handle;
    
    return result;
}

PLATFORM_OPEN_FILE_FUNCTION(Win32OpenFile)
{
    Platform_File_Handle result  = {};
	StaticAssert(sizeof(HANDLE) <= sizeof(result.platform_data));
    
    StaticAssert(sizeof(HANDLE) <= sizeof(result.platform_data));
    
    U32 access_permissions = 0;
	U32 creation_mode      = 0;
    
	if (open_flags & OpenFile_Read)
	{
		access_permissions |= GENERIC_READ;
		creation_mode	   = OPEN_EXISTING;
	}
    
	if (open_flags & OpenFile_Write)
	{
		access_permissions |= GENERIC_WRITE;
		creation_mode	   = OPEN_ALWAYS;
	}
    
    wchar_t* file_path = (wchar_t*) file_info->platform_data;
    HANDLE win32_handle = CreateFileW(file_path, access_permissions,
									  FILE_SHARE_READ, 0,
									  creation_mode, 0, 0);
    
	result.is_valid = (win32_handle != INVALID_HANDLE_VALUE);
	*((HANDLE*) &result.platform_data) = win32_handle;
    
	return result;
}

PLATFORM_CLOSE_FILE_FUNCTION(Win32CloseFile)
{
	HANDLE win32_handle = *((HANDLE*) &file_handle->platform_data);
    
	if (win32_handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(win32_handle);
	}
}

PLATFORM_READ_FROM_FILE_FUNCTION(Win32ReadFromFile)
{
	if (PLATFORM_FILE_IS_VALID(handle))
	{
		HANDLE win32_handle = *((HANDLE*) &handle->platform_data);
        
		OVERLAPPED overlapped = {};
		overlapped.Offset	  = LARGE_INT_LOW(offset);
		overlapped.OffsetHigh = LARGE_INT_HIGH(offset);
		
		Assert(size <= UINT32_MAX);
		U32 file_size_truncated = (U32) CLAMP(0, size, UINT32_MAX);
        
		U32 bytes_read;
		if (ReadFile(win32_handle, dest, file_size_truncated, (LPDWORD) &bytes_read, &overlapped)
			&& (file_size_truncated == bytes_read))
		{
			// NOTE(soimn): success
		}
        
		else
		{
			WIN32LOG_ERROR("Win32ReadFromFile failed to read the contents of the specified file");
			handle->is_valid = false;
		}
	}
}

PLATFORM_WRITE_TO_FILE_FUNCTION(Win32WriteToFile)
{
	if (PLATFORM_FILE_IS_VALID(handle))
	{
		HANDLE win32_handle = *((HANDLE*) &handle->platform_data);
        
		OVERLAPPED overlapped = {};
		overlapped.Offset	  = LARGE_INT_LOW(offset);
		overlapped.OffsetHigh = LARGE_INT_HIGH(offset);
		
		U32 file_size_truncated = (U32) CLAMP(0, size, UINT32_MAX);
		Assert(size != file_size_truncated);
        
		U32 bytes_written;
		if (WriteFile(win32_handle, source, file_size_truncated, (LPDWORD) &bytes_written, &overlapped)
			&& (file_size_truncated == bytes_written))
		{
			// NOTE(soimn): success
		}
        
		else
		{
			WIN32LOG_ERROR("Win32WriteToFile failed to write the specified data to the file");
			handle->is_valid = false;
		}
	}
}


///
/// Utility
///

/// Game Code Hot Reload

internal Win32_Game_Code
Win32LoadGameCode(const wchar_t* dll_path, const wchar_t* loaded_dll_path)
{
    Win32_Game_Code game_code = {};
    
	CopyFileW(dll_path, loaded_dll_path, FALSE);
    
	game_code.module = LoadLibraryExW(loaded_dll_path, NULL, DONT_RESOLVE_DLL_REFERENCES);
    
	if (game_code.module != NULL)
	{
		game_code.game_update_and_render_func  = (game_update_and_render_function*) GetProcAddress(game_code.module, "GameUpdateAndRender");
        
		if (game_code.game_update_and_render_func)
		{
			game_code.is_valid  = true;
            while (!Win32GetFileLastWriteTime(dll_path, &game_code.timestamp));
		}
	}
	
	else
	{
		WIN32LOG_FATAL("Could not load game code. Reason: returned module handle is invalid");
	}
    
	if (!game_code.is_valid)
	{
		game_code.game_update_and_render_func = &GameUpdateAndRenderStub;
        
		WIN32LOG_FATAL("Could not load game code. Reason: could not find the GameUpdateAndRender function");
	}
    
	return game_code;
}

internal void
Win32UnloadGameCode(Win32_Game_Code* game_code)
{
	if (game_code->module)
	{
		FreeLibrary(game_code->module);
	}
    
	game_code->is_valid = false;
	game_code->game_update_and_render_func = &GameUpdateAndRenderStub;
}

internal void
Win32ReloadGameIfNecessary(Win32_Game_Code* game_code, Win32_Game_Info* game_info)
{
#ifdef ANT_ENABLE_HOT_RELOADING
    FILETIME new_time;
    while(!Win32GetFileLastWriteTime(game_info->dll_path, &new_time));
    
    if (CompareFileTime(&new_time, &game_code->timestamp) == 1)
    {
        Win32UnloadGameCode(game_code);
        *game_code = {};
        
        U32 tries = 0;
        
        do
        {
            Win32_Game_Code temp_game_code = {};
            temp_game_code = Win32LoadGameCode(game_info->dll_path, game_info->loaded_dll_path);
            
            if (temp_game_code.is_valid)
            {
                *game_code = temp_game_code;
                
                WIN32LOG_INFO("Loaded new version of the game");
                break;
            }
            
            WIN32LOG_INFO("Failed to load the new version of the game");
        }
        while(!game_code->is_valid && tries < 10, ++tries);
        
        if (tries == 10)
        {
            WIN32LOG_FATAL("Failed to load the new version of the game, tried 10 times.");
        }
    }
#endif
}

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
        
        // NOTE(soimn): this is triggered when the window is moved
        // case WM_INPUT: INVALID_CODE_PATH; break;
        
		default:
        result = DefWindowProcW(window_handle, msg_code, w_param, l_param);
		break;
	}
    
	return result;
}

internal void
Win32ProcessPendingMessages(HWND window_handle, Platform_Game_Input* old_input, Platform_Game_Input* new_input)
{
	MSG message = {};
    
	while (PeekMessageA(&message, window_handle, 0, 0, PM_REMOVE))
	{
		switch(message.message)
		{
			case WM_QUIT:
            Running = false;
			break;
            
            case WM_INPUT:
			{
				U8 buffer[50];
				U32 required_buffer_size;
                
				if (GetRawInputData((HRAWINPUT) message.lParam, RID_INPUT, NULL, &required_buffer_size, sizeof(RAWINPUTHEADER)) == 0
					&& required_buffer_size > ARRAY_COUNT(buffer))
				{
					WIN32LOG_DEBUG("Input was not captured, as the input buffer is too small");
				}
                
                
				else
				{
					if (GetRawInputData((HRAWINPUT) message.lParam, RID_INPUT, (void*) buffer, &required_buffer_size, sizeof(RAWINPUTHEADER)))
					{
						PRAWINPUT input = (PRAWINPUT) buffer;
                        
						switch (input->header.dwType)
						{
							case RIM_TYPEKEYBOARD:
							{
								U32 keycode = (U32) input->data.keyboard.VKey;
                                
								bool is_down  = input->data.keyboard.Flags == RI_KEY_MAKE;
                                
								if (is_down)
								{
									if ((((!new_input->current_key_buffer_size && old_input->current_key_buffer_size)
                                          && old_input->key_buffer[old_input->current_key_buffer_size - 1].code == (U8) keycode)
                                         && old_input->last_key_down)
										|| (new_input->current_key_buffer_size != 0
                                            && (new_input->key_buffer[new_input->current_key_buffer_size - 1].code == (U8) keycode
												&& new_input->last_key_down)))
									{
										// NOTE(soimn): this is a repeated keystroke
									}
                                    
									else
									{
										new_input->last_key_down = true;
                                        
										if (new_input->current_key_buffer_size < PLATFORM_GAME_INPUT_KEYBUFFER_MAX_SIZE)
										{
											new_input->key_buffer[new_input->current_key_buffer_size++].code = (U8) keycode;
										}
									}
								}
                                
								// NOTE(soimn): Game action input here
                                // 								switch (keycode)
                                // 								{
                                // 									default:
                                // 									break;
                                // 								}
							} break;
                            
							case RIM_TYPEMOUSE:
							{
							}
							break;
                            
                            INVALID_DEFAULT_CASE;
						}
					}
				}
			} break;
            
			default:
            Win32MainWindowProc(window_handle, message.message, message.wParam, message.lParam);
			break;
		}
	}
}

Platform_API_Functions* Platform;

///
/// Entry Point
///

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
			/// Setup
            
			// Memory
            
            Memory_Arena win32_persistent_memory   = {};
            win32_persistent_memory.block_size = KILOBYTES(1);
            win32_persistent_memory.current_block  = Win32AllocateMemoryBlock(win32_persistent_memory.block_size);
            ++win32_persistent_memory.block_count;
            
            Game_Memory* memory = PushStruct(&win32_persistent_memory, Game_Memory);
            memory->state       = PushStruct(&win32_persistent_memory, Game_State);
            
            memory->state->persistent_memory.current_block  = Win32AllocateMemoryBlock(MEGABYTES(64));
            ++memory->state->persistent_memory.block_count;
            memory->state->frame_local_memory.current_block = Win32AllocateMemoryBlock(KILOBYTES(1));
            ++memory->state->frame_local_memory.block_count;
            
			// Platform API function pointers
			memory->platform_api = {};
            memory->platform_api.AllocateMemoryBlock    = &Win32AllocateMemoryBlock;
            memory->platform_api.FreeMemoryBlock        = &Win32FreeMemoryBlock;
            
			memory->platform_api.LogInfo				= &Win32LogInfo;
			memory->platform_api.LogError			   = &Win32LogError;
            
			memory->platform_api.GetAllFilesOfTypeBegin = &Win32GetAllFilesOfTypeBegin;
			memory->platform_api.GetAllFilesOfTypeEnd   = &Win32GetAllFilesOfTypeEnd;
			memory->platform_api.OpenFile			   = &Win32OpenFile;
			memory->platform_api.CloseFile			  = &Win32CloseFile;
			memory->platform_api.ReadFromFile		   = &Win32ReadFromFile;
			memory->platform_api.WriteToFile			= &Win32WriteToFile;
            
            Platform = &memory->platform_api;
            
			// Game Info
			bool game_info_valid = false;
            
            Win32_Game_Info game_info = {APPLICATION_NAME, APPLICATION_VERSION};
            
			game_info.cwd = Win32GetCWD(&memory->state->persistent_memory, (U32*) &game_info.cwd_length);
            
			{ // Build dll path
				const wchar_t* appendage = CONCAT(L, APPLICATION_NAME) L".dll";
				U32 buffer_length        = game_info.cwd_length + (U32) WStrLength(appendage) + 1;
                
				game_info.dll_path = PushArray(&win32_persistent_memory, wchar_t, buffer_length);
				Win32BuildFullyQualifiedPath(&game_info, appendage, (wchar_t*) game_info.dll_path, buffer_length);
			}
            
			{ // Build loaded dll path
				const wchar_t* appendage = CONCAT(L, APPLICATION_NAME) L"_loaded.dll";
				U32 buffer_length        = game_info.cwd_length + (U32) WStrLength(appendage) + 1;
                
				game_info.loaded_dll_path = PushArray(&win32_persistent_memory, wchar_t, buffer_length);
				Win32BuildFullyQualifiedPath(&game_info, appendage, (wchar_t*) game_info.loaded_dll_path, buffer_length);
			}
            
			
			{ // Set working directory
				BOOL successfully_set_wd = SetCurrentDirectoryW(game_info.cwd);
                
				if (successfully_set_wd == FALSE)
				{
					WIN32LOG_FATAL("Failed to set working directory to the proper value");
				}
                
				game_info_valid = (successfully_set_wd != 0);
			}
            
			// Input
			bool input_ready = Win32RegisterRawInputDevices(window_handle);
            
            Platform_Game_Input input[2];
            Platform_Game_Input* old_input = &input[0];
            Platform_Game_Input* new_input = &input[1];
            
			// Game code
            Win32_Game_Code game_code = {};
            
			if (game_info_valid)
			{
				game_code = Win32LoadGameCode(game_info.dll_path, game_info.loaded_dll_path);
			}
            
            // Renderer
            bool renderer_ready = InitRenderer(&memory->platform_api, instance, window_handle);
            
			ClearArena(&memory->state->frame_local_memory);
            
			if (input_ready && game_info_valid && game_code.is_valid && renderer_ready)
			{
				Running = true;
                
				ShowWindow(window_handle, SW_SHOW);
                
                // TODO(soimn): adjust this to support dynamically changing the balance of game time and real time
                F32 game_update_dt = 1.0f / 60.0f;
                
                //// Main Loop
                while (Running)
                {
                    new_input->frame_dt = game_update_dt;
                    
                    Win32ProcessPendingMessages(window_handle, old_input, new_input);
                    
                    Win32ReloadGameIfNecessary(&game_code, &game_info);
                    
                    memory->platform_api.PrepareFrame();
                    
                    game_code.game_update_and_render_func(memory, old_input, new_input);
                    
                    memory->platform_api.PresentFrame();
                    
                    // Swap new and old input
                    Platform_Game_Input* temp_input = new_input;
                    *(new_input = old_input) = {};
                    old_input = temp_input;
                    
                    ResetArena(&memory->state->frame_local_memory);
                }
                //// Main Loop End
            }
            
            else
            {
                WIN32LOG_FATAL("Failed to initialize the game");
            }
        }
        
		else
		{
			WIN32LOG_FATAL("Failed to create window");
		}
        
	}
    
	else
	{
		WIN32LOG_FATAL("Failed to register main window");
	}
}