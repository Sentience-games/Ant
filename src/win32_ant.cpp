#include "win32_ant.h"

/*
 *	GLOBAL *cough* VARIABLES
 */

// TODO(soimn): delegate responsibility, to the game, to confirm the quit message, in case saving is due or the game needs to halt the player
global_variable bool running = false;

// TODO(soimn): consider moving / changing this
global_variable const char* application_name = "APP";
global_variable uint32 application_version = 0;

[[maybe_unused]]
internal void
Win32Log(const char* message)
{
	// TODO(soimn): consider checking the return value of GetStdHandle
	local_persist HANDLE win32_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
	local_persist char buffer[256 + 1];

	int length = strlength(message, 256);
	Assert(length <= 256);

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

	if (layer_properties == NULL)
	{
		WIN32LOG_ERROR("Failed to allocate memory for layer properties in VulkanValidateLayerSupport");
	}

	else
	{

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
	}

	return foundAllLayers;
}

internal bool
VulkanValidateExtensionSupport(HANDLE vulkanInitHeap, const char** req_extensions, uint32 req_extension_count)
{
	uint32 extension_count;
	VkResult result;
	bool foundAllExtensions = true;

	result = vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
	Assert(!result);

	VkExtensionProperties*
	extension_properties = (VkExtensionProperties*) HeapAlloc(vulkanInitHeap,
															  HEAP_ZERO_MEMORY,
															  extension_count * sizeof(VkExtensionProperties));
	Assert(!extension_properties);

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
		for (char** req_extension = (char**)req_extensions;
			 req_extension < req_extensions + req_extension_count;
			 ++req_extension)
		{
			bool foundExtension = false;

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
					const char* error_message_prefix = "VulkanValidateExtensionSupport failed. \n\n"
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

				foundAllExtensions = false;
				break;
			}
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

internal VkPhysicalDevice
VulkanGetPhysicalDevice(HANDLE vulkanInitHeap, VkInstance instance)
{
	VkPhysicalDevice device = VK_NULL_HANDLE;
	
	uint32 device_count;
	VkResult result;

	result = vkEnumeratePhysicalDevices(instance, &device_count, NULL);
	Assert(!result);

	VkPhysicalDevice* physical_devices = (VkPhysicalDevice*)HeapAlloc(vulkanInitHeap, HEAP_ZERO_MEMORY,
																	   device_count * sizeof(VkPhysicalDevice));
	if (physical_devices == NULL)
	{
		WIN32LOG_ERROR("Failed to allocate memory for the list of devices in VulkanGetPhysicalDevice");
	}

	else
	{
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
				// TODO(soimn): find most capable device instead of returning the first available
				device = physical_devices[0];
			}

			else
			{
				WIN32LOG_ERROR("VulkanGetPhysicalDevice found no vulkan compatible devices");
			}
		}
	}

	return device;
}

struct vulkan_application
{
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;

	// TODO(soimn): consider dynamically allocating instead
	const char* layers[16];
	const char* extensions[16];

	uint32 layer_count;
	uint32 extension_count;
};

internal bool
Win32InitVulkan(vulkan_application* application, const char* app_name, uint32 app_version)
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
		application->extension_count = 0;
		application->layer_count = 0;

		application->extensions[application->extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
		application->extensions[application->extension_count++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;

		#ifdef ANT_VULKAN_ENABLE_VALIDATION_LAYERS
		application->extensions[application->extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
		application->layers[application->layer_count++] = "VK_LAYER_LUNARG_standard_validation";
		#endif

		bool extensions_supported = VulkanValidateExtensionSupport(vulkanInitHeap, application->extensions, application->extension_count);
		bool layers_supported     = VulkanValidateLayerSupport(vulkanInitHeap, application->layers, application->layer_count);

		if (layers_supported && extensions_supported)
		{
			application->instance = VulkanCreateInstance(app_name, app_version,
														 application->layers, application->layer_count,
														 application->extensions, application->extension_count);

			if (application->instance != VK_NULL_HANDLE)
			{
				application->physical_device = VulkanGetPhysicalDevice(vulkanInitHeap, application->instance);

				// TODO(soimn): setup debug message callback

				succeeded = true;
			}
		}

		HeapDestroy(vulkanInitHeap);
	}

	return succeeded;
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

			memory.transient_size = GIGABYTES(2);
			memory.transient_memory = VirtualAlloc(NULL, memory.transient_size,
													MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			if (memory.persistent_memory && memory.transient_memory)
			{

				// Raw input
				bool input_ready = Win32InitRawInput();

				// TODO(soimn): consider moving this to persistent memory
				// Vulkan
				vulkan_application application;
				bool vulkan_ready = Win32InitVulkan(&application, application_name, application_version);

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
