#include <iostream>
#include "Renderer.h"
#include "Debug.h"

#include <cstdint>
#include <limits>
#include <algorithm>


void Renderer::SetWindow(GLFWwindow* _window)
{
	window = _window;
}

void Renderer::CreateInstance()
{
	constexpr vk::ApplicationInfo application_info
	{
		.pApplicationName = "VulkanTuto",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "ENGINE+",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = vk::ApiVersion14

	};

	std::vector requiredExtensions = GetRequiredExtensions();

	auto extensionProperties = context.enumerateInstanceExtensionProperties();

	for (uint32_t i = 0; i < requiredExtensions.size(); i++)
	{
		auto compareExtensions = [extension = requiredExtensions[i]](auto const& extensionProperty)
			{
				return strcmp(extensionProperty.extensionName, extension) == 0;
			};

		if (std::ranges::none_of(extensionProperties, compareExtensions))
		{
			throw std::runtime_error("Required GLFW extension not supported" + std::string(requiredExtensions[i]));
		}

	}
	

	const std::vector validationLayers = { "VK_LAYER_KHRONOS_validation" };


		std::vector<char const*> requiredLayers = GetRequiredLayers();

		if (Debug::enableDebug)
			requiredLayers.assign_range(validationLayers);

		auto layerProperties = context.enumerateInstanceLayerProperties();

		auto unsupportedLayerIt = std::ranges::find_if(requiredLayers, [&layerProperties](auto const& requiredLayer)
			{
				return std::ranges::none_of(layerProperties, [requiredLayer](auto const& layerProperty)
					{
						return strcmp(layerProperty.layerName, requiredLayer) == 0;
					});
			});

		if (unsupportedLayerIt != requiredLayers.end())
		{
			throw std::runtime_error("Required Layer not supported: " + std::string(*unsupportedLayerIt));
		}

	vk::InstanceCreateInfo create_info
	{
		.pApplicationInfo = &application_info,
		.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
		.ppEnabledLayerNames = requiredLayers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
		.ppEnabledExtensionNames = requiredExtensions.data(),

	};

	instance = vk::raii::Instance(context, create_info);

	SetupCallback();
}

std::vector<char const*> Renderer::GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector <char const*> requiredExtensions = {vk::EXTDebugUtilsExtensionName};

	std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	extensions.append_range(requiredExtensions);

	return extensions;
}

std::vector<char const*> Renderer::GetRequiredLayers()
{
	const std::vector validationLayers = { "VK_LAYER_KHRONOS_validation" };

	std::vector<char const*> requiredLayers = {};

	if (Debug::enableDebug)
		requiredLayers.append_range(validationLayers);

	return requiredLayers;
}

void Renderer::SetupCallback()
{
	vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

	vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

	vk::DebugUtilsMessengerCreateInfoEXT debugInfo
	{
		.messageSeverity = severityFlags,
		.messageType = messageTypeFlags,
		.pfnUserCallback = &Callback
	};

	debugMessenger = instance.createDebugUtilsMessengerEXT(debugInfo);
}

vk::Bool32 VKAPI_CALL Renderer::Callback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::cout << "Validation layer: Type [" << vk::to_string(type) << "] - Message : " << pCallbackData->pMessage << std::endl;

	return vk::False;
}

bool isDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice)
{
	bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;

	auto queueFamilies = physicalDevice.getQueueFamilyProperties();
	bool supportsGraphics =
		std::ranges::any_of(queueFamilies, [](auto const& qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

	std::vector<const char*> requiredDeviceExtension = { vk::KHRSwapchainExtensionName };

	auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
	bool supportsAllRequiredExtensions =
		std::ranges::all_of(requiredDeviceExtension,
			[&availableDeviceExtensions](auto const& requiredDeviceExtension)
			{
				return std::ranges::any_of(availableDeviceExtensions,
					[requiredDeviceExtension](auto const& availableDeviceExtension)
					{ return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0; });
			});

	auto features = physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2,
		vk::PhysicalDeviceVulkan11Features,
		vk::PhysicalDeviceVulkan13Features,
		vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

	bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
		features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
		features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

	return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
}

void Renderer::pickPhysicalDevice()
{
	std::vector<vk::raii::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();

	auto const devIter = std::ranges::find_if(physicalDevices, [&](auto const& physicalDevice) { return isDeviceSuitable(physicalDevice); });

	if (devIter == physicalDevices.end())
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	physicalDevice = *devIter;

}

void Renderer::createLogicalDevice()
{
	std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

	uint32_t graphicsIndex = ~0;
	for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
	{
		if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
			physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface))
		{
			graphicsIndex = qfpIndex;
			break;
		}
	}
	if (graphicsIndex == ~0)
	{
		throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
	}

	auto graphicsQueueFamilyProperty = std::ranges::find_if(queueFamilyProperties, [](auto const& qfp) { return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0); });
	/* auto graphicsIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty)); */

	float queuePriority = 0.5f;
	vk::DeviceQueueCreateInfo deviceQueueCreateInfo{ .queueFamilyIndex = graphicsIndex, .queueCount = 1, .pQueuePriorities = &queuePriority };

	vk::StructureChain<vk::PhysicalDeviceFeatures2,
		vk::PhysicalDeviceVulkan11Features,
		vk::PhysicalDeviceVulkan13Features,
		vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>

		featureChain = {
		{},
		{.shaderDrawParameters = true},
		{.dynamicRendering = true},
		{.extendedDynamicState = true}
	};

	std::vector<const char*> requiredDeviceExtension = {
	vk::KHRSwapchainExtensionName };

	vk::DeviceCreateInfo deviceCreateInfo{
	.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
	.queueCreateInfoCount = 1,
	.pQueueCreateInfos = &deviceQueueCreateInfo,
	.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size()),
	.ppEnabledExtensionNames = requiredDeviceExtension.data()
	};

	device = vk::raii::Device(physicalDevice, deviceCreateInfo);

	graphicsQueue = vk::raii::Queue(device, graphicsIndex, 0);
}

void Renderer::createSurface()
{
	VkSurfaceKHR _surface;
	if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0) {
		throw std::runtime_error("failed to create window surface!");
	}
	surface = vk::raii::SurfaceKHR(instance, _surface);
}

// -->
vk::SurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
	const auto formatIt = std::ranges::find_if(
		availableFormats,
		[](const auto& format) { return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; });
	return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

vk::PresentModeKHR Renderer::chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes)
{
	assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) { return presentMode == vk::PresentModeKHR::eFifo; }));
	return std::ranges::any_of(availablePresentModes,
		[](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; }) ?
		vk::PresentModeKHR::eMailbox :
		vk::PresentModeKHR::eFifo;
}

vk::Extent2D Renderer::chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	return {
		std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
	};
}

uint32_t Renderer::chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities)
{
	auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
	if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount))
	{
		minImageCount = surfaceCapabilities.maxImageCount;
	}
	return minImageCount;
}

void Renderer::createSwapChain()
{
	vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
	swapChainExtent = chooseSwapExtent(surfaceCapabilities);
	uint32_t minImageCount = chooseSwapMinImageCount(surfaceCapabilities);

	std::vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.getSurfaceFormatsKHR(*surface);
	swapChainSurfaceFormat = chooseSwapSurfaceFormat(availableFormats);

	std::vector<vk::PresentModeKHR> availablePresentModes = physicalDevice.getSurfacePresentModesKHR(*surface);

	vk::SwapchainCreateInfoKHR swapChainCreateInfo{ .surface = *surface,
											   .minImageCount = minImageCount,
											   .imageFormat = swapChainSurfaceFormat.format,
											   .imageColorSpace = swapChainSurfaceFormat.colorSpace,
											   .imageExtent = swapChainExtent,
											   .imageArrayLayers = 1,
											   .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
											   .imageSharingMode = vk::SharingMode::eExclusive,
											   .preTransform = surfaceCapabilities.currentTransform,
											   .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
											   .presentMode = chooseSwapPresentMode(availablePresentModes),
											   .clipped = true };
	swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
	swapChainImages = swapChain.getImages();
}

void Renderer::createImageViews()
{
	assert(swapChainImageViews.empty());

	vk::ImageViewCreateInfo imageViewCreateInfo{ .viewType = vk::ImageViewType::e2D,
												 .format = swapChainSurfaceFormat.format,
												 .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };

	for (auto& image : swapChainImages)
	{
		imageViewCreateInfo.image = image;
		swapChainImageViews.emplace_back(device, imageViewCreateInfo);
	}
}

void Renderer::createGraphicsPipeline()
{

}
