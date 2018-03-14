#pragma once
#include "../Technique.h"

#include <vulkan/vulkan.hpp>

class VulkanRenderer;
class MeshVK;

class TechniqueVK : public Technique {
	friend VulkanRenderer;

public:
	TechniqueVK(VulkanRenderer* renderer, Material* m, RenderState* r);
	virtual ~TechniqueVK();

	void enable();

private:
	VulkanRenderer* _renderer;
	vk::Device _device;

	void enable(Renderer* renderer) final {}
};
