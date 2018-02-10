#include "ConstantBufferVK.h"
#include "VulkanRenderer.h"

ConstantBufferVK::ConstantBufferVK(VulkanRenderer* renderer, uint32_t binding) : _renderer(renderer), _device(renderer->_device) {
	vk::DescriptorSetLayoutBinding layoutBinding;
	layoutBinding.binding = binding;
	layoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	layoutBinding.descriptorCount = 1;
	layoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
	layoutBinding.pImmutableSamplers = nullptr;

	vk::DescriptorSetLayoutCreateInfo layoutInfo;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &layoutBinding;

	_descriptorSetLayout = renderer->_device.createDescriptorSetLayout(layoutInfo);
	EXPECT_ASSERT(_descriptorSetLayout, "Failed to create descriptor set layout");
}

ConstantBufferVK::~ConstantBufferVK() {
	STUB();
	_device.destroyDescriptorSetLayout(_descriptorSetLayout);
	_device.destroyBuffer(_buffer);
	_device.freeMemory(_bufferMemory);
}

void ConstantBufferVK::setData(const void* data, size_t size, Material* m, uint32_t binding) {
	if (!_buffer) {
		vk::DeviceSize bufferSize = size;
		_renderer->createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
		                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, _buffer, _bufferMemory);
	}

	void* ptr = _device.mapMemory(_bufferMemory, 0, size);
	memcpy(ptr, data, size);
	_device.unmapMemory(_bufferMemory);
}

void ConstantBufferVK::bind(Material* material) {
	STUB();
}
