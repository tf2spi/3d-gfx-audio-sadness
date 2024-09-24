/*
 * I wanted to still follow the Khronos tutorial but I wanted
 * to use SDL2 instead of GLFW, so I followed this tutorial from
 * Tizen in order to set up Vulkan with SDL2.
 * https://docs.tizen.org/application/native/guides/graphics/vulkan/
 * 
 * After that, I then followed Khronos's tutorial for all other setup.
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

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

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
		|| !(vkExtensions = malloc(vkExtensionsCount * sizeof(*vkExtensions)))
		|| !SDL_Vulkan_GetInstanceExtensions(window, &vkExtensionsCount, vkExtensions))
	{
		eprintf("Failed to get Vulkan extensions!\n");
		return 1;
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
		.pNext = 0,
		.pApplicationInfo = &vkaInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = 0,
		.enabledExtensionCount = vkExtensionsCount,
		.ppEnabledExtensionNames = vkExtensions,
	};
	VkInstance vkInstance = 0;
	if (VK_SUCCESS != vkCreateInstance(&vkicInfo, NULL, &vkInstance))
	{
		eprintf("Failed to create Vulkan instance!\n");
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
		.enabledExtensionCount = 0,
		.ppEnabledExtensionNames = 0,
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
	(void)vkDevice;

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
	return 0;
}
