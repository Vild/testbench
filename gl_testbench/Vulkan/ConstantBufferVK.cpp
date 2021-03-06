#include "ConstantBufferVK.h"
#include "VulkanRenderer.h"
#include "MeshVK.h"
#include "../IA.h"

ConstantBufferVK::ConstantBufferVK(VulkanRenderer* renderer, uint32_t binding) : _renderer(renderer), _device(renderer->_device) {
	// Prepares the Write Descriptor for setData function later on.
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

		_bufferInfo.buffer = _buffer;
		_bufferInfo.offset = 0;
		_bufferInfo.range = size;
		_descriptorWrite.pBufferInfo = &_bufferInfo;
	}

	void* ptr = _device.mapMemory(_bufferMemory, 0, size);
	memcpy(ptr, data, size);
	_device.unmapMemory(_bufferMemory);
}

void ConstantBufferVK::bind(Material* material) {
	// STUB();
}

void ConstantBufferVK::bindDescriptors(MeshVK* mesh) {
	if (_descriptorWrite.dstBinding == MODEL)
		_descriptorWrite.dstSet = mesh->getDescriptorSet(DescriptorType::model);
	else if (_descriptorWrite.dstBinding == CAMERA_VIEW_PROJECTION)
		_descriptorWrite.dstSet = mesh->getDescriptorSet(DescriptorType::cameraViewProjection);

	_device.updateDescriptorSets(_descriptorWrite, nullptr);
}
