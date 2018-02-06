#include "ConstantBufferVK.h"
#include "VulkanRenderer.h"

ConstantBufferVK::ConstantBufferVK(VulkanRenderer* renderer, const std::string& name, unsigned int location) {}
ConstantBufferVK::~ConstantBufferVK() {}
void ConstantBufferVK::setData(const void* data, size_t size, Material* m, unsigned int location) {}
void ConstantBufferVK::bind(Material* material) {}
