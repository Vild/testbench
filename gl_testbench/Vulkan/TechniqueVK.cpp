#include "TechniqueVK.h"

#include "VulkanRenderer.h"
#include "MeshVK.h"
#include "MaterialVK.h"
#include "RenderStateVK.h"

TechniqueVK::TechniqueVK(VulkanRenderer* renderer, Material* m, RenderState* r) : Technique(m, r), _renderer(renderer), _device(renderer->_device) {}

TechniqueVK::~TechniqueVK() {}

void TechniqueVK::enable() {
	Technique::enable(_renderer);
}
