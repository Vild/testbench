#include "MaterialVK.h"

MaterialVK::MaterialVK(const std::string& str) {}
MaterialVK::~MaterialVK() {}

void MaterialVK::setShader(const std::string& shaderFileName, ShaderType type) {}
void MaterialVK::removeShader(ShaderType type) {}
int MaterialVK::compileMaterial(std::string& errString) {}
int MaterialVK::enable() {}
void MaterialVK::disable() {}
void MaterialVK::setDiffuse(Color c) {}

void MaterialVK::updateConstantBuffer(const void* data, size_t size, unsigned int location) {}
void MaterialVK::addConstantBuffer(std::string name, unsigned int location) {}
