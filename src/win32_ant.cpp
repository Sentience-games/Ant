#include "win32_ant.h"

/*
 *	GLOBAL *cough* VARIABLES
 */

// TODO(soimn): delegate responsibility, to the game, to confirm the quit message, in case saving is due or the game needs to halt the player
global_variable bool running = false;

// TODO(soimn): consider moving / changing this
global_variable const char* application_name = "APP";
global_variable uint32 application_version = 0;


// TODO(soimn): refactor logging system
[[maybe_unused]]
internal void
Win32Log(const char* message)
{
	// TODO(soimn): consider checking the return value of GetStdHandle
	local_persist HANDLE win32_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
	local_persist char buffer[512 + 1];

	int length = strlength(message, 512);
	Assert(length <= 512);

	if (length)
	{
		memcpy(&buffer, message, length);
		buffer[length] = '\n';

		// TODO(soimn): consider checking chars written to see if the function printed the entire message
		//				and checking the return value to validate the function succeeded
		DWORD charsWritten;
		WriteConsole(win32_stdout, buffer, length + 1, &charsWritten, NULL);
	}
}


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


internal bool
VulkanValidateLayerSupport(HANDLE vulkanInitHeap, const char** req_layers, uint32 req_layer_count)
{
	uint32 extension_count;
	VkResult result;
	bool foundAllLayers = false;

	result = vkEnumerateInstanceLayerProperties(&extension_count, NULL);
	Assert(!result);

	VkLayerProperties* layer_properties = (VkLayerProperties*)HeapAlloc(vulkanInitHeap,
																		HEAP_ZERO_MEMORY,
																		extension_count * sizeof(VkLayerProperties));
	Assert(layer_properties != NULL);


	result = vkEnumerateInstanceLayerProperties(&extension_count, layer_properties);

	if (result != VK_SUCCESS)
	{
		switch (result)
		{
			case VK_SUCCESS:
			break;

			case VK_ERROR_OUT_OF_HOST_MEMORY:
				WIN32LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
			break;

			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				WIN32LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
			break;

			default:
				Assert(false);
			break;
		}
	}

	else
	{
		foundAllLayers = true; // NOTE(soimn): this assignment is necessary for the validation, since it is optimistic

		for (char** current_layer = (char**)req_layers;
			current_layer < (char**)req_layers + req_layer_count;
			++current_layer)
		{
			bool foundLayer = false;

			for (VkLayerProperties* current_property = layer_properties;
				current_property < layer_properties + extension_count;
				++current_property)
			{
				if (strcompare(*current_layer, current_property->layerName))
				{
					foundLayer = true;
					break;
				}
			}

			if (foundLayer)
			{
				continue;
			}

			else
			{
				{
					const char* error_prefix = "VulkanValidateLayerSupport failed to find the ";
					const char* error_suffix = " layer.";

					uint32 error_prefix_length = strlength(error_prefix);
					uint32 error_suffix_length = strlength(error_suffix);
					uint32 layer_name_length = strlength(*current_layer);
					uint32 error_message_length = error_prefix_length + error_suffix_length + layer_name_length;

					char* error_message = (char*)HeapAlloc(vulkanInitHeap, HEAP_ZERO_MEMORY, (error_message_length + 1) * sizeof(char));

					memcpy(error_message, error_prefix, error_prefix_length);
					memcpy(error_message + error_prefix_length, *current_layer, layer_name_length);
					memcpy(error_message + error_prefix_length + layer_name_length, error_suffix, error_suffix_length);
					error_message[error_message_length] = '\0';

					WIN32LOG_ERROR(error_message);

					HeapFree(vulkanInitHeap, 0, error_message);
				}

				foundAllLayers = false;
				break;
			}
		}
	}
		
	HeapFree(vulkanInitHeap, 0, layer_properties);

	return foundAllLayers;
}

internal bool
VulkanValidateInstanceExtensionSupport(HANDLE vulkanInitHeap, const char** req_extensions, uint32 req_extension_count)
{
	uint32 extension_count;
	VkResult result;
	bool foundAllExtensions = false;

	result = vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
	Assert(!result);

	VkExtensionProperties*
	extension_properties = (VkExtensionProperties*) HeapAlloc(vulkanInitHeap,
															  HEAP_ZERO_MEMORY,
															  extension_count * sizeof(VkExtensionProperties));
	Assert(extension_properties != NULL);

	result = vkEnumerateInstanceExtensionProperties(NULL, &extension_count, extension_properties);

	if (result != VK_SUCCESS)
	{
		switch(result)
		{
			case VK_INCOMPLETE:
				WIN32LOG_ERROR("VK_INCOMPLETE");
			break;

			case VK_ERROR_OUT_OF_HOST_MEMORY:
				WIN32LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
			break;

			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				WIN32LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
			break;

			case VK_ERROR_LAYER_NOT_PRESENT:
				WIN32LOG_ERROR("VK_ERROR_LAYER_NOT_PRESENT");
			break;

			default:
				Assert(false);
			break;
		}
	}

	else
	{
		bool foundExtension = false;

		for (char** req_extension = (char**)req_extensions;
			 req_extension < req_extensions + req_extension_count;
			 ++req_extension)
		{
			foundExtension = false;

			for (VkExtensionProperties* extension_property = extension_properties;
				 extension_property < extension_properties + extension_count;
				 ++extension_property)
			{
				if (strcompare(*req_extension, extension_property->extensionName))
				{
					foundExtension = true;
					break;
				}
			}

			if (foundExtension)
			{
				continue;
			}

			else
			{
				{
					const char* error_message_prefix = "VulkanValidateInstanceExtensionSupport failed. \n\n"
													   "Failed to validate all extensions.\n"
													   "The extension ";
					const char* error_message_suffix = " was not found.";

					uint32 error_message_prefix_length = strlength(error_message_prefix);
					uint32 error_message_suffix_length = strlength(error_message_suffix);
					uint32 req_extension_length = strlength(*req_extension);
					uint32 error_message_length = error_message_prefix_length
												  + req_extension_length
												  + error_message_suffix_length;

					char* error_message = (char*)HeapAlloc(vulkanInitHeap, HEAP_ZERO_MEMORY, (error_message_length + 1) * sizeof(char));
					
					memcpy(error_message, error_message_prefix, error_message_prefix_length);
					memcpy(error_message + error_message_prefix_length, *req_extension, req_extension_length);
					memcpy(error_message + error_message_prefix_length + req_extension_length, error_message_suffix, error_message_suffix_length);
					error_message[error_message_length] = '\0';

					WIN32LOG_ERROR(error_message);

					HeapFree(vulkanInitHeap, 0, error_message);
				}

				break;
			}
		}

		if (foundExtension)
		{
			foundAllExtensions = true;
		}
	}

	HeapFree(vulkanInitHeap, 0, extension_properties);

	return foundAllExtensions;
}

internal VkInstance
VulkanCreateInstance(const char* app_name, uint32 app_version,
					 const char** layers, uint32 layer_count,
					 const char** extensions, uint32 extension_count)
{
	VkInstance instance = VK_NULL_HANDLE;

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = app_name;
	app_info.applicationVersion = app_version;
	app_info.pEngineName = "Ant Game Engine";
	app_info.engineVersion = ANT_VERSION;
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledLayerCount = layer_count;
    create_info.ppEnabledLayerNames = layers;
    create_info.enabledExtensionCount = extension_count;
    create_info.ppEnabledExtensionNames = extensions;

	VkResult result;
	result = vkCreateInstance(&create_info, NULL, &instance);

	switch(result)
	{
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			WIN32LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
		break;

		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			WIN32LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
		break;

		case VK_ERROR_INITIALIZATION_FAILED:
			WIN32LOG_ERROR("VK_ERROR_INITIALIZATION_FAILED");
		break;

		case VK_ERROR_LAYER_NOT_PRESENT:
			WIN32LOG_ERROR("VK_ERROR_LAYER_NOT_PRESENT");
		break;

		case VK_ERROR_EXTENSION_NOT_PRESENT:
			WIN32LOG_ERROR("VK_ERROR_EXTENSION_NOT_PRESENT");
		break;

		case VK_ERROR_INCOMPATIBLE_DRIVER:
			WIN32LOG_ERROR("VK_ERROR_INCOMPATIBLE_DRIVER");
		break;
	}
	
	return instance;
}

internal bool
VulkanValidateDeviceExtensionSupport(HANDLE vulkanInitHeap, VkPhysicalDevice device,
									 const char** req_extensions, uint32 req_extension_count)
{
	bool foundAllExtensions = false;
	uint32 extension_count;
	VkResult result;

	result = vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL);
	Assert(!result);

	VkExtensionProperties* extension_properties =
		(VkExtensionProperties*)HeapAlloc(vulkanInitHeap, HEAP_ZERO_MEMORY, extension_count * sizeof(VkExtensionProperties));
	
	Assert(extension_properties != NULL);

	result = vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, extension_properties);
	
	if (result != VK_SUCCESS)
	{
		switch(result)
		{
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				WIN32LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
			break;

			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				WIN32LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
			break;

			case VK_ERROR_LAYER_NOT_PRESENT:
				WIN32LOG_ERROR("VK_ERROR_LAYER_NOT_PRESENT");
			break;

			default:
				Assert(false);
			break;
		}
	}

	else
	{
		bool foundExtension = false;

		for (char** req_extension = (char**)req_extensions;
			 req_extension < req_extensions + req_extension_count;
			 ++req_extension)
		{
			foundExtension = false;

			for (VkExtensionProperties* extension_property = extension_properties;
				 extension_property < extension_properties + extension_count;
				 ++extension_property)
			{
				if (strcompare(*req_extension, extension_property->extensionName))
				{
					foundExtension = true;
					break;
				}
			}

			if (foundExtension)
			{
				continue;
			}

			else
			{
				{
					const char* error_message_prefix = "VulkanValidateDeviceExtensionSupport failed. \n\n"
													   "Failed to validate all extensions.\n"
													   "The extension ";
					const char* error_message_suffix = " was not found.";

					uint32 error_message_prefix_length = strlength(error_message_prefix);
					uint32 error_message_suffix_length = strlength(error_message_suffix);
					uint32 req_extension_length = strlength(*req_extension);
					uint32 error_message_length = error_message_prefix_length
												  + req_extension_length
												  + error_message_suffix_length;

					char* error_message = (char*)HeapAlloc(vulkanInitHeap, HEAP_ZERO_MEMORY, (error_message_length + 1) * sizeof(char));
					
					memcpy(error_message, error_message_prefix, error_message_prefix_length);
					memcpy(error_message + error_message_prefix_length, *req_extension, req_extension_length);
					memcpy(error_message + error_message_prefix_length + req_extension_length, error_message_suffix, error_message_suffix_length);
					error_message[error_message_length] = '\0';

					WIN32LOG_ERROR(error_message);

					HeapFree(vulkanInitHeap, 0, error_message);
				}

				break;
			}
		}

		if (foundExtension)
		{
			foundAllExtensions = true;
		}
	}

	HeapFree(vulkanInitHeap, 0, extension_properties);

	return foundAllExtensions;
}

// TODO(soimn): tune score weights
internal int
VulkanRatePhysicalDeviceSuitability(HANDLE vulkanInitHeap, VkPhysicalDevice device,
									VkSurfaceKHR surface, const char** extension_names,
									uint32 extension_count)
{
	int score = 0;
	bool invalid = false;

	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;

	vkGetPhysicalDeviceProperties(device, &properties);
	vkGetPhysicalDeviceFeatures(device, &features);

	{
		uint32 queue_family_count;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

		VkQueueFamilyProperties* queue_family_properties =
			(VkQueueFamilyProperties*)HeapAlloc(vulkanInitHeap, HEAP_ZERO_MEMORY, queue_family_count * sizeof(VkQueueFamilyProperties));
		Assert(queue_family_properties != NULL);

		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_family_properties);

		bool hasGFXQueues	   = false;
		bool hasComputeQueues  = false;
		bool hasTransferQueues = false;
		VkBool32 supports_presentation = false;

		for (VkQueueFamilyProperties* it = queue_family_properties;
			 it < queue_family_properties + queue_family_count;
			 ++it)
		{
			if (it->queueFlags & VK_QUEUE_GRAPHICS_BIT)
				hasGFXQueues = true;

			if (!supports_presentation)
			{
				vkGetPhysicalDeviceSurfaceSupportKHR(device, (uint32)(it - queue_family_properties), surface, &supports_presentation);
			}

			if (it->queueFlags & VK_QUEUE_COMPUTE_BIT)
				hasComputeQueues = true;

			if (it->queueFlags & VK_QUEUE_TRANSFER_BIT)
				hasTransferQueues = true;
		}

		if (!hasGFXQueues || !hasTransferQueues)
			invalid = true;

		if (hasComputeQueues)
			score += 250;

		if (!supports_presentation)
			invalid = true;

		HeapFree(vulkanInitHeap, 0, queue_family_properties);
	}

	bool extensions_supported = VulkanValidateDeviceExtensionSupport(vulkanInitHeap, device,
																	 extension_names, extension_count);
	if (!extensions_supported)
		invalid = true;

	{
		uint32 format_count;
		uint32 present_mode_count;

		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, NULL);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, NULL);

		if (format_count == 0 || present_mode_count == 0)
			invalid = true;
	}

	if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		score += 1000;

	score += properties.limits.maxImageDimension2D;

	if (!features.geometryShader)
		invalid = true;

	if (invalid)
		score = 0;

	return score;
}

internal VkPhysicalDevice
VulkanGetPhysicalDevice(HANDLE vulkanInitHeap, VkInstance instance,
						VkSurfaceKHR surface, const char** extension_names,
						uint32 extension_count)
{
	VkPhysicalDevice device = VK_NULL_HANDLE;
	
	uint32 device_count;
	VkResult result;

	result = vkEnumeratePhysicalDevices(instance, &device_count, NULL);
	Assert(!result);

	VkPhysicalDevice* physical_devices = (VkPhysicalDevice*)HeapAlloc(vulkanInitHeap, HEAP_ZERO_MEMORY,
																	   device_count * sizeof(VkPhysicalDevice));
	Assert(physical_devices != NULL);

	result = vkEnumeratePhysicalDevices(instance, &device_count, physical_devices);
	
	if (result != VK_SUCCESS)
	{
		switch(result)
		{
			case VK_INCOMPLETE:
				WIN32LOG_ERROR("VK_INCOMPLETE");
			break;

			case VK_ERROR_OUT_OF_HOST_MEMORY:
				WIN32LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
			break;

			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				WIN32LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
			break;

			case VK_ERROR_INITIALIZATION_FAILED:
				WIN32LOG_ERROR("VK_ERROR_INITIALIZATION_FAILED");
			break;

			default:
				Assert(false);
			break;
		}
	}

	else
	{
		if (0 < device_count)
		{
			uint32 most_suitable_device = 0;
			uint32 most_suitable_device_score = 0;

			for (VkPhysicalDevice* it = physical_devices;
				 it < physical_devices + device_count;
				 ++it)
			{
				uint32 device_suitability = VulkanRatePhysicalDeviceSuitability(vulkanInitHeap, *it, surface,
																				extension_names, extension_count);

				if (device_suitability > most_suitable_device_score)
				{
					most_suitable_device = (uint32)(it - physical_devices);
					most_suitable_device_score = device_suitability;
				}
			}
			
			if (0 < most_suitable_device_score)
			{
				device = physical_devices[most_suitable_device];
			}

			else
			{
				WIN32LOG_ERROR("Vulkan compatible devices found, however no devices compatible with the requirements of the game were found");
			}
		}

		else
		{
			WIN32LOG_ERROR("VulkanGetPhysicalDevice found no vulkan compatible devices");
		}
	}

	HeapFree(vulkanInitHeap, 0, physical_devices);

	return device;
}

struct queue_family_info {
	int32 gfx_family;
	int32 present_family;
	int32 compute_family;
	int32 transfer_family;
};

// TODO(soimn): cleanup
internal queue_family_info
VulkanGetSuitableQueueFamilies(HANDLE vulkanInitHeap, VkPhysicalDevice device, VkSurfaceKHR surface)
{
	queue_family_info family_info = {-1, -1, -1, -1};

	uint32 queue_family_count;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

	VkQueueFamilyProperties* queue_family_properties = (VkQueueFamilyProperties*)HeapAlloc(vulkanInitHeap, HEAP_ZERO_MEMORY,
																						   queue_family_count * sizeof(VkQueueFamilyProperties));
	Assert(queue_family_properties != NULL);

	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_family_properties);

	for (VkQueueFamilyProperties* it = queue_family_properties;
		 it < queue_family_properties + queue_family_count;
		 ++it)
	{
		uint32 index = (uint32)(it - queue_family_properties);

		if (it->queueCount != 0)
		{
			if (it->queueFlags & VK_QUEUE_GRAPHICS_BIT
				&& family_info.gfx_family == -1)
			{
				family_info.gfx_family = index;
			}

			if (family_info.present_family == -1)
			{
				VkBool32 current_family_supports_presentation = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface,
													 &current_family_supports_presentation);

				if (current_family_supports_presentation)
				{
					family_info.present_family = index;
				}
			}

			if (it->queueCount != 0 && it->queueFlags == VK_QUEUE_COMPUTE_BIT)
			{
				family_info.compute_family = index;
			}

			if (it->queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				if (family_info.transfer_family == -1)
				{
					if (index == (uint32)family_info.gfx_family)
					{
						if (it->queueCount > 1)
						{
							family_info.transfer_family = index;
						}
					}

					else
					{
						family_info.transfer_family = index;
					}
				}

				else
				{
					if (family_info.gfx_family == family_info.transfer_family)
					{
						family_info.transfer_family = index;
					}
				}
			}
		}
	}

	HeapFree(vulkanInitHeap, 0, queue_family_properties);

	return family_info;
}

internal VkDevice
VulkanCreateDevice(VkPhysicalDevice physical_device, queue_family_info family_info,
				   const char** extension_names, uint32 extension_count,
				   VkQueue* gfx_queue, VkQueue* present_queue,
				   VkQueue* compute_queue, VkQueue* transfer_queue,
				   bool has_separate_present_queue)
{
	VkDevice device = VK_NULL_HANDLE;

	VkDeviceQueueCreateInfo queue_create_info[3] = {};
	const float priorities[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	uint32 queue_create_info_count = 0;

	queue_create_info[queue_create_info_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info[queue_create_info_count].pNext = NULL;
    queue_create_info[queue_create_info_count].flags = 0;
    queue_create_info[queue_create_info_count].queueFamilyIndex = family_info.gfx_family;
    queue_create_info[queue_create_info_count].queueCount = 1;
    queue_create_info[queue_create_info_count].pQueuePriorities = &priorities[0];
	++queue_create_info_count;

	if (has_separate_present_queue)
	{
		queue_create_info[queue_create_info_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info[queue_create_info_count].pNext = NULL;
		queue_create_info[queue_create_info_count].flags = 0;
		queue_create_info[queue_create_info_count].queueFamilyIndex = family_info.present_family;
		queue_create_info[queue_create_info_count].queueCount = 1;
		queue_create_info[queue_create_info_count].pQueuePriorities = &priorities[1];
		++queue_create_info_count;
	}

	if (family_info.transfer_family == family_info.gfx_family)
	{
		queue_create_info[0].queueCount = 2;
	}

	else
	{
		queue_create_info[queue_create_info_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info[queue_create_info_count].pNext = NULL;
		queue_create_info[queue_create_info_count].flags = 0;
		queue_create_info[queue_create_info_count].queueFamilyIndex = family_info.transfer_family;
		queue_create_info[queue_create_info_count].queueCount = 1;
		queue_create_info[queue_create_info_count].pQueuePriorities = &priorities[2];
		++queue_create_info_count;
	}

	if (family_info.compute_family != -1)
	{
		queue_create_info[queue_create_info_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info[queue_create_info_count].pNext = NULL;
		queue_create_info[queue_create_info_count].flags = 0;
		queue_create_info[queue_create_info_count].queueFamilyIndex = family_info.compute_family;
		queue_create_info[queue_create_info_count].queueCount = 1;
		queue_create_info[queue_create_info_count].pQueuePriorities = &priorities[3];
		++queue_create_info_count;
	}

	VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = NULL;
    device_create_info.flags = 0;
    device_create_info.queueCreateInfoCount = queue_create_info_count;
    device_create_info.pQueueCreateInfos = queue_create_info;
    device_create_info.enabledExtensionCount = extension_count;
    device_create_info.ppEnabledExtensionNames = extension_names;
    device_create_info.pEnabledFeatures = NULL;

	VkResult result = vkCreateDevice(physical_device, &device_create_info, NULL, &device);
	if (result != VK_SUCCESS)
	{
		switch(result)
		{
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				WIN32LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
			break;

			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				WIN32LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
			break;

			case VK_ERROR_INITIALIZATION_FAILED:
				WIN32LOG_ERROR("VK_ERROR_INITIALIZATION_FAILED");
			break;

			case VK_ERROR_EXTENSION_NOT_PRESENT:
				WIN32LOG_ERROR("VK_ERROR_EXTENSION_NOT_PRESENT");
			break;

			case VK_ERROR_FEATURE_NOT_PRESENT:
				WIN32LOG_ERROR("VK_ERROR_FEATURE_NOT_PRESENT");
			break;

			case VK_ERROR_TOO_MANY_OBJECTS:
				WIN32LOG_ERROR("VK_ERROR_TOO_MANY_OBJECTS");
			break;

			case VK_ERROR_DEVICE_LOST:
				WIN32LOG_ERROR("VK_ERROR_DEVICE_LOST");
			break;

			default:
				Assert(false);
			break;
		}
	}
	
	else
	{
		uint32 transfer_index = 0;
		if (family_info.transfer_family == family_info.gfx_family)
		{
			transfer_index = 1;
		}

		vkGetDeviceQueue(device, family_info.gfx_family, 0, gfx_queue);
		vkGetDeviceQueue(device, family_info.present_family, 0, present_queue);
		vkGetDeviceQueue(device, family_info.transfer_family, transfer_index, transfer_queue);

		if (family_info.compute_family != -1)
		{
			vkGetDeviceQueue(device, family_info.compute_family, 0, compute_queue);
		}

		else
		{
			compute_queue = VK_NULL_HANDLE;
		}
	}

	return device;
}

internal VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
					VkDebugUtilsMessageTypeFlagsEXT message_type,
					const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
					void* user_data)
{
	if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		WIN32LOG_ERROR(callback_data->pMessage);
	}

    return VK_FALSE;
}

internal void
VulkanSetupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT* debug_messenger)
{
	VkDebugUtilsMessengerCreateInfoEXT create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
								  | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
								  | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
								  | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
							  | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
							  | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	create_info.pfnUserCallback = &VulkanDebugCallback;
	create_info.pUserData = NULL;

	PFN_vkCreateDebugUtilsMessengerEXT create_debug_messenger =
		(PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	
	if (create_debug_messenger)
	{
		create_debug_messenger(instance, &create_info, NULL, debug_messenger);
	}

	else
	{
		WIN32LOG_ERROR("Could not get the address of the function \"vkCreateDebugUtilsMessengerEXT\".\n"
					   "Debug messages from Vulkan are disabled.");
	}
}

internal VkSurfaceKHR
VulkanCreateSurface(HINSTANCE processInstance, HWND windowHandle, VkInstance instance)
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	VkWin32SurfaceCreateInfoKHR create_info = {};
	create_info.sType	  = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	create_info.pNext	  = NULL;
	create_info.flags	  = 0;
	create_info.hinstance = processInstance;
	create_info.hwnd	  = windowHandle;

	VkResult result = vkCreateWin32SurfaceKHR(instance, &create_info, NULL, &surface);
	if (result != VK_SUCCESS)
	{
		switch(result)
		{
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				WIN32LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
			break;

			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				WIN32LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
			break;

			default:
				Assert(false);
			break;
		}
	}

	return surface;
}

internal VkSurfaceFormatKHR
VulkanChooseSwapchainSurfaceFormat(VkSurfaceFormatKHR* available_formats, uint32 available_format_count)
{
	VkSurfaceFormatKHR surface_format = {};

	// NOTE(soimn): this case covers devices with no preferred format and is to be set to the optimal choice
	if (available_format_count == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED)
	{
		surface_format = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
	}

	else
	{
		bool has_selected_format = false;

		for (VkSurfaceFormatKHR* it = available_formats;
			 it < available_formats + available_format_count;
			 ++it)
		{
			if (it->format == VK_FORMAT_B8G8R8A8_UNORM
				&& it->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				surface_format = *it;
				has_selected_format = true;
				break;
			}
		}

		if (!has_selected_format)
		{
			WIN32LOG_DEBUG("Optimal surface format not found, selecting first found");
			surface_format = available_formats[0];
		}
	}

	return surface_format;
}

internal VkPresentModeKHR
VulkanChooseSwapchainPresentMode(VkPresentModeKHR* available_modes, uint32 available_mode_count)
{
	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

	for (VkPresentModeKHR* it = available_modes;
		 it < available_modes + available_mode_count;
		 ++it)
	{
		if (*it == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			present_mode = *it;
			break;
		}

		else if (*it == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			present_mode = *it;
		}
	}

	return present_mode;
}

internal VkExtent2D
VulkanChooseSwapchainSwapExtent(VkSurfaceCapabilitiesKHR capabilities,
								uint32 preferred_width = 0, uint32 preferred_height = 0)
{
	VkExtent2D extent = {preferred_width, preferred_height};

	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		extent = capabilities.currentExtent;
	}

	else
	{	
		if (preferred_width == 0 || preferred_height == 0)
		{
			extent = {capabilities.maxImageExtent.width,
					  capabilities.maxImageExtent.height};
		}

		else
		{
			extent = {CLAMP(capabilities.minImageExtent.width, extent.width, capabilities.maxImageExtent.width),
					  CLAMP(capabilities.minImageExtent.height, extent.height, capabilities.maxImageExtent.height)};
		}
	}

	return extent;
}

internal VkSwapchainKHR
VulkanCreateSwapchain(HANDLE vulkanInitHeap, queue_family_info family_info,
					  VkPhysicalDevice physical_device, VkSurfaceKHR surface,
					  VkDevice device, VkFormat* format_ptr,
					  VkExtent2D* extent_ptr)
{
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;

	VkSurfaceFormatKHR chosen_surface_format;
	VkPresentModeKHR chosen_present_mode;
	VkExtent2D chosen_extent;

	VkSurfaceCapabilitiesKHR capabilities;
	VkSurfaceFormatKHR* formats = NULL;
	VkPresentModeKHR* present_modes = NULL;

	VkResult result;
	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);
	
	if (result != VK_SUCCESS)
	{
		switch(result)
		{
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				WIN32LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
			break;

			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				WIN32LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
			break;

			case VK_ERROR_SURFACE_LOST_KHR:
				WIN32LOG_ERROR("VK_ERROR_SURFACE_LOST_KHR");
			break;

			default:
				Assert(false);
			break;
		}
	}

	else
	{
		bool formats_found = false;
		bool present_modes_found = false;

		uint32 format_count;
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, NULL);
		Assert(!result);

		formats = (VkSurfaceFormatKHR*) HeapAlloc(vulkanInitHeap, HEAP_ZERO_MEMORY,
												  format_count * sizeof(VkSurfaceFormatKHR));
		Assert(formats != NULL || format_count == 0);

		result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats);
		
		if (result != VK_SUCCESS)
		{
			switch(result)
			{
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					WIN32LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
				break;

				case VK_ERROR_OUT_OF_DEVICE_MEMORY:
					WIN32LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
				break;

				case VK_ERROR_SURFACE_LOST_KHR:
					WIN32LOG_ERROR("VK_ERROR_SURFACE_LOST_KHR");
				break;

				default:
					Assert(false);
				break;
			}
		}

		else
		{
			formats_found = true;
		}

		uint32 present_mode_count;
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, NULL);
		Assert(!result);

		present_modes = (VkPresentModeKHR*) HeapAlloc(vulkanInitHeap, HEAP_ZERO_MEMORY,
													  present_mode_count * sizeof(VkPresentModeKHR));
		Assert(present_modes != NULL || present_mode_count == 0);

		result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes);
		
		if (result != VK_SUCCESS)
		{
			switch(result)
			{
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					WIN32LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
				break;

				case VK_ERROR_OUT_OF_DEVICE_MEMORY:
					WIN32LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
				break;

				case VK_ERROR_SURFACE_LOST_KHR:
					WIN32LOG_ERROR("VK_ERROR_SURFACE_LOST_KHR");
				break;

				default:
					Assert(false);
				break;
			}
		}

		else
		{
			present_modes_found = true;
		}


		if (formats_found && present_modes_found)
		{
			chosen_surface_format = VulkanChooseSwapchainSurfaceFormat(formats, format_count);
			chosen_present_mode	  = VulkanChooseSwapchainPresentMode(present_modes, present_mode_count);
			chosen_extent		  = VulkanChooseSwapchainSwapExtent(capabilities);

			*format_ptr = chosen_surface_format.format;
			*extent_ptr = chosen_extent;

			uint32 image_count = capabilities.minImageCount + 1;
			if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount)
			{
				image_count = capabilities.maxImageCount;
			}

			VkSwapchainCreateInfoKHR create_info = {};
			create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			create_info.pNext = NULL;
			create_info.flags = 0;
			create_info.surface = surface;
			create_info.minImageCount = image_count;
			create_info.imageFormat = chosen_surface_format.format;
			create_info.imageColorSpace = chosen_surface_format.colorSpace;
			create_info.imageExtent = chosen_extent;
			create_info.imageArrayLayers = 1;
			create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // NOTE(soimn): change this to VK_IMAGE_USAGE_TRANSFER_DST_BIT to enable post-processing
			create_info.preTransform = capabilities.currentTransform;
			create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			create_info.presentMode = chosen_present_mode;
			create_info.clipped = VK_TRUE;
			create_info.oldSwapchain = VK_NULL_HANDLE;

			if (family_info.gfx_family != family_info.present_family)
			{
				uint32 families[] = {(uint32)family_info.gfx_family, (uint32)family_info.present_family};
				create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				create_info.queueFamilyIndexCount = 2;
				create_info.pQueueFamilyIndices = families;
			}
			else
			{
				create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				create_info.queueFamilyIndexCount = 0;
				create_info.pQueueFamilyIndices = NULL;
			}

			result = vkCreateSwapchainKHR(device, &create_info, NULL, &swapchain);

			if (result != VK_SUCCESS)
			{
				switch(result)
				{
					case VK_ERROR_OUT_OF_HOST_MEMORY:
						WIN32LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
					break;

					case VK_ERROR_OUT_OF_DEVICE_MEMORY:
						WIN32LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
					break;

					case VK_ERROR_DEVICE_LOST:
						WIN32LOG_ERROR("VK_ERROR_DEVICE_LOST");
					break;

					case VK_ERROR_SURFACE_LOST_KHR:
						WIN32LOG_ERROR("VK_ERROR_SURFACE_LOST_KHR");
					break;

					case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
						WIN32LOG_ERROR("VK_ERROR_NATIVE_WINDOW_IN_USE_KHR");
					break;

					default:
						Assert(false);
					break;
				}
			}
		}

		HeapFree(vulkanInitHeap, 0, present_modes);

		HeapFree(vulkanInitHeap, 0, formats);
	}

	return swapchain;
}

internal uint32
VulkanCreateSwapchainImages(uint8* memory_stack_ptr, VkFormat swapchain_image_format,
							VkDevice device, VkSwapchainKHR swapchain,
							VkImage*& swapchain_images, uint32* swapchain_image_count,
							VkImageView*& swapchain_image_views, uint32* swapchain_image_view_count)
{
	bool succeeded = false;

	uint32 image_count;
	VkResult result;
	result = vkGetSwapchainImagesKHR(device, swapchain,
									 &image_count, NULL);
	Assert(!result);

	swapchain_images = (VkImage*) memory_stack_ptr;
	*swapchain_image_count = image_count;
	swapchain_image_views =
		(VkImageView*) memory_stack_ptr + image_count * sizeof(VkImage);

	*swapchain_image_view_count = image_count;

	result = vkGetSwapchainImagesKHR(device, swapchain,
									 &image_count, (VkImage*) memory_stack_ptr);

	if(result != VK_SUCCESS)
	{
		switch(result)
		{
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				WIN32LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
			break;

			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				WIN32LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
			break;
			
			default:
				Assert(false);
			break;
		}
	}

	else
	{
		uint32 i = 0;
		for (; i < image_count; ++i)
		{
			VkImageViewCreateInfo create_info = {};
			create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			create_info.pNext = NULL;
			create_info.flags = 0;
			create_info.image = swapchain_images[i];
			create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			create_info.format = swapchain_image_format;
			create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			create_info.subresourceRange.baseMipLevel = 0;
			create_info.subresourceRange.levelCount = 1;
			create_info.subresourceRange.baseArrayLayer = 0;
			create_info.subresourceRange.layerCount = 1;

			result = vkCreateImageView(device, &create_info, NULL, swapchain_image_views);
			
			if (result != VK_SUCCESS)
			{
				switch(result)
				{
					case VK_ERROR_OUT_OF_HOST_MEMORY:
						WIN32LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
					break;

					case VK_ERROR_OUT_OF_DEVICE_MEMORY:
						WIN32LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
					break;

					default:
						Assert(false);
					break;
				}

				break;
			}
		}

		if (i == image_count)
			succeeded = true;
	}

	return (succeeded ? image_count : 0);
}

internal bool
Win32InitVulkan(game_memory* memory, vulkan_application* application,
				HINSTANCE processInstance, HWND windowHandle,
				const char* app_name, uint32 app_version)
{
	bool succeeded = false;

	HANDLE vulkanInitHeap;
	vulkanInitHeap = HeapCreate(0, MEGABYTES(1), MEGABYTES(4));

	if (vulkanInitHeap == NULL)
	{
		WIN32LOG_ERROR("Failed to create heap when initializing Vulkan");
	}

	else
	{
		do
		{
			application->extension_count = 0;
			application->layer_count = 0;

			application->extensions[application->extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
			application->extensions[application->extension_count++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;

			#ifdef ANT_VULKAN_ENABLE_VALIDATION_LAYERS
			application->extensions[application->extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
			application->layers[application->layer_count++] = "VK_LAYER_LUNARG_standard_validation";
			#endif

			Assert(application->extension_count <= ANT_VULKAN_INSTANCE_EXTENSION_COUNT_LIMIT);
			Assert(application->layer_count <= ANT_VULKAN_INSTANCE_LAYER_COUNT_LIMIT);

			bool instance_extensions_supported = VulkanValidateInstanceExtensionSupport(vulkanInitHeap, application->extensions, application->extension_count);
			bool layers_supported			   = VulkanValidateLayerSupport(vulkanInitHeap, application->layers, application->layer_count);

			// TODO(soimn): setup debug message callback
			
			if (!(layers_supported && instance_extensions_supported))
				break;

			application->instance = VulkanCreateInstance(app_name, app_version,
														 application->layers, application->layer_count,
														 application->extensions, application->extension_count);

			if (application->instance == VK_NULL_HANDLE)
				break;

			#ifdef ANT_VULKAN_ENABLE_VALIDATION_LAYERS
			VulkanSetupDebugMessenger(application->instance, &application->debug_messenger);
			#endif

			application->surface = VulkanCreateSurface(processInstance, windowHandle, application->instance);

			if (application->surface == VK_NULL_HANDLE)
				break;

			application->device_extensions[application->device_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

			Assert(application->device_extension_count <= ANT_VULKAN_DEVICE_EXTENSION_COUNT_LIMIT);

			application->physical_device = VulkanGetPhysicalDevice(vulkanInitHeap, application->instance,
																   application->surface, application->device_extensions,
																   application->device_extension_count);

			if (application->physical_device == VK_NULL_HANDLE)
				break;

			queue_family_info family_info = VulkanGetSuitableQueueFamilies(vulkanInitHeap, application->physical_device, application->surface);
			application->has_compute_queue = (family_info.compute_family != -1);
			application->has_separate_present_queue = (family_info.gfx_family != family_info.present_family);

			if (family_info.gfx_family != -1 && family_info.transfer_family != -1)
			{

				application->device = VulkanCreateDevice(application->physical_device, family_info,
														 application->device_extensions, application->device_extension_count,
														 &application->gfx_queue, &application->present_queue,
														 &application->compute_queue, &application->transfer_queue,
														 application->has_separate_present_queue);
				
				if (application->device == VK_NULL_HANDLE)
					break;

				application->swapchain = VulkanCreateSwapchain(vulkanInitHeap, family_info,
															   application->physical_device, application->surface,
															   application->device, &application->swapchain_image_format,
															   &application->swapchain_extent);

				if (application->swapchain == VK_NULL_HANDLE)
					break;

				uint32 successfully_created_swapchain_images = VulkanCreateSwapchainImages(memory->persistent_stack_ptr, application->swapchain_image_format,
																						   application->device, application->swapchain,
																						   application->swapchain_images, &application->swapchain_image_count,
																						   application->swapchain_image_views, &application->swapchain_image_view_count);

				if (successfully_created_swapchain_images == 0)
					break;

				memory->persistent_stack_ptr += successfully_created_swapchain_images * (sizeof(VkImage) + sizeof(VkImageView));

				succeeded = true;
			}

			else
			{
				WIN32LOG_ERROR("Physical device does not support graphics and transfer queues");
			}
		}
		while(0);

		HeapDestroy(vulkanInitHeap);
	}

	return succeeded;
}

// TODO(soimn): Check if the function properly cleans up the resources on initialization failure
internal void
Win32CleanupVulkan(vulkan_application* application)
{
	for (VkImageView* it = application->swapchain_image_views;
		 it < application->swapchain_image_views + application->swapchain_image_view_count;
		 ++it)
	{
		vkDestroyImageView(application->device, *it, NULL);
	}

	vkDestroySwapchainKHR(application->device, application->swapchain, NULL);

	#ifdef ANT_VULKAN_ENABLE_VALIDATION_LAYERS
	{
		PFN_vkDestroyDebugUtilsMessengerEXT destroy_debug_messenger = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(application->instance, "vkDestroyDebugUtilsMessengerEXT");
		destroy_debug_messenger(application->instance, application->debug_messenger, NULL);
	}
	#endif

	vkDestroySurfaceKHR(application->instance, application->surface, NULL);
	vkDestroyInstance(application->instance, NULL);
	vkDestroyDevice(application->device, NULL);
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


/*
 *	ENTRY POINT
 */

int CALLBACK WinMain(HINSTANCE instance,
					 HINSTANCE prevInstance,
					 LPSTR commandLine,
					 int windowShowMode)
{
	UNUSED_PARAMETER(prevInstance);
	UNUSED_PARAMETER(commandLine);
	UNUSED_PARAMETER(windowShowMode);

#ifdef ANT_CONSOLE_ENABLED
	// TODO(soimn): consider logging error to system
	AllocConsole();
	AttachConsole(ATTACH_PARENT_PROCESS);
	SetConsoleTitle("Ant engine | console window");
#endif

	HWND windowHandle;
	WNDCLASSEXA windowClass = {};

	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_VREDRAW | CS_HREDRAW;
	windowClass.lpfnWndProc = &Win32MainWindowProc;
	windowClass.hInstance = instance;
	windowClass.hbrBackground = 0;
	windowClass.lpszClassName = "win32_ant";

	if (RegisterClassExA(&windowClass))
	{
		windowHandle =
			CreateWindowExA(windowClass.style,
						    windowClass.lpszClassName,
						    application_name,
						    WS_VISIBLE | WS_OVERLAPPEDWINDOW,
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
			
			// Memory
			game_memory memory = {};
			memory.persistent_size = MEGABYTES(64);
			memory.persistent_memory = VirtualAlloc(NULL, memory.persistent_size,
													MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			memory.persistent_stack_ptr = (uint8*)memory.persistent_memory;

			memory.transient_size = GIGABYTES(2);
			memory.transient_memory = VirtualAlloc(NULL, memory.transient_size,
													MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			memory.transient_stack_ptr = (uint8*)memory.transient_memory;

			if (memory.persistent_memory && memory.transient_memory)
			{

				// Raw input
				bool input_ready = Win32InitRawInput();

				// Vulkan
				vulkan_application* application = new(memory.persistent_memory) vulkan_application();
				memory.persistent_stack_ptr += sizeof(vulkan_application);
				bool vulkan_ready = Win32InitVulkan(&memory, application, instance, windowHandle, application_name, application_version);

				if (input_ready && vulkan_ready)
				{
					running = true;

					/// Main Loop
					while (running)
					{
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
					}
				}

				Win32CleanupVulkan(application);
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
 *	- Setup game input abstraction supporting future rebinding of keys
 *	- Setup file read, write, ...
 *	- Setup audio
 *	- Setup Vulkan
 *	- Implement hot reloading
 */

// NOTE(soimn): Vulkan is currently setup only for gpu support
