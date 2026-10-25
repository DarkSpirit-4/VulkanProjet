#pragma once

#include "VulkanContext.h"

class DepthBuffer
{
public:
	void Create(VulkanContext& context, vk::Extent2D extent);

	vk::Image     GetImage() const { return *image; }
	vk::ImageView GetView() const { return *view; }
	vk::Format    GetFormat() const { return format; }

private:
	vk::Format             format = vk::Format::eUndefined;
	vk::raii::Image        image = nullptr;
	vk::raii::DeviceMemory memory = nullptr;
	vk::raii::ImageView    view = nullptr;
};