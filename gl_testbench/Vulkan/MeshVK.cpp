#include "MeshVK.h"
#include "VulkanRenderer.h"
#include "VertexBufferVK.h"
#include "Texture2DVK.h"

MeshVK::MeshVK(VulkanRenderer* renderer) : _renderer(renderer) {
	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.descriptorPool = renderer->_descriptorPool;
	allocInfo.descriptorSetCount = (uint32_t)renderer->_descriptorSetLayouts.size();
	allocInfo.pSetLayouts = renderer->_descriptorSetLayouts.data();

	descriptorSets = renderer->_device.allocateDescriptorSets(allocInfo);
	for (auto& ds : descriptorSets)
		EXPECT_ASSERT(ds, "Failed to allocate descriptor sets!\n");
}
MeshVK::~MeshVK() {}

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
