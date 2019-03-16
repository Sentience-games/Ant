#include "win32_ant.h"

/*
 *	GLOBAL *cough* VARIABLES
 */

// TODO(soimn): delegate responsibility, to the game, to confirm the quit message, in case saving is due or the game needs to halt the player
global_variable bool Running						 = false;
global_variable float TargetFrameSecounds			 = (float) (1.0f / ((float)DEFAULT_TARGET_FPS * 1000.0f));
global_variable LARGE_INTEGER PerformanceCounterFreq = {};

global_variable memory_arena* FrameTempArena;

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
Win32GetCWD(memory_arena* persistent_memory, uint32* result_length)
{
	wchar_t* result = NULL;
    
	// FIXME(soimn): this will most certainly span multiple pages, which may be a problem
	struct temporary_memory { memory_arena arena; };
	temporary_memory* temp_memory = BootstrapPushSize(temporary_memory, arena, KILOBYTES(32));
    
	u32 file_path_growth_amount = 256 * sizeof(wchar_t);
	u32 file_path_memory_size = file_path_growth_amount;
    
	wchar_t* file_path = (wchar_t*) PushSize(&temp_memory->arena, file_path_growth_amount);
	u32 path_length;
    
	for (;;)
	{
		path_length = GetModuleFileNameW(NULL, file_path, file_path_memory_size);
		
		// NOTE(soimn): this asserts that this function succeeds
		Assert(path_length);
        
		if (path_length != file_path_memory_size)
		{
			break;
		}
        
		else
		{
			if (file_path_growth_amount >= temp_memory->arena.current_block->remaining_space)
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
    
	u32 length = 0;
	for (wchar_t* scan = file_path; *scan; ++scan)
	{	
		if (*scan == L'\\')
		{
			length = (u32)(scan - file_path);
		}
	}
    
	if (length == 0)
	{
		WIN32LOG_FATAL("Failed to get the current working directory of the game, no path separator found");
		return NULL;
	}
    
	++length;
    
	result = (wchar_t*) PushSize(persistent_memory, sizeof(wchar_t) * (length + 1));
	CopyArray(file_path, length, result);
	result[length] = (wchar_t)(NULL);
    
	*result_length = length;
    
	return result;
}


// FIXME(soimn): produces ant_loaded.dl instead of ant_loaded.dll
internal void
Win32BuildFullyQualifiedPath(win32_game_info* game_info, const wchar_t* appendage,
							 wchar_t* buffer, u32 buffer_length)
{
	u32 length_of_cwd = 0;
	for (wchar_t* scan = (wchar_t*) game_info->cwd; *scan; ++scan)
	{
		++length_of_cwd;
	}
    
	i32 length_of_appendage = wstrlength(appendage, buffer_length);
    
	Assert(length_of_appendage != -1);
	Assert(length_of_cwd && buffer_length > (length_of_cwd + length_of_appendage));
    
	CopyArray(game_info->cwd, length_of_cwd, buffer);
	CopyArray(appendage, length_of_appendage, buffer + length_of_cwd);
};

// TODO(soimn): make this more robust to enable hot loading capability in release mode
#ifdef ANT_ENABLE_HOT_RELOADING
FILETIME
Win32DebugGetFileCreateTime(const wchar_t* file_path)
{
	FILETIME result = {};
    
	WIN32_FILE_ATTRIBUTE_DATA data;
	if (GetFileAttributesExW(file_path, GetFileExInfoStandard, &data))
	{
		result = data.ftLastWriteTime;
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
        
        
		// NOTE(soimn): this is temporary
		case PlatformFileType_ShaderFile:
        directory = L"data\\";
        wildcard  = L"data\\*.spv";
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
		
		Assert(size <= UINT32_MAX);
		u32 file_size_truncated = (u32) CLAMP(0, size, UINT32_MAX);
        
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
/// Vulkan
///

internal VKAPI_ATTR VkBool32 VKAPI_CALL
Win32VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
						 VkDebugUtilsMessageTypeFlagsEXT message_type,
						 const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
						 void* user_data)
{
	if (message_type != VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
	{
		switch(message_severity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            WIN32LOG_ERROR(callback_data->pMessage);
			break;
            
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            WIN32LOG_INFO(callback_data->pMessage);
			break;
            
			INVALID_DEFAULT_CASE;
		}
	}
    
	return VK_FALSE;
}

internal bool
Win32InitVulkan(memory_arena* temp_memory, memory_arena* persistent_memory,
				win32_vulkan_binding* binding, vulkan_renderer_state* vulkan_state,
				HINSTANCE process_instance, HWND window_handle,
				const char* application_name, u32 application_version)
{
	bool succeeded = false;
    
	binding->module = LoadLibraryA("vulkan-1.dll");
    
	if (binding->module != INVALID_HANDLE_VALUE)
	{
		do
		{
			Vulkan = vulkan_state;
            
			//// Load exported and global level vulkan functions
#define VK_EXPORTED_FUNCTION(func)\
            Vulkan->##func = (PFN_##func) GetProcAddress(binding->module, STRINGIFY(func));\
            if (!Vulkan->##func)\
            {\
                WIN32LOG_FATAL("Failed to load the exported vulkan function '" STRINGIFY(func) "'.");\
                break;\
            }
            
#define VK_GLOBAL_LEVEL_FUNCTION(func)\
            Vulkan->##func = (PFN_##func) Vulkan->vkGetInstanceProcAddr(NULL, STRINGIFY(func));\
            if (!Vulkan->##func)\
            {\
                WIN32LOG_FATAL("Failed to load the global level vulkan function '" STRINGIFY(func) "'.");\
                break;\
            }
            
#include "vulkan/vulkan_functions.inl"
            
#undef VK_GLOBAL_LEVEL_FUNCTION
#undef VK_EXPORTED_FUNCTION
            
			VkResult result;
#define VK_CHECK(call)\
            result = call##;\
            if (result != VK_SUCCESS)\
            {\
                WIN32LOG_FATAL("The call to the vulkan function '" STRINGIFY(call) "' failed.");\
                break;\
            }
            
#define VK_NESTED_CHECK(call, encountered_error)\
            result = call##;\
            if (result != VK_SUCCESS)\
            {\
                WIN32LOG_FATAL("The call to the vulkan function '" STRINGIFY(call) "' failed.");\
                encountered_error = true;\
                break;\
            }
            
			const char* required_instance_extensions[] = {
#define INSTANCE_EXTENSIONS
#define PLATFORM_WINDOWS
                
#ifdef ANT_DEBUG
#define DEBUG_EXTENSIONS
#include "vulkan/vulkan_extensions.inl"
#undef DEBUG_EXTENSIONS
                
#else
#include "vulkan/vulkan_extensions.inl"
#endif
                
#undef PLATFORM_WINDOWS
#undef INSTANCE_EXTENSIONS
			};
            
			const char* required_layers[] = {
#ifdef ANT_DEBUG
#define LAYERS
#include "vulkan/vulkan_extensions.inl"
#undef LAYERS
#endif
			};
            
			const char* required_device_extensions[] = 
			{
#define DEVICE_EXTENSIONS
#include "vulkan/vulkan_extensions.inl"
#undef DEVICE_EXTENSIONS
			};
            
			///
			/// Setup instance
			///
            
			{ //// Validate extension support	
				u32 found_extension_count = 0;
                
				u32 extension_count;
				VK_CHECK(Vulkan->vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL));
                
				VkExtensionProperties* extension_properties = PushArray(temp_memory, VkExtensionProperties, extension_count);
				VK_CHECK(Vulkan->vkEnumerateInstanceExtensionProperties(NULL, &extension_count, extension_properties));
                
				for (char** current_extension_name = (char**) required_instance_extensions;
					 current_extension_name < required_instance_extensions + ARRAY_COUNT(required_instance_extensions);
					 ++current_extension_name)
				{
					for (VkExtensionProperties* current_property = extension_properties;
						 current_property < extension_properties + extension_count;
						 ++current_property)
					{
						if (strcompare(*current_extension_name, current_property->extensionName))
						{
							++found_extension_count;
							break;
						}
					}
				}
                
				if (found_extension_count != ARRAY_COUNT(required_instance_extensions))
				{
					WIN32LOG_FATAL("Instance extension property validation failed. One or more of the required vulkan instance extensions are not"
								   " present.");
					break;
				}
			}
            
#ifdef ANT_DEBUG
			{ //// Validate layer support
				u32 found_layer_count = 0;
                
				u32 layer_count;
				VK_CHECK(Vulkan->vkEnumerateInstanceLayerProperties(&layer_count, NULL));
                
				VkLayerProperties* layer_properties = PushArray(temp_memory, VkLayerProperties, layer_count);
				VK_CHECK(Vulkan->vkEnumerateInstanceLayerProperties(&layer_count, layer_properties));
                
				for (char** current_layer_name = (char**) required_layers;
					 current_layer_name < required_layers + ARRAY_COUNT(required_layers);
					 ++current_layer_name)
				{
					for (VkLayerProperties* current_property = layer_properties;
						 current_property < layer_properties + layer_count;
						 ++current_property)
					{
						if (strcompare(*current_layer_name, current_property->layerName))
						{
							++found_layer_count;
							break;
						}
                        
					}
				}
                
				if (found_layer_count != ARRAY_COUNT(required_layers))
				{
					WIN32LOG_FATAL("Instance layer property validation failed. One or more of the required vulkan layers are not present.");
					break;
				}
			}
#endif
			
			{ //// Create instance
				VkApplicationInfo app_info = {};
				app_info.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
				app_info.pApplicationName	= application_name;
				app_info.applicationVersion = application_version;
				app_info.pEngineName		= "Ant Engine";
				app_info.engineVersion		= ANT_VERSION;
				app_info.apiVersion			= VK_API_VERSION_1_0;
                
				VkInstanceCreateInfo create_info = {};
				create_info.sType					= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
				create_info.pNext					= NULL;
				create_info.flags					= 0;
				create_info.pApplicationInfo		= &app_info;
				create_info.enabledLayerCount		= ARRAY_COUNT(required_layers);
				create_info.ppEnabledLayerNames		= required_layers;
				create_info.enabledExtensionCount	= ARRAY_COUNT(required_instance_extensions);
				create_info.ppEnabledExtensionNames = required_instance_extensions;
                
				VK_CHECK(Vulkan->vkCreateInstance(&create_info, NULL, &Vulkan->instance));
			}
            
			// Load instance level functions
#define VK_INSTANCE_LEVEL_FUNCTION(func)\
            Vulkan->##func = (PFN_##func) Vulkan->vkGetInstanceProcAddr(Vulkan->instance, STRINGIFY(func));\
            if (!Vulkan->##func)\
            {\
                WIN32LOG_FATAL("Failed to load instance level function '" STRINGIFY(func) "'.");\
                break;\
            }
			
#include "vulkan/vulkan_functions.inl"
			
#undef VK_INSTANCE_LEVEL_FUNCTION
            
#ifdef ANT_DEBUG
            
			{ //// Setup debug messenger
                
				// TODO(soimn): consider keeping this
				VkDebugUtilsMessengerEXT error_callback = VK_NULL_HANDLE;
                
				VkDebugUtilsMessengerCreateInfoEXT create_info = {};
				create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
                
				create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
                    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
                
				create_info.messageType	    = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                    | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                    | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                
				create_info.pfnUserCallback = &Win32VulkanDebugCallback;
				create_info.pUserData		= NULL;
                
				PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT =
					(PFN_vkCreateDebugUtilsMessengerEXT) Vulkan->vkGetInstanceProcAddr(Vulkan->instance, "vkCreateDebugUtilsMessengerEXT");
				
				if (!vkCreateDebugUtilsMessengerEXT)
				{
					WIN32LOG_FATAL("Failed to load the instance level function 'vkCreateDebugUtilsMessengerEXT'.");
					break;
				}
                
				VK_CHECK(vkCreateDebugUtilsMessengerEXT(Vulkan->instance, &create_info, NULL, &error_callback));
			}
            
#endif
            
			{ //// Create window surface
				VkWin32SurfaceCreateInfoKHR create_info = {};
				create_info.sType	  = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
				create_info.pNext	  = NULL;
				create_info.flags	  = 0;
				create_info.hinstance = process_instance;
				create_info.hwnd	  = window_handle;
                
				PFN_vkCreateWin32SurfaceKHR	vkCreateWin32SurfaceKHR =
					(PFN_vkCreateWin32SurfaceKHR) Vulkan->vkGetInstanceProcAddr(Vulkan->instance, "vkCreateWin32SurfaceKHR");
                
				if (!vkCreateWin32SurfaceKHR)
				{
					WIN32LOG_FATAL("Failed to load the instance level function 'vkCreateWin32SurfaceKHR'.");
					break;
				}
                
				VK_CHECK(vkCreateWin32SurfaceKHR(Vulkan->instance, &create_info, NULL, &Vulkan->surface));
			}	
            
			///
			/// Setup physical device
			///
            
			{ //// Acquire physical device
				u32 device_count;
				VK_CHECK(Vulkan->vkEnumeratePhysicalDevices(Vulkan->instance, &device_count, NULL));
                
				VkPhysicalDevice* physical_devices = PushArray(temp_memory, VkPhysicalDevice, device_count);
				VK_CHECK(Vulkan->vkEnumeratePhysicalDevices(Vulkan->instance, &device_count, physical_devices));
                
				bool encountered_errors = false;
                
				u32 best_score = 0;
				VkPhysicalDevice chosen_device = VK_NULL_HANDLE;
				for (u32 i = 0; i < device_count; ++i)
				{
					u32 device_score = 0;
					bool device_invalid = false;
                    
					VkPhysicalDeviceProperties properties;
					VkPhysicalDeviceFeatures features;
                    
					Vulkan->vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
					Vulkan->vkGetPhysicalDeviceFeatures(physical_devices[i], &features);
                    
					bool supports_graphics	   = false;
					bool supports_compute	   = false;
					bool supports_transfer	   = false;
					bool supports_presentation = false;
                    
					{ //// Queues
						u32 queue_family_count;
						Vulkan->vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, NULL);
                        
						VkQueueFamilyProperties* queue_family_properties = PushArray(temp_memory, VkQueueFamilyProperties, queue_family_count);
						Vulkan->vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, queue_family_properties);
                        
						bool nested_encountered_errors = false;
						for (u32 j = 0; j < queue_family_count; ++j)
						{
							if (queue_family_properties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
							{
								supports_graphics = true;
							}
                            
							if (queue_family_properties[j].queueFlags & VK_QUEUE_COMPUTE_BIT)
							{
								supports_compute = true;
							}
                            
							if (queue_family_properties[j].queueFlags & VK_QUEUE_TRANSFER_BIT)
							{
								supports_transfer = true;
							}
                            
							VkBool32 queue_supports_presentation = {};
							VK_NESTED_CHECK(Vulkan->vkGetPhysicalDeviceSurfaceSupportKHR(physical_devices[i], j, Vulkan->surface,
                                                                                         &queue_supports_presentation), nested_encountered_errors);
							supports_presentation = (supports_presentation || (bool) queue_supports_presentation);
						}
                        
						if (nested_encountered_errors)
						{
							encountered_errors = true;
							break;
						}
                        
						if (!supports_graphics || !supports_transfer || !supports_presentation)
						{
							device_invalid = true;
						}
                        
						if (supports_compute)
						{
							device_score += 250;
						}
					}
                    
					{ //// Extensions
						u32 found_extension_count = 0;
                        
						u32 extension_count;
						VK_CHECK(Vulkan->vkEnumerateDeviceExtensionProperties(physical_devices[i], NULL, &extension_count, NULL));
                        
						VkExtensionProperties* extension_properties = PushArray(temp_memory, VkExtensionProperties, extension_count);
						VK_CHECK(Vulkan->vkEnumerateDeviceExtensionProperties(physical_devices[i], NULL, &extension_count, extension_properties));
                        
						for (char** current_extension_name = (char**) required_device_extensions;
							 current_extension_name < required_device_extensions + ARRAY_COUNT(required_device_extensions);
							 ++current_extension_name)
						{
							for (VkExtensionProperties* current_property = extension_properties;
								 current_property < extension_properties + extension_count;
								 ++current_property)
							{
								if (strcompare(*current_extension_name, current_property->extensionName))
								{
									++found_extension_count;
									break;
								}
							}
						}
                        
						if (found_extension_count != ARRAY_COUNT(required_device_extensions))
						{
							device_invalid = true;
						}
					}
                    
					{ //// Surface format & present mode
						u32 format_count;
						u32 present_mode_count;
                        
						Vulkan->vkGetPhysicalDeviceSurfaceFormatsKHR(physical_devices[i], Vulkan->surface, &format_count, NULL);
						Vulkan->vkGetPhysicalDeviceSurfacePresentModesKHR(physical_devices[i], Vulkan->surface, &present_mode_count, NULL);
						
						if (format_count == 0 || present_mode_count == 0)
						{
							device_invalid = true;
						}
					}
                    
					{ //// Traits
						if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
						{
							device_score += 1000;
						}
                        
						device_score += properties.limits.maxImageDimension2D;
                        
						if (!features.geometryShader)
						{
							device_invalid = true;
						}
					}
                    
					if (device_invalid)
					{
						device_score = 0;
					}
                    
					if (device_score > best_score)
					{
						chosen_device = physical_devices[i];
					}
				}
                
				if (encountered_errors)
				{
					break;
				}
                
				if (chosen_device == VK_NULL_HANDLE)
				{
					WIN32LOG_FATAL("Failed to find a suitable device supporting vulkan");
					break;
				}
                
				else
				{
					Vulkan->physical_device = chosen_device;
				}
			}
            
			{ //// Acquire queue families
				Vulkan->graphics_family		   = -1;
				Vulkan->compute_family			   = -1;
				Vulkan->dedicated_transfer_family = -1;
				Vulkan->present_family			   = -1;
                
				u32 queue_family_count;
				Vulkan->vkGetPhysicalDeviceQueueFamilyProperties(Vulkan->physical_device, &queue_family_count, NULL);
                
				VkQueueFamilyProperties* queue_family_properties = PushArray(temp_memory, VkQueueFamilyProperties, queue_family_count);
				Vulkan->vkGetPhysicalDeviceQueueFamilyProperties(Vulkan->physical_device, &queue_family_count, queue_family_properties);
                
				for (u32 i = 0; i < queue_family_count; ++i)
				{
					if (queue_family_properties[i].queueCount != 0)
					{
						if ((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
							&& Vulkan->graphics_family == -1)
						{
							Vulkan->graphics_family = (i32) i;
						}
                        
						if (queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
						{
							Vulkan->compute_family = (i32) i;
						}
                        
						if (queue_family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
						{
							Vulkan->dedicated_transfer_family = (i32) i;
						}
                        
						if (Vulkan->present_family == -1)
						{
							VkBool32 queue_family_supports_presentation;
							Vulkan->vkGetPhysicalDeviceSurfaceSupportKHR(Vulkan->physical_device, i,
                                                                         Vulkan->surface, &queue_family_supports_presentation);
                            
							if (queue_family_supports_presentation)
							{
								Vulkan->present_family = (i32) i;
							}
						}
					}
				}
                
				if (Vulkan->graphics_family == -1 || Vulkan->present_family == -1)
				{
					WIN32LOG_FATAL("Failed to acquire graphics and/or present queue families.");
					break;
				}
                
				if (Vulkan->compute_family == Vulkan->graphics_family
					|| (Vulkan->compute_family == Vulkan->present_family
						&& queue_family_properties[Vulkan->compute_family].queueCount < 2))
				{
					Vulkan->supports_compute = false;
					Vulkan->compute_family	  = -1;
				}
                
				else if ((Vulkan->compute_family == Vulkan->present_family
                          && Vulkan->compute_family == Vulkan->dedicated_transfer_family)
						 && queue_family_properties[Vulkan->compute_family].queueCount < 3)
				{
					Vulkan->supports_compute = false;
					Vulkan->compute_family   = -1;
                    
					if (Vulkan->dedicated_transfer_family == Vulkan->graphics_family
						|| (Vulkan->dedicated_transfer_family == Vulkan->present_family
							&& queue_family_properties[Vulkan->present_family].queueCount < 2))
					{
						Vulkan->supports_dedicated_transfer = false;
						Vulkan->dedicated_transfer_family   = -1;
					}
                    
					else if (Vulkan->dedicated_transfer_family != -1)
					{
                        
						Vulkan->supports_dedicated_transfer = true;
					}
				}
                
				else if (Vulkan->compute_family != -1)
				{
					Vulkan->supports_compute  = true;
				}
			}
            
			///
			/// Setup logical device
			///
            
			{ //// Create logical device
				const float queue_priorities[4] = {1.0f, 1.0f, 1.0f, 1.0f};
				
				VkDeviceQueueCreateInfo queue_create_info[4] = {};
				u32 queue_create_info_count = 0;
				u32 present_family_index	= 0;
				u32 compute_family_index	= 0;
                
				// Graphics queue
				queue_create_info[queue_create_info_count].sType			= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queue_create_info[queue_create_info_count].pNext			= NULL;
				queue_create_info[queue_create_info_count].flags			= 0;
				queue_create_info[queue_create_info_count].queueFamilyIndex = Vulkan->graphics_family;
				queue_create_info[queue_create_info_count].queueCount		= 1;
				queue_create_info[queue_create_info_count].pQueuePriorities	= &queue_priorities[0];
				++queue_create_info_count;
                
				if (Vulkan->present_family == Vulkan->graphics_family)
				{
					present_family_index = queue_create_info_count;
					++queue_create_info[queue_create_info_count].queueCount;
				}
                
				else
				{
					present_family_index = queue_create_info_count;
					queue_create_info[present_family_index].sType			 = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
					queue_create_info[present_family_index].pNext			 = NULL;
					queue_create_info[present_family_index].flags			 = 0;
					queue_create_info[present_family_index].queueFamilyIndex = Vulkan->present_family;
					queue_create_info[present_family_index].queueCount		 = 1;
					queue_create_info[present_family_index].pQueuePriorities = &queue_priorities[1];
					++queue_create_info_count;
				}
                
				if (Vulkan->supports_compute)
				{
					if (Vulkan->compute_family == Vulkan->present_family)
					{
						++queue_create_info[present_family_index].queueCount;
					}
                    
					else
					{
						compute_family_index = queue_create_info_count;
						queue_create_info[compute_family_index].sType			 = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
						queue_create_info[compute_family_index].pNext			 = NULL;
						queue_create_info[compute_family_index].flags			 = 0;
						queue_create_info[compute_family_index].queueFamilyIndex = Vulkan->compute_family;
						queue_create_info[compute_family_index].queueCount		 = 1;
						queue_create_info[compute_family_index].pQueuePriorities = &queue_priorities[2];
						++queue_create_info_count;
					}
				}
                
				if (Vulkan->supports_dedicated_transfer)
				{
					if (Vulkan->dedicated_transfer_family == Vulkan->present_family)
					{
						++queue_create_info[present_family_index].queueCount;
					}
                    
					else if (Vulkan->dedicated_transfer_family == Vulkan->compute_family)
					{
						++queue_create_info[compute_family_index].queueCount;
					}
                    
					else
					{
						queue_create_info[queue_create_info_count].sType			= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
						queue_create_info[queue_create_info_count].pNext			= NULL;
						queue_create_info[queue_create_info_count].flags			= 0;
						queue_create_info[queue_create_info_count].queueFamilyIndex = Vulkan->dedicated_transfer_family;
						queue_create_info[queue_create_info_count].queueCount		= 1;
						queue_create_info[queue_create_info_count].pQueuePriorities	= &queue_priorities[3];
						++queue_create_info_count;
					}
				}
                
				VkPhysicalDeviceFeatures physical_device_features = {};
				physical_device_features.geometryShader = VK_TRUE;
                
				VkDeviceCreateInfo device_create_info = {};
				device_create_info.sType				   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
				device_create_info.pNext				   = NULL;
				device_create_info.flags				   = 0;
				device_create_info.queueCreateInfoCount	   = queue_create_info_count;
				device_create_info.pQueueCreateInfos	   = &queue_create_info[0];
				device_create_info.enabledExtensionCount   = ARRAY_COUNT(required_device_extensions);
				device_create_info.ppEnabledExtensionNames = required_device_extensions;
				device_create_info.pEnabledFeatures		   = &physical_device_features;
                
				VK_CHECK(Vulkan->vkCreateDevice(Vulkan->physical_device, &device_create_info, NULL, &Vulkan->device));
                
				Vulkan->vkGetPhysicalDeviceMemoryProperties(Vulkan->physical_device, &Vulkan->physical_device_memory_properties);
			}
            
			// Load device level functions
#define VK_DEVICE_LEVEL_FUNCTION(func)\
            Vulkan->##func = (PFN_##func) Vulkan->vkGetDeviceProcAddr(Vulkan->device, #func);\
            if (!Vulkan->##func)\
            {\
                WIN32LOG_FATAL("Failed to load device level function '" #func "'.");\
                break;\
            }
			
#include "vulkan/vulkan_functions.inl"
			
#undef VK_DEVICE_LEVEL_FUNCTION
            
			{ //// Setup queues
				Vulkan->vkGetDeviceQueue(Vulkan->device, Vulkan->graphics_family, 0, &Vulkan->graphics_queue);
                
				u32 present_family_queue_index = 0;
				Vulkan->vkGetDeviceQueue(Vulkan->device, Vulkan->present_family, present_family_queue_index, &Vulkan->present_queue);
				
				if (Vulkan->supports_compute)
				{
					if (Vulkan->compute_family == Vulkan->present_family)
					{
						Vulkan->vkGetDeviceQueue(Vulkan->device, Vulkan->present_family,
                                                 ++present_family_queue_index, &Vulkan->compute_queue);
					}
                    
					else
					{
						Vulkan->vkGetDeviceQueue(Vulkan->device, Vulkan->compute_family, 0, &Vulkan->compute_queue);
					}
				}
                
				Vulkan->vkGetDeviceQueue(Vulkan->device, Vulkan->graphics_family, 0, &Vulkan->transfer_queue);
                
				if (Vulkan->supports_dedicated_transfer)
				{
					if (Vulkan->dedicated_transfer_family == Vulkan->present_family)
					{
						Vulkan->vkGetDeviceQueue(Vulkan->device, Vulkan->present_family,
                                                 ++present_family_queue_index, &Vulkan->dedicated_transfer_queue);
					}
                    
					else if (Vulkan->dedicated_transfer_family == Vulkan->compute_family)
					{
						Vulkan->vkGetDeviceQueue(Vulkan->device, Vulkan->compute_family, 1, &Vulkan->dedicated_transfer_queue);
					}
                    
					else
					{
						Vulkan->vkGetDeviceQueue(Vulkan->device, Vulkan->dedicated_transfer_family,
                                                 0, &Vulkan->dedicated_transfer_queue);
					}
				}
			}
            
			///
			/// Setup swapchain
			///
            
			{ //// Create swapchain
				VkExtent2D swapchain_extent				= {};
				VkSurfaceFormatKHR swapchain_format		= {};
				VkPresentModeKHR swapchain_present_mode = {};
				u32 image_count = 2;
                
				{ //// Pick swapchain extent and ensure image usage and count support
					VkSurfaceCapabilitiesKHR capabilities;
					VK_CHECK(Vulkan->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Vulkan->physical_device, Vulkan->surface, &capabilities));
                    
					if (capabilities.currentExtent.width != UINT32_MAX)
					{
						swapchain_extent = capabilities.currentExtent;
					}
                    
					else
					{
						swapchain_extent = {CLAMP(capabilities.minImageExtent.width, 2560,
												  capabilities.maxImageExtent.width),
                            CLAMP(capabilities.minImageExtent.height, 1440,
                                  capabilities.maxImageExtent.height)};
					}
                    
					if ((capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == 0)
					{
						WIN32LOG_FATAL("Failed to create swapchain, as the surface does not support image transfers to the swapchain images.");
						break;
					}
                    
					else if (capabilities.minImageCount > image_count)
					{
						image_count = capabilities.minImageCount;
					}
				}
                
				{ //// Pick swapchain format
					u32 format_count;
					VkSurfaceFormatKHR* formats;
                    
					VK_CHECK(Vulkan->vkGetPhysicalDeviceSurfaceFormatsKHR(Vulkan->physical_device, Vulkan->surface,
                                                                          &format_count, NULL));
                    
					if (format_count == 0)
					{
						WIN32LOG_FATAL("Failed to retrieve surface formats for swapchain creation.");
						break;
					}
                    
					formats = PushArray(temp_memory, VkSurfaceFormatKHR, format_count);
					VK_CHECK(Vulkan->vkGetPhysicalDeviceSurfaceFormatsKHR(Vulkan->physical_device, Vulkan->surface,
                                                                          &format_count, formats));
                    
					bool found_format = false;
                    
					if (format_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
					{
						swapchain_format = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
						found_format	 = true;
					}
                    
					else
					{
						for (u32 i = 0; i < format_count; ++i)
						{
							if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
							{
								swapchain_format = formats[i];
								found_format	 = true;
								break;
							}
						}
					}
                    
					if (!found_format)
					{
						swapchain_format = formats[0];
					}
				}
                
				{ //// Pick swapchain present mode
					u32 present_mode_count;
					VkPresentModeKHR* present_modes;
                    
					VK_CHECK(Vulkan->vkGetPhysicalDeviceSurfacePresentModesKHR(Vulkan->physical_device, Vulkan->surface,
                                                                               &present_mode_count, NULL));
                    
					present_modes = PushArray(temp_memory, VkPresentModeKHR, present_mode_count);
					VK_CHECK(Vulkan->vkGetPhysicalDeviceSurfacePresentModesKHR(Vulkan->physical_device, Vulkan->surface,
                                                                               &present_mode_count, present_modes));
                    
					// IMPORTANT(soimn): this loop is biasing FIFO_RELAXED, which may not always be optimal
					u32 best_score = 0;
					for (u32 i = 0; i < present_mode_count; ++i)
					{
						if (present_modes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
						{
							swapchain_present_mode = present_modes[i];
							best_score = 1000;
							break;
						}
                        
						else if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR
								 && best_score < 800)
						{
							best_score = 800;
							swapchain_present_mode = present_modes[i];
						}
                        
                        
						else if (present_modes[i] == VK_PRESENT_MODE_FIFO_KHR
								 && best_score < 600)
						{
							best_score = 600;
							swapchain_present_mode = present_modes[i];
						}
					}
                    
					if (!best_score)
					{
						WIN32LOG_FATAL("Failed to find a suitable present mode for swapchain creation");
						break;
					}
				}
                
				Vulkan->swapchain.extent		  = swapchain_extent;
				Vulkan->swapchain.surface_format = swapchain_format;
				Vulkan->swapchain.present_mode   = swapchain_present_mode;
                
				VkSwapchainCreateInfoKHR create_info = {};
				create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
                
				create_info.minImageCount	 = image_count;
				create_info.imageFormat		 = swapchain_format.format;
				create_info.imageColorSpace  = swapchain_format.colorSpace;
				create_info.imageExtent		 = swapchain_extent;
				create_info.imageUsage		 = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				create_info.presentMode		 = swapchain_present_mode;
				create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				create_info.clipped			 = VK_TRUE;
				create_info.surface			 = Vulkan->surface;
				create_info.imageArrayLayers = 1;
				create_info.preTransform	 = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
				create_info.compositeAlpha	 = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
                
				VK_CHECK(Vulkan->vkCreateSwapchainKHR(Vulkan->device, &create_info, NULL, &Vulkan->swapchain.handle));
			}
            
			{ //// Create swapchain images and image views
				VK_CHECK(Vulkan->vkGetSwapchainImagesKHR(Vulkan->device, Vulkan->swapchain.handle,
                                                         &Vulkan->swapchain.image_count, NULL));
                
				Vulkan->swapchain.images = PushArray(persistent_memory, VkImage, Vulkan->swapchain.image_count);
                
				VK_CHECK(Vulkan->vkGetSwapchainImagesKHR(Vulkan->device, Vulkan->swapchain.handle,
                                                         &Vulkan->swapchain.image_count, Vulkan->swapchain.images));
                
				Vulkan->swapchain.subresource_range.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
				Vulkan->swapchain.subresource_range.baseMipLevel	= 0;
				Vulkan->swapchain.subresource_range.levelCount		= 1;
				Vulkan->swapchain.subresource_range.baseArrayLayer = 0;
				Vulkan->swapchain.subresource_range.layerCount		= 1;
			}
            
			{ //// Create swapchain semaphores
				VkSemaphoreCreateInfo create_info = {};
				create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				create_info.pNext = NULL;
				create_info.flags = 0;
                
				VK_CHECK(Vulkan->vkCreateSemaphore(Vulkan->device, &create_info, NULL, &Vulkan->swapchain.image_available_semaphore));
				VK_CHECK(Vulkan->vkCreateSemaphore(Vulkan->device, &create_info, NULL, &Vulkan->swapchain.transfer_done_semaphore));
			}
            
			{ //// Setup swapchain present command buffers
				VkCommandPoolCreateInfo create_info = {};
				create_info.sType			  = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				create_info.pNext			  = NULL;
				create_info.flags			  = 0;
				create_info.queueFamilyIndex = Vulkan->present_family;
                
				VK_CHECK(Vulkan->vkCreateCommandPool(Vulkan->device, &create_info, NULL, &Vulkan->swapchain.present_pool));
                
				Vulkan->swapchain.present_buffers = PushArray(persistent_memory, VkCommandBuffer, Vulkan->swapchain.image_count);
                
				VkCommandBufferAllocateInfo allocate_info = {};
				allocate_info.sType				 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocate_info.pNext				 = NULL;
				allocate_info.commandPool		 = Vulkan->swapchain.present_pool;
				allocate_info.level				 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocate_info.commandBufferCount = Vulkan->swapchain.image_count;
                
				VK_CHECK(Vulkan->vkAllocateCommandBuffers(Vulkan->device, &allocate_info, Vulkan->swapchain.present_buffers));
			}
            
			///
			/// Setup render target
			///
			
			{ //// Create render target image
				Vulkan->render_target.extent = Vulkan->swapchain.extent;
                
				VkImageCreateInfo create_info = {};
				create_info.sType	  = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				create_info.pNext	  = NULL;
				create_info.flags	  = 0;
				create_info.imageType = VK_IMAGE_TYPE_2D;
				create_info.format	  = Vulkan->swapchain.surface_format.format;
                
				create_info.extent = {Vulkan->render_target.extent.width,
                    Vulkan->render_target.extent.height, 1};
                
				create_info.mipLevels			  = 1;
				create_info.arrayLayers			  = 1;
				create_info.samples				  = VK_SAMPLE_COUNT_1_BIT;
				create_info.tiling				  = VK_IMAGE_TILING_OPTIMAL;
				create_info.usage				  = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
				create_info.sharingMode			  = VK_SHARING_MODE_EXCLUSIVE;
				create_info.queueFamilyIndexCount = 0;
				create_info.pQueueFamilyIndices	  = NULL;
				create_info.initialLayout		  = VK_IMAGE_LAYOUT_UNDEFINED;
                
				VK_CHECK(Vulkan->vkCreateImage(Vulkan->device, &create_info, NULL, &Vulkan->render_target.image));
                
				VkMemoryRequirements memory_requirements;
				Vulkan->vkGetImageMemoryRequirements(Vulkan->device, Vulkan->render_target.image, &memory_requirements);
                
				VkMemoryPropertyFlags required_memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                
				i32 selected_memory_type = -1;
				for (u32 i = 0; i < Vulkan->physical_device_memory_properties.memoryTypeCount; ++i)
				{
					if ((memory_requirements.memoryTypeBits & (1 << i)) &&
						(Vulkan->physical_device_memory_properties.memoryTypes[i].propertyFlags & required_memory_properties) == required_memory_properties)
					{
						selected_memory_type = (i32) i;
					}
				}
                
				if (selected_memory_type == -1)
				{
					WIN32LOG_FATAL("Failed to find an appropriate memory type for the main render target image");
					break;
				}
                
				VkMemoryAllocateInfo allocate_info = {};
				allocate_info.sType			  = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocate_info.pNext			  = NULL;
				allocate_info.allocationSize  = memory_requirements.size;
				allocate_info.memoryTypeIndex = (u32) selected_memory_type;
                
				VK_CHECK(Vulkan->vkAllocateMemory(Vulkan->device, &allocate_info, NULL, &Vulkan->render_target.image_memory));
				VK_CHECK(Vulkan->vkBindImageMemory(Vulkan->device, Vulkan->render_target.image, Vulkan->render_target.image_memory, 0));
			}
            
			{ //// Create render target image view
				VkImageViewCreateInfo create_info = {};
				create_info.sType							= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				create_info.pNext							= NULL;
				create_info.flags							= 0;
				create_info.image							= Vulkan->render_target.image;
				create_info.viewType						= VK_IMAGE_VIEW_TYPE_2D;
				create_info.format							= Vulkan->swapchain.surface_format.format;
                
				create_info.components						= {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                    VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
                
				create_info.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
				create_info.subresourceRange.baseMipLevel	= 0;
				create_info.subresourceRange.levelCount		= 1;
				create_info.subresourceRange.baseArrayLayer = 0;
				create_info.subresourceRange.layerCount		= 1;
                
                
				VK_CHECK(Vulkan->vkCreateImageView(Vulkan->device, &create_info, NULL, &Vulkan->render_target.image_view));
			}
            
			{ //// Create render target render pass
				VkAttachmentDescription attachment_description = {};
				attachment_description.flags		  = 0;
				attachment_description.format		  = Vulkan->swapchain.surface_format.format;
				attachment_description.samples		  = VK_SAMPLE_COUNT_1_BIT;
				attachment_description.loadOp		  = VK_ATTACHMENT_LOAD_OP_CLEAR;
				attachment_description.storeOp		  = VK_ATTACHMENT_STORE_OP_STORE;
				attachment_description.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachment_description.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
				attachment_description.finalLayout	  = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                
				VkAttachmentReference attachment_reference = {
					0,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
				};
                
				VkSubpassDescription primary_subpass = {};
				{
					primary_subpass.flags					= 0;
					primary_subpass.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
					primary_subpass.inputAttachmentCount	= 0;
					primary_subpass.pInputAttachments		= NULL;
					primary_subpass.colorAttachmentCount	= 1;
					primary_subpass.pColorAttachments		= &attachment_reference;
					primary_subpass.pResolveAttachments		= NULL;
					primary_subpass.pDepthStencilAttachment = NULL;
					primary_subpass.preserveAttachmentCount = 0;
					primary_subpass.pPreserveAttachments	= NULL;
				}
                
				VkRenderPassCreateInfo create_info = {};
				create_info.sType			= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
				create_info.pNext			= NULL;
				create_info.flags			= 0;
				create_info.attachmentCount = 1;
				create_info.pAttachments	= &attachment_description;
				create_info.subpassCount	= 1;
				create_info.pSubpasses		= &primary_subpass;
				create_info.dependencyCount = 0;
				create_info.pDependencies	= NULL;
                
				VK_CHECK(Vulkan->vkCreateRenderPass(Vulkan->device, &create_info, NULL, &Vulkan->render_target.render_pass));
			}
            
			{ //// Create render target framebuffer
				VkFramebufferCreateInfo create_info = {};
				create_info.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				create_info.pNext			= NULL;
				create_info.flags			= 0;
				create_info.renderPass		= Vulkan->render_target.render_pass;
				create_info.attachmentCount = 1;
				create_info.pAttachments	= &Vulkan->render_target.image_view;
				create_info.width			= Vulkan->swapchain.extent.width;
				create_info.height			= Vulkan->swapchain.extent.height;
				create_info.layers			= 1;
                
				VK_CHECK(Vulkan->vkCreateFramebuffer(Vulkan->device, &create_info, NULL, &Vulkan->render_target.framebuffer));
			}
            
			{ //// Create render target semaphores
				VkSemaphoreCreateInfo create_info = {};
				create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				create_info.pNext = NULL;
				create_info.flags = 0;
                
				VK_CHECK(Vulkan->vkCreateSemaphore(Vulkan->device, &create_info, NULL, &Vulkan->render_target.render_done_semaphore));
			}
            
			{ //// Create render target immediate pipeline
				platform_file_group file_group = Win32GetAllFilesOfTypeBegin(PlatformFileType_ShaderFile);
                
				VkShaderModule vertex_shader   = {};
				VkShaderModule frag_shader	   = {};
				VkShaderModule geometry_shader = {};
				VkPipelineShaderStageCreateInfo vertex_stage   = {};
				VkPipelineShaderStageCreateInfo frag_stage	   = {};
				VkPipelineShaderStageCreateInfo geometry_stage = {};
                
				bool encountered_errors = false;
				do
				{
					VkShaderModule* current_shader				   = NULL;
					VkPipelineShaderStageCreateInfo* current_stage = NULL;
					VkShaderStageFlagBits current_stage_flag	   = {};
                    
					if (strcompare(file_group.first_file_info->base_name, "polygon_v"))
					{
						current_stage = &vertex_stage;
						current_shader = &vertex_shader;
						current_stage_flag = VK_SHADER_STAGE_VERTEX_BIT;
					}
					
					else if (strcompare(file_group.first_file_info->base_name, "polygon_f"))
					{
						current_stage = &frag_stage;
						current_shader = &frag_shader;
						current_stage_flag = VK_SHADER_STAGE_FRAGMENT_BIT;
					}
					
					else if (strcompare(file_group.first_file_info->base_name, "polygon_g"))
					{
						current_stage = &geometry_stage;
						current_shader = &geometry_shader;
						current_stage_flag = VK_SHADER_STAGE_GEOMETRY_BIT;
					}
                    
					if (current_shader && current_stage && current_stage_flag)
					{
						void* code_memory = PushSize(FrameTempArena, file_group.first_file_info->file_size, alignof(u32), 0);
                        
						platform_file_handle file_handle = Win32OpenFile(file_group.first_file_info, OpenFile_Read);
						Win32ReadFromFile(&file_handle, 0, file_group.first_file_info->file_size, code_memory);
                        
						VkShaderModuleCreateInfo create_info = {};
						create_info.sType	 = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
						create_info.codeSize = file_group.first_file_info->file_size;
						create_info.pCode	 = (u32*) code_memory;
						
						VK_NESTED_CHECK(Vulkan->vkCreateShaderModule(Vulkan->device, &create_info, NULL, current_shader),
										encountered_errors);
                        
						Win32CloseFile(&file_handle);
                        
						current_stage->sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
						current_stage->stage  = current_stage_flag;
						current_stage->module = *current_shader;
						current_stage->pName  = "main";
					}
                    
					file_group.first_file_info = file_group.first_file_info->next;
				}
                
				while(file_group.first_file_info
					  && (vertex_shader == VK_NULL_HANDLE || frag_shader == VK_NULL_HANDLE || geometry_shader == VK_NULL_HANDLE));
                
				Win32GetAllFilesOfTypeEnd(&file_group);
                
				if (encountered_errors || (vertex_shader == VK_NULL_HANDLE || frag_shader == VK_NULL_HANDLE || geometry_shader == VK_NULL_HANDLE))
				{
					WIN32LOG_FATAL("Could not find the immediate pipeline shader files");
					break;
				}
                
				else
				{
					VkPipelineVertexInputStateCreateInfo vertex_input = {};
					vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                    
					VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
					input_assembly.sType	= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
					input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
                    
					VkViewport viewport = {};
					viewport.width	  = (float) Vulkan->swapchain.extent.width;
					viewport.height   = (float) Vulkan->swapchain.extent.height;
					viewport.maxDepth = 1.0f;
                    
					VkRect2D scissor = {};
					scissor.extent	 = Vulkan->swapchain.extent;
                    
					VkPipelineViewportStateCreateInfo viewport_state = {};
					viewport_state.sType		 = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
					viewport_state.viewportCount = 1;
					viewport_state.pViewports	 = &viewport;
					viewport_state.scissorCount  = 1;
					viewport_state.pScissors	 = &scissor;
                    
					VkPipelineRasterizationStateCreateInfo rasterizer = {};
					rasterizer.sType	   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
					rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
					rasterizer.lineWidth   = 1.0f;
                    
					VkPipelineMultisampleStateCreateInfo multisampling = {};
					multisampling.sType				   = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
					multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
					multisampling.minSampleShading	   = 1.0f;
                    
					VkPipelineColorBlendAttachmentState color_blend_attachment = {};
					color_blend_attachment.colorWriteMask =   VK_COLOR_COMPONENT_R_BIT
                        | VK_COLOR_COMPONENT_G_BIT
                        | VK_COLOR_COMPONENT_B_BIT
                        | VK_COLOR_COMPONENT_A_BIT;
                    
					VkPipelineColorBlendStateCreateInfo color_blend = {};
					color_blend.sType			= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
					color_blend.attachmentCount = 1;
					color_blend.pAttachments	= &color_blend_attachment;
                    
					VkPushConstantRange push_constant_range = {};
					push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
					push_constant_range.offset	   = 0;
					push_constant_range.size	   = sizeof(immediate_polygon_push_info);
                    
					VkPipelineLayoutCreateInfo pipeline_layout_info = {};
					pipeline_layout_info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
					pipeline_layout_info.pushConstantRangeCount = 1;
					pipeline_layout_info.pPushConstantRanges    = &push_constant_range;
                    
					VK_CHECK(Vulkan->vkCreatePipelineLayout(Vulkan->device, &pipeline_layout_info,
                                                            NULL, &Vulkan->render_target.immediate_layout));
                    
					VkPipelineShaderStageCreateInfo shader_stages[] = {vertex_stage, frag_stage, geometry_stage};
                    
					VkGraphicsPipelineCreateInfo pipeline_info = {};
					pipeline_info.sType				  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
					pipeline_info.stageCount		  = ARRAY_COUNT(shader_stages);
					pipeline_info.pStages			  = shader_stages;
					pipeline_info.pVertexInputState	  = &vertex_input;
					pipeline_info.pInputAssemblyState = &input_assembly;
					pipeline_info.pViewportState	  = &viewport_state;
					pipeline_info.pRasterizationState = &rasterizer;
					pipeline_info.pMultisampleState	  = &multisampling;
					pipeline_info.pColorBlendState	  = &color_blend;
					pipeline_info.layout			  = Vulkan->render_target.immediate_layout;
					pipeline_info.renderPass		  = Vulkan->render_target.render_pass;
					pipeline_info.subpass			  = 0;
                    
					VK_CHECK(Vulkan->vkCreateGraphicsPipelines(Vulkan->device, VK_NULL_HANDLE, 1, &pipeline_info,
                                                               NULL, &Vulkan->render_target.immediate_pipeline));
                    
					Vulkan->vkDestroyShaderModule(Vulkan->device, vertex_shader, NULL);
					Vulkan->vkDestroyShaderModule(Vulkan->device, frag_shader, NULL);
					Vulkan->vkDestroyShaderModule(Vulkan->device, geometry_shader, NULL);
				}
			}
            
			{ //// Create render target command buffer
				VkCommandPoolCreateInfo create_info = {};
				create_info.sType			  = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				create_info.pNext			  = NULL;
				create_info.flags			  = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				create_info.queueFamilyIndex  = Vulkan->graphics_family;
                
				VK_CHECK(Vulkan->vkCreateCommandPool(Vulkan->device, &create_info, NULL, &Vulkan->render_target.render_pool));
                
				VkCommandBufferAllocateInfo allocate_info = {};
				allocate_info.sType				 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocate_info.pNext				 = NULL;
				allocate_info.commandPool		 = Vulkan->render_target.render_pool;
				allocate_info.level				 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocate_info.commandBufferCount = 1;
                
				VK_CHECK(Vulkan->vkAllocateCommandBuffers(Vulkan->device, &allocate_info, &Vulkan->render_target.render_buffer));
			}
            
			///
			/// Setup rendering
			///
            
			{ //// Record present comand buffer
				bool encountered_errors = false;
                
				for (u32 i = 0; i < Vulkan->swapchain.image_count; ++i)
				{
					VkCommandBufferBeginInfo begin_info = {};
					begin_info.sType			= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
					begin_info.pNext			= NULL;
					begin_info.flags			= 0;
					begin_info.pInheritanceInfo = NULL;
                    
					VK_NESTED_CHECK(Vulkan->vkBeginCommandBuffer(Vulkan->swapchain.present_buffers[i], &begin_info),
									encountered_errors);
                    
					VkImageMemoryBarrier transition_to_transfer_barrier = {};
					transition_to_transfer_barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					transition_to_transfer_barrier.pNext				= NULL;
					transition_to_transfer_barrier.srcAccessMask		= 0;
					transition_to_transfer_barrier.dstAccessMask		= VK_ACCESS_TRANSFER_WRITE_BIT;
					transition_to_transfer_barrier.oldLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
					transition_to_transfer_barrier.newLayout			= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					transition_to_transfer_barrier.srcQueueFamilyIndex  = Vulkan->present_family;
					transition_to_transfer_barrier.dstQueueFamilyIndex  = Vulkan->present_family;
					transition_to_transfer_barrier.image				= Vulkan->swapchain.images[i];
					transition_to_transfer_barrier.subresourceRange     = Vulkan->swapchain.subresource_range;
                    
					Vulkan->vkCmdPipelineBarrier(Vulkan->swapchain.present_buffers[i], VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                 VK_PIPELINE_STAGE_TRANSFER_BIT, NULL /*VK_VK_DEPENDENCY_BY_REGION_BIT*/,
                                                 0, NULL, 0, NULL, 1, &transition_to_transfer_barrier);
                    
					VkImageSubresourceLayers render_target_layers= {};
					render_target_layers.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
					render_target_layers.mipLevel		= 0;
					render_target_layers.baseArrayLayer = 0;
					render_target_layers.layerCount		= 1;
                    
					VkImageSubresourceLayers swapchain_layers= {};
					swapchain_layers.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
					swapchain_layers.mipLevel		= 0;
					swapchain_layers.baseArrayLayer = 0;
					swapchain_layers.layerCount		= 1;
                    
					VkImageCopy image_copy = {};
					image_copy.srcSubresource = render_target_layers;
					image_copy.srcOffset	  = {0, 0, 0};
					image_copy.dstSubresource = swapchain_layers;
					image_copy.dstOffset	  = {0, 0, 0};
                    
					// TODO(soimn): check if this is problematic when the swapchain is suboptimal
					image_copy.extent		  = {Vulkan->render_target.extent.width,
                        Vulkan->render_target.extent.height,
                        1};
                    
					Vulkan->vkCmdCopyImage(Vulkan->swapchain.present_buffers[i], Vulkan->render_target.image,
                                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Vulkan->swapchain.images[i],
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                                           &image_copy);
                    
					VkImageMemoryBarrier transition_to_present_barrier = {};
					transition_to_present_barrier.sType				  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					transition_to_present_barrier.pNext				  = NULL;
					transition_to_present_barrier.srcAccessMask		  = VK_ACCESS_TRANSFER_WRITE_BIT;
					transition_to_present_barrier.dstAccessMask		  = 0;
					transition_to_present_barrier.oldLayout			  = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					transition_to_present_barrier.newLayout			  = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
					transition_to_present_barrier.srcQueueFamilyIndex = Vulkan->present_family;
					transition_to_present_barrier.dstQueueFamilyIndex = Vulkan->present_family;
					transition_to_present_barrier.image				  = Vulkan->swapchain.images[i];
					transition_to_present_barrier.subresourceRange    = Vulkan->swapchain.subresource_range;
                    
					Vulkan->vkCmdPipelineBarrier(Vulkan->swapchain.present_buffers[i], VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                 VK_PIPELINE_STAGE_TRANSFER_BIT, NULL /*VK_VK_DEPENDENCY_BY_REGION_BIT*/,
                                                 0, NULL, 0, NULL, 1, &transition_to_present_barrier);
                    
					VK_NESTED_CHECK(Vulkan->vkEndCommandBuffer(Vulkan->swapchain.present_buffers[i]), encountered_errors);
				}
                
				if (encountered_errors)
				{
					WIN32LOG_FATAL("Failed to record swapchain present command buffers");
					break;
				}
			}
            
#undef VK_NESTED_CHECK
#undef VK_CHECK
            
			succeeded = true;
		}
		while(0);
	}
    
	else
	{
		WIN32LOG_FATAL("Failed to load vulkan-1.dll");
	}
    
	return succeeded;
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

internal win32_game_code
Win32LoadGameCode(const wchar_t* dll_path, const wchar_t* loaded_dll_path)
{
	win32_game_code game_code = {};
    
	CopyFileW(dll_path, loaded_dll_path, FALSE);
    
	game_code.module = LoadLibraryExW(loaded_dll_path, NULL, DONT_RESOLVE_DLL_REFERENCES);
    
	if (game_code.module != NULL)
	{
		game_code.game_init_func			   = (game_init_function*)				 GetProcAddress(game_code.module, "GameInit");
		game_code.game_update_and_render_func  = (game_update_and_render_function*)  GetProcAddress(game_code.module, "GameUpdateAndRender");
		game_code.game_cleanup_func			   = (game_cleanup_function*)			 GetProcAddress(game_code.module, "GameCleanup");
        
		if (game_code.game_init_func && game_code.game_update_and_render_func && game_code.game_cleanup_func)
		{
			game_code.is_valid  = true;
			game_code.timestamp = Win32DebugGetFileCreateTime(dll_path);
		}
	}
	
	else
	{
		WIN32LOG_FATAL("Could not load game code. Reason: returned module handle is invalid");
	}
    
	if (!game_code.is_valid)
	{
		game_code.game_init_func			  = &GameInitStub;
		game_code.game_update_and_render_func = &GameUpdateAndRenderStub;
		game_code.game_cleanup_func			  = &GameCleanupStub;
        
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
        
        // 		case WM_INPUT: INVALID_CODE_PATH; break;
        
		default:
        result = DefWindowProc(window_handle, msg_code, w_param, l_param);
		break;
	}
    
	return result;
}

internal void
Win32ProcessPendingMessages(HWND window_handle, platform_game_input* old_input, platform_game_input* new_input)
{
	MSG message  {};
    
	while (PeekMessageA(&message, window_handle, 0, 0, PM_REMOVE))
	{
		switch(message.message)
		{
			case WM_QUIT:
            Running = false;
			break;
            
            case WM_INPUT:
			{
				u8 buffer[50];
				u32 required_buffer_size;
                
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
								u32 keycode = (u32) input->data.keyboard.VKey;
                                
								bool is_down  = input->data.keyboard.Flags == RI_KEY_MAKE;
                                
								if (is_down)
								{
									if ((((!new_input->current_key_buffer_size && old_input->current_key_buffer_size)
                                          && old_input->key_buffer[old_input->current_key_buffer_size - 1].code == (u8) keycode)
                                         && old_input->last_key_down)
										|| (new_input->current_key_buffer_size != 0
                                            && (new_input->key_buffer[new_input->current_key_buffer_size - 1].code == (u8) keycode
												&& new_input->last_key_down)))
									{
										// NOTE(soimn): this is a repeated keystroke
									}
                                    
									else
									{
										new_input->last_key_down = true;
                                        
										if (new_input->current_key_buffer_size < PLATFORM_GAME_INPUT_KEYBUFFER_MAX_SIZE)
										{
											new_input->key_buffer[new_input->current_key_buffer_size++].code = (u8) keycode;
										}
									}
								}
                                
								// NOTE(soimn): this is temporary
								if (keycode == VK_TAB)
								{
									new_input->tab = is_down;
								}
                                
								else if (keycode == VK_RETURN)
								{
									new_input->enter = is_down;
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
								u32 keycode = (u32) input->data.mouse.usButtonFlags;
                                
								switch (keycode)
								{
									case RI_MOUSE_LEFT_BUTTON_DOWN:
                                    new_input->mouse.left = true;
									break;
                                    
									case RI_MOUSE_LEFT_BUTTON_UP:
                                    new_input->mouse.left = false;
									break;
                                    
									case RI_MOUSE_RIGHT_BUTTON_DOWN:
                                    new_input->mouse.right = true;
									break;
                                    
									case RI_MOUSE_RIGHT_BUTTON_UP:
                                    new_input->mouse.right = false;
									break;
                                    
									case RI_MOUSE_MIDDLE_BUTTON_DOWN:
                                    new_input->mouse.middle = true;
									break;
                                    
									case RI_MOUSE_MIDDLE_BUTTON_UP:
                                    new_input->mouse.middle = false;
									break;
								}
							}
							break;
                            
							default:
                            INVALID_CODE_PATH;
							break;
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
	WNDCLASSEXA window_class = {};
    
	window_class.cbSize = sizeof(WNDCLASSEX);
	window_class.style = CS_VREDRAW | CS_HREDRAW;
	window_class.lpfnWndProc = &Win32MainWindowProc;
	window_class.hInstance = instance;
	window_class.hbrBackground = 0;
	window_class.lpszClassName = APPLICATION_NAME;
    
	if (RegisterClassExA(&window_class))
	{
		window_handle =
			CreateWindowExA(window_class.style,
							window_class.lpszClassName,
							APPLICATION_NAME,
							WS_VISIBLE | WS_OVERLAPPED | WS_SYSMENU,
							CW_USEDEFAULT, CW_USEDEFAULT,
							CW_USEDEFAULT, CW_USEDEFAULT,
							NULL, NULL,
							instance, NULL);
		if (window_handle)
		{
			/// Setup
			
			// Memory
			game_memory memory = {};
            
			memory.default_allocation_routines = {&Win32AllocateMemoryBlock, &Win32FreeMemoryBlock,
                &Win32AllocateMemoryBlock, &Win32FreeMemoryBlock,
                MEGABYTES(1), KILOBYTES(1), BYTES(16),
                MEMORY_64BIT_ALIGNED, MEMORY_64BIT_ALIGNED};
            
			DefaultMemoryArenaAllocationRoutines   = memory.default_allocation_routines;
            
			memory.persistent_arena.current_block  = Win32AllocateMemoryBlock(MEGABYTES(64), 4);
			++memory.persistent_arena.block_count;
            
			memory.frame_temp_arena.current_block  = Win32AllocateMemoryBlock(GIGABYTES(1), 4);
			++memory.frame_temp_arena.block_count;
            
			memory.debug_arena.current_block	   = Win32AllocateMemoryBlock(GIGABYTES(1), 4);
			++memory.debug_arena.block_count;
            
			FrameTempArena = &memory.frame_temp_arena;
            
			// Platform API function pointers
			memory.platform_api = {};
			memory.platform_api.LogInfo				= &Win32LogInfo;
			memory.platform_api.LogError			   = &Win32LogError;
			memory.platform_api.GetAllFilesOfTypeBegin = &Win32GetAllFilesOfTypeBegin;
			memory.platform_api.GetAllFilesOfTypeEnd   = &Win32GetAllFilesOfTypeEnd;
			memory.platform_api.OpenFile			   = &Win32OpenFile;
			memory.platform_api.CloseFile			  = &Win32CloseFile;
			memory.platform_api.ReadFromFile		   = &Win32ReadFromFile;
			memory.platform_api.WriteToFile			= &Win32WriteToFile;
            
			// Game Info
			bool game_info_valid = false;
            
			win32_game_info game_info = {APPLICATION_NAME, APPLICATION_VERSION};
			game_info.cwd             = Win32GetCWD(&memory.persistent_arena, (u32*) &game_info.cwd_length);
            
			{ //// Build dll path
				const wchar_t* appendage = CONCAT(L, APPLICATION_NAME) L".dll";
				u32 buffer_length        = game_info.cwd_length + (u32) wstrlength(appendage) + 1;
                
				game_info.dll_path = PushArray(&memory.persistent_arena, wchar_t, buffer_length);
				Win32BuildFullyQualifiedPath(&game_info, appendage, (wchar_t*) game_info.dll_path, buffer_length);
			}
            
			{ //// Build loaded dll path
				const wchar_t* appendage = CONCAT(L, APPLICATION_NAME) L"_loaded.dll";
				u32 buffer_length        = game_info.cwd_length + (u32) wstrlength(appendage) + 1;
                
				game_info.loaded_dll_path = PushArray(&memory.persistent_arena, wchar_t, buffer_length);
				Win32BuildFullyQualifiedPath(&game_info, appendage, (wchar_t*) game_info.loaded_dll_path, buffer_length);
			}
            
			
			{ //// Set working directory
				BOOL successfully_set_wd = SetCurrentDirectoryW(game_info.cwd);
                
				if (successfully_set_wd == FALSE)
				{
					WIN32LOG_FATAL("Failed to set working directory to the proper value");
				}
                
				game_info_valid = (successfully_set_wd != 0);
			}
            
			//// Raw Input
			bool input_ready = Win32RegisterRawInputDevices(window_handle);
            
			//// Game code
			win32_game_code game_code = {};
            
			if (game_info_valid)
			{
				game_code = Win32LoadGameCode(game_info.dll_path, game_info.loaded_dll_path);
			}
            
			//// Vulkan
			win32_vulkan_binding vulkan_binding = {};
			bool vulkan_ready = Win32InitVulkan(FrameTempArena, &memory.persistent_arena,
												&vulkan_binding, &memory.vulkan_state,
												instance, window_handle,
												APPLICATION_NAME, APPLICATION_VERSION);
            
			ClearMemoryArena(FrameTempArena);
            
			if (input_ready && game_info_valid && game_code.is_valid && vulkan_ready)
			{
				Running = true;
                
				ShowWindow(window_handle, SW_SHOW);
                
				if (game_code.game_init_func(&memory))
				{
					// TODO(soimn): adjust this to support dynamically changing the balance of game time and real time
					f32 game_update_dt = 1.0f / 60.0f;
                    
					//// Main Loop
					while (Running)
					{
						// TODO(soimn): check if the window was resized, and recreate swapchain if resized or the recreate flag is set
						
						memory.new_input.frame_dt = game_update_dt;
                        
						Win32ProcessPendingMessages(window_handle, &memory.old_input, &memory.new_input);
                        
						{
							POINT cursor_position = {};
							RECT window_rect	  = {};
							GetCursorPos(&cursor_position);
							ScreenToClient(window_handle, &cursor_position);
							GetClientRect(window_handle, &window_rect);
							memory.new_input.mouse.position = {
								((f32) (cursor_position.x) / (f32) (window_rect.right)) * 16.0f,
								((f32) (cursor_position.y) / (f32) (window_rect.bottom)) * 9.0f
							};
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
                        
						game_code.game_update_and_render_func(&memory, game_update_dt);
                        
						{//// Present render target
							u32 image_index;
							VkResult result = Vulkan->vkAcquireNextImageKHR(Vulkan->device, Vulkan->swapchain.handle,
                                                                            UINT64_MAX, Vulkan->swapchain.image_available_semaphore,
                                                                            VK_NULL_HANDLE, &image_index);
                            
							if (result != VK_SUCCESS)
							{
								switch(result)
								{
									case VK_SUBOPTIMAL_KHR:
                                    // TEMPORARY(soimn)
                                    INVALID_CODE_PATH;
									break;
                                    
									case VK_ERROR_OUT_OF_DATE_KHR:
                                    // TEMPORARY(soimn)
                                    INVALID_CODE_PATH;
									break;
                                    
                                    
									// TODO(soimn): find a way to handle these without crashing
                                    
									case VK_ERROR_OUT_OF_HOST_MEMORY:
                                    WIN32LOG_FATAL("Failed to acquire next swapchain image. Reason: the host is out of memory");
									break;
                                    
									case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                                    WIN32LOG_FATAL("Failed to acquire next swapchain image. Reason: the device is out of memory");
									break;
                                    
									case VK_ERROR_DEVICE_LOST:
                                    WIN32LOG_FATAL("Failed to acquire next swapchain image. Reason: the device was lost");
									break;
                                    
									case VK_ERROR_SURFACE_LOST_KHR:
                                    WIN32LOG_FATAL("Failed to acquire next swapchain image. Reason: the surface was lost");
									break;
                                    
									INVALID_DEFAULT_CASE;
								}
							}
                            
							else
							{
								VkSubmitInfo submit_info = {};
								submit_info.sType				 = VK_STRUCTURE_TYPE_SUBMIT_INFO;
								submit_info.pNext				 = NULL;
                                
								VkSemaphore wait_semaphores[] = {Vulkan->swapchain.image_available_semaphore,
                                    Vulkan->render_target.render_done_semaphore};
                                
								submit_info.waitSemaphoreCount	 = ARRAY_COUNT(wait_semaphores);
								submit_info.pWaitSemaphores		 = wait_semaphores;
                                
								VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_TRANSFER_BIT,
                                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
								submit_info.pWaitDstStageMask	 = wait_stages;
                                
								submit_info.commandBufferCount	 = 1;
								submit_info.pCommandBuffers		 = &Vulkan->swapchain.present_buffers[image_index];
								submit_info.signalSemaphoreCount = 1;
								submit_info.pSignalSemaphores	 = &Vulkan->swapchain.transfer_done_semaphore;
                                
								Vulkan->vkQueueSubmit(Vulkan->present_queue, 1, &submit_info, VK_NULL_HANDLE);
                                
								VkPresentInfoKHR present_info = {};
								present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
								present_info.waitSemaphoreCount = 1;
								present_info.pWaitSemaphores = &Vulkan->swapchain.transfer_done_semaphore;
								present_info.swapchainCount = 1;
								present_info.pSwapchains = &Vulkan->swapchain.handle;
								present_info.pImageIndices = &image_index;
								present_info.pResults = NULL;
                                
								Vulkan->vkQueuePresentKHR(Vulkan->present_queue, &present_info);
								Vulkan->vkDeviceWaitIdle(Vulkan->device);
							}
						}
                        
                        // 						if (memory.new_input.current_key_buffer_size != 0)
                        // 						{
                        // 							char buffer[50];
                        // 
                        // 							for (u32 i = 0; i < memory.new_input.current_key_buffer_size; ++i)
                        // 							{
                        // 								wsprintf(buffer, "%c", memory.new_input.key_buffer[i].code);
                        // 								WIN32LOG_DEBUG(buffer);
                        // 							}
                        // 						}
                        
						Copy((void*) &memory.new_input, sizeof(platform_game_input), (void*) &memory.old_input);
                        
						ResetMemoryArena(FrameTempArena);
					}
					//// Main Loop End
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
 *	- Refactor input handling to allow hot plugging of devices
 *	- Setup audio
 *	- Set viewport dimensions and implement restricted resizing of the window
 *	- Implement proper text input handling
 *	- Implement memory usage shadowing
 *    - Verify the memory allocator is working as excpected
 */

// FIXME(soimn):
/*
 *	- vkGetPhysicalDeviceFeatures fails with the error message "invalid physicalDevice object handle"
 *	  find out if this is a bug in vulkan or in the Win32InitVulkan code. The application seems to work
 *	  when the device features are not queried.
 *	- The application crashes without an error log when the data directory is not found.
 */
