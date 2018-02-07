#include "MaterialVK.h"
#include "VulkanRenderer.h"

#include <cstdint>
#include <fstream>

static void split(const char* text, std::vector<std::string>* const temp, const char delim = ' ') {
	uint32_t delimPos = strcspn(text, (const char*)&delim);
	if (delimPos == strlen(text))
		temp->push_back(std::string(text));
	else {
		temp->push_back(std::string(text, delimPos));
		split(text + delimPos, temp, delim);
	}
}

MaterialVK::MaterialVK(VulkanRenderer* renderer, const std::string& name) : _renderer(renderer), _name(name) {
	isValid = false;
	_mapShaderEnum[(uint32_t)ShaderType::VS] = vk::ShaderStageFlagBits::eVertex;
	_mapShaderEnum[(uint32_t)ShaderType::PS] = vk::ShaderStageFlagBits::eFragment;
	_mapShaderEnum[(uint32_t)ShaderType::GS] = vk::ShaderStageFlagBits::eGeometry;
	_mapShaderEnum[(uint32_t)ShaderType::CS] = vk::ShaderStageFlagBits::eCompute;
	_mapShaderExt[(uint32_t)ShaderType::VS] = ".vert";
	_mapShaderExt[(uint32_t)ShaderType::PS] = ".frag";
	_mapShaderExt[(uint32_t)ShaderType::GS] = ".geom";
	_mapShaderExt[(uint32_t)ShaderType::CS] = ".comp";
}
MaterialVK::~MaterialVK() {
	for (auto& module : _shaderModules)
		_renderer->_device.destroyShaderModule(module.second);
}

void MaterialVK::setShader(const std::string& shaderFileName, ShaderType type) {
	if (shaderFileNames.find(type) != shaderFileNames.end())
		removeShader(type);
	shaderFileNames[type] = shaderFileName;
}
void MaterialVK::removeShader(ShaderType type) {
	// check if name exists (if it doesn't there should not be a shader here.
	if (shaderFileNames.find(type) == shaderFileNames.end())
		return;
	if (_shaderModules.find(type) == _shaderModules.end())
		return;

	_renderer->_device.destroyShaderModule(_shaderModules[type]);
	_shaderModules.erase(type);
}
int MaterialVK::compileMaterial(std::string& errString) {
	removeShader(ShaderType::VS);
	removeShader(ShaderType::PS);

	if (_compileShader(ShaderType::VS, errString) < 0) {
		std::cerr << errString << std::endl;
		exit(-1);
	};
	if (_compileShader(ShaderType::PS, errString) < 0) {
		std::cerr << errString << std::endl;
		exit(-1);
	};

	_program.clear();
	_program.push_back(vk::PipelineShaderStageCreateInfo{{}, vk::ShaderStageFlagBits::eVertex, _shaderModules[ShaderType::VS], "main"});
	_program.push_back(vk::PipelineShaderStageCreateInfo{{}, vk::ShaderStageFlagBits::eFragment, _shaderModules[ShaderType::PS], "main"});

	isValid = true;
	return 0;
}
int MaterialVK::enable() {
	if (!_program.size() || isValid == false)
		return -1;

	for (auto cb : constantBuffers)
		cb.second->bind(this);

	// TODO: renderer.bindMaterial(this);
	return 0;
}
void MaterialVK::disable() {
	// TODO: renderer.bindMaterial(nullptr);
}
void MaterialVK::setDiffuse(Color c) {}

void MaterialVK::updateConstantBuffer(const void* data, size_t size, unsigned int location) {
	constantBuffers[location]->setData(data, size, this, location);
}
void MaterialVK::addConstantBuffer(std::string name, unsigned int location) {
	constantBuffers[location] = new ConstantBufferVK(_renderer, name, location);
}

std::vector<std::string> MaterialVK::_expandShaderText(std::string& shaderSource, ShaderType type) {
	std::vector<std::string> result{"#version 450\n#extension GL_ARB_separate_shader_objects : enable\n\0"};
	for (auto define : shaderDefines[type])
		result.push_back(define);
	result.push_back(shaderSource);
	return result;
};

static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("failed to open file: " + filename);

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

int MaterialVK::_compileShader(ShaderType type, std::string& errString) {
	std::string file = shaderFileNames[type];

	{
		std::vector<char> data = readFile(file);
		std::string shaderText = std::string(data.begin(), data.end());
		;
		std::vector<std::string> shaderLines = _expandShaderText(shaderText, type);

		{ // Compile shader
			auto tmpFile = file + ".preprocessed" + _mapShaderExt[(size_t)type];
			std::ofstream tmpStream(tmpFile);
			for (std::string& text : shaderLines)
				tmpStream.write(text.data(), text.size());
			tmpStream.close();

			auto r = system((std::string("glslangValidator -V -e \"main\" -o ") + file + ".spv " + tmpFile).c_str());
			if (r) {
				errString = "glslangValidator status wasn't 0";
				return -1;
			}
		}
	}

	std::vector<char> shaderBinary = readFile(file + ".spv");
	vk::ShaderModuleCreateInfo createInfo{vk::ShaderModuleCreateFlags(), shaderBinary.size(), (const uint32_t*)shaderBinary.data()};

	_shaderModules[type] = _renderer->_device.createShaderModule(createInfo);
	return 0;
}
