#pragma once

#include <string>
#include "VulkanContext.h"

class Pipeline
{
public:
	void Create(VulkanContext& context, vk::Format colorFormat, const std::string& shaderPath);

	vk::Pipeline            GetPipeline() const { return *pipeline; }
	vk::PipelineLayout      GetLayout() const { return *pipelineLayout; }
	vk::DescriptorSetLayout GetDescriptorSetLayout() const { return *descriptorSetLayout; }

private:
	void createDescriptorSetLayout(VulkanContext& ctx);
	vk::raii::ShaderModule createShaderModule(VulkanContext& ctx, const std::vector<char>& code) const;

	vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
	vk::raii::PipelineLayout      pipelineLayout = nullptr;
	vk::raii::Pipeline            pipeline = nullptr;
};