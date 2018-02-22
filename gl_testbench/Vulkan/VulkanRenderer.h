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

#include <cstdio>

#include <functional>

#ifdef _WIN32
#define COLOR_ERROR ""
#define COLOR_RESET ""
#else
#define COLOR_ERROR "\x1b[93;41m"
#define COLOR_RESET "\x1b[0m"
#endif

#define EXPECT(b, msg) \
	do { \
		if (!(b)) { \
			fprintf(stderr, COLOR_ERROR "%s" COLOR_RESET "\n", msg); \
			return false; \
		} \
	} while (0)

#define EXPECT_ASSERT(b, msg) \
	do { \
		if (!(b)) { \
			fprintf(stderr, COLOR_ERROR "%s" COLOR_RESET "\n", msg); \
			assert(0); \
		} \
	} while (0)

#ifdef _WIN32
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#define STUB() printf("%s\n", __PRETTY_FUNCTION__)

class MaterialVK;
class ConstantBufferVK;
class VertexBufferVK;
class Texture2DVK;
class Sampler2DVK;
class MeshVK;
class TechniqueVK;

struct EasyCommandQueue;

class VulkanRenderer : public Renderer {
	friend MaterialVK;
	friend ConstantBufferVK;
	friend VertexBufferVK;
	friend Texture2DVK;
	friend Sampler2DVK;
	friend MeshVK;
	friend TechniqueVK;

	friend EasyCommandQueue;

public:
	VulkanRenderer();
	virtual ~VulkanRenderer();

	Material* makeMaterial(const std::string& name) final;
	Mesh* makeMesh() final;
	// VertexBuffer* makeVertexBuffer();
	VertexBuffer* makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage) final;
	ConstantBuffer* makeConstantBuffer(std::string name, unsigned int location) final;
	// ResourceBinding* makeResourceBinding();
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
	// void setRenderTarget(RenderTarget* rt); // complete parameters
	void setRenderState(RenderState* ps) final;
	void submit(Mesh* mesh) final;
	void frame() final;
	void present() final;

	EasyCommandQueue acquireEasyCommandQueue();

	void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
	void createImage(uint32_t width,
	                 uint32_t height,
	                 vk::Format format,
	                 vk::ImageTiling tiling,
	                 vk::ImageUsageFlags usage,
	                 vk::MemoryPropertyFlags properties,
	                 vk::Image& image,
	                 vk::DeviceMemory& imageMemory);

private:
	struct InitFunction {
		typedef bool (VulkanRenderer::*initFunction)();
		std::string name;
		initFunction function;
		bool rebuild;
	};

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

	static const std::vector<InitFunction> _inits;

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

	vk::Image _depthImage;
	vk::DeviceMemory _depthImageMemory;
	vk::ImageView _depthImageView;

	vk::RenderPass _renderPass;
	std::vector<vk::DescriptorSetLayout> _descriptorSetLayouts;
	std::vector<vk::DescriptorSetLayout> _descriptorTextureSetLayouts;
	vk::DescriptorPool _descriptorPool;

	struct BasePipelineInfo {
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
		vk::Viewport viewport;
		vk::Rect2D scissor;
		vk::PipelineViewportStateCreateInfo viewportState;
		vk::PipelineRasterizationStateCreateInfo rasterizer;
		vk::PipelineMultisampleStateCreateInfo multisampling;
		vk::PipelineColorBlendAttachmentState colorBlendAttachment;
		vk::PipelineColorBlendStateCreateInfo colorBlending;
		vk::PipelineDepthStencilStateCreateInfo depthStencil;

		vk::GraphicsPipelineCreateInfo pipelineInfo;
	};
	BasePipelineInfo _basePipelineInfo;

	std::vector<vk::Framebuffer> _swapChainFramebuffers;
	vk::CommandPool _commandPool;
	std::vector<vk::CommandBuffer> _commandBuffers;

	uint32_t _currentImageIndex;
	vk::Semaphore _imageAvailableSemaphore;
	vk::Semaphore _renderFinishedSemaphore;

	std::unordered_map<Technique*, std::vector<MeshVK*>> _drawList;

	bool _globalWireframeMode = false;
	float _clearColor[4] = {0, 0, 0, 0};

	uint32_t _findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
	vk::Format _findDepthFormat();

	void _cleanupSwapChain();
	void _recreateSwapChain();

	bool _initSDL();
	bool _createVulkanInstance();
	bool _createSDLSurface();
	bool _createVulkanPhysicalDevice();
	bool _createVulkanLogicalDevice();
	bool _createVulkanSwapChain();
	bool _createVulkanImageViews();
	bool _createVulkanRenderPass();
	bool _createVulkanPipeline();
	bool _createDescriptorPool();
	bool _createDescriptorSets();
	bool _createVulkanCommandPool();
	bool _createVulkanDepthResources();
	bool _createVulkanFramebuffers();
	bool _createVulkanCommandBuffers();
	bool _createVulkanSemaphores();
};

struct EasyCommandQueue {
public:
	EasyCommandQueue(VulkanRenderer* renderer, vk::Queue queue);
	EasyCommandQueue(const EasyCommandQueue& that) = delete;
	EasyCommandQueue(EasyCommandQueue&&) = default;
	EasyCommandQueue& operator=(EasyCommandQueue&&) = default;
	~EasyCommandQueue();

	void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
	void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);

	void run();

private:
	VulkanRenderer* _renderer;
	vk::Device _device;
	vk::CommandBuffer _commandBuffer;
	vk::Queue _queue;
};
