#include <cstring>
#include "Mesh.h"

void Mesh::Create(VulkanContext& ctx, const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices)
{
	indexCount = static_cast<uint32_t>(indices.size());

	// Vertex buffer
	{
		vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		auto [stagingBuffer, stagingBufferMemory] =
			ctx.createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		void* dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
		memcpy(dataStaging, vertices.data(), bufferSize);
		stagingBufferMemory.unmapMemory();

		std::tie(vertexBuffer, vertexBufferMemory) =
			ctx.createBuffer(bufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);

		ctx.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
	}

	// Index buffer
	{
		vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		auto [stagingBuffer, stagingBufferMemory] =
			ctx.createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		void* dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
		memcpy(dataStaging, indices.data(), static_cast<size_t>(bufferSize));
		stagingBufferMemory.unmapMemory();

		std::tie(indexBuffer, indexBufferMemory) =
			ctx.createBuffer(bufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);

		ctx.copyBuffer(stagingBuffer, indexBuffer, bufferSize);
	}
}

void Mesh::Draw(vk::raii::CommandBuffer& commandBuffer) const
{
	commandBuffer.bindVertexBuffers(0, *vertexBuffer, { 0 });
	commandBuffer.bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint16);
	commandBuffer.drawIndexed(indexCount, 1, 0, 0, 0);
}