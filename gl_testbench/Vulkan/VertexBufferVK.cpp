#include "VertexBufferVK.h"
#include "VulkanRenderer.h"
#include "../IA.h"

VertexBufferVK::VertexBufferVK(VulkanRenderer* renderer, size_t size) : _renderer(renderer), _device(renderer->_device), _size(size) {}

VertexBufferVK::~VertexBufferVK() {
	_device.destroyBuffer(_buffer);
	_device.freeMemory(_bufferMemory);
}

void VertexBufferVK::setData(const void* data, size_t size, size_t offset) {
	static vk::PhysicalDeviceProperties pdp = _renderer->_physicalDevice.getProperties();
	static auto minSBOA = pdp.limits.minStorageBufferOffsetAlignment;
	EXPECT_ASSERT((minSBOA & (minSBOA - 1)) == 0, "minSBOA is not power-of-2");
	// TODO: Make a staging buffer and a final buffer
	if (!_buffer) {
		// _size = sizeof(elementtype) * COUNT
		size_t count = _size / size;
		size_t elementSize = (size + minSBOA - 1) & ~(minSBOA - 1);
		vk::DeviceSize bufferSize = elementSize * count;
		_renderer->createBuffer(bufferSize, vk::BufferUsageFlagBits::eStorageBuffer,
		                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, _buffer, _bufferMemory);
	}

	EXPECT_ASSERT(offset % size == 0, "offset % size must be 0");
	size_t idx = offset / size;
	size = (size + minSBOA - 1) & ~(minSBOA - 1);
	offset = idx * size;

	void* ptr = _device.mapMemory(_bufferMemory, offset, size);
	memcpy(ptr, data, size);
	_device.unmapMemory(_bufferMemory);
}
void VertexBufferVK::bind(size_t offset, size_t size, uint32_t location) {
	static vk::PhysicalDeviceProperties pdp = _renderer->_physicalDevice.getProperties();
	static auto minSBOA = pdp.limits.minStorageBufferOffsetAlignment;
	EXPECT_ASSERT((minSBOA & (minSBOA - 1)) == 0, "minSBOA is not power-of-2");
	// This is very ugly, temporary until I can think of any other way.
	if (location == POSITION)
		_descriptorWrite.dstSet = _renderer->_descriptorSets[2];
	else if (location == NORMAL)
		_descriptorWrite.dstSet = _renderer->_descriptorSets[3];
	else
		_descriptorWrite.dstSet = _renderer->_descriptorSets[4];

	EXPECT_ASSERT(offset % size == 0, "offset % size must be 0");
	size_t idx = offset / size;
	size = (size + minSBOA - 1) & ~(minSBOA - 1);
	offset = idx * size;

	vk::DescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = _buffer;
	bufferInfo.offset = offset;
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
	//printf("%s(offset: 0x%zX, size: 0x%zX, location: %u);\n", __PRETTY_FUNCTION__, offset, size, location);
}
void VertexBufferVK::unbind() {
	STUB();
}
size_t VertexBufferVK::getSize() {
	STUB();
	return _size;
}
