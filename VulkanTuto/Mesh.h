#pragma once

#include <cstdint>
#include <vector>
#include "VulkanContext.h"
#include "Vertex.h"

class Mesh
{
public:
	void Create(VulkanContext& context, const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices);

	void Draw(vk::raii::CommandBuffer& commandBuffer) const;

	uint32_t GetIndexCount() const { return indexCount; }

private:
	vk::raii::Buffer       vertexBuffer = nullptr;
	vk::raii::DeviceMemory vertexBufferMemory = nullptr;
	vk::raii::Buffer       indexBuffer = nullptr;
	vk::raii::DeviceMemory indexBufferMemory = nullptr;
	uint32_t               indexCount = 0;
};