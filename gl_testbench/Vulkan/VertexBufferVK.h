#pragma once

#include "../VertexBuffer.h"

#include <cstdint>

class VulkanRenderer;

class VertexBufferVK : public VertexBuffer {
public:
	static uint32_t usageMapping[3];

	VertexBufferVK(VulkanRenderer* renderer, size_t size, VertexBuffer::DATA_USAGE usage);
	virtual ~VertexBufferVK();

	void setData(const void* data, size_t size, size_t offset) final;
	void bind(size_t offset, size_t size, unsigned int location) final;
	void unbind() final;
	size_t getSize() final;

private:
	size_t totalSize;
	uint32_t _handle;
};
