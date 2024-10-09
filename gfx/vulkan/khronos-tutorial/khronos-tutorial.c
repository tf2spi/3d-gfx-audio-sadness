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
#include <stdint.h>

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a) / sizeof(*a))
#endif

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

static void *readfile(const char *fname, size_t *len_p) {
	FILE *fp = fopen(fname, "rb");
	if (fp == NULL)
		return NULL;

	size_t lenmax = UINT16_MAX;
	void *mem = malloc(lenmax);

	size_t i = 0;
	int err = -1;
	if (mem != NULL) {
		while (i < lenmax) {
			size_t lenread = fread(mem, 1, lenmax - i, fp);
			if (lenread == 0)
				break;
			i += lenread;
		}
		err = ferror(fp);
		fclose(fp);
	}
	if (err != 0 || i == lenmax) {
		free(mem);
		mem = NULL;
	}
	*len_p = i;
	return mem;
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
	VkResult err;
	if (VK_SUCCESS != (err = vkCreateInstance(&vkicInfo, NULL, &vkInstance)))
	{
		eprintf("Failed to create Vulkan instance! %d\n", err);
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
	// I fucking hate Vulkan so much
	if (vkSurfaceCaps.maxImageCount == 0)
		vkSurfaceCaps.maxImageCount = vkSurfaceCaps.minImageCount;
	uint32_t imageCount = minu32(vkSurfaceCaps.minImageCount + 1, vkSurfaceCaps.maxImageCount);
	eprintf("Min,Count,Max Image Count=%u,%u,%u\n", vkSurfaceCaps.minImageCount, imageCount, vkSurfaceCaps.maxImageCount);
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
	(void)vkExtentDesired;
	VkSwapchainKHR vkSwapchain;
	if (VK_SUCCESS != vkCreateSwapchainKHR(vkDevice, &vkscInfo, 0, &vkSwapchain))
	{
		eprintf("Failed to create the swapchain!\n");
		return 1;
	}

	VkImage *vkSwapchainImages = 0;
	uint32_t vkSwapchainImagesCount = 0;
	VkImageView *vkSwapchainImageViews = 0;

	if (VK_SUCCESS != vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &vkSwapchainImagesCount, vkSwapchainImages)
		|| !(vkSwapchainImages = malloc(vkSwapchainImagesCount * sizeof(*vkSwapchainImages)))
		|| !(vkSwapchainImageViews = calloc(sizeof(*vkSwapchainImageViews), vkSwapchainImagesCount))
		|| VK_SUCCESS != vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &vkSwapchainImagesCount, vkSwapchainImages))
	{
		eprintf("Failed to get swapchain images!\n");
		return 1;
	}
	if (vkSwapchainImagesCount == 0)
	{
		eprintf("No swapchain images? Excuse me?");
		return 1;
	}
	
	for (uint32_t i = 0; i < vkSwapchainImagesCount; i++)
	{
		VkImageViewCreateInfo vkivcInfo =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = vkSwapchainImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = vkFormatDesired->format,
			.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.subresourceRange.baseMipLevel = 0,
			.subresourceRange.levelCount = 1,
			.subresourceRange.baseArrayLayer = 0,
			.subresourceRange.layerCount = 1,
		};

		if (VK_SUCCESS != vkCreateImageView(vkDevice, &vkivcInfo, 0, &vkSwapchainImageViews[i]))
		{
			eprintf("Failed to create image views!\n");
			return 1;
		}
	}
	for (uint32_t i = 0; i < vkSwapchainImagesCount; i++)
	{
		eprintf("Swapchain Image, View: %p, %p\n", vkSwapchainImages[i], vkSwapchainImageViews[i]);
	}
	eprintf("Finally created the swap chain + views! (My god...)\n");


	VkPipelineLayoutCreateInfo vkplcInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 0,
		.pSetLayouts = 0,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = 0,
	};

	VkPipelineLayout vkPipelineLayout;
	if (VK_SUCCESS != vkCreatePipelineLayout(vkDevice, &vkplcInfo, 0, &vkPipelineLayout))
	{
		eprintf("Pipeline layout creation failed!\n");
		return 1;
	}

	VkAttachmentDescription vkAttachmentDescriptions[] =
	{
		{
			.format = vkFormatDesired->format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		},
	};
	VkAttachmentReference vkAttachmentReferences[] =
	{
		{
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		},
	};
	VkSubpassDescription vkSubpassDescriptions[] =
	{
		{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = ARRAYSIZE(vkAttachmentReferences),
			.pColorAttachments = vkAttachmentReferences,
		}
	};
	VkSubpassDependency subpassDependencies[] =
	{
		{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		}
	};
	VkRenderPassCreateInfo vkrpcInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = ARRAYSIZE(vkAttachmentDescriptions),
		.pAttachments = vkAttachmentDescriptions,
		.subpassCount = ARRAYSIZE(vkSubpassDescriptions),
		.pSubpasses = vkSubpassDescriptions,
		.dependencyCount = ARRAYSIZE(subpassDependencies),
		.pDependencies = subpassDependencies,
	};
	VkRenderPass vkRenderPass;
	if (VK_SUCCESS != vkCreateRenderPass(vkDevice, &vkrpcInfo, 0, &vkRenderPass))
	{
		eprintf("Failed to create render pass!\n");
		return 1;
	}

	size_t shadervlen, shaderflen;
	void *shaderv = readfile("vertex.spv", &shadervlen);
	void *shaderf = readfile("fragment.spv", &shaderflen);
	if (shaderv == NULL || shaderf == NULL)
	{
		eprintf("Failed to read shaders. Sadge...\n");
		return 1;
	}

	VkShaderModuleCreateInfo vksmcInfoVertex =
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = shadervlen,
		.pCode = shaderv,
	};
	VkShaderModuleCreateInfo vksmcInfoFragment =
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = shaderflen,
		.pCode = shaderf,
	};

	VkShaderModule shaderModuleVertex;
	VkShaderModule shaderModuleFragment;
	if (VK_SUCCESS != vkCreateShaderModule(vkDevice, &vksmcInfoVertex, 0, &shaderModuleVertex))
	{
		eprintf("Failed to create vertex shader module!\n");
		return 1;
	}
	if (VK_SUCCESS != vkCreateShaderModule(vkDevice, &vksmcInfoFragment, 0, &shaderModuleFragment))
	{
		eprintf("Failed to create fragment shader module!\n");
		return 1;
	}

	VkDynamicState vkDynStates[] =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	VkPipelineDynamicStateCreateInfo vkpdscInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = ARRAYSIZE(vkDynStates),
		.pDynamicStates = vkDynStates,
	};
	VkPipelineVertexInputStateCreateInfo vkpviscInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = 0,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = 0,
	};
	VkPipelineInputAssemblyStateCreateInfo vkpiascInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};
	VkViewport vkViewports[] =
	{
		{
			.x = 0,
			.y = 0,
			.width = vkSurfaceCaps.currentExtent.width,
			.height = vkSurfaceCaps.currentExtent.height,
			.minDepth = 0,
			.maxDepth = 1,
		}
	};
	VkRect2D vkScissors[] =
	{
		{
			.offset = {0, 0},
			.extent = vkSurfaceCaps.currentExtent,
		}
	};
	VkPipelineViewportStateCreateInfo vkpvscInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = ARRAYSIZE(vkViewports),
		.pViewports = vkViewports,
		.scissorCount = ARRAYSIZE(vkViewports),
		.pScissors = vkScissors,
	};
	VkPipelineRasterizationStateCreateInfo vkprscInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.lineWidth = 1,
		.depthBiasEnable = VK_FALSE,
#if 0
		// Would've been required if depth bias enabled
		.depthBiasConstantFactor = 0,
		.depthBiasClamp = 0,
		.depthBiasSlopeFactor = 0,
#endif
	};
	VkPipelineMultisampleStateCreateInfo vkpmscInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.sampleShadingEnable = VK_FALSE,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.minSampleShading = 1.0f,
		.pSampleMask = 0,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE,
	};
	// TODO: Depth and stenciling (ugh...)
	// Ah yes. Color blending. My favorite...
	VkPipelineColorBlendAttachmentState vkpcbaStates[] =
	{
		{
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
			.blendEnable = VK_FALSE,
#if 0
			// Would have been required if blending was enabled
			.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
#endif
		},
	};
	VkPipelineColorBlendStateCreateInfo vkpcbscInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = ARRAYSIZE(vkpcbaStates),
		.pAttachments = vkpcbaStates,
		.logicOpEnable = VK_FALSE,
#if 0
		// Would be required if logical operations were enabled
		.logicOp = VK_LOGIC_OP_COPY,
		.blendConstants = {0, 0, 0, 0},
#endif
	};

	VkPipelineShaderStageCreateInfo vkpsscInfos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = shaderModuleVertex,
			.pName = "main",
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = shaderModuleFragment,
			.pName = "main",
		},
	};

	VkGraphicsPipelineCreateInfo vkgpcInfo =
	{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = ARRAYSIZE(vkpsscInfos),
		.pStages = vkpsscInfos,
		.pVertexInputState = &vkpviscInfo,
		.pInputAssemblyState = &vkpiascInfo,
		.pViewportState = &vkpvscInfo,
		.pRasterizationState = &vkprscInfo,
		.pMultisampleState = &vkpmscInfo,
		.pDepthStencilState = 0,
		.pColorBlendState = &vkpcbscInfo,
		.pDynamicState = &vkpdscInfo,
		.layout = vkPipelineLayout,
		.renderPass = vkRenderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
	};

	VkPipeline vkGraphicsPipeline;
	if (VK_SUCCESS != vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &vkgpcInfo, 0, &vkGraphicsPipeline))
	{
		eprintf("Failed to create the graphics pipeline! AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA!\n");
		return 1;
	}
	eprintf("I created the graphics pipeline and I wanna kill someone!\n");

	uint32_t vkFramebuffersCount = vkSwapchainImagesCount;
	VkFramebuffer *vkFramebuffers = calloc(vkFramebuffersCount, sizeof(*vkFramebuffers));
	if (vkFramebuffers == NULL)
	{
		eprintf("Failed to allocate array of Framebuffers!\n");
		return 1;
	}
	for (size_t i = 0; i < vkFramebuffersCount; i++)
	{
		VkImageView vkImageViewAttachments[] = { vkSwapchainImageViews[i] };
		VkFramebufferCreateInfo vkfcInfo =
		{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = vkRenderPass,
			.attachmentCount = ARRAYSIZE(vkImageViewAttachments),
			.pAttachments = vkImageViewAttachments,
			.width = vkSurfaceCaps.currentExtent.width,
			.height = vkSurfaceCaps.currentExtent.height,
			.layers = 1,
		};
		if (VK_SUCCESS != vkCreateFramebuffer(vkDevice, &vkfcInfo, 0, &vkFramebuffers[i]))
		{
			eprintf("Failed to allocate framebuffer!\n");
			return 1;
		}
		eprintf("Created framebuffer %zu from view %p: %p\n", i,  vkImageViewAttachments[0], vkFramebuffers[i]);
	}
	eprintf("Created Framebuffers!\n");

	VkCommandPoolCreateInfo vkpcInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = vkQueueNodeIndex,
	};
	VkCommandPool vkPool;
	if (VK_SUCCESS != vkCreateCommandPool(vkDevice, &vkpcInfo, 0, &vkPool))
	{
		eprintf("Failed to create command pool!\n");
		return 1;
	}
	VkCommandBuffer vkCommandBuffers[1];
	VkCommandBufferAllocateInfo vkcbaInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = vkPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = ARRAYSIZE(vkCommandBuffers),
	};
	if (VK_SUCCESS != vkAllocateCommandBuffers(vkDevice, &vkcbaInfo, vkCommandBuffers))
	{
		eprintf("Failed to create command buffers!\n");
		return 1;
	}
	eprintf("I did a command buffer!\n");

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkFence inFlightFence;
	VkSemaphoreCreateInfo vksemcInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};
	VkFenceCreateInfo vkfcInfo =
	{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};
	if (VK_SUCCESS != vkCreateSemaphore(vkDevice, &vksemcInfo, 0, &imageAvailableSemaphore)
		|| VK_SUCCESS != vkCreateSemaphore(vkDevice, &vksemcInfo, 0, &renderFinishedSemaphore)
		|| VK_SUCCESS != vkCreateFence(vkDevice, &vkfcInfo, 0, &inFlightFence))
	{
		eprintf("Wasn't able to create synchronizatino primitive? What?\n");
		return 1;
	}

	// "Record" a command buffer, whatever the hell that means... 
#if 0
#endif


	SDL_Event e;
	bool quit = false;
	VkQueue vkGraphicsQueue, vkPresentQueue;
	vkGetDeviceQueue(vkDevice, vkQueueNodeIndex, 0, &vkGraphicsQueue);
	vkGetDeviceQueue(vkDevice, vkQueueNodeIndex, 0, &vkPresentQueue);
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
		uint32_t imageIndex = 0;
		// vkWaitForFences(vkDevice, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
		vkAcquireNextImageKHR(vkDevice, vkSwapchain, UINT16_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		vkResetCommandBuffer(vkCommandBuffers[0], 0);

		VkCommandBufferBeginInfo vkcbbInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = 0,
			.pInheritanceInfo = 0,
		};
		if (VK_SUCCESS != vkBeginCommandBuffer(vkCommandBuffers[0], &vkcbbInfo))
		{
			eprintf("Beginning command buffer failed!\n");
			return 1;
		}
		VkClearValue vkClearColors[] =
		{
			{
				.color =
				{
					.float32 =
					{
						0, 0, 0, 1
					}
				}
			}
		};
		VkRenderPassBeginInfo vkrpbInfo =
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = vkRenderPass,
			.framebuffer = vkFramebuffers[imageIndex],
			.renderArea.offset = {0, 0},
			.renderArea.extent = vkSurfaceCaps.currentExtent,
			.clearValueCount = ARRAYSIZE(vkClearColors),
			.pClearValues = vkClearColors,
		};
		vkCmdBeginRenderPass(vkCommandBuffers[0], &vkrpbInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdSetViewport(vkCommandBuffers[0], 0, ARRAYSIZE(vkViewports), vkViewports);
		vkCmdSetScissor(vkCommandBuffers[0], 0, ARRAYSIZE(vkScissors), vkScissors);
		vkCmdDraw(vkCommandBuffers[0], 3, 1, 0, 0);
		vkCmdEndRenderPass(vkCommandBuffers[0]);
		if (VK_SUCCESS != vkEndCommandBuffer(vkCommandBuffers[0]))
		{
			eprintf("Failed to record the command buffer!\n");
			return 1;
		}

		VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo vkSubmitInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &imageAvailableSemaphore,
			.pWaitDstStageMask = &stageFlags,
			.commandBufferCount = ARRAYSIZE(vkCommandBuffers),
			.pCommandBuffers = vkCommandBuffers,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &renderFinishedSemaphore,
		};

		if (VK_SUCCESS != vkQueueSubmit(vkGraphicsQueue, 1, &vkSubmitInfo, inFlightFence))
		{
			eprintf("Fasiled to submit queue!\n");
			return 1;
		}

		VkSwapchainKHR vkSwapChains[] = {vkSwapchain};
		VkPresentInfoKHR vkPresentInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &renderFinishedSemaphore,
			.swapchainCount = 1,
			.pSwapchains = vkSwapChains,
			.pImageIndices = &imageIndex,
			.pResults = 0,
		};
		vkQueuePresentKHR(vkPresentQueue, &vkPresentInfo);
		// vkResetFences(vkDevice, 1, &inFlightFence);
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	// This is only one of the many things we need to clean up.
	// This is here now only to test that debug messenger flags this.
	vkDestroyInstance(vkInstance, 0);
	return 0;
}
