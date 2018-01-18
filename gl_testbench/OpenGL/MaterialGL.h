#pragma once
#include "../Material.h"
#include <GL/glew.h>
#include <vector>
#include "ConstantBufferGL.h"
#include <sstream>
#include <iostream>

class OpenGLRenderer;

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
char buff[1024];\
memset(buff, 0, 1024);\
glGet##X##InfoLog(S, 1024, nullptr, buff);\
DBOUTW(buff);\
}

// use X = {Program or Shader}
#define COMPILE_LOG(S,X,OUT) { \
char buff[1024];\
memset(buff, 0, 1024);\
glGet##X##InfoLog(S, 1024, nullptr, buff);\
OUT=std::string(buff);\
}


class MaterialGL :
	public Material
{
	friend OpenGLRenderer;

public:
	MaterialGL(const std::string& name);
	~MaterialGL();


	void setShader(const std::string& shaderFileName, ShaderType type);
	void removeShader(ShaderType type);
	int compileMaterial(std::string& errString);
	int enable();
	void disable();
	GLuint getProgram() { return program; };
	void setDiffuse(Color c);

	// location identifies the constant buffer in a unique way
	void updateConstantBuffer(const void* data, size_t size, unsigned int location);
	// slower version using a string
	void addConstantBuffer(std::string name, unsigned int location);
	std::map<unsigned int, ConstantBufferGL*> constantBuffers;

private:
	// map from ShaderType to GL_VERTEX_SHADER, should be static.
	GLuint mapShaderEnum[4];

	std::string shaderNames[4];

	// opengl shader object
	GLuint shaderObjects[4] = { 0,0,0,0 };

	// TODO: change to PIPELINE
	// opengl program object
	std::string name;
	GLuint program;
	int compileShader(ShaderType type, std::string& errString);
	std::vector<std::string> expandShaderText(std::string& shaderText, ShaderType type);

};

