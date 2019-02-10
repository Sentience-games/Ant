#include "win32_ant.h"

/*
 *	GLOBAL *cough* VARIABLES
 */

// TODO(soimn): delegate responsibility, to the game, to confirm the quit message, in case saving is due or the game needs to halt the player
global_variable bool running = false;

///
/// Logging
///

WIN32_EXPORT PLATFORM_LOG_INFO(Win32LogInfo)
{
	char buffer[512 + 15] = {};
	char formatted_message[1024] = {};

	strconcat("[%s] [%s] %s @ %u: ", message, buffer, ARRAY_COUNT(buffer));
	strcopy("\n", buffer + strlength(buffer), ARRAY_COUNT(buffer) - strlength(buffer));

	wsprintf(formatted_message, buffer, module, (is_debug ? "DEBUG" : "INFO"), function_name, line_nr);

	OutputDebugStringA(formatted_message);
}

WIN32_EXPORT PLATFORM_LOG_ERROR(Win32LogError)
{
	char buffer[1024 + 61] = {};
	char formatted_message[2048] = {};

	Assert(strlength(message) < ARRAY_COUNT(buffer));

	strconcat("An error has occurred in the %s module.\nFunction: %s, line: %u\n\n", message, buffer, ARRAY_COUNT(buffer));
	wsprintf(formatted_message, buffer, module, function_name, line_nr);

	MessageBoxA(NULL, formatted_message, APPLICATION_NAME, MB_ICONERROR | MB_OK);

	if (is_fatal)
		running = false;
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
		}
		while(0);
	}

	return succeeded;
}

internal bool
Win32LoadRendererFunctionPointers(HMODULE renderer_module, platform_renderer_functions* renderer)
{
	bool succeeded = false;

	#define LOAD_RENDERER_FUNCTION(func, type, name)\
		renderer->name = (type*) GetProcAddress(renderer_module, func);\
		if (!renderer->name)\
		{\
			WIN32LOG_FATAL("Win32LoadRendererFunctionPointers failed. Reason: failed to load the function pointer to the '" ##func "' function");\
			break;\
		}

	do
	{
		LOAD_RENDERER_FUNCTION("VulkanCreateSwapchain", renderer_create_swapchain_function, CreateSwapchain);
		LOAD_RENDERER_FUNCTION("VulkanCreateSwapchainImages", renderer_create_swapchain_images_function, CreateSwapchainImages);
		LOAD_RENDERER_FUNCTION("VulkanCreateRenderPass", renderer_create_render_pass_function, CreateRenderPass);
		LOAD_RENDERER_FUNCTION("VulkanCreatePipelineLayout", renderer_create_pipeline_layout_function, CreatePipelineLayout);
		LOAD_RENDERER_FUNCTION("VulkanCreateShaderModule", renderer_create_shader_module_function, CreateShaderModule);
		LOAD_RENDERER_FUNCTION("VulkanCreateShaderStageInfos", renderer_create_shader_stage_infos_function, CreateShaderStageInfos);
		LOAD_RENDERER_FUNCTION("VulkanCreateGraphicsPipelines", renderer_create_graphics_pipelines_function, CreateGraphicsPipelines);
		LOAD_RENDERER_FUNCTION("VulkanCreateFramebuffer", renderer_create_framebuffer_function, CreateFramebuffer);
		LOAD_RENDERER_FUNCTION("VulkanCreateCommandPool", renderer_create_command_pool_function, CreateCommandPool);
		LOAD_RENDERER_FUNCTION("VulkanCreateSemaphore", renderer_create_semaphore_function, CreateSemaphore);
		LOAD_RENDERER_FUNCTION("VulkanGetOptimalSwapchainExtent", renderer_get_optimal_swapchain_extent_function, GetOptimalSwapchainExtent);
		LOAD_RENDERER_FUNCTION("VulkanGetOptimalSwapchainSurfaceFormat", renderer_get_optimal_swapchain_surface_format_function,
							   GetOptimalSwapchainSurfaceFormat);
		LOAD_RENDERER_FUNCTION("VulkanGetOptimalSwapchainPresentMode", renderer_get_optimal_swapchain_present_mode_function,
							   GetOptimalSwapchainPresentMode);

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

WIN32_EXPORT DEBUG_FREE_FILE_MEMORY(Win32DebugFreeFileMemory)
{
	VirtualFree((uint8*)file_handle.data - file_handle.adjustment, 0, MEM_RELEASE);
	file_handle = {};
}

WIN32_EXPORT DEBUG_READ_FILE(Win32DebugReadFile)
{
	platform_file_handle file_handle = {};

	HANDLE win32_file_handle = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (win32_file_handle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER file_size;
		if (GetFileSizeEx(win32_file_handle, &file_size))
		{
			Assert(file_size.QuadPart + data_alignment - 1 <= UINT32_MAX);
			uint32 file_size_32 = (uint32)(file_size.QuadPart + data_alignment - 1);
			file_handle.data = VirtualAlloc(0, file_size_32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (file_handle.data != NULL)
			{
				uint8 adjustment = (uint8)(ALIGN(file_handle.data, data_alignment) - (uint8*)file_handle.data);

				DWORD bytes_read;
				if (ReadFile(win32_file_handle, (uint8*)file_handle.data + adjustment, file_size_32, &bytes_read, 0)
					&& bytes_read == (uint32) file_size.QuadPart)
				{
					file_handle.size = (uint32) file_size.QuadPart;
					file_handle.adjustment = adjustment;
				}

				else
				{
					Win32DebugFreeFileMemory(file_handle);
					file_handle.data = NULL;
				}
			}

			CloseHandle(win32_file_handle);
		}
	}

	return file_handle;
}

WIN32_EXPORT DEBUG_WRITE_FILE(Win32DebugWriteFile)
{
	bool succeeded = false;

	HANDLE win32_file_handle = CreateFileA(path, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
	if (win32_file_handle != INVALID_HANDLE_VALUE)
	{
		DWORD bytes_written;
		if (WriteFile(win32_file_handle, memory, memory_size, &bytes_written, 0))
		{
			succeeded = (bytes_written == memory_size);
		}

		CloseHandle(win32_file_handle);
	}

	return succeeded;
}

///
/// Utility
///

/// Game Code Hot Reload

internal bool
Win32SetupGameInfo(win32_game_info& game_info, uint8*& persistent_stack_ptr, uint32 remaining_memory)
{
	bool succeeded = false;

	char* memory_ptr = (char*)persistent_stack_ptr;
	DWORD path_length = GetModuleFileNameA(NULL, memory_ptr, remaining_memory);
	remaining_memory -= path_length + 1;
	uint32 index = strfind(memory_ptr, '\\', true);

	if (index == npos)
	{
		WIN32LOG_ERROR("An error occurred whilst trying to extrapolate the working directory of the game. Reason: module path is invalid");
	}

	else
	{
		uint32 length_of_dir = index + 1;

		game_info.root_dir = memory_ptr;
		memory_ptr[length_of_dir] = '\0';
		remaining_memory -= length_of_dir;
		memory_ptr += length_of_dir + 1;
		persistent_stack_ptr += length_of_dir + 1;

		// Main DLL path
		strconcat3(memory_ptr - (length_of_dir + 1), game_info.name, ".dll", memory_ptr, remaining_memory);
		uint32 dll_path_length = strlength(memory_ptr);
		remaining_memory -= dll_path_length + 1;

		// Loaded DLL path
		substrconcat3(memory_ptr - (length_of_dir + 1), 0, game_info.name, 0, "_loaded.dll", 0, memory_ptr + dll_path_length + 1, remaining_memory);
		uint32 loaded_dll_path_length = strlength(memory_ptr + dll_path_length + 1);
		remaining_memory -= loaded_dll_path_length;

		game_info.dll_path = memory_ptr;
		game_info.loaded_dll_path = memory_ptr + dll_path_length + 1;

		persistent_stack_ptr += dll_path_length + loaded_dll_path_length + 2;

		// Resource directory
		game_info.resource_dir = (char*) persistent_stack_ptr;
		strconcat(game_info.root_dir, "res\\", (char*) game_info.resource_dir,
				  remaining_memory);

		persistent_stack_ptr += strlength((char*) persistent_stack_ptr) + 1;

		succeeded = true;
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
		game_code.game_init_func	= (game_init_function*)    GetProcAddress(game_code.module, "GameInit");
		game_code.game_update_func  = (game_update_function*)  GetProcAddress(game_code.module, "GameUpdate");
		game_code.game_cleanup_func = (game_cleanup_function*) GetProcAddress(game_code.module, "GameCleanup");

		if (game_code.game_init_func && game_code.game_update_func && game_code.game_cleanup_func)
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
		game_code.game_update_func = &GameUpdateStub;
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
	game_code->game_init_func = &GameInitStub;
	game_code->game_update_func = &GameUpdateStub;
}

LRESULT CALLBACK Win32MainWindowProc(HWND windowHandle, UINT msgCode,
									 WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	switch (msgCode)
	{
		// TODO(soimn): call games' exit handler instead of closing immediately
		case WM_CLOSE:
			running = false;
		break;

		case WM_QUIT:
			running = false;
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
						    CW_USEDEFAULT,
						    CW_USEDEFAULT,
						    CW_USEDEFAULT,
						    CW_USEDEFAULT,
						    NULL,
						    NULL,
						    instance,
						    NULL);
		if (windowHandle)
		{
			/// Setup
			
			// Game Info
			win32_game_info game_info = {APPLICATION_NAME, APPLICATION_VERSION};

			// Memory
			game_memory memory = {};

			// TODO(soimn): implement a more robust solution which does
			//				not require the Win32InitVulkan and Win32SetupGameInfo
			//				to mutate this pointer
			uint8* persistent_stack_ptr = NULL;

			memory.persistent_size = MEGABYTES(64);
			memory.persistent_memory = VirtualAlloc(NULL, memory.persistent_size,
													MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
			persistent_stack_ptr = (uint8*)memory.persistent_memory;

			memory.transient_size = GIGABYTES(2);
			memory.transient_memory = VirtualAlloc(NULL, memory.transient_size,
													MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			{
				// NOTE(soimn): this is temporary
				uint32 debug_arena_size = MEGABYTES(10);
				memory.debug_arena = {};
				memory.debug_arena.memory = VirtualAlloc(NULL, debug_arena_size,
														MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
				memory.debug_arena.push_ptr = (uint8*) memory.debug_arena.memory;
				memory.debug_arena.remaining_space = debug_arena_size;
			}

			{
				uint32 renderer_arena_size = MEGABYTES(64);
				memory.renderer_arena = {};
				memory.renderer_arena.memory = VirtualAlloc(NULL, renderer_arena_size,
															MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

				memory.renderer_arena.push_ptr = (uint8*) memory.renderer_arena.memory;
				memory.renderer_arena.remaining_space = renderer_arena_size;
			}


			// Platform API function pointers
			memory.platform_api = {};
			memory.platform_api.LogInfo				 = &Win32LogInfo;
			memory.platform_api.LogError			 = &Win32LogError;
			memory.platform_api.DebugReadFile	     = &Win32DebugReadFile;
			memory.platform_api.DebugWriteFile		 = &Win32DebugWriteFile;
			memory.platform_api.DebugFreeFileMemory	 = &Win32DebugFreeFileMemory;


			if (memory.persistent_memory && memory.transient_memory && memory.renderer_arena.memory)
			{
				// Game code
				win32_game_code game_code = {};

				bool game_dll_info_setup = Win32SetupGameInfo(game_info, persistent_stack_ptr,
															 CLAMPED_REMAINING_MEMORY(persistent_stack_ptr, UINT32_MAX));
				if (game_dll_info_setup)
				{
					game_code = Win32LoadGameCode(game_info.dll_path, game_info.loaded_dll_path);
				}

				// Raw input
				bool input_ready = Win32InitRawInput();

				// Vulkan
				bool renderer_ready = false;

				{
					HMODULE renderer_module = LoadLibraryA("renderer.dll");

					if (renderer_module != INVALID_HANDLE_VALUE)
					{
						renderer_ready = Win32InitVulkanRenderer(renderer_module, &memory.renderer_arena,
																 instance, windowHandle,
																 memory.platform_api.RendererAPI.RendererState,
																 memory.platform_api.RendererAPI.VulkanAPI);
						renderer_ready = Win32LoadRendererFunctionPointers(renderer_module, &memory.platform_api.RendererAPI);
					}

					else
					{
						WIN32LOG_FATAL("Failed to load the renderer.dll module");
					}
				}

				if (game_code.is_valid && input_ready && renderer_ready)
				{
					running = true;
					if (game_code.game_init_func(&memory))
					{

						/// Main Loop
						while (running)
						{

							MSG message;
							while (PeekMessage(&message, windowHandle, 0, 0, PM_REMOVE))
							{
// 								if (message.message == WM_INPUT)
// 								{
// 									Win32HandleRawInput(message.message, message.wParam,
// 														message.lParam);
// 								}
								
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

							// TODO(soimn): setup timed loop and pass proper delta-t
							game_code.game_update_func(&memory, 3.0f);


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
				WIN32LOG_FATAL("Failed to allocated memory for the game");
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
 *	- Fix hot reloading bug which causes hot reloading to act weird when run in and out of VS,
 *	  and seems to reload the dll correctly only twice.
 *	- Setup game input abstraction supporting future rebinding of keys
 *	- Refactor input handling to allow more explicit additions and deletions of devices
 *	- Setup XAudio2
 *	- Add integrated graphics support in the physical device
 *	  selection, and check setup for compatibility
 *	- Improve Vulkan initialization error messages
 *	- Implement proper file handling with relative paths based on
 *	  a base directory
 *	- Architecture a memory management system with a low headache factor
 *	- Sanity check the temporary memory allocations in Win32InitVulkan,
 *	  as a failed heap allocation is currently considered fatal.
 *	- Consider supporting multiple window surfaces
 */
