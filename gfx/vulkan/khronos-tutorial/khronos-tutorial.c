/*
 * I wanted to still follow the Khronos tutorial but I wanted
 * to use SDL2 instead of GLFW, so I followed this tutorial from
 * Tizen in order to set up Vulkan with SDL2.
 * https://docs.tizen.org/application/native/guides/graphics/vulkan/
 * 
 * After that, I then followed Khronos's tutorial for all other setup.
 * https://docs.vulkan.org/tutorial/latest/00_Introduction.html
 */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <Windows.h>
#include <stdint.h>

#define WIDTH 640
#define HEIGHT 480
#define APP_SHORT_NAME "KhrTut"

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

static uint32_t clampu32(uint32_t x, uint32_t l, uint32_t h)
{
	return x < l ? l : (x > h ? h : x);
}
static uint32_t minu32(uint32_t x, uint32_t y)
{
	return x < y ? x : y;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vkDbgCb(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT flags,
	const VkDebugUtilsMessengerCallbackDataEXT *data,
	void *userdata)
{
	(void)userdata;
	eprintf("Super-Epic Validation Layer: %08x,%08x,%s\n", severity, flags, data->pMessage);
	return VK_FALSE;
}
static VkDebugUtilsMessengerCreateInfoEXT vkdumcInfo =
{
	.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
	.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
	.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
	| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
	| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
	.pfnUserCallback = vkDbgCb,
	.pUserData = 0,
};

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	// Debug variables
	const char *vkicLayers[] = {
		"VK_LAYER_KHRONOS_validation",
	};
	const char *vkExtensionsExtra[] = {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	};
	VkDebugUtilsMessengerCreateInfoEXT *pVkDumcInfo = &vkdumcInfo;

	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		eprintf("SDL not initialized! %s\n", SDL_GetError());
		return 1;
	}

	SDL_Window *window = SDL_CreateWindow(
		"Khronos Tutorial",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		WIDTH,
		HEIGHT,
		SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
	if (window == NULL)
	{
		eprintf("Window could not be created! %s", SDL_GetError());
		return 1;
	}

	const char **vkExtensions = 0;
	uint32_t vkExtensionsCount = 0;
	if (!SDL_Vulkan_GetInstanceExtensions(window, &vkExtensionsCount, vkExtensions)
		|| !(vkExtensions = malloc((vkExtensionsCount + ARRAYSIZE(vkExtensionsExtra)) * sizeof(*vkExtensions)))
		|| !SDL_Vulkan_GetInstanceExtensions(window, &vkExtensionsCount, vkExtensions))
	{
		eprintf("Failed to get Vulkan extensions!\n");
		return 1;
	}
	for (uint32_t i = 0; i < ARRAYSIZE(vkExtensionsExtra); i++)
	{
		vkExtensions[vkExtensionsCount++] = vkExtensionsExtra[i];
	}
	eprintf("Vulkan Extensions:\n");
	for (uint32_t i = 0; i < vkExtensionsCount; i++)
	{
		eprintf("\t%s\n", vkExtensions[i]);
	}
	VkApplicationInfo vkaInfo =
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = NULL,
		.pApplicationName = APP_SHORT_NAME,
		.applicationVersion = 1337,
		.pEngineName = APP_SHORT_NAME,
		.engineVersion = 1337,
		.apiVersion = VK_API_VERSION_1_0,
	};
	VkInstanceCreateInfo vkicInfo =
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = pVkDumcInfo,
		.pApplicationInfo = &vkaInfo,
		.enabledLayerCount = ARRAYSIZE(vkicLayers),
		.ppEnabledLayerNames = vkicLayers,
		.enabledExtensionCount = vkExtensionsCount,
		.ppEnabledExtensionNames = vkExtensions,
	};
	VkInstance vkInstance = 0;
	if (VK_SUCCESS != vkCreateInstance(&vkicInfo, NULL, &vkInstance))
	{
		eprintf("Failed to create Vulkan instance!\n");
		return 1;
	}
	VkDebugUtilsMessengerEXT vkDebugMessenger = 0;
	PFN_vkCreateDebugUtilsMessengerEXT vkcdumProcAddr = (PFN_vkCreateDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT");
	if (!vkcdumProcAddr || VK_SUCCESS != vkcdumProcAddr(vkInstance, pVkDumcInfo, 0, &vkDebugMessenger))
	{
		eprintf("Failed to create debug messenger!\n");
		return 1;
	}

	VkPhysicalDevice *vkPhysicalDevices = 0;
	uint32_t vkPhysicalDevicesCount = 0;
	if (VK_SUCCESS != vkEnumeratePhysicalDevices(vkInstance, &vkPhysicalDevicesCount, vkPhysicalDevices)
		|| !(vkPhysicalDevices = malloc(vkPhysicalDevicesCount * sizeof(*vkPhysicalDevices)))
		|| VK_SUCCESS != vkEnumeratePhysicalDevices(vkInstance, &vkPhysicalDevicesCount, vkPhysicalDevices))
	{
		eprintf("Failed to enumerate physical devices!\n");
		return 1;
	}
	if (vkPhysicalDevicesCount == 0)
	{
		eprintf("No physical devices. Cringe...\n");
		return 1;
	}
	VkPhysicalDevice vkPhysDevice = vkPhysicalDevices[0];

	VkExtensionProperties *vkDeviceExtensions = 0;
	uint32_t vkDeviceExtensionsCount = 0;
	if (VK_SUCCESS != vkEnumerateDeviceExtensionProperties(vkPhysDevice, 0, &vkDeviceExtensionsCount, vkDeviceExtensions)
		|| !(vkDeviceExtensions = malloc(vkDeviceExtensionsCount * sizeof(*vkDeviceExtensions)))
		|| VK_SUCCESS != vkEnumerateDeviceExtensionProperties(vkPhysDevice, 0, &vkDeviceExtensionsCount, vkDeviceExtensions))
	{
		eprintf("Failed to enumerate device extension properties!\n");
		return 1;
	}

	eprintf("Device extension names:\n");
	for (uint32_t i = 0; i < vkDeviceExtensionsCount; i++)
	{
		eprintf("\t%s\n", vkDeviceExtensions[i].extensionName);
	}

	VkQueueFamilyProperties *vkQueueProps = 0;
	uint32_t vkQueueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vkPhysDevice, &vkQueueCount, vkQueueProps);
	if (!(vkQueueProps = malloc(vkQueueCount * sizeof(*vkQueueProps))))
	{
		eprintf("Failed to get physical device queue family properties!\n");
		return 1;
	}
	vkGetPhysicalDeviceQueueFamilyProperties(vkPhysDevice, &vkQueueCount, vkQueueProps);

	VkPhysicalDeviceFeatures vkPhysFeatures;
	vkGetPhysicalDeviceFeatures(vkPhysDevice, &vkPhysFeatures);
	uint32_t vkQueueNodeIndex = UINT32_MAX;
	for (uint32_t i = 0; i < vkQueueCount; i++) {
		if (vkQueueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			vkQueueNodeIndex = i;
	}
	if (vkQueueNodeIndex == UINT32_MAX) {
		eprintf("Failed to get a graphics queue. Sadge...\n");
		return 1;
	}

	const float vkQueuePriorities[1] = { 0.0f };
	const char *vkdcEnabledExtensions[] =
	{
		"VK_KHR_swapchain",
	};
	const VkDeviceQueueCreateInfo vkdqcInfo[1] =
	{
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.pNext = 0,
			.queueFamilyIndex = vkQueueNodeIndex,
			.queueCount = ARRAYSIZE(vkQueuePriorities),
			.pQueuePriorities = vkQueuePriorities,
		}
	};
	VkDeviceCreateInfo vkdcInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = 0,
		.queueCreateInfoCount = ARRAYSIZE(vkdqcInfo),
		.pQueueCreateInfos = vkdqcInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = 0,
		.enabledExtensionCount = ARRAYSIZE(vkdcEnabledExtensions),
		.ppEnabledExtensionNames = vkdcEnabledExtensions,
		.pEnabledFeatures = 0,
	};
	VkDevice vkDevice = 0;
	if (VK_SUCCESS != vkCreateDevice(vkPhysDevice, &vkdcInfo, 0, &vkDevice))
	{
		eprintf("Failed to create logical device!\n");
		return 1;
	}

	VkSurfaceKHR vkSurface;
	if (!SDL_Vulkan_CreateSurface(window, vkInstance, &vkSurface))
	{
		eprintf("Failed to create Vulkan surface!\n");
		return 1;
	}
	uint32_t vkSurfaceSupported;
	if (VK_SUCCESS != vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysDevice, vkQueueNodeIndex, vkSurface, &vkSurfaceSupported)
		|| !vkSurfaceSupported)
	{
		eprintf("Failed to confirm that physical device supports surface!\n");
		return 1;
	}

	VkSurfaceCapabilitiesKHR vkSurfaceCaps;
	if (VK_SUCCESS != vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysDevice, vkSurface, &vkSurfaceCaps))
	{
		eprintf("Failed to get physical surface capabilities!\n");
		return 1;
	}

	VkSurfaceFormatKHR *vkFormats = 0;
	uint32_t vkFormatsCount = 0;
	if (VK_SUCCESS != vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysDevice, vkSurface, &vkFormatsCount, vkFormats)
		|| !(vkFormats = malloc(vkFormatsCount * sizeof(*vkFormats)))
		|| VK_SUCCESS != vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysDevice, vkSurface, &vkFormatsCount, vkFormats))
	{
		eprintf("Failed to get surface formats!\n");
		return 1;
	}

	VkPresentModeKHR *vkPresentModes = 0;
	uint32_t vkPresentModesCount = 0;
	if (VK_SUCCESS != vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysDevice, vkSurface, &vkPresentModesCount, vkPresentModes)
		|| !(vkPresentModes = malloc(vkPresentModesCount * sizeof(*vkPresentModes)))
		|| VK_SUCCESS != vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysDevice, vkSurface, &vkPresentModesCount, vkPresentModes))
	{
		eprintf("Failed to get presentation modes!\n");
		return 1;
	}

	eprintf("VK Surface Formats:\n");
	VkSurfaceFormatKHR *vkFormatDesired = &vkFormats[0];
	for (uint32_t i = 0; i < vkFormatsCount; i++)
	{
		VkSurfaceFormatKHR *vkFormatCurrent = &vkFormats[i];
		eprintf("Format,Colorspace = %u,%u\n", vkFormatCurrent->format, vkFormatCurrent->colorSpace);
		if ((VK_FORMAT_B8G8R8_SNORM == vkFormatCurrent->format
			|| VK_FORMAT_B8G8R8A8_UNORM == vkFormatCurrent->format)
			&& VK_COLOR_SPACE_SRGB_NONLINEAR_KHR == vkFormatCurrent->colorSpace)
		{
			eprintf("I have found my desired colorspace!\n");
			vkFormatDesired = vkFormatCurrent;
		}
	}
	eprintf("VK desired format: %u,%u\n", vkFormatDesired->format, vkFormatDesired->colorSpace);

	VkPresentModeKHR vkPresentModeDesired = VK_PRESENT_MODE_FIFO_KHR;
	eprintf("VK presentation modes\n");
	for (uint32_t i = 0; i < vkPresentModesCount; i++)
	{
		VkPresentModeKHR vkPresentModeCurrent = vkPresentModes[i];
		eprintf("\t%d\n", vkPresentModeCurrent);
		if (VK_PRESENT_MODE_MAILBOX_KHR == vkPresentModeCurrent)
			eprintf("Found Mailbox! Don't care!\n");
	}
	eprintf("VK desired presentation mode: %d\n", vkPresentModeDesired);

	uint32_t extentWidth = vkSurfaceCaps.currentExtent.width;
	uint32_t extentHeight = vkSurfaceCaps.currentExtent.height;
	if (extentWidth == UINT32_MAX || extentHeight == UINT32_MAX)
	{
		int w, h;
		SDL_GetWindowSize(window, &w, &h);
		extentWidth = clampu32(w, vkSurfaceCaps.minImageExtent.width, vkSurfaceCaps.maxImageExtent.width);
		extentHeight = clampu32(w, vkSurfaceCaps.minImageExtent.height, vkSurfaceCaps.maxImageExtent.height);
	}
	vkSurfaceCaps.currentExtent.width = extentWidth;
	vkSurfaceCaps.currentExtent.height = extentHeight;

	VkExtent2D vkExtentDesired = vkSurfaceCaps.currentExtent;
	uint32_t imageCount = minu32(vkSurfaceCaps.minImageCount + 1, vkSurfaceCaps.maxImageCount);
	VkSwapchainCreateInfoKHR vkscInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = vkSurface,
		.minImageCount = imageCount,
		.imageFormat = vkFormatDesired->format,
		.imageColorSpace = vkFormatDesired->colorSpace,
		.imageExtent = vkExtentDesired,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		/*
		 * NOTE:
		 * We differ from the Khronos tutorial right now by having
		 * the graphics queue family equal to the presentation queue family.
		 * If we couldn't find such a queue, we would set the fields
		 * like this instead...
		 * 
		 * .imageSharingMode = VK_SHARING_MODE_CONCURRENT,
		 * .queueFamilyIndexCount = 2,
		 * .pQueueFamilyIndices = (uint32_t[2]){grFamily,prFamily},
		 */
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = 0,
		.preTransform = vkSurfaceCaps.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = vkPresentModeDesired,
		.clipped = VK_TRUE,
		.oldSwapchain = 0,
	};
	VkSwapchainKHR vkSwapChain;
	if (VK_SUCCESS != vkCreateSwapchainKHR(vkDevice, &vkscInfo, 0, &vkSwapChain))
	{
		eprintf("Failed to create the swapchain!\n");
		return 1;
	}
	eprintf("Finally created the swap chain! (My god...)\n");

	SDL_Event e;
	bool quit = false;
	while (!quit)
	{
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			}
		}
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	// This is only one of the many things we need to clean up.
	// This is here now only to test that debug messenger flags this.
	vkDestroyInstance(vkInstance, 0);
	return 0;
}
