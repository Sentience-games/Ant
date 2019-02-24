#include "win32_ant.h"

/*
 *	GLOBAL *cough* VARIABLES
 */

// TODO(soimn): delegate responsibility, to the game, to confirm the quit message, in case saving is due or the game needs to halt the player
global_variable bool Running						 = false;
global_variable float TargetFrameSecounds			 = (float) (1.0f / ((float)DEFAULT_TARGET_FPS * 1000.0f));
global_variable LARGE_INTEGER PerformanceCounterFreq = {};

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
Win32InitVulkan(memory_arena* temp_memory, win32_vulkan_binding* binding,
				vulkan_renderer_state* vulkan_state, HINSTANCE process_instance,
				HWND window_handle, const char* application_name,
				u32 application_version)
{
	bool succeeded = false;

	binding->module = LoadLibraryA("vulkan-1.dll");

	if (binding->module != INVALID_HANDLE_VALUE)
	{
		do
		{
			{ //// Load exported and global level vulkan functions
				#define VK_EXPORTED_FUNCTION(func)\
					binding->api.##func = (PFN_##func) GetProcAddress(binding->module, #func);\
					if (!binding->api.##func)\
					{\
						WIN32LOG_FATAL("Failed to load the exported vulkan function '" #func "'.");\
						break;\
					}
			
				#define VK_GLOBAL_LEVEL_FUNCTION(func)\
					binding->api.##func = (PFN_##func) binding->api.vkGetInstanceProcAddr(NULL, #func);\
					if (!binding->api.##func)\
					{\
						WIN32LOG_FATAL("Failed to load the global level vulkan function '" #func "'.");\
						break;\
					}
	
				#include "vulkan_functions.inl"
			
				#undef VK_GLOBAL_LEVEL_FUNCTION
				#undef VK_EXPORTED_FUNCTION
			}

			vulkan_api_functions& VulkanAPI = binding->api;

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
					WIN32LOG_FATAL("The call to the vulkan function '" #call "' failed.");\
					encountered_error = true;\
					break;\
				}

			const char* required_instance_extensions[] = {
				#define INSTANCE_EXTENSIONS

				#ifdef ANT_DEBUG
				#define DEBUG_EXTENSIONS
				#include "vulkan_extensions.inl"
				#undef DEBUG_EXTENSIONS

				#else
				#include "vulkan_extensions.inl"
				#endif

				#undef INSTANCE_EXTENSIONS
			};

			const char* required_layers[] = {
				#ifdef ANT_DEBUG
				#define LAYERS
				#include "vulkan_extensions.inl"
				#undef LAYERS
				#endif
			};

			const char* required_device_extensions[] = 
			{
				#define DEVICE_EXTENSIONS
				#include "vulkan_extensions.inl"
				#undef DEVICE_EXTENSIONS
			};

			///
			/// Setup instance
			///

			{ //// Validate extension support	
				u32 found_extension_count = 0;

				u32 extension_count;
				VK_CHECK(VulkanAPI.vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL));

				VkExtensionProperties* extension_properties = PushArray(temp_memory, VkExtensionProperties, extension_count);
				VK_CHECK(VulkanAPI.vkEnumerateInstanceExtensionProperties(NULL, &extension_count, extension_properties));

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
					WIN32LOG_ERROR("Instance extension property validation failed. One or more of the required vulkan instance extensions are not"
								   " present.");
					break;
				}
			}

			#ifdef ANT_DEBUG
			{ //// Validate layer support
				u32 found_layer_count = 0;

				u32 layer_count;
				VK_CHECK(VulkanAPI.vkEnumerateInstanceLayerProperties(&layer_count, NULL));

				VkLayerProperties* layer_properties = PushArray(temp_memory, VkLayerProperties, layer_count);
				VK_CHECK(VulkanAPI.vkEnumerateInstanceLayerProperties(&layer_count, layer_properties));

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
					WIN32LOG_ERROR("Instance layer property validation failed. One or more of the required vulkan layers are not present.");
					break;
				}
			}
			#endif
			
			{ //// Create instance
				VkApplicationInfo app_info = {};
				app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
				app_info.pApplicationName = application_name;
				app_info.applicationVersion = application_version;
				app_info.pEngineName = "Ant Engine";
				app_info.engineVersion = ANT_VERSION;
				app_info.apiVersion = VK_API_VERSION_1_0;

				VkInstanceCreateInfo create_info = {};
				create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
				create_info.pNext = NULL;
				create_info.flags = 0;
				create_info.pApplicationInfo = &app_info;
				create_info.enabledLayerCount = ARRAY_COUNT(required_layers);
				create_info.ppEnabledLayerNames = required_layers;
				create_info.enabledExtensionCount = ARRAY_COUNT(required_instance_extensions);
				create_info.ppEnabledExtensionNames = required_instance_extensions;

				VK_CHECK(VulkanAPI.vkCreateInstance(&create_info, NULL, &vulkan_state->instance));
			}

			// Load instance level functions
			#define VK_INSTANCE_LEVEL_FUNCTION(func)\
				VulkanAPI.##func = (PFN_##func) VulkanAPI.vkGetInstanceProcAddr(vulkan_state->instance, #func);\
				if (!VulkanAPI.##func)\
				{\
					WIN32LOG_ERROR("Failed to load instance level function '" #func "'.");\
					break;\
				}
			
			#include "vulkan_functions.inl"
			
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
				create_info.pUserData = NULL;

				PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT =
					(PFN_vkCreateDebugUtilsMessengerEXT) VulkanAPI.vkGetInstanceProcAddr(vulkan_state->instance, "vkCreateDebugUtilsMessengerEXT");
				
				if (!vkCreateDebugUtilsMessengerEXT)
				{
					WIN32LOG_ERROR("Failed to load the instance level function 'vkCreateDebugUtilsMessengerEXT'.");
					break;
				}

				VK_CHECK(vkCreateDebugUtilsMessengerEXT(vulkan_state->instance, &create_info, NULL, &error_callback));
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
					(PFN_vkCreateWin32SurfaceKHR) VulkanAPI.vkGetInstanceProcAddr(vulkan_state->instance, "vkCreateWin32SurfaceKHR");

				if (!vkCreateWin32SurfaceKHR)
				{
					WIN32LOG_ERROR("Failed to load the instance level function 'vkCreateWin32SurfaceKHR'.");
					break;
				}

				VK_CHECK(vkCreateWin32SurfaceKHR(vulkan_state->instance, &create_info, NULL, &vulkan_state->surface));
			}	

			///
			/// Setup physical device
			///

			{ //// Acquire physical device
				u32 device_count;
				VK_CHECK(VulkanAPI.vkEnumeratePhysicalDevices(vulkan_state->instance, &device_count, NULL));

				VkPhysicalDevice* physical_devices = PushArray(temp_memory, VkPhysicalDevice, device_count);
				VK_CHECK(VulkanAPI.vkEnumeratePhysicalDevices(vulkan_state->instance, &device_count, physical_devices));

				bool encountered_errors = false;

				u32 best_score = 0;
				VkPhysicalDevice chosen_device = VK_NULL_HANDLE;
				for (u32 i = 0; i < device_count; ++i)
				{
					u32 device_score = 0;
					bool device_invalid = false;

					VkPhysicalDeviceProperties properties;
					VkPhysicalDeviceFeatures features;

					VulkanAPI.vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
					VulkanAPI.vkGetPhysicalDeviceFeatures(physical_devices[i], &features);

					bool supports_graphics	   = false;
					bool supports_compute	   = false;
					bool supports_transfer	   = false;
					bool supports_presentation = false;

					{ //// Queues
						u32 queue_family_count;
						VulkanAPI.vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, NULL);

						VkQueueFamilyProperties* queue_family_properties = PushArray(temp_memory, VkQueueFamilyProperties, queue_family_count);
						VulkanAPI.vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, queue_family_properties);

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
							VK_NESTED_CHECK(VulkanAPI.vkGetPhysicalDeviceSurfaceSupportKHR(physical_devices[i], j, vulkan_state->surface,
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
						VK_CHECK(VulkanAPI.vkEnumerateDeviceExtensionProperties(physical_devices[i], NULL, &extension_count, NULL));

						VkExtensionProperties* extension_properties = PushArray(temp_memory, VkExtensionProperties, extension_count);
						VK_CHECK(VulkanAPI.vkEnumerateDeviceExtensionProperties(physical_devices[i], NULL, &extension_count, extension_properties));

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

						VulkanAPI.vkGetPhysicalDeviceSurfaceFormatsKHR(physical_devices[i], vulkan_state->surface, &format_count, NULL);
						VulkanAPI.vkGetPhysicalDeviceSurfacePresentModesKHR(physical_devices[i], vulkan_state->surface, &present_mode_count, NULL);
						
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
					WIN32LOG_ERROR("Failed to find a suitable device supporting vulkan");
					break;
				}

				else
				{
					vulkan_state->physical_device = chosen_device;
				}
			}

			{ //// Acquire queue families
				vulkan_state->graphics_family			= -1;
				vulkan_state->compute_family			= -1;
				vulkan_state->dedicated_transfer_family = -1;
				vulkan_state->present_family			= -1;

				u32 queue_family_count;
				VulkanAPI.vkGetPhysicalDeviceQueueFamilyProperties(vulkan_state->physical_device, &queue_family_count, NULL);

				VkQueueFamilyProperties* queue_family_properties = PushArray(temp_memory, VkQueueFamilyProperties, queue_family_count);
				VulkanAPI.vkGetPhysicalDeviceQueueFamilyProperties(vulkan_state->physical_device, &queue_family_count, queue_family_properties);

				for (u32 i = 0; i < queue_family_count; ++i)
				{
					if (queue_family_properties[i].queueCount != 0)
					{
						if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT
							&& vulkan_state->graphics_family == -1)
						{
							vulkan_state->graphics_family = (i32) i;
						}

						if (queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
						{
							vulkan_state->compute_family = (i32) i;
						}

						if (queue_family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
						{
							vulkan_state->dedicated_transfer_family = (i32) i;
						}

						if (vulkan_state->present_family == -1)
						{
							VkBool32 queue_family_supports_presentation;
							VulkanAPI.vkGetPhysicalDeviceSurfaceSupportKHR(vulkan_state->physical_device, i,
																		   vulkan_state->surface, &queue_family_supports_presentation);

							if (queue_family_supports_presentation)
							{
								vulkan_state->present_family = (i32) i;
							}
						}
					}
				}

				if (vulkan_state->graphics_family == -1 || vulkan_state->present_family == -1)
				{
					WIN32LOG_ERROR("Failed to acquire graphics and/or present queue families.");
					break;
				}

				if (vulkan_state->compute_family == vulkan_state->graphics_family
					|| (vulkan_state->compute_family == vulkan_state->present_family
						&& queue_family_properties[vulkan_state->compute_family].queueCount < 2))
				{
					vulkan_state->supports_compute = false;
					vulkan_state->compute_family = -1;
				}

				else if ((vulkan_state->compute_family == vulkan_state->present_family
							&& vulkan_state->compute_family == vulkan_state->dedicated_transfer_family)
						 && queue_family_properties[vulkan_state->compute_family].queueCount < 3)
				{
					vulkan_state->supports_compute = false;
					vulkan_state->compute_family = -1;

					if (vulkan_state->dedicated_transfer_family == vulkan_state->graphics_family
						|| (vulkan_state->dedicated_transfer_family == vulkan_state->present_family
							&& queue_family_properties[vulkan_state->present_family].queueCount < 2))
					{
						vulkan_state->supports_dedicated_transfer = false;
						vulkan_state->dedicated_transfer_family = -1;
					}

					else if (vulkan_state->dedicated_transfer_family != -1)
					{
					
						vulkan_state->supports_dedicated_transfer = true;
					}
				}

				else if (vulkan_state->compute_family != -1)
				{
					vulkan_state->supports_compute  = true;
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
				queue_create_info[queue_create_info_count].queueFamilyIndex = vulkan_state->graphics_family;
				queue_create_info[queue_create_info_count].queueCount		= 1;
				queue_create_info[queue_create_info_count].pQueuePriorities	= &queue_priorities[0];
				++queue_create_info_count;

				if (vulkan_state->present_family == vulkan_state->graphics_family)
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
					queue_create_info[present_family_index].queueFamilyIndex = vulkan_state->present_family;
					queue_create_info[present_family_index].queueCount		 = 1;
					queue_create_info[present_family_index].pQueuePriorities = &queue_priorities[1];
					++queue_create_info_count;
				}

				if (vulkan_state->supports_compute)
				{
					if (vulkan_state->compute_family == vulkan_state->present_family)
					{
						++queue_create_info[present_family_index].queueCount;
					}

					else
					{
						compute_family_index = queue_create_info_count;
						queue_create_info[compute_family_index].sType			 = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
						queue_create_info[compute_family_index].pNext			 = NULL;
						queue_create_info[compute_family_index].flags			 = 0;
						queue_create_info[compute_family_index].queueFamilyIndex = vulkan_state->compute_family;
						queue_create_info[compute_family_index].queueCount		 = 1;
						queue_create_info[compute_family_index].pQueuePriorities = &queue_priorities[2];
						++queue_create_info_count;
					}
				}

				if (vulkan_state->supports_dedicated_transfer)
				{
					if (vulkan_state->dedicated_transfer_family == vulkan_state->present_family)
					{
						++queue_create_info[present_family_index].queueCount;
					}

					else if (vulkan_state->dedicated_transfer_family == vulkan_state->compute_family)
					{
						++queue_create_info[compute_family_index].queueCount;
					}

					else
					{
						queue_create_info[queue_create_info_count].sType			= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
						queue_create_info[queue_create_info_count].pNext			= NULL;
						queue_create_info[queue_create_info_count].flags			= 0;
						queue_create_info[queue_create_info_count].queueFamilyIndex = vulkan_state->dedicated_transfer_family;
						queue_create_info[queue_create_info_count].queueCount		= 1;
						queue_create_info[queue_create_info_count].pQueuePriorities	= &queue_priorities[3];
						++queue_create_info_count;
					}
				}

				VkDeviceCreateInfo device_create_info = {};
				device_create_info.sType				   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
				device_create_info.pNext				   = NULL;
				device_create_info.flags				   = 0;
				device_create_info.queueCreateInfoCount	   = queue_create_info_count;
				device_create_info.pQueueCreateInfos	   = &queue_create_info[0];
				device_create_info.enabledExtensionCount   = ARRAY_COUNT(required_device_extensions);
				device_create_info.ppEnabledExtensionNames = required_device_extensions;
				device_create_info.pEnabledFeatures		   = NULL;

				VK_CHECK(VulkanAPI.vkCreateDevice(vulkan_state->physical_device, &device_create_info, NULL, &vulkan_state->device));
			}

			// Load device level functions
			#define VK_DEVICE_LEVEL_FUNCTION(func)\
				VulkanAPI.##func = (PFN_##func) VulkanAPI.vkGetDeviceProcAddr(vulkan_state->device, #func);\
				if (!VulkanAPI.##func)\
				{\
					WIN32LOG_ERROR("Failed to load device level function '" #func "'.");\
					break;\
				}
			
			#include "vulkan_functions.inl"
			
			#undef VK_DEVICE_LEVEL_FUNCTION

			{ //// Setup queues
				VulkanAPI.vkGetDeviceQueue(vulkan_state->device, vulkan_state->graphics_family, 0, &vulkan_state->graphics_queue);

				u32 present_family_queue_index = 0;
				VulkanAPI.vkGetDeviceQueue(vulkan_state->device, vulkan_state->present_family, present_family_queue_index, &vulkan_state->present_queue);
				
				if (vulkan_state->supports_compute)
				{
					if (vulkan_state->compute_family == vulkan_state->present_family)
					{
						VulkanAPI.vkGetDeviceQueue(vulkan_state->device, vulkan_state->present_family,
												   ++present_family_queue_index, &vulkan_state->compute_queue);
					}

					else
					{
						VulkanAPI.vkGetDeviceQueue(vulkan_state->device, vulkan_state->compute_family, 0, &vulkan_state->compute_queue);
					}
				}

				VulkanAPI.vkGetDeviceQueue(vulkan_state->device, vulkan_state->graphics_family, 0, &vulkan_state->transfer_queue);

				if (vulkan_state->supports_dedicated_transfer)
				{
					if (vulkan_state->dedicated_transfer_family == vulkan_state->present_family)
					{
						VulkanAPI.vkGetDeviceQueue(vulkan_state->device, vulkan_state->present_family,
												   ++present_family_queue_index, &vulkan_state->dedicated_transfer_queue);
					}

					else if (vulkan_state->dedicated_transfer_family == vulkan_state->compute_family)
					{
						VulkanAPI.vkGetDeviceQueue(vulkan_state->device, vulkan_state->compute_family, 1, &vulkan_state->dedicated_transfer_queue);
					}

					else
					{
						VulkanAPI.vkGetDeviceQueue(vulkan_state->device, vulkan_state->dedicated_transfer_family,
												   0, &vulkan_state->dedicated_transfer_queue);
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
					VK_CHECK(VulkanAPI.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan_state->physical_device, vulkan_state->surface, &capabilities));

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
						WIN32LOG_ERROR("Failed to create swapchain, as the surface does not support image transfers to the swapchain images.");
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

					VK_CHECK(VulkanAPI.vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan_state->physical_device, vulkan_state->surface,
																			&format_count, NULL));

					if (format_count == 0)
					{
						WIN32LOG_ERROR("Failed to retrieve surface formats for swapchain creation.");
						break;
					}

					formats = PushArray(temp_memory, VkSurfaceFormatKHR, format_count);
					VK_CHECK(VulkanAPI.vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan_state->physical_device, vulkan_state->surface,
																			&format_count, formats));

					bool found_format = false;

					if (format_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
					{
						swapchain_format = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
						found_format = true;
					}

					else
					{
						for (u32 i = 0; i < format_count; ++i)
						{
							if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
							{
								swapchain_format = formats[i];
								found_format = true;
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

					VK_CHECK(VulkanAPI.vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan_state->physical_device, vulkan_state->surface,
																						  &present_mode_count, NULL));

					present_modes = PushArray(temp_memory, VkPresentModeKHR, present_mode_count);
					VK_CHECK(VulkanAPI.vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan_state->physical_device, vulkan_state->surface,
																						  &present_mode_count, present_modes));

					// IMPORTANT(soimn): this loop is biasing FIFO_RELAXED, which may not always be optimal
					u32 best_score = 0;
					for (u32 i = 0; i < present_mode_count; ++i)
					{
						if (present_modes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
						{
							swapchain_present_mode = present_modes[i];
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
						WIN32LOG_ERROR("Failed to find a suitable present mode for swapchain creation");
					}
				}

				vulkan_state->swapchain.extent		   = swapchain_extent;
				vulkan_state->swapchain.surface_format = swapchain_format;
				vulkan_state->swapchain.present_mode   = swapchain_present_mode;

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
				create_info.surface			 = vulkan_state->surface;
				create_info.imageArrayLayers = 1;
				create_info.preTransform	 = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
				create_info.compositeAlpha	 = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

				VK_CHECK(VulkanAPI.vkCreateSwapchainKHR(vulkan_state->device, &create_info, NULL, &vulkan_state->swapchain.handle));
			}

			{ //// Create swapchain images
				VK_CHECK(VulkanAPI.vkGetSwapchainImagesKHR(vulkan_state->device, vulkan_state->swapchain.handle,
														   &vulkan_state->swapchain.image_count, NULL));

				vulkan_state->swapchain.images		= PushArray(temp_memory, VkImage, vulkan_state->swapchain.image_count);

				VK_CHECK(VulkanAPI.vkGetSwapchainImagesKHR(vulkan_state->device, vulkan_state->swapchain.handle,
														   &vulkan_state->swapchain.image_count, vulkan_state->swapchain.images));
			}

			{ //// Create swapchain semaphores
				VkSemaphoreCreateInfo create_info = {};
				create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				create_info.pNext = NULL;
				create_info.flags = 0;

				VK_CHECK(VulkanAPI.vkCreateSemaphore(vulkan_state->device, &create_info, NULL, &vulkan_state->swapchain.image_available_semaphore));
			}

			///
			/// Setup render target
			///
			
			{ //// Create render target image
				VkImageCreateInfo create_info = {};
				create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				create_info.pNext = NULL;
				create_info.flags = 0;
				create_info.imageType = VK_IMAGE_TYPE_2D;
				create_info.format = vulkan_state->swapchain.surface_format.format;
				create_info.extent = {vulkan_state->swapchain.extent.width,
									  vulkan_state->swapchain.extent.height,
									  1};
				create_info.mipLevels = 1;
				create_info.arrayLayers = 1;
				create_info.samples = VK_SAMPLE_COUNT_1_BIT;
				create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
				create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
				create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				create_info.queueFamilyIndexCount = 0;
				create_info.pQueueFamilyIndices = NULL;
				create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

				VK_CHECK(VulkanAPI.vkCreateImage(vulkan_state->device, &create_info, NULL, &vulkan_state->render_target.image));

				VkMemoryRequirements memory_requirements;
				VulkanAPI.vkGetImageMemoryRequirements(vulkan_state->device, vulkan_state->render_target.image, &memory_requirements);

				VkPhysicalDeviceMemoryProperties memory_properties;
				VulkanAPI.vkGetPhysicalDeviceMemoryProperties(vulkan_state->physical_device, &memory_properties);
				VkMemoryPropertyFlags required_memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

				i32 selected_memory_type = -1;
				for (u32 i = 0; i < memory_properties.memoryTypeCount; ++i)
				{
					if ((memory_requirements.memoryTypeBits & (1 << i)) &&
						(memory_properties.memoryTypes[i].propertyFlags & required_memory_properties) == required_memory_properties)
					{
						selected_memory_type = (i32) i;
					}
				}

				if (selected_memory_type == -1)
				{
					WIN32LOG_ERROR("Failed to find an appropriate memory type for the main render target image");
					break;
				}

				VkMemoryAllocateInfo allocate_info = {};
				allocate_info.sType			  = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocate_info.pNext			  = NULL;
				allocate_info.allocationSize  = memory_requirements.size;
				allocate_info.memoryTypeIndex = (u32) selected_memory_type;

				VK_CHECK(VulkanAPI.vkAllocateMemory(vulkan_state->device, &allocate_info, NULL, &vulkan_state->render_target.image_memory));
				VK_CHECK(VulkanAPI.vkBindImageMemory(vulkan_state->device, vulkan_state->render_target.image, vulkan_state->render_target.image_memory, 0));
			}

			{ //// Create render target image view
				VkImageViewCreateInfo create_info = {};
				create_info.sType							= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				create_info.pNext							= NULL;
				create_info.flags							= 0;
				create_info.image							= vulkan_state->render_target.image;
				create_info.viewType						= VK_IMAGE_VIEW_TYPE_2D;
				create_info.format							= vulkan_state->swapchain.surface_format.format;

				create_info.components						= {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
															   VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};

				create_info.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
				create_info.subresourceRange.baseMipLevel	= 0;
				create_info.subresourceRange.levelCount		= 1;
				create_info.subresourceRange.baseArrayLayer = 0;
				create_info.subresourceRange.layerCount		= 1;

				VK_CHECK(VulkanAPI.vkCreateImageView(vulkan_state->device, &create_info, NULL, &vulkan_state->render_target.image_view));
			}

			{ //// Create render target render pass
				VkAttachmentDescription attachment_description = {};
				attachment_description.flags		  = 0;
				attachment_description.format		  = vulkan_state->swapchain.surface_format.format;
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

				VK_CHECK(VulkanAPI.vkCreateRenderPass(vulkan_state->device, &create_info, NULL, &vulkan_state->render_target.render_pass));
			}

			{ //// Create render target framebuffer
				VkFramebufferCreateInfo create_info = {};
				create_info.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				create_info.pNext			= NULL;
				create_info.flags			= 0;
				create_info.renderPass		= vulkan_state->render_target.render_pass;
				create_info.attachmentCount = 1;
				create_info.pAttachments	= &vulkan_state->render_target.image_view;
				create_info.width			= vulkan_state->swapchain.extent.width;
				create_info.height			= vulkan_state->swapchain.extent.height;
				create_info.layers			= 1;

				VK_CHECK(VulkanAPI.vkCreateFramebuffer(vulkan_state->device, &create_info, NULL, &vulkan_state->render_target.framebuffer));
			}

			{ //// Create render target semaphores
				VkSemaphoreCreateInfo create_info = {};
				create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				create_info.pNext = NULL;
				create_info.flags = 0;

				VK_CHECK(VulkanAPI.vkCreateSemaphore(vulkan_state->device, &create_info, NULL, &vulkan_state->render_target.render_done_semaphore));
			}

			#undef VK_CHECK

			succeeded = true;
		}
		while(0);
	}

	else
	{
		WIN32LOG_ERROR("Failed to load vulkan-1.dll");
	}

	return succeeded;
}

///
/// File System Interaction
///

// TODO(soimn): make this more robust to enable hot loading capability in release mode
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

			// Vulkan
			win32_vulkan_binding vulkan_binding = {};
			bool vulkan_ready = Win32InitVulkan(&FrameTempArena, &vulkan_binding,
												&memory.vulkan_state, instance,
												window_handle, APPLICATION_NAME,
												APPLICATION_VERSION);

			// Misc
			QueryPerformanceFrequency(&PerformanceCounterFreq);

			ClearMemoryArena(&FrameTempArena);

			if (game_code.is_valid && input_ready && vulkan_ready)
			{
				Running = true;

				ShowWindow(window_handle, SW_SHOW);

				if (game_code.game_init_func(&memory))
				{

					/// Main Loop
					while (Running)
					{
						LARGE_INTEGER frame_start_time = Win32GetWallClock();

						// TODO(soimn): check if the window was resized, and recreate swapchain if resized or the recreate flag is set

						MSG message;
						while (PeekMessage(&message, window_handle, 0, 0, PM_REMOVE))
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

						game_code.game_update_and_render_func(&memory, TargetFrameSecounds);

						LARGE_INTEGER frame_end_time = Win32GetWallClock();

						float frame_time_elapsed = Win32GetSecoundsElapsed(frame_start_time, frame_end_time);

						while (frame_time_elapsed < TargetFrameSecounds)
						{
							frame_time_elapsed += Win32GetSecoundsElapsed(frame_start_time, Win32GetWallClock());
						}

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
 *	- Implement support for DirectInput
 *	- Setup XAudio2
 *	- Implement a solution for freeing allocations instead of entire arenas
 *	- Change the explicit sleep at the end of the global loop to a wait on v-blank, and
 *	  find a way to handle 144 Hz monitor refresh rates and update synchronization, and consider
 *	  supporting several frames in flight.
 */
