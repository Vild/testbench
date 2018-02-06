#pragma once
#include "../Material.h"
#include "ConstantBufferVK.h"
#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <vector>
#include <sstream>
#include <iostream>

class VulkanRenderer;

class MaterialVK : public Material {
	friend VulkanRenderer;

public:
	MaterialVK(VulkanRenderer* renderer, const std::string& name);
	virtual ~MaterialVK();

	void setShader(const std::string& shaderFileName, ShaderType type) final;
	void removeShader(ShaderType type) final;
	int compileMaterial(std::string& errString) final;
	int enable() final;
	void disable() final;
	void setDiffuse(Color c) final;

	// location identifies the constant buffer in a unique way
	void updateConstantBuffer(const void* data, size_t size, unsigned int location) final;
	// slower version using a string
	void addConstantBuffer(std::string name, unsigned int location) final;
	std::map<unsigned int, ConstantBufferVK*> constantBuffers;

private:
	VulkanRenderer* _renderer;
	vk::ShaderStageFlagBits _mapShaderEnum[4];
	std::string _mapShaderExt[4];

	std::map<ShaderType, vk::ShaderModule> _shaderModules;
	std::vector<vk::PipelineShaderStageCreateInfo> _program;

	std::string _name;

	int _compileShader(ShaderType type, std::string& errString);
	std::vector<std::string> _expandShaderText(std::string& shaderText, ShaderType type);
};

