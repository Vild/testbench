#pragma once

#include "../Renderer.h"

#ifdef _WIN32
#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include <cstdint>
#include <SDL.h>
#include <vulkan/vulkan.hpp>

class MaterialVK;

class VulkanRenderer : public Renderer {
	friend MaterialVK;
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
	struct QueueInformation {
		uint32_t graphics = -1;
		uint32_t present = -1;

		inline bool completed() { return graphics != -1UL && present != -1UL; }
	};
	struct SwapChainInformation {
		vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
	};

	uint32_t _width;
	uint32_t _height;
	SDL_Window* _window;
	vk::Instance _instance;
	vk::DebugReportCallbackEXT _debugCallback;
	vk::SurfaceKHR _surface;
	vk::PhysicalDevice _physicalDevice;
	QueueInformation _queueInformation;
	SwapChainInformation _swapChainInformation;
	vk::Device _device;
	vk::Queue _graphicsQueue;
	vk::Queue _presentQueue;
	vk::SwapchainKHR _swapChain;
	vk::Format _swapChainImageFormat;
	vk::Extent2D _swapChainExtent;
	std::vector<vk::Image> _swapChainImages;
	std::vector<vk::ImageView> _swapChainImageViews;

	std::vector<Mesh*> _drawList;
	std::unordered_map<Technique*, std::vector<Mesh*>> _drawList2;

	bool _globalWireframeMode = false;

	//int initializeVulkan(int major, int minor, unsigned int width, unsigned int height);
	float _clearColor[4] = { 0,0,0,0 };

	bool _initSDL();
	bool _createVulkanInstance();
	bool _createSDLSurface();
	bool _createVulkanPhysicalDevice();
	bool _createVulkanLogicalDevice();
	bool _createVulkanSwapChain();
	bool _createVulkanImageViews();
};

