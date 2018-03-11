#include "TechniqueVK.h"

#include "VulkanRenderer.h"
#include "MeshVK.h"
#include "MaterialVK.h"
#include "RenderStateVK.h"

TechniqueVK::TechniqueVK(VulkanRenderer* renderer, Material* m, RenderState* r) : Technique(m, r), _renderer(renderer), _device(renderer->_device) {}

TechniqueVK::~TechniqueVK() {
	_device.destroyPipelineLayout(_pipelineLayout);
	_device.destroyPipeline(_graphicsPipeline);
}

void TechniqueVK::enable(MeshVK* mesh) {
	auto& bpi = _renderer->_basePipelineInfo;
	if (!_graphicsPipeline) {
		auto& descriptorSetLayouts = mesh->textures.size() ? _renderer->_descriptorTextureSetLayouts : _renderer->_descriptorSetLayouts;

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
		pipelineLayoutInfo.setLayoutCount = (uint32_t)descriptorSetLayouts.size();
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

		_pipelineLayout = _device.createPipelineLayout(pipelineLayoutInfo);
		EXPECT_ASSERT(_pipelineLayout, "Pipeline Layout is null!");

		bpi.pipelineInfo.stageCount = (uint32_t) static_cast<MaterialVK*>(material)->_program.size();
		bpi.pipelineInfo.pStages = static_cast<MaterialVK*>(material)->_program.data();
		bpi.rasterizer.polygonMode = vk::PolygonMode::eFill;
		bpi.pipelineInfo.layout = _pipelineLayout;

		_graphicsPipeline = _device.createGraphicsPipeline(vk::PipelineCache(), bpi.pipelineInfo);
		EXPECT_ASSERT(_graphicsPipeline, "Can't create Graphics pipeline!");
	}

	Technique::enable(_renderer);
}
