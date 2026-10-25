#pragma once

#include "Dependancies.h"
#include "VulkanContext.h"
#include "SwapChain.h"
#include "DepthBuffer.h"
#include "Texture.h"
#include "Mesh.h"
#include "Pipeline.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class Renderer
{
public:
	void Init(GLFWwindow* window);
	void WaitIdle() { ctx.WaitIdle(); }
	void SetFramebufferResized() { framebufferResized = true; }
	void drawFrame();

private:
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void createCommandBuffers();
	void createSyncObjects();

	void recreateSwapChain();
	void updateUniformBuffer(uint32_t currentImage);
	void recordCommandBuffer(uint32_t imageIndex);
	void transitionImageLayout(
		vk::raii::CommandBuffer& commandBuffer,
		vk::Image               image,
		vk::ImageLayout         old_layout,
		vk::ImageLayout         new_layout,
		vk::AccessFlags2        src_access_mask,
		vk::AccessFlags2        dst_access_mask,
		vk::PipelineStageFlags2 src_stage_mask,
		vk::PipelineStageFlags2 dst_stage_mask,
		vk::ImageAspectFlags    image_aspect_flags);

	GLFWwindow* window = nullptr;

	VulkanContext ctx;
	SwapChain     swapChain;
	DepthBuffer   depthBuffer;
	Pipeline      pipeline;
	Texture       texture;
	Mesh          mesh;

	std::vector<vk::raii::Buffer>        uniformBuffers;
	std::vector<vk::raii::DeviceMemory>  uniformBuffersMemory;
	std::vector<void*>                   uniformBuffersMapped;
	vk::raii::DescriptorPool             descriptorPool = nullptr;
	std::vector<vk::raii::DescriptorSet> descriptorSets;
	std::vector<vk::raii::CommandBuffer> commandBuffers;

	std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
	std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
	std::vector<vk::raii::Fence>     inFlightFences;

	uint32_t frameIndex = 0;
	bool     framebufferResized = false;
};