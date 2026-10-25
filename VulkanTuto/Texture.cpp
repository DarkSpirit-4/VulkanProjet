#include <cstring>
#include <stdexcept>
#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void Texture::Load(VulkanContext& ctx, const std::string& path)
{
	int            texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
	{
		throw std::runtime_error("failed to load texture image!");
	}

	vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(texWidth) * texHeight * 4;

	auto [stagingBuffer, stagingBufferMemory] =
		ctx.createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	void* data = stagingBufferMemory.mapMemory(0, imageSize);
	memcpy(data, pixels, imageSize);
	stagingBufferMemory.unmapMemory();

	stbi_image_free(pixels);

	std::tie(image, memory) = ctx.createImage(texWidth,
		texHeight,
		vk::Format::eR8G8B8A8Srgb,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	vk::raii::CommandBuffer commandBuffer = ctx.beginSingleTimeCommands();
	transitionImageLayout(commandBuffer, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
	copyBufferToImage(commandBuffer, stagingBuffer, image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	transitionImageLayout(commandBuffer, image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
	ctx.endSingleTimeCommands(std::move(commandBuffer));

	view = ctx.createImageView(*image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
	createSampler(ctx);
}

void Texture::createSampler(VulkanContext& ctx)
{
	vk::PhysicalDeviceProperties properties = ctx.GetPhysicalDevice().getProperties();
	vk::SamplerCreateInfo samplerInfo{ .magFilter = vk::Filter::eLinear,
									   .minFilter = vk::Filter::eLinear,
									   .mipmapMode = vk::SamplerMipmapMode::eLinear,
									   .addressModeU = vk::SamplerAddressMode::eRepeat,
									   .addressModeV = vk::SamplerAddressMode::eRepeat,
									   .addressModeW = vk::SamplerAddressMode::eRepeat,
									   .mipLodBias = 0.0f,
									   .anisotropyEnable = vk::True,
									   .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
									   .compareEnable = vk::False,
									   .compareOp = vk::CompareOp::eAlways,
									   .minLod = 0.0f,
									   .maxLod = 0.0f,
									   .borderColor = vk::BorderColor::eIntOpaqueBlack,
									   .unnormalizedCoordinates = vk::False };

	sampler = vk::raii::Sampler(ctx.GetDevice(), samplerInfo);
}

void Texture::transitionImageLayout(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
	vk::ImageMemoryBarrier barrier{ .oldLayout = oldLayout,
									   .newLayout = newLayout,
									   .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
									   .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
									   .image = image,
									   .subresourceRange = { .aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1 } };

	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;

	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else
	{
		throw std::invalid_argument("unsupported layout transition!");
	}
	commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, barrier);
}

void Texture::copyBufferToImage(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height)
{
	vk::BufferImageCopy region{ .bufferOffset = 0,
						   .bufferRowLength = 0,
						   .bufferImageHeight = 0,
						   .imageSubresource = { .aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 },
						   .imageOffset = { 0, 0, 0 },
						   .imageExtent = { width, height, 1 } };
	commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
}