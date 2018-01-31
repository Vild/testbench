#pragma once
#include "../Material.h"
#include "ConstantBufferVK.h"

#include <cstdint>
#include <vector>
#include <sstream>
#include <iostream>

class VulkanRenderer;

template <typename T>
inline void DBOUTW(T&& s) {
  std::wostringstream os_;
  os_ << s;
  #ifdef _WIN32
  OutputDebugStringW(os_.str().c_str());
  #else
	std::wcerr << os_.str() << std::endl;
  #endif
}

template <typename T>
inline void DBOUT(T&& s) {
  std::ostringstream os_;
  os_ << s;
  #ifdef _WIN32
  OutputDebugString(os_.str().c_str());
  #else
	std::cerr << os_.str() << std::endl;
  #endif
}

// use X = {Program or Shader}
#define INFO_OUT(S,X) { \
		char buff[1024];		\
		memset(buff, 0, 1024);											\
		glGet##X##InfoLog(S, 1024, nullptr, buff);	\
		DBOUTW(buff);																\
	}

// use X = {Program or Shader}
#define COMPILE_LOG(S,X,OUT) { \
		char buff[1024];					 \
		memset(buff, 0, 1024);											\
		glGet##X##InfoLog(S, 1024, nullptr, buff);	\
		OUT=std::string(buff);											\
	}


class MaterialVK : public Material {
	friend VulkanRenderer;

public:
	MaterialVK(const std::string& name);
	virtual ~MaterialVK();

	void setShader(const std::string& shaderFileName, ShaderType type) final;
	void removeShader(ShaderType type) final;
	int compileMaterial(std::string& errString) final;
	int enable() final;
	void disable() final;
	inline uint32_t getProgram() { return program; };
	void setDiffuse(Color c) final;

	// location identifies the constant buffer in a unique way
	void updateConstantBuffer(const void* data, size_t size, unsigned int location) final;
	// slower version using a string
	void addConstantBuffer(std::string name, unsigned int location) final;
	std::map<unsigned int, ConstantBufferVK*> constantBuffers;

private:
	uint32_t program = -1;
};

