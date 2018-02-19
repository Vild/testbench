#include "VertexBufferVK.h"
#include "VulkanRenderer.h"
#include "../IA.h"

VertexBufferVK::VertexBufferVK(VulkanRenderer* renderer, size_t size) : _renderer(renderer), _device(renderer->_device), _size(size) {}

VertexBufferVK::~VertexBufferVK() {
	STUB();
	_device.destroyBuffer(_buffer);
	_device.freeMemory(_bufferMemory);
}

void VertexBufferVK::setData(const void* data, size_t size, size_t offset) {
	// TODO: Make a staging buffer and a final buffer
	if (!_buffer) {
		vk::DeviceSize bufferSize = _size;
		_renderer->createBuffer(bufferSize, vk::BufferUsageFlagBits::eStorageBuffer,
		                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, _buffer, _bufferMemory);
	}

	void* ptr = _device.mapMemory(_bufferMemory, offset, size);
	memcpy(ptr, data, size);
	_device.unmapMemory(_bufferMemory);
}
void VertexBufferVK::bind(size_t offset, size_t size, uint32_t location) {
	// This is very ugly, temporary until I can think of any other way.
	if (location == POSITION)
		_descriptorWrite.dstSet = _renderer->_descriptorSets[2];
	else if (location == NORMAL)
		_descriptorWrite.dstSet = _renderer->_descriptorSets[3];
	else
		_descriptorWrite.dstSet = _renderer->_descriptorSets[4];

	vk::DescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = _buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = size;

	_descriptorWrite.dstBinding = location;
	_descriptorWrite.dstArrayElement = 0;
	_descriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
	_descriptorWrite.descriptorCount = 1;
	_descriptorWrite.pBufferInfo = &bufferInfo;
	_descriptorWrite.pImageInfo = nullptr;
	_descriptorWrite.pTexelBufferView = nullptr;

	// Applies the final info needed to update descriptor sets.
	_device.updateDescriptorSets(_descriptorWrite, nullptr);
	STUB();
}
void VertexBufferVK::unbind() {
	STUB();
}
size_t VertexBufferVK::getSize() {
	STUB();
	return _size;
}
