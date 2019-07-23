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
    
    if (log_options & PlatformLog_MessagePrompt)
    {
        U32 prompt = MB_ICONINFORMATION;
        
        if (log_options & PlatformLog_Fatal)
        {
            prompt = MB_ICONHAND;
        }
        
        else if (log_options & PlatformLog_Error)
        {
            prompt = MB_ICONEXCLAMATION;
        }
        
        else if (log_options & PlatformLog_Warning)
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



/// Game Loading

internal bool
Win32ReloadGameCodeIfNecessary(Win32_Game_Code* game_code)
{
    const wchar_t* game_code_path        = L".\\..\\" CONCAT(L, APPLICATION_NAME) L".dll";
    const wchar_t* loaded_game_code_path = L".\\..\\" CONCAT(L, APPLICATION_NAME) L"_loaded.dll";
    
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
                    
                    Win32Log(PlatformLog_Info, "Successfully loaded new game code");
                }
                
                else
                {
                    Win32Log(PlatformLog_Fatal | PlatformLog_MessagePrompt, "Failed to load game code entry point. Win32 error code %u", GetLastError());
                }
            }
            
            else
            {
                Win32Log(PlatformLog_Fatal | PlatformLog_MessagePrompt, "Failed to load game code dll. Win32 error code: %u", GetLastError());
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
            bool all_clear = false;
            
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
            
            Platform = &game_memory.platform_api;
            
            game_memory.state = PushStruct(&persistent_memory, Game_State);
            
            Win32_Game_Code game_code = {};
            
            { /// Set current working directory to /data
                wchar_t* buffer = (wchar_t*) PushSize(&frame_local_memory, MEGABYTES(4), alignof(wchar_t));
                U32 buffer_size = MEGABYTES(4) / sizeof(wchar_t);
                
                U32 result = GetModuleFileNameW(0, buffer, buffer_size);
                
                U32 error_code = GetLastError();
                if (result && error_code != ERROR_INSUFFICIENT_BUFFER)
                {
                    UMM last_slash = 0;
                    for (UMM i = 0; i < result; ++i)
                    {
                        if (buffer[i] == '/' || buffer[i] == '\\')
                        {
                            last_slash = i;
                        }
                    }
                    
                    const wchar_t* data_dir = L"\\data\\";
                    UMM data_dir_length     = 6;
                    
                    if (last_slash < buffer_size - data_dir_length)
                    {
                        Copy((void*) data_dir, buffer + last_slash, data_dir_length * sizeof(wchar_t));
                        buffer[last_slash + data_dir_length] = 0;
                        
                        if (SetCurrentDirectoryW(buffer))
                        {
                            all_clear = true;
                        }
                    }
                }
                
                if (!all_clear)
                {
                    Win32Log(PlatformLog_Fatal | PlatformLog_MessagePrompt, "Failed to set current working directory to the data directory");
                }
            }
            
            if (all_clear)
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
            Win32Log(PlatformLog_Fatal | PlatformLog_MessagePrompt, "Failed to create a window. Win32 error code: %u", GetLastError());
        }
    }
    
    else
    {
        Win32Log(PlatformLog_Fatal | PlatformLog_MessagePrompt, "Failed to register window class. Win32 error code: %u", GetLastError());
    }
}