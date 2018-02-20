#pragma once

#include <unordered_map>
#include "../Mesh.h"

#include <vulkan/vulkan.hpp>

class VulkanRenderer;

enum class DescriptorType {
	translation = 0,
	diffuseTint = 1,
	position = 2,
	normal = 3,
	textcoord = 4
};

class MeshVK : public Mesh {
public:
	MeshVK(VulkanRenderer* renderer);
	virtual ~MeshVK();

	void bindIAVertexBuffers();

	inline vk::DescriptorSet& getDescriptorSet(DescriptorType type) {
		return descriptorSets[static_cast<int>(type)];
	}

	std::vector<vk::DescriptorSet> descriptorSets;
};
