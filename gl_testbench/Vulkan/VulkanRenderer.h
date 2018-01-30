#pragma once

#include "../Renderer.h"

#include <SDL.h>

#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")



class VulkanRenderer : public Renderer
{
public:
	VulkanRenderer();
	virtual ~VulkanRenderer();

	Material* makeMaterial(const std::string& name) final;
	Mesh* makeMesh() final;
	//VertexBuffer* makeVertexBuffer();
	VertexBuffer* makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage) final;
	ConstantBuffer* makeConstantBuffer(std::string NAME, unsigned int location) final;
//	ResourceBinding* makeResourceBinding();
	RenderState* makeRenderState() final;
	Technique* makeTechnique(Material* m, RenderState* r) final;
	Texture2D* makeTexture2D() final;
	Sampler2D* makeSampler2D() final;
	std::string getShaderPath() final;
	std::string getShaderExtension() final;

	int initialize(unsigned int width = 640, unsigned int height = 480) final;
	void setWinTitle(const char* title) final;
	int shutdown() final;

	void setClearColor(float, float, float, float) final;
	void clearBuffer(unsigned int) final;
//	void setRenderTarget(RenderTarget* rt); // complete parameters
	void setRenderState(RenderState* ps) final;
	void submit(Mesh* mesh) final;
	void frame() final;
	void present() final;

private:
	SDL_Window* window;

	std::vector<Mesh*> drawList;
	std::unordered_map<Technique*, std::vector<Mesh*>> drawList2;

	bool globalWireframeMode = false;

	//int initializeVulkan(int major, int minor, unsigned int width, unsigned int height);
	float clearColor[4] = { 0,0,0,0 };
};

