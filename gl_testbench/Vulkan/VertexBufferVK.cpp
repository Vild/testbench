#include "VertexBufferVK.h"
#include "VulkanRenderer.h"

VertexBufferVK::VertexBufferVK(VulkanRenderer* renderer, size_t size) : _renderer(renderer), _device(renderer->_device), _size(size) {}
VertexBufferVK::~VertexBufferVK() {
	STUB();
	_device.destroyDescriptorSetLayout(_descriptorSetLayout);
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
	if (!_descriptorSetLayout) {
		vk::DescriptorSetLayoutBinding layoutBinding;
		layoutBinding.binding = location;
		layoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
		layoutBinding.descriptorCount = 1;
		layoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
		layoutBinding.pImmutableSamplers = nullptr;

		vk::DescriptorSetLayoutCreateInfo layoutInfo;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &layoutBinding;

		_descriptorSetLayout = _renderer->_device.createDescriptorSetLayout(layoutInfo);
		EXPECT_ASSERT(_descriptorSetLayout, "Failed to create descriptor set layout");
	}
	STUB();
}
void VertexBufferVK::unbind() {
	STUB();
}
size_t VertexBufferVK::getSize() {
	STUB();
	return _size;
}
