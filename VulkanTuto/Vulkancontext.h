#pragma once

#include "Dependancies.h"

class VulkanContext
{
public:
	void Init(GLFWwindow* window);
	void WaitIdle() { device.waitIdle(); }

	vk::raii::Device& GetDevice() { return device; }
	vk::raii::PhysicalDevice& GetPhysicalDevice() { return physicalDevice; }
	vk::raii::SurfaceKHR& GetSurface() { return surface; }
	vk::raii::Queue& GetGraphicsQueue() { return graphicsQueue; }
	vk::raii::CommandPool& GetCommandPool() { return commandPool; }
	uint32_t                  GetQueueIndex() const { return queueIndex; }

	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
	std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
	std::pair<vk::raii::Image, vk::raii::DeviceMemory> createImage(
		uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
		vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
	vk::raii::ImageView createImageView(vk::Image const& image, vk::Format format, vk::ImageAspectFlags aspectFlags);
	void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size);

	vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
	vk::Format findDepthFormat();

	vk::raii::CommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(vk::raii::CommandBuffer&& commandBuffer);

private:
	void createInstance();
	void SetupCallback();
	void createSurface(GLFWwindow* window);
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createCommandPool();

	std::vector<char const*> getRequiredExtensions();
	std::vector<char const*> getRequiredLayers();

	static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

	vk::raii::Context                context;
	vk::raii::Instance               instance = nullptr;
	vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
	vk::raii::SurfaceKHR             surface = nullptr;
	vk::raii::PhysicalDevice         physicalDevice = nullptr;
	vk::raii::Device                 device = nullptr;
	vk::raii::Queue                  graphicsQueue = nullptr;
	vk::raii::CommandPool            commandPool = nullptr;
	uint32_t                         queueIndex = ~0;

	std::vector<const char*> requiredDeviceExtension = { vk::KHRSwapchainExtensionName };
};