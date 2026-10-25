#include <algorithm>
#include <cassert>
#include <limits>
#include "SwapChain.h"

void SwapChain::Create(VulkanContext& context, GLFWwindow* _window)
{
	ctx = &context;
	window = _window;

	create();
	createImageViews();
}

bool SwapChain::Recreate()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while ((width == 0 || height == 0) && !glfwWindowShouldClose(window)) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}
	if (glfwWindowShouldClose(window)) {
		return false;
	}

	ctx->WaitIdle();

	Cleanup();

	create();
	createImageViews();
	return true;
}

void SwapChain::Cleanup()
{
	imageViews.clear();
	swapChain = nullptr;
}

void SwapChain::create()
{
	auto& physicalDevice = ctx->GetPhysicalDevice();
	auto& surface = ctx->GetSurface();

	vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
	extent = chooseExtent(surfaceCapabilities);
	uint32_t minImageCount = chooseMinImageCount(surfaceCapabilities);

	std::vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.getSurfaceFormatsKHR(*surface);
	surfaceFormat = chooseSurfaceFormat(availableFormats);

	std::vector<vk::PresentModeKHR> availablePresentModes = physicalDevice.getSurfacePresentModesKHR(*surface);

	vk::SwapchainCreateInfoKHR swapChainCreateInfo{ .surface = *surface,
											   .minImageCount = minImageCount,
											   .imageFormat = surfaceFormat.format,
											   .imageColorSpace = surfaceFormat.colorSpace,
											   .imageExtent = extent,
											   .imageArrayLayers = 1,
											   .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
											   .imageSharingMode = vk::SharingMode::eExclusive,
											   .preTransform = surfaceCapabilities.currentTransform,
											   .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
											   .presentMode = choosePresentMode(availablePresentModes),
											   .clipped = true };
	swapChain = vk::raii::SwapchainKHR(ctx->GetDevice(), swapChainCreateInfo);
	images = swapChain.getImages();
}

void SwapChain::createImageViews()
{
	assert(imageViews.empty());

	imageViews.reserve(images.size());
	for (auto& image : images)
	{
		imageViews.emplace_back(ctx->createImageView(image, surfaceFormat.format, vk::ImageAspectFlagBits::eColor));
	}
}

vk::SurfaceFormatKHR SwapChain::chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
	const auto formatIt = std::ranges::find_if(
		availableFormats,
		[](const auto& format) { return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; });
	return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

vk::PresentModeKHR SwapChain::choosePresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes)
{
	assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) { return presentMode == vk::PresentModeKHR::eFifo; }));
	return std::ranges::any_of(availablePresentModes,
		[](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; }) ?
		vk::PresentModeKHR::eMailbox :
		vk::PresentModeKHR::eFifo;
}

vk::Extent2D SwapChain::chooseExtent(vk::SurfaceCapabilitiesKHR const& capabilities)
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

uint32_t SwapChain::chooseMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities)
{
	auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
	if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount))
	{
		minImageCount = surfaceCapabilities.maxImageCount;
	}
	return minImageCount;
}