#pragma once

#include "../VertexBuffer.h"
#include <vulkan/vulkan.hpp>

#include <cstdint>

class VulkanRenderer;
class MeshVK;

class VertexBufferVK : public VertexBuffer {
public:
	VertexBufferVK(VulkanRenderer* renderer, size_t size);
	virtual ~VertexBufferVK();

	void setData(const void* data, size_t size, size_t offset) final;
	inline void bind(size_t offset, size_t size, uint32_t location) final { assert(0); }
	void bind(MeshVK* mesh, size_t offset, size_t size, uint32_t location);
	void unbind() final;
	size_t getSize() final;

private:
	VulkanRenderer* _renderer;
	vk::Device _device;

	size_t _size;

	vk::Buffer _buffer;
	vk::DescriptorBufferInfo _bufferInfo;
	vk::WriteDescriptorSet _descriptorWrite;
	vk::DeviceMemory _bufferMemory;
};
