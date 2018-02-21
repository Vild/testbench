#include "MeshVK.h"
#include "VulkanRenderer.h"
#include "VertexBufferVK.h"
#include "Texture2DVK.h"

MeshVK::MeshVK(VulkanRenderer* renderer) : _renderer(renderer) {}
MeshVK::~MeshVK() {}

void MeshVK::finalize() {
	if (descriptorSets.size())
		return;
	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.descriptorPool = _renderer->_descriptorPool;
	if (textures.size()) {
		allocInfo.descriptorSetCount = (uint32_t)_renderer->_descriptorTextureSetLayouts.size();
		allocInfo.pSetLayouts = _renderer->_descriptorTextureSetLayouts.data();
	} else {
		allocInfo.descriptorSetCount = (uint32_t)_renderer->_descriptorSetLayouts.size();
		allocInfo.pSetLayouts = _renderer->_descriptorSetLayouts.data();
	}

	descriptorSets = _renderer->_device.allocateDescriptorSets(allocInfo);
	for (auto& ds : descriptorSets)
		EXPECT_ASSERT(ds, "Failed to allocate descriptor sets!\n");
}

void MeshVK::bindIAVertexBuffers() {
	for (auto& it : geometryBuffers)
		static_cast<VertexBufferVK*>(it.second.buffer)->bind(this, it.second.offset, it.second.numElements * it.second.sizeElement, it.first);
}

void MeshVK::bindTextures() {
	for (auto t : textures) {
		Texture2DVK* tVK = (Texture2DVK*)t.second;
		tVK->updateSampler(this, t.first);
	}
}
