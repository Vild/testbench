#include "VulkanRenderer.h"

#include "ConstantBufferVK.h"
#include "MaterialVK.h"
#include "RenderStateVK.h"
#include "Sampler2DVK.h"
#include "Texture2DVK.h"
#include "TransformVK.h"
#include "VertexBufferVK.h"
#include "MeshVK.h"

#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_vulkan.h>
#include <cstdint>
#include <cstdio>

#ifdef _WIN32
#define ASSETS_FOLDER "../assets"
#else
#define ASSETS_FOLDER "assets"
#endif

#ifndef NDEBUG
#define VK_LAYER_LUNARG_standard_validation "VK_LAYER_LUNARG_standard_validation"

#define DEBUG_LAYER VK_LAYER_LUNARG_standard_validation,
#define DEBUG_EXTENSION VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#else
#define DEBUG_LAYER
#define DEBUG_EXTENSION
#endif

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

const std::vector<VulkanRenderer::InitFunction> VulkanRenderer::_inits = {
  {"initSDL", &VulkanRenderer::_initSDL, false},
  {"createVulkanInstance", &VulkanRenderer::_createVulkanInstance, false},
  {"createSDLSurface", &VulkanRenderer::_createSDLSurface, false},
  {"createVulkanPhysicalDevice", &VulkanRenderer::_createVulkanPhysicalDevice, false},
  {"createVulkanLogicalDevice", &VulkanRenderer::_createVulkanLogicalDevice, false},
  {"createVulkanSwapChain", &VulkanRenderer::_createVulkanSwapChain, true},
  {"createVulkanImageViews", &VulkanRenderer::_createVulkanImageViews, true},
  {"createVulkanRenderPass", &VulkanRenderer::_createVulkanRenderPass, true},
  {"createVulkanPipeline", &VulkanRenderer::_createVulkanPipeline, true},
  {"createVulkanFramebuffers", &VulkanRenderer::_createVulkanFramebuffers, true},
  {"createVulkanCommandPool", &VulkanRenderer::_createVulkanCommandPool, false},
  {"createVulkanCommandBuffers", &VulkanRenderer::_createVulkanCommandBuffers, true},
  {"createVulkanSemaphores", &VulkanRenderer::_createVulkanSemaphores, false},
};

static VkResult CreateDebugReportCallbackEXT(VkInstance instance,
                                             const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
                                             const VkAllocationCallbacks* pAllocator,
                                             VkDebugReportCallbackEXT* pCallback) {
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr)
		return func(instance, pCreateInfo, pAllocator, pCallback);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr)
		func(instance, callback, pAllocator);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(VkDebugReportFlagsEXT flags,
                                                          VkDebugReportObjectTypeEXT objType,
                                                          uint64_t obj,
                                                          size_t location,
                                                          int32_t code,
                                                          const char* layerPrefix,
                                                          const char* msg,
                                                          void* userData) {
	fprintf(stderr, "Validation layer: %s\n", msg);

	return VK_FALSE;
}

VulkanRenderer::VulkanRenderer() {}
VulkanRenderer::~VulkanRenderer() {}

Material* VulkanRenderer::makeMaterial(const std::string& name) {
	return new MaterialVK(this, name);
}
Mesh* VulkanRenderer::makeMesh() {
	return new MeshVK();
}
// VertexBuffer* VulkanRenderer::makeVertexBuffer();
VertexBuffer* VulkanRenderer::makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage) {
	return new VertexBufferVK(this, size, usage);
}
ConstantBuffer* VulkanRenderer::makeConstantBuffer(std::string name, unsigned int location) {
	return new ConstantBufferVK(this, name, location);
}
//	ResourceBinding* VulkanRenderer::makeResourceBinding();
RenderState* VulkanRenderer::makeRenderState() {
	RenderStateVK* newRS = new RenderStateVK();
	newRS->setGlobalWireFrame(&_globalWireframeMode);
	newRS->setWireFrame(false);
	return (RenderState*)newRS;
}
Technique* VulkanRenderer::makeTechnique(Material* m, RenderState* r) {
	return new Technique(m, r);
}
Texture2D* VulkanRenderer::makeTexture2D() {
	return new Texture2DVK(this);
}
Sampler2D* VulkanRenderer::makeSampler2D() {
	return new Sampler2DVK(this);
}
std::string VulkanRenderer::getShaderPath() {
	return ASSETS_FOLDER "/VK/";
}
std::string VulkanRenderer::getShaderExtension() {
	return ".glsl";
}

int VulkanRenderer::initialize(unsigned int width, unsigned int height) {
	_width = width;
	_height = height;

	for (auto init : _inits) {
		printf("Running %s():\n", init.name.c_str());
		try {
			bool r = (this->*(init.function))();
			EXPECT(r, "\tINIT FAILED!");
		} catch (const std::exception&) {
			EXPECT(0, "\tINIT FAILED!");
		}
	}
	return true;
}
void VulkanRenderer::setWinTitle(const char* title) {
	SDL_SetWindowTitle(_window, title);
}
int VulkanRenderer::shutdown() {
	_device.waitIdle();
	_cleanupSwapChain();

	_device.destroySemaphore(_renderFinishedSemaphore);
	_device.destroySemaphore(_imageAvailableSemaphore);

	_device.destroyCommandPool(_commandPool);

	_device.destroySwapchainKHR(_swapChain);
	_device.destroy();
	DestroyDebugReportCallbackEXT(_instance, _debugCallback, nullptr);
	_instance.destroy();
	SDL_DestroyWindow(_window);
	SDL_Vulkan_UnloadLibrary();
	SDL_Quit();
	return 0;
}

void VulkanRenderer::setClearColor(float r, float g, float b, float a) {
	_clearColor[0] = r;
	_clearColor[1] = g;
	_clearColor[2] = b;
	_clearColor[3] = a;
}

void VulkanRenderer::clearBuffer(unsigned int) {
	// TODO: NOP?
}
//	void VulkanRenderer::setRenderTarget(RenderTarget* rt); // complete parameters
void VulkanRenderer::setRenderState(RenderState* ps) {
	ps->set();
}
void VulkanRenderer::submit(Mesh* mesh) {
	_drawList[mesh->technique].push_back(mesh);
}
void VulkanRenderer::frame() {
	vk::ResultValue<uint32_t> rv = _device.acquireNextImageKHR(_swapChain, std::numeric_limits<uint64_t>::max(), _imageAvailableSemaphore, vk::Fence());
	if (rv.result == vk::Result::eErrorOutOfDateKHR) {
		_currentImageIndex = UINT32_MAX;
		return _recreateSwapChain();
	}
	EXPECT_ASSERT(rv.result == vk::Result::eSuccess || rv.result == vk::Result::eSuboptimalKHR, "Failed to acquire new image");
	_currentImageIndex = rv.value;

	vk::Semaphore waitSemaphores[] = {_imageAvailableSemaphore};
	vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
	vk::Semaphore signalSemaphores[] = {_renderFinishedSemaphore};
	vk::SubmitInfo submitInfo{1, waitSemaphores, waitStages, 1, &_commandBuffers[_currentImageIndex], 1, signalSemaphores};
	_graphicsQueue.submit(1, &submitInfo, vk::Fence());
}
void VulkanRenderer::present() {
	if (_currentImageIndex == UINT32_MAX)
		return;
	_presentQueue.waitIdle();
	vk::Semaphore signalSemaphores[] = {_renderFinishedSemaphore};
	vk::SwapchainKHR swapChains[] = {_swapChain};
	vk::PresentInfoKHR presentInfo{1, signalSemaphores, 1, swapChains, &_currentImageIndex};
	vk::Result r = _presentQueue.presentKHR(presentInfo);
	if (r == vk::Result::eErrorOutOfDateKHR || r == vk::Result::eSuboptimalKHR)
		return _recreateSwapChain();

	EXPECT_ASSERT(r == vk::Result::eSuccess, "Failed to present");
}

void VulkanRenderer::_cleanupSwapChain() {
	for (auto framebuffer : _swapChainFramebuffers)
		_device.destroyFramebuffer(framebuffer);

	_device.freeCommandBuffers(_commandPool, (uint32_t)_commandBuffers.size(), _commandBuffers.data());

	_device.destroyPipeline(_graphicsPipeline);
	_device.destroyPipelineLayout(_pipelineLayout);
	_device.destroyRenderPass(_renderPass);

	for (vk::ImageView& iv : _swapChainImageViews)
		_device.destroyImageView(iv);

	_device.destroySwapchainKHR(_swapChain);
}

void VulkanRenderer::_recreateSwapChain() {
	_device.waitIdle();
	_cleanupSwapChain();

	for (auto init : _inits) {
		if (!init.rebuild)
			continue;
		printf("Running %s():\n", init.name.c_str());
		try {
			bool r = (this->*(init.function))();
			EXPECT_ASSERT(r, "\tREBUILD FAILED!");
		} catch (const std::exception&) {
			EXPECT_ASSERT(0, "\tREBUILD FAILED!");
		}
	}
}

bool VulkanRenderer::_initSDL() {
	EXPECT(!SDL_Init(SDL_INIT_EVERYTHING), SDL_GetError());
	EXPECT(!SDL_Vulkan_LoadLibrary(nullptr), SDL_GetError());

	_window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _width, _height, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
	EXPECT(_window, SDL_GetError());

	return true;
}

bool VulkanRenderer::_createVulkanInstance() {
	printf("Available Extensions:\n");
	auto availableExtensions = vk::enumerateInstanceExtensionProperties();
	for (const vk::ExtensionProperties& e : availableExtensions)
		printf("\t%s Version %d.%d.%d\n", e.extensionName, VK_VERSION_MAJOR(e.specVersion), VK_VERSION_MINOR(e.specVersion), VK_VERSION_PATCH(e.specVersion));

	printf("Available Layers:\n");
	auto availableLayers = vk::enumerateInstanceLayerProperties();
	for (const vk::LayerProperties& l : availableLayers)
		printf("\t%s: Version %d.%d.%d, ImplVersion %d.%d.%d\n\t\t%s\n", l.layerName, VK_VERSION_MAJOR(l.specVersion), VK_VERSION_MINOR(l.specVersion),
		       VK_VERSION_PATCH(l.specVersion), VK_VERSION_MAJOR(l.implementationVersion), VK_VERSION_MINOR(l.implementationVersion),
		       VK_VERSION_PATCH(l.implementationVersion), l.description);

	std::vector<const char*> layers{DEBUG_LAYER};
	std::vector<const char*> extensions{DEBUG_EXTENSION};
	{
		uint32_t count;
		EXPECT(SDL_Vulkan_GetInstanceExtensions(_window, &count, nullptr), SDL_GetError());

		extensions.resize(count + extensions.size());

		EXPECT(SDL_Vulkan_GetInstanceExtensions(_window, &count, &extensions[1]), SDL_GetError());
	}

	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = "Bogdan das game";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Nilsong the motor";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 33, 7);
	appInfo.apiVersion = VK_API_VERSION_1_0;
	vk::InstanceCreateInfo createInfo;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledLayerCount = (uint32_t)layers.size();
	createInfo.ppEnabledLayerNames = layers.data();
	createInfo.enabledExtensionCount = (uint32_t)extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

	_instance = vk::createInstance(createInfo);
	EXPECT(_instance, "Failed to create instance!");

#ifndef NDEBUG
	// VK_LAYER_LUNARG_standard_validation HOOK callback
	VkDebugReportCallbackCreateInfoEXT createInfo2 = {};
	createInfo2.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo2.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo2.pfnCallback = &vulkanDebugCallback;
	// can fail. check for VK_SUCCESS
	VkDebugReportCallbackEXT debugCallback;
	CreateDebugReportCallbackEXT(_instance, &createInfo2, nullptr, &debugCallback);
	_debugCallback = debugCallback;
#endif

	return true;
}

bool VulkanRenderer::_createSDLSurface() {
	VkSurfaceKHR surface;
	bool r = SDL_Vulkan_CreateSurface(_window, _instance, &surface);
	_surface = surface;
	return r;
}

bool VulkanRenderer::_createVulkanPhysicalDevice() {
	auto physicalDevices = _instance.enumeratePhysicalDevices();

	auto isDeviceSuitable = [this](vk::PhysicalDevice device, QueueInformation& qi, SwapChainInformation& sci) -> bool {
		{
			vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
			vk::PhysicalDeviceFeatures deviceFeatures = device.getFeatures();

			if (!deviceFeatures.geometryShader)
				return false;

			auto queueFamilies = device.getQueueFamilyProperties();

			for (uint32_t i = 0; i < queueFamilies.size(); i++) {
				// TODO: Rate queues
				const vk::QueueFamilyProperties& q = queueFamilies[i];
				vk::Bool32 hasPresent;
				device.getSurfaceSupportKHR(i, _surface, &hasPresent);
				if (!q.queueCount)
					continue;
				if (q.queueFlags & vk::QueueFlagBits::eGraphics)
					qi.graphics = i;
				if (hasPresent)
					qi.present = i;
			}
		}

		{
			sci.capabilities = device.getSurfaceCapabilitiesKHR(_surface);
			sci.formats = device.getSurfaceFormatsKHR(_surface);
			sci.presentModes = device.getSurfacePresentModesKHR(_surface);
		}

		return qi.completed() && !sci.formats.empty() && !sci.presentModes.empty();
	};

	for (auto& device : physicalDevices) {
		QueueInformation qi;
		SwapChainInformation sci;
		if (isDeviceSuitable(device, qi, sci)) {
			_physicalDevice = device;
			_queueInformation = qi;
			_swapChainInformation = sci;
			break;
		}
	}
	EXPECT(_physicalDevice, "Failed to create physical device!");
	return true;
}

bool VulkanRenderer::_createVulkanLogicalDevice() {
	float queuePriority = 1.0f;

	// TODO: if queue index is same, only create one CreateInfo with count of two
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{
	  vk::DeviceQueueCreateInfo{vk::DeviceQueueCreateFlags(), _queueInformation.graphics, 1, &queuePriority},
	  vk::DeviceQueueCreateInfo{vk::DeviceQueueCreateFlags(), _queueInformation.present, 1, &queuePriority},
	};
	std::vector<const char*> layers{DEBUG_LAYER};
	std::vector<const char*> extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	vk::PhysicalDeviceFeatures deviceFeatures;
	vk::DeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.enabledLayerCount = (uint32_t)layers.size();
	deviceCreateInfo.ppEnabledLayerNames = layers.data();
	deviceCreateInfo.enabledExtensionCount = (uint32_t)extensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = extensions.data();
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	_device = _physicalDevice.createDevice(deviceCreateInfo);
	EXPECT(_device, "Create device failed!");
	_graphicsQueue = _device.getQueue(_queueInformation.graphics, 0);
	EXPECT(_graphicsQueue, "Graphic queue is null!");
	_presentQueue = _device.getQueue(_queueInformation.present, 0);
	EXPECT(_presentQueue, "Present queue is null!");

	return true;
}

bool VulkanRenderer::_createVulkanSwapChain() {
	vk::SurfaceFormatKHR surfaceFormat = [](const std::vector<vk::SurfaceFormatKHR>& formats) {
		if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined) {
			vk::SurfaceFormatKHR iHateYouVulkanHPPWhyDontYouHaveAConstructorForThis;
			iHateYouVulkanHPPWhyDontYouHaveAConstructorForThis.format = vk::Format::eB8G8R8Unorm;
			iHateYouVulkanHPPWhyDontYouHaveAConstructorForThis.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
			return iHateYouVulkanHPPWhyDontYouHaveAConstructorForThis;
		}

		for (const vk::SurfaceFormatKHR& f : formats)
			if (f.format == vk::Format::eB8G8R8A8Unorm && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
				return f;

		return formats[0];
	}(_swapChainInformation.formats);
	_swapChainImageFormat = surfaceFormat.format;

	vk::PresentModeKHR presentMode = [](const std::vector<vk::PresentModeKHR>& presentModes) {
		vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;
		for (const vk::PresentModeKHR& pm : presentModes)
			if (pm == vk::PresentModeKHR::eMailbox)
				return pm;
			else if (pm == vk::PresentModeKHR::eImmediate)
				bestMode = pm;
		return bestMode;
	}(_swapChainInformation.presentModes);

	vk::Extent2D extent = [this](const vk::SurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		} else {
			vk::Extent2D actualExtent = {_width, _height};

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}(_swapChainInformation.capabilities);
	_swapChainExtent = extent;

	uint32_t imageCount = _swapChainInformation.capabilities.minImageCount + 1;
	if (_swapChainInformation.capabilities.maxImageCount > 0 && imageCount > _swapChainInformation.capabilities.maxImageCount)
		imageCount = _swapChainInformation.capabilities.maxImageCount;

	vk::SwapchainCreateInfoKHR createInfo;
	createInfo.surface = _surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

	uint32_t queueFamilyIndices[] = {_queueInformation.graphics, _queueInformation.present};

	if (_queueInformation.graphics != _queueInformation.present) {
		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
		createInfo.queueFamilyIndexCount = 0;     // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = _swapChainInformation.capabilities.currentTransform;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	createInfo.presentMode = presentMode;
	createInfo.clipped = true;
	createInfo.oldSwapchain = vk::SwapchainKHR();

	_swapChain = _device.createSwapchainKHR(createInfo, nullptr);
	EXPECT(_swapChain, "Create swapchain failed!");

	_swapChainImages = _device.getSwapchainImagesKHR(_swapChain);

	_swapChainImageFormat = surfaceFormat.format;
	_swapChainExtent = extent;
	return true;
}

bool VulkanRenderer::_createVulkanImageViews() {
	_swapChainImageViews.resize(_swapChainImages.size());

	for (size_t i = 0; i < _swapChainImages.size(); i++) {
		vk::ImageViewCreateInfo createInfo;
		createInfo.image = _swapChainImages[i];
		createInfo.viewType = vk::ImageViewType::e2D;
		createInfo.format = _swapChainImageFormat;
		createInfo.components.r = vk::ComponentSwizzle::eIdentity;
		createInfo.components.g = vk::ComponentSwizzle::eIdentity;
		createInfo.components.b = vk::ComponentSwizzle::eIdentity;
		createInfo.components.a = vk::ComponentSwizzle::eIdentity;
		createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		_swapChainImageViews[i] = _device.createImageView(createInfo);
	}
	return true;
}

bool VulkanRenderer::_createVulkanRenderPass() {
	vk::AttachmentDescription colorAttachment;
	colorAttachment.format = _swapChainImageFormat;
	colorAttachment.samples = vk::SampleCountFlagBits::e1;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

	vk::AttachmentReference colorAttachmentRef;
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	vk::SubpassDependency dependency;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	// dependency.dstSubpass = vk::PipelineStageFlags();
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	// dependency.srcAccessMask = vk::PipelineStageFlags();
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	_renderPass = _device.createRenderPass(renderPassInfo);
	EXPECT(_renderPass, "Create renderpass failed!");
	return true;
}

bool VulkanRenderer::_createVulkanPipeline() {
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembly.primitiveRestartEnable = false;

	vk::Viewport viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)_swapChainExtent.width;
	viewport.height = (float)_swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor;
	scissor.offset = vk::Offset2D{0, 0};
	scissor.extent = _swapChainExtent;

	vk::PipelineViewportStateCreateInfo viewportState;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	vk::PipelineRasterizationStateCreateInfo rasterizer;
	rasterizer.depthClampEnable = false;
	rasterizer.rasterizerDiscardEnable = false;
	rasterizer.polygonMode = vk::PolygonMode::eFill;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = vk::CullModeFlagBits::eBack;
	rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
	rasterizer.depthBiasEnable = false;

	vk::PipelineMultisampleStateCreateInfo multisampling{{}, vk::SampleCountFlagBits::e1, false};

	vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
	                                       vk::ColorComponentFlagBits::eA);

	vk::PipelineColorBlendStateCreateInfo colorBlending{{}, false, vk::LogicOp::eCopy, 1, &colorBlendAttachment, {{0, 0, 0, 0}}};

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo{{}, 0, nullptr, 0, nullptr};

	_pipelineLayout = _device.createPipelineLayout(pipelineLayoutInfo);
	EXPECT(_pipelineLayout, "Pipeline Layout is null!");

	MaterialVK m{this, "Test"};
	m.setShader(getShaderPath() + "VertexShader.glsl", Material::ShaderType::VS);
	m.setShader(getShaderPath() + "FragmentShader.glsl", Material::ShaderType::PS);

	std::string definePos = "#define POSITION " + std::to_string(POSITION) + "\n";
	std::string defineNor = "#define NORMAL " + std::to_string(NORMAL) + "\n";
	std::string defineUV = "#define TEXTCOORD " + std::to_string(TEXTCOORD) + "\n";

	std::string defineTX = "#define TRANSLATION " + std::to_string(TRANSLATION) + "\n";
	std::string defineTXName = "#define TRANSLATION_NAME " + std::string(TRANSLATION_NAME) + "\n";

	std::string defineDiffCol = "#define DIFFUSE_TINT " + std::to_string(DIFFUSE_TINT) + "\n";
	std::string defineDiffColName = "#define DIFFUSE_TINT_NAME " + std::string(DIFFUSE_TINT_NAME) + "\n";

	std::string defineDiffuse = "#define DIFFUSE_SLOT " + std::to_string(DIFFUSE_SLOT) + "\n";
	m.addDefine(definePos + defineNor + defineUV + defineTX + defineTXName + defineDiffCol + defineDiffColName, Material::ShaderType::VS);
	m.addDefine(definePos + defineNor + defineUV + defineTX + defineTXName + defineDiffCol + defineDiffColName, Material::ShaderType::PS);

	std::string err;
	m.compileMaterial(err);

	vk::GraphicsPipelineCreateInfo pipelineInfo{{},
	                                            (uint32_t)m._program.size(),
	                                            m._program.data(),
	                                            &vertexInputInfo,
	                                            &inputAssembly,
	                                            nullptr,
	                                            &viewportState,
	                                            &rasterizer,
	                                            &multisampling,
	                                            nullptr,
	                                            &colorBlending,
	                                            nullptr,
	                                            _pipelineLayout,
	                                            _renderPass,
	                                            0,
	                                            vk::Pipeline(),
	                                            -1};

	_graphicsPipeline = _device.createGraphicsPipeline(vk::PipelineCache(), pipelineInfo);
	EXPECT(_graphicsPipeline, "Can't create Graphics pipeline!");

	return true;
}

bool VulkanRenderer::_createVulkanFramebuffers() {
	_swapChainFramebuffers.resize(_swapChainImageViews.size());
	for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
		vk::ImageView attachments[] = {_swapChainImageViews[i]};

		vk::FramebufferCreateInfo framebufferInfo = {{}, _renderPass, 1, attachments, _swapChainExtent.width, _swapChainExtent.height, 1};

		_swapChainFramebuffers[i] = _device.createFramebuffer(framebufferInfo);
		EXPECT(_swapChainFramebuffers[i], "Failed to create framebuffer!");
	}
	return true;
}

bool VulkanRenderer::_createVulkanCommandPool() {
	vk::CommandPoolCreateInfo createInfo{{}, _queueInformation.graphics};
	_commandPool = _device.createCommandPool(createInfo);
	EXPECT(_commandPool, "Failed to create command pool!");
	return true;
}

bool VulkanRenderer::_createVulkanCommandBuffers() {
	vk::CommandBufferAllocateInfo allocInfo{_commandPool, vk::CommandBufferLevel::ePrimary, (uint32_t)_swapChainFramebuffers.size()};

	_commandBuffers = _device.allocateCommandBuffers(allocInfo);
	EXPECT(_commandBuffers.size(), "Failed to allocate command buffers");

	for (size_t i = 0; i < _commandBuffers.size(); i++) {
		vk::CommandBufferBeginInfo beginInfo{vk::CommandBufferUsageFlagBits::eSimultaneousUse};

		_commandBuffers[i].begin(beginInfo);

		vk::ClearColorValue c = vk::ClearColorValue{std::array<float, 4>{{0.0f, 0.0f, 0.0f, 1.0f}}};
		vk::ClearValue clearColor{c};
		vk::RenderPassBeginInfo renderPassInfo = {_renderPass, _swapChainFramebuffers[i], {{0, 0}, _swapChainExtent}, 1, &clearColor};
		_commandBuffers[i].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

		_commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, _graphicsPipeline);
		_commandBuffers[i].draw(3, 1, 0, 0);
		_commandBuffers[i].endRenderPass();
		_commandBuffers[i].end();
	}
	return true;
}

bool VulkanRenderer::_createVulkanSemaphores() {
	vk::SemaphoreCreateInfo semaphoreInfo{};
	_imageAvailableSemaphore = _device.createSemaphore(semaphoreInfo);
	EXPECT(_imageAvailableSemaphore, "Failed to create semaphore!");
	_renderFinishedSemaphore = _device.createSemaphore(semaphoreInfo);
	EXPECT(_renderFinishedSemaphore, "Failed to create semaphore!");
	return true;
}
