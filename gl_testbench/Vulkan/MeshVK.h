#pragma once

#include <unordered_map>
#include "../Mesh.h"

#include <vulkan/vulkan.hpp>
#include "../IA.h"

class VulkanRenderer;
enum class DescriptorType {
	position = POSITION,
	index = INDEX,
	model = MODEL,
	cameraViewProjection = CAMERA_VIEW_PROJECTION
};

class MeshVK : public Mesh {
public:
	MeshVK(VulkanRenderer* renderer);
	virtual ~MeshVK();

	void finalize() final;
	void bindIAVertexBuffers();
	void bindTextures();

	inline vk::DescriptorSet& getDescriptorSet(DescriptorType type) { return descriptorSets[static_cast<int>(type)]; }

	std::vector<vk::DescriptorSet> descriptorSets;

private:
	VulkanRenderer* _renderer;
};
