#define RENDERER_INTERNAL
#include "renderer.h"
#undef RENDERER_INTERNAL

global_variable memory_arena* Memory;
global_variable vulkan_api VulkanAPI;
global_variable renderer_state RendererState;
global_variable error_callback* ErrorCallback;

#ifdef PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif

#ifdef Assert
#undef Assert
#endif

#ifdef DEBUG_MODE
#define Assert(EX) if (!(EX)) *(int*)0 = 0;
#else
#define Assert(EX)
#endif

#define LOG_ERROR(message)	 ErrorCallback(message, __FUNCTION__, __LINE__, RENDERER_ERROR_MESSAGE)

#ifdef DEBUG_MODE
#define LOG_WARNING(message) ErrorCallback(message, __FUNCTION__, __LINE__, RENDERER_WATNING_MESSAGE)
#define LOG_INFO(message)    ErrorCallback(message, __FUNCTION__, __LINE__, RENDERER_INFO_MESSAGE)
#define LOG_DEBUG(message)   ErrorCallback(message, __FUNCTION__, __LINE__, RENDERER_DEBUG_MESSAGE)
#else
#define LOG_WATNING(message)
#define LOG_INFO(message)
#define LOG_DEBUG(message)
#endif

/// DEBUG
#ifdef DEBUG_MODE
internal VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
					VkDebugUtilsMessageTypeFlagsEXT message_type,
					const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
					void* user_data)
{
	RENDERER_ERROR_MESSAGE_SEVERITY severity = {};

	switch(message_severity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			severity = RENDERER_ERROR_MESSAGE;
		break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			severity = RENDERER_WARNING_MESSAGE;
		break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			severity = RENDERER_INFO_MESSAGE;
		break;
	}

	ErrorCallback(callback_data->pMessage, __FUNCTION__, __LINE__, severity);

    return VK_FALSE;
}

internal VkDebugUtilsMessengerEXT
VulkanSetupDebugMessenger()
{
	VkDebugUtilsMessengerEXT error_callback = VK_NULL_HANDLE;

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
		(PFN_vkCreateDebugUtilsMessengerEXT) VulkanAPI.vkGetInstanceProcAddr(RendererState.instance, "vkCreateDebugUtilsMessengerEXT");
	
	if (create_debug_messenger)
	{
		VkResult result = create_debug_messenger(RendererState.instance, &create_info, NULL, &error_callback);

		if (result != VK_SUCCESS)
		{
			switch (result)
			{
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					LOG_ERROR("Could not create debug messenger. Reason: out of host memory");
				break;

				default:
					Assert(false);
				break;
			}
		}
	}

	else
	{
		LOG_ERROR("Could not get the address of the function 'vkCreateDebugUtilsMessengerEXT'.\n"
				  "Debug and error messages from Vulkan are now disabled.");
	}

	return error_callback;
}
#endif


/// RENDERER FUNCTIONS

internal bool
VulkanLoadInstanceLevelFunctions()
{
	bool succeeded = false;

	#define VK_INSTANCE_LEVEL_FUNCTION(func)\
		if (VulkanAPI.func = (PFN_##func) VulkanAPI.vkGetInstanceProcAddr(RendererState.instance, #func);\
			VulkanAPI.func == NULL)\
		{\
			LOG_ERROR("VulkanLoadInstanceLevelFunctions failed. Reason: the function '" #func "' was not found.");\
			break;\
		}

	do
	{
		#include "vulkan_functions.inl"

		succeeded = true;
	}
	while(0);

	#undef VK_INSTANCE_LEVEL_FUNCTION

	return succeeded;
}

internal bool
VulkanLoadDeviceLevelFunctions()
{
	bool succeeded = false;

	#define VK_DEVICE_LEVEL_FUNCTION(func)\
		if (VulkanAPI.func = (PFN_##func) VulkanAPI.vkGetDeviceProcAddr(RendererState.device, #func);\
			VulkanAPI.func == NULL)\
		{\
			LOG_ERROR("VulkanLoadInstanceDeviceFunctions failed. Reason: the function '" #func "' was not found.");\
			break;\
		}

	do
	{
		#include "vulkan_functions.inl"

		succeeded = true;
	}
	while(0);

	#undef VK_DEVICE_LEVEL_FUNCTION

	return succeeded;
}

RENDERER_EXPORT RENDERER_VERIFY_LAYER_SUPPORT_FUNCTION(VulkanVerifyLayerSupport)
{
	Assert(req_layer_names && req_layer_count);

	bool succeeded = false;
	uint32 found_layer_count = 0;

	uint32 layer_count;
	VkResult result = VulkanAPI.vkEnumerateInstanceLayerProperties(&layer_count, NULL);

	if (result != VK_SUCCESS)
	{
		LOG_ERROR("VulkanVerifyLayerSupport failed. Reason: the function call to 'vkEnumerateInstanceLayerProperties'"
				  "was not successful, when querying for the supported layer count");
	}

	else
	{
		VkLayerProperties* layer_properties = PushArray(Memory, VkLayerProperties, layer_count);

		if (!layer_properties)
		{
			LOG_ERROR("VulkanVerifyLayerSupport failed. Reason: failed to allocate memory for the layer properties");
		}

		else
		{
			result = VulkanAPI.vkEnumerateInstanceLayerProperties(&layer_count, layer_properties);

			if (result != VK_SUCCESS)
			{
				switch(result)
				{
					case VK_ERROR_OUT_OF_HOST_MEMORY:
						LOG_ERROR("VulkanVerifyLayerSupport failed. Reason: the host is out of memory,\nVK_ERROR_OUT_OF_HOST_MEMORY");
					break;

					case VK_ERROR_OUT_OF_DEVICE_MEMORY:
						LOG_ERROR("VulkanVerifyLayerSupport failed. Reason: the device is out of memory,\nVK_ERROR_OUT_OF_DEVICE_MEMORY");
					break;

					default:
						Assert(false);
					break;
				}
			}

			else
			{
				for (char** current_layer_name = (char**) req_layer_names;
					 current_layer_name < req_layer_names + req_layer_count;
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

				if (found_layer_count == req_layer_count)
				{
					succeeded = true;
				}
			}
		}
	}

	return succeeded;
}

RENDERER_EXPORT RENDERER_VERIFY_INSTANCE_EXTENSION_SUPPORT_FUNCTION(VulkanVerifyInstanceExtensionSupport)
{
	Assert(req_extension_names && req_extension_count);

	bool succeeded = false;
	uint32 found_extension_count = 0;

	uint32 extension_count;
	VkResult result = VulkanAPI.vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);

	if (result != VK_SUCCESS)
	{
		LOG_ERROR("VulkanVerifyExtensionSupport failed. Reason: the function call to 'vkEnumerateInstanceExtensionProperties'"
					  "was not successful, when querying for the supported extension count");
	}

	else
	{
		VkExtensionProperties* extension_properties = PushArray(Memory, VkExtensionProperties, extension_count);

		if (!extension_properties)
		{
			LOG_ERROR("VulkanVerifyExtensionSupport failed. Reason: failed to allocate memory for the extension properties");
		}

		else
		{
			result = VulkanAPI.vkEnumerateInstanceExtensionProperties(NULL, &extension_count, extension_properties);

			if (result != VK_SUCCESS)
			{
				switch(result)
				{
					case VK_ERROR_OUT_OF_HOST_MEMORY:
						LOG_ERROR("VulkanVerifyExtensionSupport failed. Reason: the host is out of memory,\nVK_ERROR_OUT_OF_HOST_MEMORY");
					break;

					case VK_ERROR_OUT_OF_DEVICE_MEMORY:
						LOG_ERROR("VulkanVerifyExtensionSupport failed. Reason: the device is out of memory,\nVK_ERROR_OUT_OF_DEVICE_MEMORY");
					break;

					default:
						Assert(false);
					break;
				}
			}

			else
			{
				for (char** current_extension_name = (char**) req_extension_names;
					 current_extension_name < req_extension_names + req_extension_count;
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

				if (found_extension_count == req_extension_count)
				{
					succeeded = true;
				}
			}
		}
	}

	return succeeded;
}

RENDERER_EXPORT RENDERER_CREATE_INSTANCE_FUNCTION(VulkanCreateInstance)
{
	bool succeeded = false;

	if (!VulkanVerifyLayerSupport(layers, layer_count) || !VulkanVerifyInstanceExtensionSupport(extensions, extension_count))
	{
		LOG_ERROR("VulkanCreateInstance failed. Reason: the requested layers and extensions are not supported");
	}

	else
	{
		VkApplicationInfo app_info = {};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = app_name;
		app_info.applicationVersion = app_version;
		app_info.pEngineName = engine_name;
		app_info.engineVersion = engine_version;
		app_info.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pNext = extension_ptr;
		create_info.flags = create_flags;
		create_info.pApplicationInfo = &app_info;
		create_info.enabledLayerCount = layer_count;
		create_info.ppEnabledLayerNames = layers;
		create_info.enabledExtensionCount = extension_count;
		create_info.ppEnabledExtensionNames = extensions;

		VkResult result;
		result = VulkanAPI.vkCreateInstance(&create_info, NULL, &RendererState.instance);
		
		if (result != VK_SUCCESS)
		{
			switch(result)
			{
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					LOG_ERROR("VulkanCreateInstance failed. Reason: the host is out of memory,\nVK_ERROR_OUT_OF_HOST_MEMORY");
				break;

				case VK_ERROR_OUT_OF_DEVICE_MEMORY:
					LOG_ERROR("VulkanCreateInstance failed. Reason: the device is out of memory,\nVK_ERROR_OUT_OF_DEVICE_MEMORY");
				break;

				case VK_ERROR_INITIALIZATION_FAILED:
					LOG_ERROR("VulkanCreateInstance failed. Reason: the initialization of the instance failed,\nVK_ERROR_INITIALIZATION_FAILED");
				break;

				case VK_ERROR_LAYER_NOT_PRESENT:
					LOG_ERROR("VulkanCreateInstance failed. Reason: one, or more, of the layers requested are not present,\nVK_ERROR_LAYER_NOT_PRESENT");
				break;

				case VK_ERROR_EXTENSION_NOT_PRESENT:
					LOG_ERROR("VulkanCreateInstance failed. Reason: one, or more, of the extensions requested are not present,\nVK_ERROR_EXTENSION_NOT_PRESENT");
				break;

				case VK_ERROR_INCOMPATIBLE_DRIVER:
					LOG_ERROR("VulkanCreateInstance failed. Reason: the driver is incompatible,\nVK_ERROR_INCOMPATIBLE_DRIVER");
				break;

				default:
					Assert(false);
				break;
			}
		}

		else
		{
			succeeded = VulkanLoadInstanceLevelFunctions();

			#if DEBUG_MODE
			VulkanSetupDebugMessenger();
			#endif
		}
	}
	
	return succeeded;
}

RENDERER_EXPORT RENDERER_VERIFY_PHYSICAL_DEVICE_EXTENSION_SUPPORT_FUNCTION(VulkanVerifyPhysicalDeviceExtensionSupport)
{
	Assert(req_extension_names && req_extension_count);

	bool succeeded = false;
	uint32 found_extension_count = 0;

	uint32 extension_count;
	VkResult result = VulkanAPI.vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL);

	if (result != VK_SUCCESS)
	{
		LOG_ERROR("VulkanVerifyPhysicalDeviceExtensionSupport failed. Reason: the function call to 'vkEnumerateDeviceExtensionProperties'"
					  "was not successful, when querying for the supported extension count");
	}

	else
	{
		VkExtensionProperties* extension_properties = PushArray(Memory, VkExtensionProperties, extension_count);

		if (!extension_properties)
		{
			LOG_ERROR("VulkanVerifyPhysicalDeviceExtensionSupport failed. Reason: failed to allocate memory for the extension properties");
		}

		else
		{
			result = VulkanAPI.vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, extension_properties);

			if (result != VK_SUCCESS)
			{
				switch(result)
				{
					case VK_ERROR_OUT_OF_HOST_MEMORY:
						LOG_ERROR("VulkanVerifyPhysicalDeviceExtensionSupport failed. Reason: the host is out of memory,\n"
								  "VK_ERROR_OUT_OF_HOST_MEMORY");
					break;

					case VK_ERROR_OUT_OF_DEVICE_MEMORY:
						LOG_ERROR("VulkanVerifyPhysicalDeviceExtensionSupport failed. Reason: the device is out of memory,\n"
								  "VK_ERROR_OUT_OF_DEVICE_MEMORY");
					break;

					default:
						Assert(false);
					break;
				}
			}

			else
			{
				for (char** current_extension_name = (char**) req_extension_names;
					 current_extension_name < req_extension_names + req_extension_count;
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

				if (found_extension_count == req_extension_count)
				{
					succeeded = true;
				}
			}
		}
	}

	return succeeded;
}

internal int
VulkanRatePhysicalDeviceSuitability(VkPhysicalDevice device, const char** req_extension_names,
									uint32 req_extension_count)
{
	int score = -1;
	bool invalid = false;

	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;

	VulkanAPI.vkGetPhysicalDeviceProperties(device, &properties);
	VulkanAPI.vkGetPhysicalDeviceFeatures(device, &features);

	/// QUEUES
	{
		uint32 queue_family_count;
		VkQueueFamilyProperties* queue_family_properties;
		VulkanAPI.vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

		queue_family_properties = PushArray(Memory, VkQueueFamilyProperties, queue_family_count);

		VulkanAPI.vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_family_properties);

		bool supports_gfx		   = false;
		bool supports_compute	   = false;
		bool supports_transfer	   = false;
		bool supports_presentation = false;

		for (uint32 i = 0; i < queue_family_count; ++i)
		{
			if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				supports_gfx = true;
			}

			if (queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				supports_compute = true;
			}

			if (queue_family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				supports_transfer = true;
			}

			VkBool32 queue_supports_presentation = {};
			VulkanAPI.vkGetPhysicalDeviceSurfaceSupportKHR(device, i, RendererState.surface, &queue_supports_presentation);
			supports_presentation = (supports_presentation || (bool) queue_supports_presentation);
		}

		if (!supports_gfx || !supports_transfer || !supports_presentation)
		{
			invalid = true;
		}

		if (supports_compute)
		{
			score += 250;
		}
	}

	/// EXTENSIONS
	bool extensions_supported = VulkanVerifyPhysicalDeviceExtensionSupport(device, req_extension_names,
																		   req_extension_count);
	if (!extensions_supported)
	{
		invalid = true;
	}

	/// SURFACE FORMAT & PRESENT MODE
	{
		uint32 format_count;
		uint32 present_mode_count;

		VulkanAPI.vkGetPhysicalDeviceSurfaceFormatsKHR(device, RendererState.surface, &format_count, NULL);
		VulkanAPI.vkGetPhysicalDeviceSurfacePresentModesKHR(device, RendererState.surface, &present_mode_count, NULL);

		if (format_count == 0 || present_mode_count == 0)
		{
			invalid = true;
		}
	}

	/// TRAITS

	if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		score += 1000;
	}

	score += properties.limits.maxImageDimension2D;

	if (!features.geometryShader)
	{
		invalid = true;
	}

	if (invalid)
	{
		score = 0;
	}

	return score;
}

RENDERER_EXPORT RENDERER_ACQUIRE_PHYSICAL_DEVICE_FUNCTION(VulkanAcquirePhysicalDevice)
{
	Assert(RendererState.instance && RendererState.surface);

	bool succeeded = false;

	VkPhysicalDevice chosen_device = VK_NULL_HANDLE;
	VkPhysicalDevice* physical_devices;
	uint32 device_count;
	VkResult result;

	result = VulkanAPI.vkEnumeratePhysicalDevices(RendererState.instance, &device_count, NULL);

	if (result != VK_SUCCESS)
	{
		LOG_ERROR("VulkanAcquirePhysicalDevice failed. Reason: the function 'vkEnumeratePhysicalDevices' returned an erroneous value, when"
				  "querying for physical device count");
	}

	else
	{
		physical_devices = PushArray(Memory, VkPhysicalDevice, device_count);

		result = VulkanAPI.vkEnumeratePhysicalDevices(RendererState.instance, &device_count, physical_devices);

		if (result != VK_SUCCESS)
		{
			switch(result)
			{
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					LOG_ERROR("VulkanAcquirePhysicalDevice failed. Reason: the host is out of memory,\nVK_ERROR_OUT_OF_HOST_MEMORY");
				break;

				case VK_ERROR_OUT_OF_DEVICE_MEMORY:
					LOG_ERROR("VulkanAcquirePhysicalDevice failed. Reason: the device is out of memory,\nVK_ERROR_OUT_OF_DEVICE_MEMORY");
				break;

				case VK_ERROR_INITIALIZATION_FAILED:
					LOG_ERROR("VulkanAcquirePhysicalDevice failed. Reason: initialization failed,\nVK_ERROR_INITIALIZATION_FAILED");
				break;

				default:
					Assert(false);
				break;
			}
		}

		else
		{
			uint32 best_score = 0;
			for (uint32 i = 0; i < device_count; ++i)
			{
				uint32 device_suitability = VulkanRatePhysicalDeviceSuitability(physical_devices[i], req_extension_names,
																				req_extension_count);

				if (best_score < device_suitability)
				{
					best_score = device_suitability;
					chosen_device = physical_devices[i];
				}
			}

			if (chosen_device != VK_NULL_HANDLE)
			{
				RendererState.physical_device = chosen_device;
				succeeded = true;
			}

			else
			{
				LOG_ERROR("VulkanAcquirePhysicalDevice failed. Reason: no Vulkan compliant device was found");
			}
		}
	}

	return succeeded;
}

internal void
VulkanAcquirePhysicalDeviceQueueProperties()
{
	RendererState.graphics_family = -1;
	RendererState.compute_family  = -1;
	RendererState.transfer_family = -1;
	RendererState.present_family  = -1;

	uint32 queue_family_count;
	VulkanAPI.vkGetPhysicalDeviceQueueFamilyProperties(RendererState.physical_device, &queue_family_count, NULL);

	VkQueueFamilyProperties* queue_family_properties = PushArray(Memory, VkQueueFamilyProperties, queue_family_count);

	if (!queue_family_properties)
	{
		LOG_ERROR("VulkanAcquireDeviceQueueProperties failed. Reason: failed to allocate memory for the queue properties");
	}

	else
	{
		VulkanAPI.vkGetPhysicalDeviceQueueFamilyProperties(RendererState.physical_device, &queue_family_count, queue_family_properties);

		for (uint32 i = 0; i < queue_family_count; ++i)
		{
			if (queue_family_properties[i].queueCount != 0)
			{
				if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT
					&& RendererState.graphics_family == -1)
				{
					RendererState.graphics_family = (int8) i;
				}

				if (RendererState.present_family == -1)
				{
					VkBool32 queue_family_supports_presentation = VK_FALSE;
					VulkanAPI.vkGetPhysicalDeviceSurfaceSupportKHR(RendererState.physical_device, i,
																   RendererState.surface, &queue_family_supports_presentation);

					if (queue_family_supports_presentation == VK_TRUE)
					{
						RendererState.present_family = (int8) i;
					}
				}

				if (queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
				{
					RendererState.compute_family = (int8) i;
				}

				if (queue_family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
				{
					RendererState.transfer_family = (int8) i;
				}
			}
		}

		Assert(RendererState.graphics_family != -1 && RendererState.present_family != -1);

		// TODO(soimn): evaluate these
		if (RendererState.compute_family == RendererState.graphics_family
			|| (RendererState.compute_family == RendererState.present_family
				&& queue_family_properties[RendererState.compute_family].queueCount < 2))
		{
			RendererState.supports_compute = false;
			RendererState.compute_family = -1;
		}

		else if ((RendererState.compute_family == RendererState.present_family
					&& RendererState.compute_family == RendererState.transfer_family)
				 && queue_family_properties[RendererState.compute_family].queueCount < 3)
		{
			RendererState.supports_compute = false;
			RendererState.compute_family = -1;

			RendererState.supports_transfer = true;
		}

		else
		{
			RendererState.supports_compute  = true;
			RendererState.supports_transfer = true;
		}
	}
}

RENDERER_EXPORT RENDERER_CREATE_LOGICAL_DEVICE_FUNCTION(VulkanCreateLogicalDevice)
{
	bool succeeded = false;
	bool additional_info = (bool)(additional_create_info);

	VulkanAcquirePhysicalDeviceQueueProperties();

	// NOTE(soimn): graphics, present, compute, transfer
	const float priorities[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	const char** extensions;
	uint32 extension_count;

	if (additional_info)
	{
		memcpy((void*) priorities, (void*) additional_create_info->priorities, 4);

		extension_count = MAX(additional_create_info->extension_count, 1);
		extensions = PushArray(Memory, const char*, extension_count);

		extensions[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
		memcpy((void*) (extensions + 1), (void*) additional_create_info->extensions, additional_create_info->extension_count);
	}

	else
	{
		extensions		= PushStruct(Memory, const char*);
		*extensions		= VK_KHR_SWAPCHAIN_EXTENSION_NAME;
		extension_count = 1;
	}

	VkDeviceQueueCreateInfo queue_create_info[4] = {};
	uint32 queue_create_info_count = 0;
	uint32 present_family_index = 0;
	uint32 compute_family_index = 0;

	queue_create_info[queue_create_info_count].sType			= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info[queue_create_info_count].pNext			= NULL;
	queue_create_info[queue_create_info_count].flags			= 0;
	queue_create_info[queue_create_info_count].queueFamilyIndex = RendererState.graphics_family;
	queue_create_info[queue_create_info_count].queueCount		= 1;
	queue_create_info[queue_create_info_count].pQueuePriorities	= &priorities[0];
	++queue_create_info_count;

	if (RendererState.present_family == RendererState.graphics_family)
	{
		present_family_index = queue_create_info_count;
		++queue_create_info[queue_create_info_count].queueCount;
	}

	else
	{
		present_family_index = queue_create_info_count;
		queue_create_info[present_family_index].sType			= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info[present_family_index].pNext			= NULL;
		queue_create_info[present_family_index].flags			= 0;
		queue_create_info[present_family_index].queueFamilyIndex = RendererState.present_family;
		queue_create_info[present_family_index].queueCount		= 1;
		queue_create_info[present_family_index].pQueuePriorities	= &priorities[1];
		++queue_create_info_count;
	}

	if (RendererState.supports_compute)
	{
		if (RendererState.compute_family == RendererState.present_family)
		{
			++queue_create_info[present_family_index].queueCount;
		}

		else
		{
			compute_family_index = queue_create_info_count;
			queue_create_info[compute_family_index].sType			= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info[compute_family_index].pNext			= NULL;
			queue_create_info[compute_family_index].flags			= 0;
			queue_create_info[compute_family_index].queueFamilyIndex = RendererState.compute_family;
			queue_create_info[compute_family_index].queueCount		= 1;
			queue_create_info[compute_family_index].pQueuePriorities	= &priorities[2];
			++queue_create_info_count;
		}
	}

	if (RendererState.supports_transfer)
	{
		if (RendererState.transfer_family == RendererState.present_family)
		{
			++queue_create_info[present_family_index].queueCount;
		}

		else if (RendererState.transfer_family == RendererState.compute_family)
		{
			++queue_create_info[compute_family_index].queueCount;
		}

		else
		{
			queue_create_info[queue_create_info_count].sType			= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info[queue_create_info_count].pNext			= NULL;
			queue_create_info[queue_create_info_count].flags			= 0;
			queue_create_info[queue_create_info_count].queueFamilyIndex = RendererState.transfer_family;
			queue_create_info[queue_create_info_count].queueCount		= 1;
			queue_create_info[queue_create_info_count].pQueuePriorities	= &priorities[3];
			++queue_create_info_count;
		}
	}

	VkDeviceCreateInfo device_create_info = {};
	device_create_info.sType				   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.pNext				   = (additional_info ? additional_create_info->extension_ptr : NULL);
	device_create_info.flags				   = (additional_info ? additional_create_info->create_flags  : 0);
	device_create_info.queueCreateInfoCount	   = queue_create_info_count;
	device_create_info.pQueueCreateInfos	   = &queue_create_info[0];
	device_create_info.enabledExtensionCount   = extension_count;
	device_create_info.ppEnabledExtensionNames = extensions;
	device_create_info.pEnabledFeatures		   = (additional_info ? additional_create_info->features		: NULL);

	VkResult result = VulkanAPI.vkCreateDevice(RendererState.physical_device, &device_create_info, NULL, &RendererState.device);
	if (result != VK_SUCCESS)
	{
		switch(result)
		{
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				LOG_ERROR("VulkanCreateLogicalDevice failed. Reason: the host is out of memory,\nVK_ERROR_OUT_OF_HOST_MEMORY");
			break;

			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				LOG_ERROR("VulkanCreateLogicalDevice failed. Reason: the device is out of memory,\nVK_ERROR_OUT_OF_DEVICE_MEMORY");
			break;

			case VK_ERROR_INITIALIZATION_FAILED:
				LOG_ERROR("VulkanCreateLogicalDevice failed. Reason: the initialization of the logical device failed,\nVK_ERROR_INITIALIZATION_FAILED");
			break;

			case VK_ERROR_FEATURE_NOT_PRESENT:
				LOG_ERROR("VulkanCreateLogicalDevice failed. Reason: the features requested are unavailable,\nVK_ERROR_FEATURE_NOT_PRESENT");
			break;

			case VK_ERROR_TOO_MANY_OBJECTS:
				LOG_ERROR("VulkanCreateLogicalDevice failed. Reason: too may objects created,\nVK_ERROR_TOO_MANY_OBJECTS");
			break;

			case VK_ERROR_DEVICE_LOST:
				LOG_ERROR("VulkanCreateLogicalDevice failed. Reason: the device was lost,\nVK_ERROR_DEVICE_LOST");
			break;

			default:
				Assert(false);
			break;
		}
	}

	else
	{
		succeeded = VulkanLoadDeviceLevelFunctions();
		
		if (succeeded)
		{
			VulkanAPI.vkGetDeviceQueue(RendererState.device, RendererState.graphics_family, 0, &RendererState.graphics_queue);

			uint32 present_family_queue_index = 0;
			VulkanAPI.vkGetDeviceQueue(RendererState.device, RendererState.present_family, present_family_queue_index++, &RendererState.present_queue);

			if (RendererState.supports_compute)
			{
				if (RendererState.compute_family == RendererState.present_family)
				{
					VulkanAPI.vkGetDeviceQueue(RendererState.device, RendererState.present_family, present_family_queue_index++, &RendererState.compute_queue);
				}

				else
				{
					VulkanAPI.vkGetDeviceQueue(RendererState.device, RendererState.compute_family, 0, &RendererState.compute_queue);
				}
			}

			if (RendererState.supports_transfer)
			{
				if (RendererState.transfer_family == RendererState.present_family)
				{
					VulkanAPI.vkGetDeviceQueue(RendererState.device, RendererState.present_family, present_family_queue_index++, &RendererState.transfer_queue);
				}

				else if (RendererState.transfer_family == RendererState.compute_family)
				{
					VulkanAPI.vkGetDeviceQueue(RendererState.device, RendererState.compute_family, 1, &RendererState.transfer_queue);
				}

				else
				{
					VulkanAPI.vkGetDeviceQueue(RendererState.device, RendererState.transfer_family, 0, &RendererState.transfer_queue);
				}
			}
		}
	}

	return succeeded;
}

#ifdef PLATFORM_WINDOWS
RENDERER_EXPORT RENDERER_CREATE_SURFACE_FUNCTION(VulkanCreateWin32Surface)
{
	bool succeeded = false;

	VkWin32SurfaceCreateInfoKHR create_info = {};
	create_info.sType	  = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	create_info.pNext	  = extension_ptr;
	create_info.flags	  = create_flags;
	create_info.hinstance = process_instance;
	create_info.hwnd	  = window_handle;

	VkResult result = VulkanAPI.vkCreateWin32SurfaceKHR(RendererState.instance, &create_info, NULL, &RendererState.surface);
	if (result != VK_SUCCESS)
	{
		switch(result)
		{
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				LOG_ERROR("VulkanCreateWin32Surface failed. Reason: the host is out of memory,\nVK_ERROR_OUT_OF_HOST_MEMORY");
			break;

			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				LOG_ERROR("VulkanCreateWin32Surface failed. Reason: the device is out of memory,\nVK_ERROR_OUT_OF_DEVICE_MEMORY");
			break;

			default:
				Assert(false);
			break;
		}
	}

	else
	{
		succeeded = true;
	}

	return succeeded;
}

internal bool
Win32LoadVulkan()
{
	bool succeeded = false;

	RendererState.vulkan_module = LoadLibraryA("vulkan-1.dll");

	if (RendererState.vulkan_module != INVALID_HANDLE_VALUE)
	{
		VulkanAPI.vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) GetProcAddress((HMODULE) RendererState.vulkan_module, "vkGetInstanceProcAddr");

		if (VulkanAPI.vkGetInstanceProcAddr)
		{
			VulkanAPI.vkCreateInstance = 
				(PFN_vkCreateInstance) VulkanAPI.vkGetInstanceProcAddr(NULL, "vkCreateInstance");

			VulkanAPI.vkEnumerateInstanceExtensionProperties = 
				(PFN_vkEnumerateInstanceExtensionProperties) VulkanAPI.vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceExtensionProperties");

			VulkanAPI.vkEnumerateInstanceLayerProperties = 
				(PFN_vkEnumerateInstanceLayerProperties) VulkanAPI.vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceLayerProperties");

			if (VulkanAPI.vkCreateInstance
				&& VulkanAPI.vkEnumerateInstanceExtensionProperties
				&& VulkanAPI.vkEnumerateInstanceLayerProperties)
			{
				succeeded = true;
			}

			else
			{
				LOG_ERROR("Could not load Vulkan. Reason: the functions vkCreateInstance,"
						  "vkEnumerateInstanceExtensionProperties and vkEnumerateInstanceLayerProperties were not found");
			}
		}

		else
		{
			LOG_ERROR("Could not load Vulkan. Reason: the vkGetInstanceProcAddr function was not found");
		}
	}

	else
	{
		LOG_ERROR("Could not load Vulkan. Reason: the Vulkan Runtime Library was not found");
	}

	return succeeded;
}

RENDERER_EXPORT RENDERER_INIT_FUNCTION(Win32InitRenderer)
{
	bool succeeded = false;

	Assert(callback);
	ErrorCallback = callback;

	bool vulkan_loaded = Win32LoadVulkan();


	if (!vulkan_loaded)
	{
		LOG_ERROR("Win32InitVulkan failed to load Vulkan");
	}

	else
	{
		if (!arena)
		{
			LOG_ERROR("Win32InitVulkan failed to initialize renderer. Reason: the pointer to the provided memory arena is NULL");
		}

		else
		{
			Memory = arena;

			if (!Memory->memory || Memory->remaining_space < RENDERER_MINIMUM_REQUIRED_MEMORY)
			{
				LOG_ERROR("Win32InitVulkan failed. Reason: the provided memory arena has a size which is"
						  " below th RENDERER_MINIMUM_REQUIRED_MEMORY size");
			}

			else
			{
				state = &RendererState;
				api = &VulkanAPI;
				succeeded = true;
			}
		}
	}

	return succeeded;
}
#endif

RENDERER_EXPORT RENDERER_GET_OPTIMAL_SWAPCHAIN_EXTENT_FUNCTION(VulkanGetOptimalSwapchainExtent)
{
	bool succeeded = false;

	VkSurfaceCapabilitiesKHR capabilities;
	VkResult result = VulkanAPI.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(RendererState.physical_device, RendererState.surface,
																		  &capabilities);
	if (result != VK_SUCCESS)
	{
		switch(result)
		{
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				LOG_ERROR("VulkanGetOptimalSwapchainExtent failed. Reason: the host is out of memory,\nVK_ERROR_OUT_OF_HOST_MEMORY");
			break;

			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				LOG_ERROR("VulkanGetOptimalSwapchainExtent failed. Reason: the device is out of memory,\nVK_ERROR_OUT_OF_DEVICE_MEMORY");
			break;

			case VK_ERROR_SURFACE_LOST_KHR:
				LOG_ERROR("VulkanGetOptimalSwapchainExtent failed. Reason: the surface was lost,\nVK_ERROR_SURFACE_LOST_KHR");
			break;

			default:
				Assert(false);
			break;
		}
	}

	else
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			*extent = capabilities.currentExtent;
			succeeded = true;
		}

		else
		{
			*extent = {CLAMP(capabilities.minImageExtent.width, RENDERER_DEFAULT_SWAPCHAIN_WIDTH, capabilities.maxImageExtent.width),
					   CLAMP(capabilities.minImageExtent.height, RENDERER_DEFAULT_SWAPCHAIN_HEIGHT, capabilities.maxImageExtent.height)};
			succeeded = true;
		}
	}

	return succeeded;
}

RENDERER_EXPORT RENDERER_GET_OPTIMAL_SWAPCHAIN_SURFACE_FORMAT_FUNCTION(VulkanGetOptimalSwapchainSurfaceFormat)
{
	bool succeeded = false;

	uint32 format_count;
	VkSurfaceFormatKHR* formats;

	VkResult result = VulkanAPI.vkGetPhysicalDeviceSurfaceFormatsKHR(RendererState.physical_device, RendererState.surface,
																	 &format_count, NULL);

	if (result != VK_SUCCESS)
	{
		LOG_ERROR("VulkanGetOptimalSwapchainSurfaceFormat failed. Reason: the call to 'vkGetPhysicalDeviceSurfaceFormatsKHR' failed when querying"
				  "for the surface format count");
	}

	else
	{
		formats = PushArray(Memory, VkSurfaceFormatKHR, format_count);
		Assert(formats && format_count);

		result = VulkanAPI.vkGetPhysicalDeviceSurfaceFormatsKHR(RendererState.physical_device, RendererState.surface,
																&format_count, formats);

		if (result != VK_SUCCESS)
		{
			switch(result)
			{
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					LOG_ERROR("VulkanGetOptimalSwapchainSurfaceFormat failed. Reason: the host is out of memory,\nVK_ERROR_OUT_OF_HOST_MEMORY");
				break;

				case VK_ERROR_OUT_OF_DEVICE_MEMORY:
					LOG_ERROR("VulkanGetOptimalSwapchainSurfaceFormat failed. Reason: the device is out of memory,\nVK_ERROR_OUT_OF_DEVICE_MEMORY");
				break;

				case VK_ERROR_SURFACE_LOST_KHR:
					LOG_ERROR("VulkanGetOptimalSwapchainSurfaceFormat failed. Reason: the surface was lost,\nVK_ERROR_SURFACE_LOST_KHR");
				break;

				default:
					Assert(false);
				break;
			}
		}

		else
		{
			if (format_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
			{
				*format = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
				succeeded = true;
			}

			else
			{
				for (uint32 i = 0; i < format_count; ++i)
				{
					if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
					{
						*format = formats[i];
						succeeded = true;
						break;
					}
				}
			}

			if (!succeeded)
			{
				// NOTE(soimn): this might not be optimal, consider providing a ranked list instead
				*format = formats[0];
				succeeded = true;
			}
		}
	}

	return succeeded;
}

RENDERER_EXPORT RENDERER_GET_OPTIMAL_SWAPCHAIN_PRESENT_MODE_FUNCTION(VulkanGetOptimalSwapchainPresentMode)
{
	bool succeeded = false;

	uint32 present_mode_count;
	VkPresentModeKHR* present_modes;

	VkResult result = VulkanAPI.vkGetPhysicalDeviceSurfacePresentModesKHR(RendererState.physical_device, RendererState.surface,
																		  &present_mode_count, NULL);

	if (result != VK_SUCCESS)
	{
		LOG_ERROR("VulkanGetOptimalSwapchainPresentModes failed. Reason: the call to 'vkGetPhysicalDevicePresentModesKHR' failed when querying"
				  "for the physical devices' present modes");
	}

	else
	{
		present_modes = PushArray(Memory, VkPresentModeKHR, present_mode_count);
		Assert(present_modes);

		result = VulkanAPI.vkGetPhysicalDeviceSurfacePresentModesKHR(RendererState.physical_device, RendererState.surface,
																	 &present_mode_count, present_modes);

		if (result != VK_SUCCESS)
		{
			switch(result)
			{
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					LOG_ERROR("VulkanGetOptimalSwapchainPresentMode failed. Reason: the host is out of memory,\nVK_ERROR_OUT_OF_HOST_MEMORY");
				break;

				case VK_ERROR_OUT_OF_DEVICE_MEMORY:
					LOG_ERROR("VulkanGetOptimalSwapchainPresentMode failed. Reason: the device is out of memory,\nVK_ERROR_OUT_OF_DEVICE_MEMORY");
				break;

				case VK_ERROR_SURFACE_LOST_KHR:
					LOG_ERROR("VulkanGetOptimalSwapchainPresentMode failed. Reason: the surface was lost,\nVK_ERROR_SURFACE_LOST_KHR");
				break;

				default:
					Assert(false);
				break;
			}
		}

		else
		{
			for (uint32 i = 0; i < present_mode_count; ++i)
			{
				if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					*present_mode = present_modes[i];
					succeeded = true;
					break;
				}

				else if (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					*present_mode = present_modes[i];
					succeeded = true;
					break;
				}
			}
		}
	}

	return succeeded;
}

RENDERER_EXPORT RENDERER_CREATE_SWAPCHAIN_FUNCTION(VulkanCreateSwapchain)
{
	bool succeeded = false;

	VkExtent2D default_extent				  = {};
	VkSurfaceFormatKHR default_surface_format = {};
	VkPresentModeKHR default_present_mode	  = {};

	bool successfully_acquired_defaults = false;
	successfully_acquired_defaults = VulkanGetOptimalSwapchainExtent(&default_extent)				 || successfully_acquired_defaults;
	successfully_acquired_defaults = VulkanGetOptimalSwapchainSurfaceFormat(&default_surface_format) || successfully_acquired_defaults;
	successfully_acquired_defaults = VulkanGetOptimalSwapchainPresentMode(&default_present_mode)	 || successfully_acquired_defaults;

	if (successfully_acquired_defaults)
	{
		VkSwapchainCreateInfoKHR create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

		if (swapchain_create_info)
		{
			create_info.pNext				   = swapchain_create_info->extension_ptr;
			create_info.flags				   = swapchain_create_info->creation_flags;

			create_info.minImageCount		   = (swapchain_create_info->image_count == 0
												  ? RENDERER_DEFAULT_SWAPCHAIN_IMAGE_COUNT
												  : swapchain_create_info->image_count);

			create_info.imageFormat			   = (swapchain_create_info->format == 0x0
												  ? default_surface_format.format
												  : swapchain_create_info->format);

			create_info.imageColorSpace		   = (swapchain_create_info->color_space == 0x0
												  ? default_surface_format.colorSpace
												  : swapchain_create_info->color_space);

			create_info.imageExtent			   = ((swapchain_create_info->extent.width == 0 || swapchain_create_info->extent.height == 0)
												  ? default_extent
												  : swapchain_create_info->extent);

			create_info.imageUsage			   = (swapchain_create_info->image_usage_flags == 0x0
												  ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
												  : swapchain_create_info->image_usage_flags);

			create_info.presentMode			   = (swapchain_create_info->use_different_present_mode
												  ? swapchain_create_info->present_mode
												  : default_present_mode);

			create_info.imageSharingMode	   = (swapchain_create_info->enable_image_sharing
												  ? VK_SHARING_MODE_CONCURRENT 
												  : VK_SHARING_MODE_EXCLUSIVE);

			create_info.queueFamilyIndexCount  = swapchain_create_info->whitelisted_family_count;
			create_info.pQueueFamilyIndices    = swapchain_create_info->whitelisted_families;
			create_info.clipped				   = (swapchain_create_info->disable_clipping ? VK_FALSE : VK_TRUE);
			create_info.surface				   = RendererState.surface;

			create_info.imageArrayLayers	   = (swapchain_create_info->enable_stereoscopic_3d
												  ? swapchain_create_info->image_view_count
												  : 1);

			create_info.preTransform		   = (swapchain_create_info->use_transform
												  ? swapchain_create_info->transform_flags
												  : VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR);

			create_info.compositeAlpha		   = (swapchain_create_info->use_compositing
												  ? swapchain_create_info->compositing_flags
												  : VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);

			create_info.oldSwapchain		   = (swapchain_create_info->reuse_swapchain
												  ? swapchain_create_info->old_swapchain
												  : NULL);
		}

		else
		{
			create_info.minImageCount	 = RENDERER_DEFAULT_SWAPCHAIN_IMAGE_COUNT;
			create_info.imageFormat		 = default_surface_format.format;
			create_info.imageColorSpace  = default_surface_format.colorSpace;
			create_info.imageExtent		 = default_extent;
			create_info.imageUsage		 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			create_info.presentMode		 = default_present_mode;
			create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			create_info.clipped			 = VK_TRUE;
			create_info.surface			 = RendererState.surface;
			create_info.imageArrayLayers = 1;
			create_info.preTransform	 = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			create_info.compositeAlpha	 = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		}

		RendererState.swapchain.format = create_info.imageFormat;
		RendererState.swapchain.extent = create_info.imageExtent;

		VkResult result = VulkanAPI.vkCreateSwapchainKHR(RendererState.device, &create_info,
														 NULL, &RendererState.swapchain.handle);

		succeeded = (RendererState.swapchain.handle != VK_NULL_HANDLE);

		if (result != VK_SUCCESS)
		{
			switch(result)
			{
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					LOG_ERROR("VulkanCreateSwapchain failed. Reason: the host is out of memory,\nVK_ERROR_OUT_OF_HOST_MEMORY");
				break;

				case VK_ERROR_OUT_OF_DEVICE_MEMORY:
					LOG_ERROR("VulkanCreateSwapchain failed. Reason: the host is out of memory,\nVK_ERROR_OUT_OF_DEVICE_MEMORY");
				break;

				case VK_ERROR_DEVICE_LOST:
					LOG_ERROR("VulkanCreateSwapchain failed. Reason: the device was lost,\nVK_ERROR_DEVICE_LOST");
				break;

				case VK_ERROR_SURFACE_LOST_KHR:
					LOG_ERROR("VulkanCreateSwapchain failed. Reason: the surface was lost,\nVK_ERROR_SURFACE_LOST_KHR");
				break;

				case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
					LOG_ERROR("VulkanCreateSwapchain failed. Reason: the window is already in use,\nVK_ERROR_NATIVE_WINDOW_IN_USE_KHR");
				break;

				default:
					Assert(false);
				break;
			}
		}
	}

	return succeeded;
}

RENDERER_EXPORT RENDERER_CREATE_SWAPCHAIN_IMAGES_FUNCTION(VulkanCreateSwapchainImages)
{
	bool succeeded = false;

	VkResult result;
	result = VulkanAPI.vkGetSwapchainImagesKHR(RendererState.device, RendererState.swapchain.handle,
											   &RendererState.swapchain.image_count, NULL);

	if (result != VK_SUCCESS)
	{
		LOG_ERROR("VulkanCreateSwapchainImages failed. Reason: the call to 'vkGetSwapchainImagesKHR' failed when querying for swapchain image count");
	}

	else
	{
		RendererState.swapchain.images = PushArray(Memory, VkImage, RendererState.swapchain.image_count);
		RendererState.swapchain.image_views = PushArray(Memory, VkImageView, RendererState.swapchain.image_count);

		result = VulkanAPI.vkGetSwapchainImagesKHR(RendererState.device, RendererState.swapchain.handle,
												   &RendererState.swapchain.image_count, RendererState.swapchain.images);

		if(result != VK_SUCCESS)
		{
			switch(result)
			{
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					LOG_ERROR("VulkanCreateSwapchainImages failed. Reason: the host is out of memory,\nVK_ERROR_OUT_OF_HOST_MEMORY");
				break;

				case VK_ERROR_OUT_OF_DEVICE_MEMORY:
					LOG_ERROR("VulkanCreateSwapchainImages failed. Reason: the device is out of memory,\nVK_ERROR_OUT_OF_DEVICE_MEMORY");
				break;
				
				default:
					Assert(false);
				break;
			}
		}

		else
		{
			uint32 i = 0;
			for (; i < RendererState.swapchain.image_count; ++i)
			{
				VkImageViewCreateInfo create_info = {};
				create_info.sType	   = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				create_info.pNext	   = (image_create_info ? image_create_info->extension_ptr : NULL);
				create_info.flags	   = (image_create_info ? image_create_info->create_flags  : 0);
				create_info.image	   = RendererState.swapchain.images[i];

				create_info.viewType   = ((image_create_info && image_create_info->unordinary_view_type)
										  ? image_create_info->view_type 
										  : VK_IMAGE_VIEW_TYPE_2D);

				create_info.format	   = RendererState.swapchain.format;

				VkComponentMapping default_component_mapping = {{VK_COMPONENT_SWIZZLE_IDENTITY},
																{VK_COMPONENT_SWIZZLE_IDENTITY},
																{VK_COMPONENT_SWIZZLE_IDENTITY},
																{VK_COMPONENT_SWIZZLE_IDENTITY}};

				create_info.components = ((image_create_info && image_create_info->use_different_mapping)
										  ? image_create_info->component_mapping
										  : default_component_mapping);

				VkImageSubresourceRange default_subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

				create_info.subresourceRange = ((image_create_info && image_create_info->use_different_subresource_range)
												? image_create_info->subresource_range
												: default_subresource_range);

				result = VulkanAPI.vkCreateImageView(RendererState.device, &create_info, NULL, &RendererState.swapchain.image_views[i]);
				
				if (result != VK_SUCCESS)
				{
					switch(result)
					{
						case VK_ERROR_OUT_OF_HOST_MEMORY:
							LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
						break;

						case VK_ERROR_OUT_OF_DEVICE_MEMORY:
							LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
						break;

						default:
							Assert(false);
						break;
					}

					break;
				}
			}

			if (i == RendererState.swapchain.image_count)
				succeeded = true;
		}
	}

	return succeeded;
}

RENDERER_EXPORT RENDERER_CREATE_RENDER_PASS_FUNCTION(VulkanCreateRenderPass)
{
	VkRenderPass render_pass = VK_NULL_HANDLE;

	if (!render_pass_create_info.use_default_attachment
			&& (render_pass_create_info.attachments == NULL || render_pass_create_info.attachment_count == 0))
	{
		LOG_ERROR("VulkanCreateRenderPass failed. Reason: the use_default_attachment flag was not set and no attachments were provided");
	}
	
	else if (!render_pass_create_info.use_default_subpass
				&& (render_pass_create_info.subpasses == NULL || render_pass_create_info.subpass_count == 0))
	{
		LOG_ERROR("VulkanCreateRenderPass failed. Reason: the use_default_subpass flag was not set and no subpasses were provided");
	}

	else
	{
		VkAttachmentDescription default_attachment = {};
		default_attachment.flags = 0;
		default_attachment.format = RendererState.swapchain.format;
		default_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		default_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		default_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		default_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		default_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		default_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		default_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference default_attachment_reference = {};
		default_attachment_reference.attachment = 0;
		default_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription default_subpass = {};
		default_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		default_subpass.colorAttachmentCount = 1;
		default_subpass.pColorAttachments = &default_attachment_reference;

		VkRenderPassCreateInfo create_info = {};
		create_info.sType			= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		create_info.pNext			= render_pass_create_info.extension_ptr;
		create_info.flags			= render_pass_create_info.create_flags;
		create_info.attachmentCount = (render_pass_create_info.use_default_attachment ? 1                   : render_pass_create_info.attachment_count);
		create_info.pAttachments	= (render_pass_create_info.use_default_attachment ? &default_attachment	: render_pass_create_info.attachments);
		create_info.subpassCount	= (render_pass_create_info.use_default_subpass    ? 1					: render_pass_create_info.subpass_count);
		create_info.pSubpasses		= (render_pass_create_info.use_default_subpass	  ? &default_subpass	: render_pass_create_info.subpasses);
		create_info.dependencyCount = render_pass_create_info.subpass_dependency_count;
		create_info.pDependencies	= render_pass_create_info.subpass_dependencies;

		VkResult result = VulkanAPI.vkCreateRenderPass(RendererState.device, &create_info, NULL, &render_pass);
		if (result != VK_SUCCESS)
		{
			switch (result)
			{
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					LOG_ERROR("VulkanCreateRenderPass failed. Reason: the host is out of memory,\nVK_ERROR_OUT_OF_HOST_MEMORY");
				break;
				
				case VK_ERROR_OUT_OF_DEVICE_MEMORY:
					LOG_ERROR("VulkanCreateRenderPass failes. Reason: the device is out of memory,\nVK_ERROR_OUT_OF_DEVICE_MEMORY");
				break;

				default:
					Assert(false);
				break;
			}
		}
	}

	return render_pass;
}

RENDERER_EXPORT RENDERER_CREATE_PIPELINE_LAYOUT_FUNCTION(VulkanCreatePipelineLayout)
{
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
	VkResult result;

	VkPipelineLayoutCreateInfo pipeline_create_info = {};
	pipeline_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_create_info.setLayoutCount = descriptor_count;
	pipeline_create_info.pSetLayouts = descriptors;
	pipeline_create_info.pushConstantRangeCount = push_constant_range_count;
	pipeline_create_info.pPushConstantRanges = push_constant_ranges;

	result = VulkanAPI.vkCreatePipelineLayout(RendererState.device, &pipeline_create_info, NULL, &pipeline_layout);
	if (result != VK_SUCCESS)
	{
		switch (result)
		{
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				LOG_ERROR("VulkanCreatePipelineLayout failed. Reason: the host is out of memory,\nVK_ERROR_OUT_OF_HOST_MEMORY");
			break;
			
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				LOG_ERROR("VulkanCreatePipelineLayout failed. Reason: the device is out of memory,\nVK_ERROR_OUT_OF_DEVICE_MEMORY");
			break;

			default:
				Assert(false);
			break;
		}
	}

	return pipeline_layout;
}

RENDERER_EXPORT RENDERER_CREATE_SHADER_MODULE_FUNCTION(VulkanCreateShaderModule)
{
	VkShaderModule module = VK_NULL_HANDLE;

	if (shader_file == NULL)
	{
		LOG_ERROR("VulkanCreateShaderModule failed. Reason: the shader file handle provided is NULL");
	}

	else
	{
		VkShaderModuleCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.pNext = extension_ptr;
		create_info.flags = create_flags;
		create_info.codeSize = shader_file_size;
		create_info.pCode = shader_file;

		VkResult result = VulkanAPI.vkCreateShaderModule(RendererState.device, &create_info, NULL, &module);
		if (result != VK_SUCCESS)
		{
			switch(result)
			{
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					LOG_ERROR("VulkanCreateShaderModule failed. Reason: the host is out of memory,\nVK_ERROR_OUT_OF_HOST_MEMORY");
				break;
				
				case VK_ERROR_OUT_OF_DEVICE_MEMORY:
					LOG_ERROR("VulkanCreateShaderModule failed. Reason: the device is out of memory,\nVK_ERROR_OUT_OF_DEVICE_MEMORY");
				break;
				
				case VK_ERROR_INVALID_SHADER_NV:
					LOG_ERROR("VulkanCreateShaderModule failed. Reason: the shader code provided failed to compile and/or link,\n"
							  "VK_ERROR_INVALID_SHADER_NV");
				break;

				default:
					Assert(false);
				break;
			}
		}
	}

	return module;
}

RENDERER_EXPORT RENDERER_CREATE_SHADER_STAGE_INFOS_FUNCTION(VulkanCreateShaderStageInfos)
{
	bool succeeded = false;

	bool has_vertex_shader	 = (stage_create_info.vertex_shader	  != VK_NULL_HANDLE);
	bool has_fragment_shader = (stage_create_info.fragment_shader != VK_NULL_HANDLE);
	bool has_geometry_shader = (stage_create_info.geometry_shader != VK_NULL_HANDLE);

	uint32 required_size = ((uint8) has_vertex_shader
							+ (uint8) has_fragment_shader
							+ (uint8) has_geometry_shader);
	
	if (required_size <= shader_stage_info_array_size)
	{
		uint32 shader_stage_count = 0;
		if (has_vertex_shader)
		{
			shader_stage_info_array[shader_stage_count].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_stage_info_array[shader_stage_count].stage  = VK_SHADER_STAGE_VERTEX_BIT;
			shader_stage_info_array[shader_stage_count].module = stage_create_info.vertex_shader;
			shader_stage_info_array[shader_stage_count].pName  = 
				(stage_create_info.vertex_main_function_name ? stage_create_info.vertex_main_function_name : "main");

			shader_stage_info_array[shader_stage_count].pSpecializationInfo = stage_create_info.vertex_specialization_info;
			++shader_stage_count;
		}

		if (has_fragment_shader)
		{
			shader_stage_info_array[shader_stage_count].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_stage_info_array[shader_stage_count].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
			shader_stage_info_array[shader_stage_count].module = stage_create_info.fragment_shader;
			shader_stage_info_array[shader_stage_count].pName  = 
				(stage_create_info.fragment_main_function_name ? stage_create_info.fragment_main_function_name : "main");

			shader_stage_info_array[shader_stage_count].pSpecializationInfo = stage_create_info.fragment_specialization_info;
			++shader_stage_count;
		}

		if (has_geometry_shader)
		{
			shader_stage_info_array[shader_stage_count].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_stage_info_array[shader_stage_count].stage  = VK_SHADER_STAGE_GEOMETRY_BIT;
			shader_stage_info_array[shader_stage_count].module = stage_create_info.geometry_shader;
			shader_stage_info_array[shader_stage_count].pName  = 
				(stage_create_info.geometry_main_function_name ? stage_create_info.geometry_main_function_name : "main");

			shader_stage_info_array[shader_stage_count].pSpecializationInfo = stage_create_info.geometry_specialization_info;
			++shader_stage_count;
		}

		succeeded = true;
	}

	else
	{
		LOG_ERROR("VulkanCreateShaderStageInfos failed. Reason: the size of the array passed is of an inadequate size");
	}

	return succeeded;
}

RENDERER_EXPORT RENDERER_CREATE_GRAPHICS_PIPELINES_FUNCTION(VulkanCreateGraphicsPipelines)
{
	bool succeeded = false;
	VkResult result;

	if (pipeline_array_count < gfx_pipeline_info.pipeline_count)
	{
		LOG_ERROR("VulkanCreateGraphicsPipelines failed. Reason: the pipeline array size and the requested pipeline count are a mismatch");
	}

	else
	{
		VkGraphicsPipelineCreateInfo* graphics_pipeline_create_info = PushArray(Memory, VkGraphicsPipelineCreateInfo, gfx_pipeline_info.pipeline_count);
		Assert(graphics_pipeline_create_info);

		VkPipelineVertexInputStateCreateInfo default_vertex_input_state = {};
		default_vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		VkPipelineInputAssemblyStateCreateInfo default_input_assembly_state = {};
		default_input_assembly_state.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		default_input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkViewport default_viewport = {0.0f, 0.0f,
									   (float) RendererState.swapchain.extent.width,
									   (float) RendererState.swapchain.extent.height,
									   0.0f, 1.0f};

		VkRect2D default_scissor = {};
		default_scissor.offset = {0, 0};
		default_scissor.extent = RendererState.swapchain.extent;

		VkPipelineViewportStateCreateInfo default_viewport_state = {};
		default_viewport_state.sType		 = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		default_viewport_state.viewportCount = 1;
		default_viewport_state.pViewports	 = &default_viewport;
		default_viewport_state.scissorCount  = 1;
		default_viewport_state.pScissors	 = &default_scissor;

		VkPipelineRasterizationStateCreateInfo default_rasterization_state = {};
		default_rasterization_state.sType					= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		default_rasterization_state.polygonMode				= VK_POLYGON_MODE_FILL;
		default_rasterization_state.cullMode				= VK_CULL_MODE_BACK_BIT;
		default_rasterization_state.frontFace				= VK_FRONT_FACE_CLOCKWISE;
		default_rasterization_state.lineWidth				= 1.0f;

		VkPipelineColorBlendAttachmentState default_color_blend_attachment = {};
		default_color_blend_attachment.colorWriteMask	   =  VK_COLOR_COMPONENT_R_BIT
															| VK_COLOR_COMPONENT_G_BIT
															| VK_COLOR_COMPONENT_B_BIT
															| VK_COLOR_COMPONENT_A_BIT;
		default_color_blend_attachment.blendEnable		   = VK_FALSE;
		default_color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		default_color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		default_color_blend_attachment.colorBlendOp		   = VK_BLEND_OP_ADD;
		default_color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		default_color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		default_color_blend_attachment.alphaBlendOp		   = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo default_color_blend_state = {};
		default_color_blend_state.sType			  = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		default_color_blend_state.logicOpEnable	  = VK_FALSE;
		default_color_blend_state.logicOp		  = VK_LOGIC_OP_COPY;
		default_color_blend_state.attachmentCount = 1;
		default_color_blend_state.pAttachments	  = &default_color_blend_attachment;

		VkPipelineMultisampleStateCreateInfo default_multisample_state = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, NULL,
																		  0, VK_SAMPLE_COUNT_1_BIT,
																		  0, 1.0f};

		for (uint32 i = 0; i < gfx_pipeline_info.pipeline_count; ++i)
		{
			VkGraphicsPipelineCreateInfo& pipeline_info = graphics_pipeline_create_info[i];

			pipeline_info.sType	= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipeline_info.pNext	= (gfx_pipeline_info.extension_ptr ? gfx_pipeline_info.extension_ptr[i] : NULL);
			pipeline_info.flags	= (gfx_pipeline_info.create_flags  ? gfx_pipeline_info.create_flags[i]  : NULL);

			pipeline_info.stageCount = (gfx_pipeline_info.use_same_shader_stage_count
										? gfx_pipeline_info.shader_stage_count[0] 
										: gfx_pipeline_info.shader_stage_count[i]);

			if (gfx_pipeline_info.use_default_fixed_functions)
			{
				pipeline_info.pStages = (gfx_pipeline_info.use_pointer_arrays
										 ? ((VkPipelineShaderStageCreateInfo**) gfx_pipeline_info.shader_stage_info)[i]
										 : &gfx_pipeline_info.shader_stage_info[i]);

				pipeline_info.pVertexInputState	  = &default_vertex_input_state;
				pipeline_info.pInputAssemblyState = &default_input_assembly_state;
				pipeline_info.pViewportState	  = &default_viewport_state;
				pipeline_info.pRasterizationState = &default_rasterization_state;
				pipeline_info.pColorBlendState	  = &default_color_blend_state;
				pipeline_info.pMultisampleState = &default_multisample_state;
			}

			else
			{
				if (gfx_pipeline_info.use_pointer_arrays)
				{
					pipeline_info.pStages			  = ((VkPipelineShaderStageCreateInfo**) gfx_pipeline_info.shader_stage_info)[i];

					pipeline_info.pVertexInputState	  = (gfx_pipeline_info.use_default_vertex_input_state
														 ? &default_vertex_input_state
														 : ((VkPipelineVertexInputStateCreateInfo**)   gfx_pipeline_info.vertex_input_state)[i]);

					pipeline_info.pInputAssemblyState = (gfx_pipeline_info.use_default_input_assembly_state
														 ? &default_input_assembly_state
														 : ((VkPipelineInputAssemblyStateCreateInfo**) gfx_pipeline_info.input_assembly_state)[i]);

					pipeline_info.pViewportState	  = (gfx_pipeline_info.use_default_viewport_state
														 ? &default_viewport_state
														 : ((VkPipelineViewportStateCreateInfo**)	   gfx_pipeline_info.viewport_state)[i]);

					pipeline_info.pRasterizationState = (gfx_pipeline_info.use_default_rasterization_state
														 ? &default_rasterization_state
														 : ((VkPipelineRasterizationStateCreateInfo**) gfx_pipeline_info.rasterization_state)[i]);

					pipeline_info.pDepthStencilState  = (gfx_pipeline_info.use_default_depth_stencil_state
														 ? NULL
														 : ((VkPipelineDepthStencilStateCreateInfo**)  gfx_pipeline_info.depth_stencil_state)[i]);

					pipeline_info.pColorBlendState	  = (gfx_pipeline_info.use_default_color_blend_state
														 ? &default_color_blend_state
														 : ((VkPipelineColorBlendStateCreateInfo**)	   gfx_pipeline_info.color_blend_state)[i]);

					pipeline_info.pTessellationState  = (gfx_pipeline_info.tessellation_state
														 ? ((VkPipelineTessellationStateCreateInfo**)  gfx_pipeline_info.tessellation_state)[i]
														 : NULL);

					pipeline_info.pDynamicState		  = (gfx_pipeline_info.dynamic_state
														 ? ((VkPipelineDynamicStateCreateInfo**)	   gfx_pipeline_info.dynamic_state)[i]
														 : NULL);

					pipeline_info.pMultisampleState	  = (gfx_pipeline_info.use_default_multisample_state
														 ? &default_multisample_state
														 : ((VkPipelineMultisampleStateCreateInfo**)   gfx_pipeline_info.multisampling_state)[i]);
				}

				else
				{
					pipeline_info.pStages = &gfx_pipeline_info.shader_stage_info[i];

					pipeline_info.pVertexInputState	  = (gfx_pipeline_info.use_default_vertex_input_state
														 ? &default_vertex_input_state
														 : &gfx_pipeline_info.vertex_input_state[i]);

					pipeline_info.pInputAssemblyState = (gfx_pipeline_info.use_default_input_assembly_state
														 ? &default_input_assembly_state
														 : &gfx_pipeline_info.input_assembly_state[i]);

					pipeline_info.pViewportState	  = (gfx_pipeline_info.use_default_viewport_state
														 ? &default_viewport_state
														 : &gfx_pipeline_info.viewport_state[i]);

					pipeline_info.pRasterizationState = (gfx_pipeline_info.use_default_rasterization_state
														 ? &default_rasterization_state
														 : &gfx_pipeline_info.rasterization_state[i]);

					pipeline_info.pDepthStencilState  = (gfx_pipeline_info.use_default_depth_stencil_state
														 ? NULL
														 : &gfx_pipeline_info.depth_stencil_state[i]);

					pipeline_info.pColorBlendState	  = (gfx_pipeline_info.use_default_color_blend_state
														 ? &default_color_blend_state
														 : &gfx_pipeline_info.color_blend_state[i]);

					pipeline_info.pTessellationState  = (gfx_pipeline_info.tessellation_state
														 ? &gfx_pipeline_info.tessellation_state[i]
														 : NULL);

					pipeline_info.pDynamicState		  = (gfx_pipeline_info.dynamic_state
														 ? &gfx_pipeline_info.dynamic_state[i]
														 : NULL);

					pipeline_info.pMultisampleState	  = (gfx_pipeline_info.use_default_multisample_state
														 ? &default_multisample_state
														 : &gfx_pipeline_info.multisampling_state[i]);
				}
			}

			pipeline_info.layout     = (gfx_pipeline_info.use_single_pipeline_layout
										? gfx_pipeline_info.pipeline_layout[0]
										: gfx_pipeline_info.pipeline_layout[i]);

			pipeline_info.renderPass = (gfx_pipeline_info.use_multiple_render_passes
										? gfx_pipeline_info.render_pass[i]
										: gfx_pipeline_info.render_pass[0]);

			pipeline_info.subpass			 = (gfx_pipeline_info.subpass_index			  ? gfx_pipeline_info.subpass_index[i]        : NULL);
			pipeline_info.basePipelineHandle = (gfx_pipeline_info.base_pipeline_handle    ? gfx_pipeline_info.base_pipeline_handle[i] : NULL);
			pipeline_info.basePipelineIndex	 = (gfx_pipeline_info.use_base_pipeline_index ? gfx_pipeline_info.base_pipeline_index[i]  : -1);
		}

		result = VulkanAPI.vkCreateGraphicsPipelines(RendererState.device, VK_NULL_HANDLE,
													 gfx_pipeline_info.pipeline_count, graphics_pipeline_create_info,
													 NULL, pipeline_array);
		if (result != VK_SUCCESS)
		{
			switch (result)
			{
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					LOG_ERROR("VulkanCreateGraphicsPipelines failed. Reason: not enough host memory,\nVK_ERROR_OUT_OF_HOST_MEMORY");
				break;
				
				case VK_ERROR_OUT_OF_DEVICE_MEMORY:
					LOG_ERROR("VulkanCreateGraphicsPipelines failed. Reason: not enough graphics device memory,\nVK_ERROR_OUT_OF_DEVICE_MEMORY");
				break;
				
				case VK_ERROR_INVALID_SHADER_NV:
					LOG_ERROR("VulkanCreateGraphicsPipelines failed. Reason: invalid shaders passed through create info,\nVK_ERROR_INVALID_SHADER_NV");
				break;

				default:
					Assert(false);
				break;
			}
		}

		else
		{
			succeeded = true;
		}
	}

	return succeeded;
}

RENDERER_EXPORT RENDERER_CREATE_FRAMEBUFFER_FUNCTION(VulkanCreateFramebuffer)
{
	VkFramebuffer framebuffer = VK_NULL_HANDLE;

	VkFramebufferCreateInfo framebuffer_create_info = {};
	framebuffer_create_info.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_create_info.pNext			= buffer_create_info.extension_ptr;
	framebuffer_create_info.flags			= buffer_create_info.create_flags;
	framebuffer_create_info.renderPass		= render_pass;
	framebuffer_create_info.attachmentCount = buffer_create_info.attachment_count;
	framebuffer_create_info.pAttachments	= buffer_create_info.attachments;

	framebuffer_create_info.width = (buffer_create_info.use_width_and_height
									 ? buffer_create_info.width
									 : buffer_create_info.extent.width);

	framebuffer_create_info.height = (buffer_create_info.use_width_and_height
									  ? buffer_create_info.height
									  : buffer_create_info.extent.height);

	framebuffer_create_info.layers = buffer_create_info.layers;

	VkResult result = VulkanAPI.vkCreateFramebuffer(RendererState.device, &framebuffer_create_info, NULL, &framebuffer);
	if (result != VK_SUCCESS)
	{
		switch (result)
		{
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				LOG_ERROR("VulkanCreateFramebuffer failed. Reason: the host is out of memory,\nVK_ERROR_OUT_OF_HOST_MEMORY");
			break;
			
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				LOG_ERROR("VulkanCreateFramebuffer failed. Reason: the device is out of memory,\nVK_ERROR_OUT_OF_DEVICE_MEMORY");
			break;
			
			case VK_ERROR_INVALID_SHADER_NV:
				LOG_ERROR("VulkanCreateFramebuffer failed. Reason: the passed shaders are invalid,\nVK_ERROR_INVALID_SHADER_NV");
			break;

			default:
				Assert(false);
			break;
		}
	}

	return framebuffer;
}

RENDERER_EXPORT RENDERER_CREATE_COMMAND_POOL_FUNCTION(VulkanCreateCommandPool)
{
	VkCommandPool command_pool = VK_NULL_HANDLE;

	VkCommandPoolCreateInfo pool_create_info = {};
	pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_create_info.pNext = extension_ptr;
	pool_create_info.flags = create_flags;
	pool_create_info.queueFamilyIndex = queue_family;

	VkResult result = VulkanAPI.vkCreateCommandPool(RendererState.device, &pool_create_info, NULL, &command_pool);
	if (result != VK_SUCCESS)
	{
		switch (result)
		{
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				LOG_ERROR("VulkanCreateCommandPool failed. Reason: the host is out of memory,\nVK_ERROR_OUT_OF_HOST_MEMORY");
			break;
			
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				LOG_ERROR("VulkanCreateCommandPool failed. Reason: the device is out of memory,\nVK_ERROR_OUT_OF_DEVICE_MEMORY");
			break;
			
			case VK_ERROR_INVALID_SHADER_NV:
				LOG_ERROR("VulkanCreateCommandPool failed. Reason: the passed shaders are invalid,\nVK_ERROR_INVALID_SHADER_NV");
			break;

			default:
				Assert(false);
			break;
		}
	}

	return command_pool;
}

RENDERER_EXPORT RENDERER_CREATE_SEMAPHORE_FUNCTION(VulkanCreateSemaphore)
{
	VkSemaphore semaphore = VK_NULL_HANDLE;

	VkSemaphoreCreateInfo semaphore_create_info = {};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphore_create_info.pNext = extension_ptr;
	semaphore_create_info.flags = create_flags;

	VkResult result = VulkanAPI.vkCreateSemaphore(RendererState.device, &semaphore_create_info, NULL, &semaphore);
	if (result != VK_SUCCESS)
	{
		switch(result)
		{
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				LOG_ERROR("VulkanCreateSemaphore failed. Reason: the host is out of memory,\nVK_ERROR_OUT_OF_DEVICE_MEMORY");
			break;

			case VK_ERROR_OUT_OF_HOST_MEMORY:
				LOG_ERROR("VulkanCreateSemaphore failed. Reason: the device is out of memory,\nVK_ERROR_OUT_OF_HOST_MEMORY");
			break;

			default:
				Assert(false);
			break;
		}
	}

	return semaphore;
}
