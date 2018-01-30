#include "VulkanRenderer.h"

VulkanRenderer::VulkanRenderer() {}
VulkanRenderer::~VulkanRenderer() {}

Material* VulkanRenderer::makeMaterial(const std::string& name) { return nullptr; }
Mesh* VulkanRenderer::makeMesh() { return nullptr; }
//VertexBuffer* VulkanRenderer::makeVertexBuffer();
VertexBuffer* VulkanRenderer::makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage) { return nullptr; }
ConstantBuffer* VulkanRenderer::makeConstantBuffer(std::string NAME, unsigned int location) { return nullptr; }
//	ResourceBinding* VulkanRenderer::makeResourceBinding();
RenderState* VulkanRenderer::makeRenderState() { return nullptr; }
Technique* VulkanRenderer::makeTechnique(Material* m, RenderState* r) { return nullptr; }
Texture2D* VulkanRenderer::makeTexture2D() { return nullptr; }
Sampler2D* VulkanRenderer::makeSampler2D() { return nullptr; }
std::string VulkanRenderer::getShaderPath() { return ""; }
std::string VulkanRenderer::getShaderExtension() { return ""; }

int VulkanRenderer::initialize(unsigned int width, unsigned int height) { return 0; }
void VulkanRenderer::setWinTitle(const char* title) {}
int VulkanRenderer::shutdown() { return 0; }

void VulkanRenderer::setClearColor(float, float, float, float) {}
void VulkanRenderer::clearBuffer(unsigned int) {}
//	void VulkanRenderer::setRenderTarget(RenderTarget* rt); // complete parameters
void VulkanRenderer::setRenderState(RenderState* ps) {}
void VulkanRenderer::submit(Mesh* mesh) {}
void VulkanRenderer::frame() {}
void VulkanRenderer::present() {}
