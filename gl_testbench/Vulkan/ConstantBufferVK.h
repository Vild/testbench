#pragma once

#include "../ConstantBuffer.h"
#include <vulkan/vulkan.hpp>

#include <cstdint>

class VulkanRenderer;

class ConstantBufferVK : public ConstantBuffer {
public:
	ConstantBufferVK(VulkanRenderer* renderer, uint32_t binding);
	virtual ~ConstantBufferVK();
	void setData(const void* data, size_t size, Material* m, uint32_t binding) final;
	void bind(Material* material) final;

private:
	VulkanRenderer* _renderer;
	vk::Device _device;
	vk::Buffer _buffer;
	vk::WriteDescriptorSet _descriptorWrite;
	vk::DeviceMemory _bufferMemory;
};
