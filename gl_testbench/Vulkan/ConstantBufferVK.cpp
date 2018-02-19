#include "ConstantBufferVK.h"
#include "VulkanRenderer.h"
#include "../IA.h"

ConstantBufferVK::ConstantBufferVK(VulkanRenderer* renderer, uint32_t binding) : _renderer(renderer), _device(renderer->_device) {
	// Prepares the Write Descriptor for setData function later on.
	if (binding == TRANSLATION)
		_descriptorWrite.dstSet = renderer->_descriptorSets[0];
	else
		_descriptorWrite.dstSet = renderer->_descriptorSets[1];
	_descriptorWrite.dstBinding = binding;
	_descriptorWrite.dstArrayElement = 0;
	_descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
	_descriptorWrite.descriptorCount = 1;
	_descriptorWrite.pBufferInfo = nullptr;
	_descriptorWrite.pImageInfo = nullptr;
	_descriptorWrite.pTexelBufferView = nullptr;
}

ConstantBufferVK::~ConstantBufferVK() {
	_device.destroyBuffer(_buffer);
	_device.freeMemory(_bufferMemory);
}

void ConstantBufferVK::setData(const void* data, size_t size, Material* m, uint32_t binding) {
	if (!_buffer) {
		vk::DeviceSize bufferSize = size;
		_renderer->createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
		                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, _buffer, _bufferMemory);

		vk::DescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = _buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = size;

		// Applies the final info needed to update descriptor sets.
		_descriptorWrite.pBufferInfo = &bufferInfo;
		_device.updateDescriptorSets(_descriptorWrite, nullptr);
	}

	void* ptr = _device.mapMemory(_bufferMemory, 0, size);
	memcpy(ptr, data, size);
	_device.unmapMemory(_bufferMemory);
}

void ConstantBufferVK::bind(Material* material) {
	STUB();
}
