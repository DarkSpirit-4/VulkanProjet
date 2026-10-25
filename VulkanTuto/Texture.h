#pragma once

#include <string>
#include "VulkanContext.h"

class Texture
{
public:
	void Load(VulkanContext& context, const std::string& path);

	vk::ImageView GetView() const { return *view; }
	vk::Sampler   GetSampler() const { return *sampler; }

private:
	void createSampler(VulkanContext& ctx);
	void transitionImageLayout(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
	void copyBufferToImage(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height);

	vk::raii::Image        image = nullptr;
	vk::raii::DeviceMemory memory = nullptr;
	vk::raii::ImageView    view = nullptr;
	vk::raii::Sampler      sampler = nullptr;
};