#include "win32_ant.h"

/*
 *	GLOBAL *cough* VARIABLES
 */

// TODO(soimn): delegate responsibility, to the game, to confirm the quit message, in case saving is due or the game needs to halt the player
global_variable bool Running						 = false;
global_variable float TargetFrameSecounds			 = (float) (1.0f / ((float)DEFAULT_TARGET_FPS * 1000.0f));
global_variable LARGE_INTEGER PerformanceCounterFreq = {};

global_variable win32_renderer_api Win32RendererAPI;
global_variable memory_arena FrameTempArena;

///
/// Logging
///

WIN32_EXPORT PLATFORM_LOG_INFO_FUNCTION(Win32LogInfo)
{
	char buffer[512 + 15] = {};
	char formatted_message[1024] = {};

	strconcat("[%s] [%s] %s @ %u: ", message, buffer, ARRAY_COUNT(buffer));
	strcopy("\n", buffer + strlength(buffer), ARRAY_COUNT(buffer) - strlength(buffer));

	wsprintf(formatted_message, buffer, module, (is_debug ? "DEBUG" : "INFO"), function_name, line_nr);

	OutputDebugStringA(formatted_message);
}

WIN32_EXPORT PLATFORM_LOG_ERROR_FUNCTION(Win32LogError)
{
	char buffer[1024 + 61] = {};
	char formatted_message[2048] = {};

	Assert(strlength(message) < ARRAY_COUNT(buffer));

	strconcat("An error has occurred in the %s module.\nFunction: %s, line: %u\n\n", message, buffer, ARRAY_COUNT(buffer));
	wsprintf(formatted_message, buffer, module, function_name, line_nr);

	MessageBoxA(NULL, formatted_message, APPLICATION_NAME, MB_ICONERROR | MB_OK);

	if (is_fatal)
		Running = false;
}

///
/// Raw Input
///

internal bool
Win32InitRawInput()
{
	RAWINPUTDEVICE rawInputDevices[2];

	// Keyboard
	rawInputDevices[0].usUsagePage = 0x01;
	rawInputDevices[0].usUsage	   = 0x06;
	rawInputDevices[0].dwFlags	   = RIDEV_NOHOTKEYS; // TODO(soimn): check if RIDEV_NOLEGACY improves performance
	rawInputDevices[0].hwndTarget  = NULL;

	// Mouse
	rawInputDevices[1].usUsagePage = 0x01;
	rawInputDevices[1].usUsage	   = 0x02;
	rawInputDevices[1].dwFlags	   = 0x00;  // TODO(soimn): check if RIDEV_NOLEGACY improves performance
	rawInputDevices[1].hwndTarget  = NULL;

	BOOL result = RegisterRawInputDevices((PRAWINPUTDEVICE)&rawInputDevices, 2, sizeof(RAWINPUTDEVICE));
	
	if (!result)
	{
		WIN32LOG_ERROR("Failed to register raw input devices");
	}

	return result;
}

internal void
Win32HandleRawInput(UINT msgCode, WPARAM wParam,
					LPARAM lParam)
{
	UNUSED_PARAMETER(msgCode);
	UNUSED_PARAMETER(wParam);
	UNUSED_PARAMETER(lParam);

	WIN32LOG_DEBUG("Input!");
}

///
/// Renderer
///

internal void
Win32RendererErrorCallback(const char* message, const char* function_name,
						   unsigned int line_nr, RENDERER_ERROR_MESSAGE_SEVERITY severity)
{
	if (severity >= RENDERER_WARNING_MESSAGE)
	{
		Win32LogError("RENDERER", false, function_name, line_nr, message);
	}
}

internal bool
Win32InitVulkanRenderer(HMODULE renderer_module, memory_arena* renderer_arena,
						HINSTANCE instance, HWND windowHandle,
						renderer_state*& state, vulkan_api*& api)
{
	bool succeeded = false;

	renderer_init_function* Win32InitRenderer = 
		(renderer_init_function*) GetProcAddress(renderer_module, "Win32InitRenderer");

	renderer_start_function* StartRenderer = 
		(renderer_start_function*) GetProcAddress(renderer_module, "RendererStart");

	renderer_create_instance_function* VulkanCreateInstance = 
		(renderer_create_instance_function*) GetProcAddress(renderer_module, "VulkanCreateInstance");

	renderer_create_surface_function* VulkanCreateWin32Surface = 
		(renderer_create_surface_function*) GetProcAddress(renderer_module, "VulkanCreateWin32Surface");

	renderer_acquire_physical_device_function* VulkanAcquirePhysicalDevice = 
		(renderer_acquire_physical_device_function*) GetProcAddress(renderer_module, "VulkanAcquirePhysicalDevice");

	renderer_create_logical_device_function* VulkanCreateLogicalDevice = 
		(renderer_create_logical_device_function*) GetProcAddress(renderer_module, "VulkanCreateLogicalDevice");

	succeeded = Win32InitRenderer(renderer_arena, &Win32RendererErrorCallback, state, api);
	if (succeeded)
	{
		do
		{
			const char* extensions[]		= {
												  #define INSTANCE_EXTENSIONS
												  #include "vulkan_extensions.inl"
												  #undef INSTANCE_EXTENSIONS
											  };

			const char* device_extensions[] = {
												  #define DEVICE_EXTENSIONS
												  #include "vulkan_extensions.inl"
												  #undef DEVICE_EXTENSIONS
											  };

			const char* layers[]			= {
												  #define LAYERS
												  #include "vulkan_extensions.inl"
												  #undef LAYERS
											  };

			succeeded = VulkanCreateInstance("Ant Game Engine", ANT_VERSION,
											 APPLICATION_NAME, APPLICATION_VERSION,
											 layers, ARRAY_COUNT(layers),
											 extensions, ARRAY_COUNT(extensions),
											 NULL, 0);

			if (!succeeded)
				break;

			succeeded = VulkanCreateWin32Surface(instance, windowHandle, NULL, 0);

			if (!succeeded)
				break;

			succeeded = VulkanAcquirePhysicalDevice(device_extensions, ARRAY_COUNT(device_extensions));

			if (!succeeded)
				break;

			succeeded = VulkanCreateLogicalDevice(NULL);

			StartRenderer();
		}
		while(0);
	}

	return succeeded;
}

internal bool
Win32LoadRendererFunctionPointers(HMODULE renderer_module, platform_renderer_functions* renderer,
								  win32_renderer_api* win32_api)
{
	bool succeeded = false;

	#define LOAD_RENDERER_FUNCTION(func, type, name)\
		renderer->name = (type*) GetProcAddress(renderer_module, func);\
		if (!renderer->name)\
		{\
			WIN32LOG_FATAL("Win32LoadRendererFunctionPointers failed. Reason: failed to load the function pointer to the '" #func "' function");\
			break;\
		}

	#define LOAD_WIN32_RENDERER_FUNCTION(func, type, name)\
		win32_api->name = (type*) GetProcAddress(renderer_module, func);\
		if (!win32_api)\
		{\
			WIN32LOG_FATAL("Win32LoadRendererFunctionPointers failed. Reason: failed to load the function pointer to the '" #func "' function");\
			break;\
		}

	do
	{
		LOAD_RENDERER_FUNCTION("VulkanCreateRenderPass", renderer_create_render_pass_function, CreateRenderPass);
		LOAD_RENDERER_FUNCTION("VulkanCreatePipelineLayout", renderer_create_pipeline_layout_function, CreatePipelineLayout);
		LOAD_RENDERER_FUNCTION("VulkanCreateShaderModule", renderer_create_shader_module_function, CreateShaderModule);
		LOAD_RENDERER_FUNCTION("VulkanCreateShaderStageInfos", renderer_create_shader_stage_infos_function, CreateShaderStageInfos);
		LOAD_RENDERER_FUNCTION("VulkanCreateGraphicsPipelines", renderer_create_graphics_pipelines_function, CreateGraphicsPipelines);
		LOAD_RENDERER_FUNCTION("VulkanCreateFramebuffer", renderer_create_framebuffer_function, CreateFramebuffer);
		LOAD_RENDERER_FUNCTION("VulkanCreateCommandPool", renderer_create_command_pool_function, CreateCommandPool);
		LOAD_RENDERER_FUNCTION("VulkanAllocateCommandBuffers", renderer_allocate_command_buffers_function, AllocateCommandBuffers);
		LOAD_RENDERER_FUNCTION("VulkanCreateSemaphore", renderer_create_semaphore_function, CreateSemaphore);
		LOAD_RENDERER_FUNCTION("VulkanGetOptimalSwapchainExtent", renderer_get_optimal_swapchain_extent_function, GetOptimalSwapchainExtent);
		LOAD_RENDERER_FUNCTION("VulkanGetOptimalSwapchainSurfaceFormat", renderer_get_optimal_swapchain_surface_format_function,
							   GetOptimalSwapchainSurfaceFormat);
		LOAD_RENDERER_FUNCTION("VulkanGetOptimalSwapchainPresentMode", renderer_get_optimal_swapchain_present_mode_function,
							   GetOptimalSwapchainPresentMode);
		LOAD_RENDERER_FUNCTION("VulkanCopyBuffer", renderer_copy_buffer_function, CopyBuffer);
		LOAD_RENDERER_FUNCTION("VulkanCreateVertexBuffer", renderer_create_vertex_buffer_function, CreateVertexBuffer);
		LOAD_RENDERER_FUNCTION("VulkanCreateIndexBuffer", renderer_create_index_buffer_function, CreateIndexBuffer);


		LOAD_WIN32_RENDERER_FUNCTION("VulkanBeginFrame", renderer_begin_frame_function, BeginFrame);
		LOAD_WIN32_RENDERER_FUNCTION("VulkanEndFrame", renderer_end_frame_function, EndFrame);
		LOAD_WIN32_RENDERER_FUNCTION("VulkanCreateSwapchain", renderer_create_swapchain_function, CreateSwapchain);
		LOAD_WIN32_RENDERER_FUNCTION("VulkanCreateSwapchainImages", renderer_create_swapchain_images_function, CreateSwapchainImages);
		LOAD_WIN32_RENDERER_FUNCTION("VulkanCreateDefaultRenderPass", renderer_create_default_render_pass_function, CreateDefaultRenderPass);
		LOAD_WIN32_RENDERER_FUNCTION("VulkanCreateSwapchainFramebuffers", renderer_create_swapchain_framebuffers_function, CreateSwapchainFramebuffers);
		LOAD_WIN32_RENDERER_FUNCTION("VulkanCreateSwapchainCommandBuffers", renderer_create_swapchain_command_buffers_function, CreateSwapchainCommandBuffers);
		LOAD_WIN32_RENDERER_FUNCTION("VulkanCreateSwapchainSemaphores", renderer_create_swapchain_semaphores_function, CreateSwapchainSemaphores);

		succeeded = true;
	}
	while(0);

	#undef LOAD_RENDERER_FUNCTION

	return succeeded;
}

///
/// File System Interaction
///

#ifdef ANT_ENABLE_HOT_RELOADING
FILETIME
Win32DebugGetFileCreateTime(const char* file_path)
{
	FILETIME result = {};

	HANDLE file = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file != INVALID_HANDLE_VALUE)
	{
		GetFileTime(file, &result, NULL, NULL);
		CloseHandle(file);
	}

	return result;
}
#endif

PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN_FUNCTION(Win32GetAllFilesOfTypeBegin)
{
	platform_file_group result = {};

	win32_platform_file_group* win32_file_group = BootstrapPushSize(win32_platform_file_group, memory, KILOBYTES(1), alignof(win32_platform_file_group));
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

	u32 directory_length = 0;

	for (wchar_t* it = directory; *it; ++it)
	{
		++directory_length;
	}

	WIN32_FIND_DATAW find_data;
	HANDLE find_handle = FindFirstFileW(wildcard, &find_data);
	while(find_handle != INVALID_HANDLE_VALUE)
	{
		++result.file_count;
		platform_file_info* info = PushStruct(&win32_file_group->memory, platform_file_info);
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

		u32 base_name_length = (u32)(base_name_end - base_name_begin);
		u32 required_base_name_storage = WideCharToMultiByte(CP_UTF8, 0,
															 base_name_begin, base_name_length,
															 info->base_name, 0,
															 0, 0);

		info->base_name = (char*) PushSize(&win32_file_group->memory, required_base_name_storage + 1);

		required_base_name_storage = WideCharToMultiByte(CP_UTF8, 0, base_name_begin, base_name_length,
														 info->base_name, required_base_name_storage, 0, 0);

		info->base_name[required_base_name_storage] = 0;

		u32 file_name_size = (u32)(scan - find_data.cFileName) + 1;
		info->platform_data = PushArray(&win32_file_group->memory, wchar_t, directory_length + file_name_size);
		CopyArray(directory, directory_length, info->platform_data);
		CopyArray(find_data.cFileName, file_name_size, (wchar_t*)info->platform_data + directory_length);

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
	win32_platform_file_group* win32_file_group = (win32_platform_file_group*)file_group->platform_data;

	if (win32_file_group)
	{
		ClearMemoryArena(&win32_file_group->memory);
	}
}

PLATFORM_OPEN_FILE_FUNCTION(Win32OpenFile)
{
	platform_file_handle result = {};
	StaticAssert(sizeof(HANDLE) <= sizeof(result.platform_data));

	DWORD access_permissions = 0;
	DWORD creation_mode		= 0;

	if (open_params & OpenFile_Read)
	{
		access_permissions |= GENERIC_READ;
		creation_mode	   = OPEN_EXISTING;
	}

	if (open_params & OpenFile_Write)
	{
		access_permissions |= GENERIC_WRITE;
		creation_mode	    = OPEN_ALWAYS;
	}

	wchar_t* file_name  = (wchar_t*) file_info->platform_data;
	HANDLE win32_handle = CreateFileW(file_name, access_permissions,
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
		
		u32 file_size_truncated = (u32) CLAMP(0, size, UINT32_MAX);
		Assert(size != file_size_truncated);

		DWORD bytes_read;
		if (ReadFile(win32_handle, dest, file_size_truncated, &bytes_read, &overlapped)
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
		
		u32 file_size_truncated = (u32) CLAMP(0, size, UINT32_MAX);
		Assert(size != file_size_truncated);

		DWORD bytes_written;
		if (WriteFile(win32_handle, source, file_size_truncated, &bytes_written, &overlapped)
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

/// Time
internal LARGE_INTEGER
Win32GetWallClock()
{
	LARGE_INTEGER result = {};
	QueryPerformanceCounter(&result);
	return result;
}

internal float
Win32GetSecoundsElapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
	float result = (float)(end.QuadPart - start.QuadPart) / (float) PerformanceCounterFreq.QuadPart;
	return result;
}

/// Memory Allocation

MEMORY_ALLOCATE_MEMORY_BLOCK_FUNCTION(Win32AllocateMemoryBlock)
{
	Assert(size || !"Zero passed as size to Win32AllocateMemoryBlock");
	Assert(alignment == 1 || alignment == 2 ||
		   alignment == 4 || alignment == 8 ||
		   !"Invalid alignment passed to Win32AllocateMemoryBlock");

	memory_index total_block_size	   = size + (alignment - 1);
	memory_index total_block_info_size = sizeof(memory_block) + (alignof(memory_block) - 1);

	void* memory = VirtualAlloc(NULL, total_block_info_size + total_block_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Assert(memory || !"Failed to allocate memory block");

	memory_block* block = (memory_block*) Align(memory, alignof(memory_block));
	*block = {};

	void* memory_start = (void*)(block + 1);
	uint8 adjustment   = AlignOffset(memory_start, alignment);

	block->memory		   = memory;
	block->push_ptr		   = (uint8*) memory_start + adjustment;
	block->remaining_space = size + ((alignment - 1) - adjustment);

	return block;
}

MEMORY_FREE_MEMORY_BLOCK_FUNCTION(Win32FreeMemoryBlock)
{
	Assert(block || !"Invalid block pointer passed to Win32FreeMemoryBlock. Block pointer was NULL");
	VirtualFree(block->memory, block->remaining_space + (block->push_ptr - (uint8*) block->memory), MEM_RELEASE);
}

/// Game Code Hot Reload

internal bool
Win32SetupGameInfo(win32_game_info& game_info, memory_arena* persistent_memory)
{
	bool succeeded = false;

	uint32 file_path_growth_amount = 256;
	char* file_path = (char*) PushSize(&FrameTempArena, file_path_growth_amount);
	uint32 file_path_memory_size = file_path_growth_amount;
	uint32 path_length;

	for (;;)
	{
		path_length = GetModuleFileNameA(NULL, file_path, file_path_memory_size);

		if (path_length != file_path_memory_size)
		{
			break;
		}

		else
		{
			if (1024 > FrameTempArena.current_block->remaining_space)
			{
				WIN32LOG_ERROR("Win32SetupGameInfo failed. Out of memory");
				return false;
			}

			else
			{
				file_path_memory_size += file_path_growth_amount;

				PushSize(&FrameTempArena, file_path_memory_size);
				continue;
			}
		}
	}

	uint32 index = strfind(file_path, '\\', true);

	if (index == npos)
	{
		WIN32LOG_ERROR("An error occurred whilst trying to extrapolate the working directory of the game. Reason: module path is invalid");
	}

	else
	{
		uint32 length_of_dir = index + 1;

		game_info.root_dir = PushString(persistent_memory, file_path, length_of_dir);

		if (game_info.root_dir)
		{
			// Main DLL path
			char* dll_suffix			  = ".dll";
			char* loaded_dll_suffix		  = "_loaded.dll";
			uint32 game_name_length		  = strlength(game_info.name);
			uint32 dll_path_length		  = length_of_dir + game_name_length + strlength(dll_suffix);
			uint32 loaded_dll_path_length = length_of_dir + game_name_length + strlength(loaded_dll_suffix);

			int successfully_concatenated = -1;

			do
			{
				game_info.dll_path = (char*) PushSize(persistent_memory, dll_path_length + 1);
				if (game_info.dll_path)
				{
					successfully_concatenated = strconcat3(game_info.root_dir, game_info.name, dll_suffix, (char*) game_info.dll_path, dll_path_length + 1);
					if (successfully_concatenated == -1)
					{
						break;
					}
				}

				// Loaded DLL path
				game_info.loaded_dll_path = (char*) PushSize(persistent_memory, loaded_dll_path_length + 1);
				if (game_info.loaded_dll_path)
				{
					successfully_concatenated = strconcat3(game_info.root_dir, game_info.name, loaded_dll_suffix, (char*) game_info.loaded_dll_path, loaded_dll_path_length + 1);
					if (successfully_concatenated == -1)
					{
						break;
					}
				}
				
// 					// Resource directory
// 					char* resource_suffix = "res\\";
// 					uint32 resource_dir_length = length_of_dir + strlength(resource_suffix);
// 
// 					game_info.resource_dir = (char*) PushSize(persistent_memory, resource_dir_length + 1);
// 					if (game_info.resource_dir)
// 					{
// 						successfully_concatenated = strconcat(game_info.root_dir, resource_suffix, (char*) game_info.resource_dir, resource_dir_length + 1);
// 						if (successfully_concatenated == -1)
// 						{
// 							break;
// 						}
// 					}
			}
			while(0);

			if ((game_info.dll_path && game_info.loaded_dll_path /*&& game_info.resource_dir*/) &&
				successfully_concatenated != -1)
			{
				succeeded = true;
			}
		}
	}

	return succeeded;
}

internal win32_game_code
Win32LoadGameCode(const char* dll_path, const char* loaded_dll_path)
{
	win32_game_code game_code = {};

	CopyFileA(dll_path, loaded_dll_path, FALSE);

	game_code.module = LoadLibraryExA(loaded_dll_path, NULL, DONT_RESOLVE_DLL_REFERENCES);

	if (game_code.module != NULL)
	{
		game_code.game_init_func			   = (game_init_function*)				 GetProcAddress(game_code.module, "GameInit");
		game_code.game_update_and_render_func  = (game_update_and_render_function*)  GetProcAddress(game_code.module, "GameUpdateAndRender");
		game_code.game_cleanup_func			   = (game_cleanup_function*)			 GetProcAddress(game_code.module, "GameCleanup");

		if (game_code.game_init_func && game_code.game_update_and_render_func && game_code.game_cleanup_func)
		{
			game_code.is_valid = true;
			game_code.timestamp = Win32DebugGetFileCreateTime(dll_path);
		}
	}
	
	else
	{
		WIN32LOG_FATAL("Could not load game code. Reason: returned module handle is invalid");
	}

	if (!game_code.is_valid)
	{
		game_code.game_init_func = &GameInitStub;
		game_code.game_update_and_render_func = &GameUpdateAndRenderStub;
		game_code.game_cleanup_func = &GameCleanupStub;

		WIN32LOG_FATAL("Could not load game code. Reason: could not find update, cleanup and init functions");
	}

	return game_code;
}

internal void
Win32UnloadGameCode(win32_game_code* game_code)
{
	if (game_code->module)
	{
		FreeLibrary(game_code->module);
	}

	game_code->is_valid = false;
	game_code->game_init_func			   = &GameInitStub;
	game_code->game_update_and_render_func = &GameUpdateAndRenderStub;
}

LRESULT CALLBACK Win32MainWindowProc(HWND windowHandle, UINT msgCode,
									 WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	switch (msgCode)
	{
		// TODO(soimn): call games' exit handler instead of closing immediately
		case WM_CLOSE:
			Running = false;
		break;

		case WM_QUIT:
			Running = false;
		break;

		default:
			result = DefWindowProc(windowHandle, msgCode, wParam, lParam);
		break;
	}

	return result;
}


///
/// Entry Point
///

int CALLBACK WinMain(HINSTANCE instance,
					 HINSTANCE prevInstance,
					 LPSTR commandLine,
					 int windowShowMode)
{
	UNUSED_PARAMETER(prevInstance);
	UNUSED_PARAMETER(commandLine);
	UNUSED_PARAMETER(windowShowMode);

	HWND windowHandle;
	WNDCLASSEXA windowClass = {};

	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_VREDRAW | CS_HREDRAW;
	windowClass.lpfnWndProc = &Win32MainWindowProc;
	windowClass.hInstance = instance;
	windowClass.hbrBackground = 0;
	windowClass.lpszClassName = APPLICATION_NAME;

	if (RegisterClassExA(&windowClass))
	{
		windowHandle =
			CreateWindowExA(windowClass.style,
							windowClass.lpszClassName,
							APPLICATION_NAME,
							WS_VISIBLE | WS_OVERLAPPED | WS_SYSMENU,
							CW_USEDEFAULT, CW_USEDEFAULT,
							CW_USEDEFAULT, CW_USEDEFAULT,
							NULL, NULL,
							instance, NULL);
		if (windowHandle)
		{
			/// Setup
			
			// Game Info
			win32_game_info game_info = {APPLICATION_NAME, APPLICATION_VERSION};

			// Memory
			game_memory memory = {};

			DefaultMemoryArenaAllocationRoutines = {&Win32AllocateMemoryBlock, &Win32FreeMemoryBlock,
													&Win32AllocateMemoryBlock, &Win32FreeMemoryBlock,
													MEGABYTES(1), KILOBYTES(1),
													BYTES(16), 4, 4};

			memory.persistent_arena.current_block	= Win32AllocateMemoryBlock(MEGABYTES(64), 4);
			++memory.persistent_arena.block_count;

			memory.transient_arena.current_block	= Win32AllocateMemoryBlock(GIGABYTES(1), 4);
			++memory.transient_arena.block_count;

			memory.debug_arena.current_block		= Win32AllocateMemoryBlock(GIGABYTES(1), 4);
			++memory.transient_arena.block_count;

			memory.renderer_arena.current_block		= Win32AllocateMemoryBlock(MEGABYTES(1), 4);
			memory.renderer_arena.allocate_function = &Win32AllocateMemoryBlock;
			memory.renderer_arena.free_function		= &Win32FreeMemoryBlock;
			++memory.renderer_arena.block_count;

			FrameTempArena.current_block			= Win32AllocateMemoryBlock(MEGABYTES(1), 4);
			++FrameTempArena.block_count;

			// Platform API function pointers
			memory.platform_api = {};
			memory.platform_api.LogInfo				   = &Win32LogInfo;
			memory.platform_api.LogError			   = &Win32LogError;
			memory.platform_api.GetAllFilesOfTypeBegin = &Win32GetAllFilesOfTypeBegin;
			memory.platform_api.GetAllFilesOfTypeEnd   = &Win32GetAllFilesOfTypeEnd;
			memory.platform_api.OpenFile			   = &Win32OpenFile;
			memory.platform_api.CloseFile			   = &Win32CloseFile;
			memory.platform_api.ReadFromFile		   = &Win32ReadFromFile;
			memory.platform_api.WriteToFile			   = &Win32WriteToFile;

			// Game code
			win32_game_code game_code = {};

			if (Win32SetupGameInfo(game_info, &memory.persistent_arena))
			{
				game_code = Win32LoadGameCode(game_info.dll_path, game_info.loaded_dll_path);
			}

			// Raw input
			bool input_ready = Win32InitRawInput();

			// Renderer
			bool renderer_ready = false;

			{
				HMODULE renderer_module = LoadLibraryA("renderer.dll");

				if (renderer_module != INVALID_HANDLE_VALUE)
				{
					renderer_ready = Win32InitVulkanRenderer(renderer_module, &memory.renderer_arena,
															 instance, windowHandle,
															 memory.platform_api.RendererAPI.RendererState,
															 memory.platform_api.RendererAPI.VulkanAPI);
					renderer_ready = Win32LoadRendererFunctionPointers(renderer_module, &memory.platform_api.RendererAPI, &Win32RendererAPI);

					if (renderer_ready)
					{
						do
						{
							BREAK_ON_FALSE(Win32RendererAPI.CreateSwapchain(NULL),			 renderer_ready);
							BREAK_ON_FALSE(Win32RendererAPI.CreateSwapchainImages(NULL),	 renderer_ready);
							BREAK_ON_FALSE(Win32RendererAPI.CreateDefaultRenderPass(),		 renderer_ready);
							BREAK_ON_FALSE(Win32RendererAPI.CreateSwapchainFramebuffers(),	 renderer_ready);
							BREAK_ON_FALSE(Win32RendererAPI.CreateSwapchainCommandBuffers(), renderer_ready);
							BREAK_ON_FALSE(Win32RendererAPI.CreateSwapchainSemaphores(),	 renderer_ready);
						}
						while(0);
					}
				}

				else
				{
					WIN32LOG_FATAL("Failed to load the renderer.dll module");
				}
			}

			// Misc
			QueryPerformanceFrequency(&PerformanceCounterFreq);

			ClearMemoryArena(&FrameTempArena);

			if (game_code.is_valid && input_ready && renderer_ready)
			{
				Running = true;
				if (game_code.game_init_func(&memory))
				{

					/// Main Loop
					while (Running)
					{
						LARGE_INTEGER frame_start_time = Win32GetWallClock();

						MSG message;
						while (PeekMessage(&message, windowHandle, 0, 0, PM_REMOVE))
						{
// 							if (message.message == WM_INPUT)
// 							{
// 								Win32HandleRawInput(message.message, message.wParam,
// 													message.lParam);
// 							}
							
							Win32MainWindowProc(message.hwnd, message.message,
												message.wParam, message.lParam);
						}

						#ifdef ANT_ENABLE_HOT_RELOADING
						{
							FILETIME new_time = Win32DebugGetFileCreateTime(game_info.dll_path);
							if (CompareFileTime(&new_time, &game_code.timestamp) == 1)
							{
								Win32UnloadGameCode(&game_code);

								do
								{
									win32_game_code temp_game_code = {};
									temp_game_code = Win32LoadGameCode(game_info.dll_path, game_info.loaded_dll_path);

									if (temp_game_code.is_valid)
									{
										game_code = temp_game_code;
										bool succeeded = game_code.game_init_func(&memory);
										Assert(succeeded);

										WIN32LOG_DEBUG("Loaded new version of the game");
										break;
									}

									WIN32LOG_DEBUG("Failed to load the new version of the game");
								}
								while(!game_code.is_valid);
							}
						}
						#endif

// 						Win32RendererAPI.BeginFrame();

						game_code.game_update_and_render_func(&memory, TargetFrameSecounds);

// 						Win32RendererAPI.EndFrame();

						LARGE_INTEGER frame_end_time = Win32GetWallClock();

						float frame_time_elapsed = Win32GetSecoundsElapsed(frame_start_time, frame_end_time);

						while (frame_time_elapsed < TargetFrameSecounds)
						{
							frame_time_elapsed += Win32GetSecoundsElapsed(frame_start_time, Win32GetWallClock());
						}

// 						char buffer[100];
// 						frame_time_elapsed += Win32GetSecoundsElapsed(frame_start_time, Win32GetWallClock());
// 						sprintf(buffer, "Frame finished in %f ms\n", frame_time_elapsed * 1000.0f);
// 						WIN32LOG_DEBUG(buffer);
			
						// TODO(soimn): reset temporary frame local memory
					}
					/// Main Loop End
				}

				else
				{
					WIN32LOG_FATAL("Failed to initialize the game");
				}
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

/* TODO(soimn):
 *
 *	- Setup game input abstraction supporting future rebinding of keys
 *	- Refactor input handling to allow more explicit additions and deletions of devices
 *	- Setup XAudio2
 *	- Add integrated graphics support in the physical device
 *	  selection, and check setup for compatibility
 *	- Implement proper file handling with relative paths based on
 *	  a base directory
 *	- Implement a solution for freeing allocations instead of entire arenas
 *	- Setup a temp memory arena
 */
