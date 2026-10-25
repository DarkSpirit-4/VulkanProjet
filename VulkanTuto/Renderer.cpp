#include <cassert>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include "Renderer.h"
#include "Vertex.h"
#include "Geometry.h"

void Renderer::Init(GLFWwindow* _window)
{
	window = _window;

	ctx.Init(window);
	swapChain.Create(ctx, window);
	pipeline.Create(ctx, swapChain.GetFormat(), "shaders/slang.spv");
	depthBuffer.Create(ctx, swapChain.GetExtent());
	texture.Load(ctx, "textures/texture.jpg");
	mesh.Create(ctx, vertices, indices);

	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
	createSyncObjects();
}

void Renderer::createUniformBuffers()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
		auto [buffer, bufferMem] = ctx.createBuffer(
			bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		uniformBuffers.emplace_back(std::move(buffer));
		uniformBuffersMemory.emplace_back(std::move(bufferMem));
		uniformBuffersMapped.emplace_back(uniformBuffersMemory.back().mapMemory(0, bufferSize));
	}
}

void Renderer::createDescriptorPool()
{
	std::array<vk::DescriptorPoolSize, 2> poolSize{ {
		{ .type = vk::DescriptorType::eUniformBuffer, .descriptorCount = MAX_FRAMES_IN_FLIGHT },
		{ .type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = MAX_FRAMES_IN_FLIGHT }
	} };

	vk::DescriptorPoolCreateInfo poolInfo{ .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
										   .maxSets = MAX_FRAMES_IN_FLIGHT,
										   .poolSizeCount = static_cast<uint32_t>(poolSize.size()),
										   .pPoolSizes = poolSize.data() };
	descriptorPool = vk::raii::DescriptorPool(ctx.GetDevice(), poolInfo);
}

void Renderer::createDescriptorSets()
{
	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, pipeline.GetDescriptorSetLayout());
	vk::DescriptorSetAllocateInfo        allocInfo{ .descriptorPool = descriptorPool,
												   .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
												   .pSetLayouts = layouts.data() };
	descriptorSets = ctx.GetDevice().allocateDescriptorSets(allocInfo);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vk::DescriptorBufferInfo bufferInfo{ .buffer = uniformBuffers[i], .offset = 0, .range = sizeof(UniformBufferObject) };
		vk::DescriptorImageInfo  imageInfo{ .sampler = texture.GetSampler(), .imageView = texture.GetView(), .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };

		std::array<vk::WriteDescriptorSet, 2> descriptorWrites{ {
			{ .dstSet = descriptorSets[i],
			  .dstBinding = 0,
			  .dstArrayElement = 0,
			  .descriptorCount = 1,
			  .descriptorType = vk::DescriptorType::eUniformBuffer,
			  .pBufferInfo = &bufferInfo },
			{ .dstSet = descriptorSets[i],
			  .dstBinding = 1,
			  .dstArrayElement = 0,
			  .descriptorCount = 1,
			  .descriptorType = vk::DescriptorType::eCombinedImageSampler,
			  .pImageInfo = &imageInfo }
		} };
		ctx.GetDevice().updateDescriptorSets(descriptorWrites, {});
	}
}

void Renderer::createCommandBuffers()
{
	vk::CommandBufferAllocateInfo allocInfo{ .commandPool = ctx.GetCommandPool(), .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = MAX_FRAMES_IN_FLIGHT };
	commandBuffers = vk::raii::CommandBuffers(ctx.GetDevice(), allocInfo);
}

void Renderer::createSyncObjects()
{
	assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() && inFlightFences.empty());

	for (size_t i = 0; i < swapChain.GetImageCount(); i++)
	{
		renderFinishedSemaphores.emplace_back(ctx.GetDevice(), vk::SemaphoreCreateInfo());
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		presentCompleteSemaphores.emplace_back(ctx.GetDevice(), vk::SemaphoreCreateInfo());
		inFlightFences.emplace_back(ctx.GetDevice(), vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
	}
}

void Renderer::recreateSwapChain()
{
	if (!swapChain.Recreate())
		return;

	depthBuffer.Create(ctx, swapChain.GetExtent());
}


void Renderer::transitionImageLayout(
	vk::raii::CommandBuffer& commandBuffer,
	vk::Image               image,
	vk::ImageLayout         old_layout,
	vk::ImageLayout         new_layout,
	vk::AccessFlags2        src_access_mask,
	vk::AccessFlags2        dst_access_mask,
	vk::PipelineStageFlags2 src_stage_mask,
	vk::PipelineStageFlags2 dst_stage_mask,
	vk::ImageAspectFlags    image_aspect_flags)
{
	vk::ImageMemoryBarrier2 barrier = {
		.srcStageMask = src_stage_mask,
		.srcAccessMask = src_access_mask,
		.dstStageMask = dst_stage_mask,
		.dstAccessMask = dst_access_mask,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = image_aspect_flags,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1 } };
	vk::DependencyInfo dependency_info = {
		.dependencyFlags = {},
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier };
	commandBuffer.pipelineBarrier2(dependency_info);
}

void Renderer::recordCommandBuffer(uint32_t imageIndex)
{
	auto& commandBuffer = commandBuffers[frameIndex];
	const vk::Extent2D extent = swapChain.GetExtent();
	commandBuffer.begin({});

	transitionImageLayout(
		commandBuffer,
		swapChain.GetImage(imageIndex),
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eColorAttachmentOptimal,
		{},
		vk::AccessFlagBits2::eColorAttachmentWrite,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		vk::ImageAspectFlagBits::eColor);

	transitionImageLayout(
		commandBuffer,
		depthBuffer.GetImage(),
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eDepthAttachmentOptimal,
		vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
		vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
		vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
		vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
		vk::ImageAspectFlagBits::eDepth);

	vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
	vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

	vk::RenderingAttachmentInfo colorAttachmentInfo = {
		.imageView = swapChain.GetImageView(imageIndex),
		.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.clearValue = clearColor };

	vk::RenderingAttachmentInfo depthAttachmentInfo = {
		.imageView = depthBuffer.GetView(),
		.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eDontCare,
		.clearValue = clearDepth };

	vk::RenderingInfo renderingInfo = {
		.renderArea = { .offset = { 0, 0 }, .extent = extent },
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentInfo,
		.pDepthAttachment = &depthAttachmentInfo };

	commandBuffer.beginRendering(renderingInfo);
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.GetPipeline());
	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.GetLayout(), 0, *descriptorSets[frameIndex], nullptr);
	commandBuffer.setViewport(0, vk::Viewport(0.0f, static_cast<float>(extent.height), static_cast<float>(extent.width), -static_cast<float>(extent.height), 0.0f, 1.0f));
	commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));
	mesh.Draw(commandBuffer);
	commandBuffer.endRendering();

	transitionImageLayout(
		commandBuffer,
		swapChain.GetImage(imageIndex),
		vk::ImageLayout::eColorAttachmentOptimal,
		vk::ImageLayout::ePresentSrcKHR,
		vk::AccessFlagBits2::eColorAttachmentWrite,
		{},
		vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		vk::PipelineStageFlagBits2::eBottomOfPipe,
		vk::ImageAspectFlagBits::eColor);

	commandBuffer.end();
}


void Renderer::updateUniformBuffer(uint32_t currentImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	const vk::Extent2D extent = swapChain.GetExtent();

	UniformBufferObject ubo{};
	ubo.model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	ubo.proj =
		glm::perspective(glm::radians(45.0f), static_cast<float>(extent.width) / static_cast<float>(extent.height), 0.1f, 10.0f);

	memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void Renderer::drawFrame()
{
	auto& device = ctx.GetDevice();
	auto& graphicsQueue = ctx.GetGraphicsQueue();

	auto fenceResult = device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
	if (fenceResult != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to wait for fence!");
	}

	auto [result, imageIndex] = swapChain.GetHandle().acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);

	if (result == vk::Result::eErrorOutOfDateKHR)
	{
		recreateSwapChain();
		return;
	}
	if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
	{
		assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	device.resetFences(*inFlightFences[frameIndex]);

	commandBuffers[frameIndex].reset();
	recordCommandBuffer(imageIndex);
	vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);

	updateUniformBuffer(frameIndex);

	const vk::SubmitInfo   submitInfo{ .waitSemaphoreCount = 1,
									  .pWaitSemaphores = &*presentCompleteSemaphores[frameIndex],
									  .pWaitDstStageMask = &waitDestinationStageMask,
									  .commandBufferCount = 1,
									  .pCommandBuffers = &*commandBuffers[frameIndex],
									  .signalSemaphoreCount = 1,
									  .pSignalSemaphores = &*renderFinishedSemaphores[imageIndex] };
	graphicsQueue.submit(submitInfo, *inFlightFences[frameIndex]);

	const vk::PresentInfoKHR presentInfoKHR{ .waitSemaphoreCount = 1,
											.pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
											.swapchainCount = 1,
											.pSwapchains = &*swapChain.GetHandle(),
											.pImageIndices = &imageIndex };

	result = graphicsQueue.presentKHR(presentInfoKHR);
	if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR) || framebufferResized)
	{
		framebufferResized = false;
		recreateSwapChain();
	}
	else
	{
		assert(result == vk::Result::eSuccess);
	}
	frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}