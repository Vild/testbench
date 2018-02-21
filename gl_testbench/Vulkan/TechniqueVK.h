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

	void enable(MeshVK* mesh);

private:
	VulkanRenderer* _renderer;
	vk::Device _device;
	vk::PipelineLayout _pipelineLayout;
	vk::Pipeline _graphicsPipeline;

	void enable(Renderer* renderer) final {}
};
