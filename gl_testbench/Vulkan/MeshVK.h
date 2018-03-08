#pragma once

#include <unordered_map>
#include "../Mesh.h"

#include <vulkan/vulkan.hpp>

class VulkanRenderer;
// cameraViewProjection = 2,
enum class DescriptorType { translation = 0, diffuseTint = 1, cameraViewProjection = 2, position = 3, normal = 4, textcoord = 5, diffuseSlot = 6};

class MeshVK : public Mesh {
public:
	MeshVK(VulkanRenderer* renderer);
	virtual ~MeshVK();

	void finalize();
	void bindIAVertexBuffers();
	void bindTextures();

	inline vk::DescriptorSet& getDescriptorSet(DescriptorType type) { return descriptorSets[static_cast<int>(type)]; }

	std::vector<vk::DescriptorSet> descriptorSets;

private:
	VulkanRenderer* _renderer;
};
