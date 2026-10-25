#include "DepthBuffer.h"

void DepthBuffer::Create(VulkanContext& ctx, vk::Extent2D extent)
{
	format = ctx.findDepthFormat();

	std::tie(image, memory) = ctx.createImage(extent.width, extent.height, format,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eDepthStencilAttachment,
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	view = ctx.createImageView(*image, format, vk::ImageAspectFlagBits::eDepth);
}