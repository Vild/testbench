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

#define EXPECT(b, msg) do { if (!(b)) { fprintf(stderr, "%s\n", msg); return false; } } while(0)

static VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
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

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
	VkDebugReportFlagsEXT flags,
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

VulkanRenderer::VulkanRenderer() {
}
VulkanRenderer::~VulkanRenderer() {
}

Material* VulkanRenderer::makeMaterial(const std::string& name) { return new MaterialVK(this, name); }
Mesh* VulkanRenderer::makeMesh() { return new MeshVK(); }
//VertexBuffer* VulkanRenderer::makeVertexBuffer();
VertexBuffer* VulkanRenderer::makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage) { return new VertexBufferVK(this, size, usage); }
ConstantBuffer* VulkanRenderer::makeConstantBuffer(std::string name, unsigned int location) { return new ConstantBufferVK(this, name, location); }
//	ResourceBinding* VulkanRenderer::makeResourceBinding();
RenderState* VulkanRenderer::makeRenderState() {
	RenderStateVK* newRS = new RenderStateVK();
	newRS->setGlobalWireFrame(&_globalWireframeMode);
	newRS->setWireFrame(false);
	return (RenderState*)newRS;
}
Technique* VulkanRenderer::makeTechnique(Material* m, RenderState* r) { return new Technique(m, r); }
Texture2D* VulkanRenderer::makeTexture2D() { return new Texture2DVK(this); }
Sampler2D* VulkanRenderer::makeSampler2D() { return new Sampler2DVK(this); }
std::string VulkanRenderer::getShaderPath() { return ASSETS_FOLDER "/VK/"; }
std::string VulkanRenderer::getShaderExtension() { return ".glsl"; }

int VulkanRenderer::initialize(unsigned int width, unsigned int height) {
	struct InitFunction {
		typedef bool(VulkanRenderer::*initFunction)();
		std::string name;
		initFunction function;
	};

	static const std::vector<InitFunction> _inits = {
		{"initSDL", &VulkanRenderer::_initSDL},
		{"createVulkanInstance", &VulkanRenderer::_createVulkanInstance},
		{"createSDLSurface", &VulkanRenderer::_createSDLSurface},
		{"createVulkanPhysicalDevice", &VulkanRenderer::_createVulkanPhysicalDevice},
		{"createVulkanLogicalDevice", &VulkanRenderer::_createVulkanLogicalDevice},
		{"createVulkanSwapChain", &VulkanRenderer::_createVulkanSwapChain},
		{"createVulkanImageViews", &VulkanRenderer::_createVulkanImageViews},
		{"createVulkanRenderPass", &VulkanRenderer::_createVulkanRenderPass},
		{"createVulkanPipeline", &VulkanRenderer::_createVulkanPipeline},
	};

	_width = width;
	_height = height;

	for (auto init : _inits) {
		printf("Running %s():\n", init.name.c_str());
		bool r = (this->*(init.function))();
		EXPECT(r, "\tINIT FAILED!");
	}
	return true;
}
void VulkanRenderer::setWinTitle(const char* title) {
	SDL_SetWindowTitle(_window, title);
}
int VulkanRenderer::shutdown() {
	_device.destroyPipeline(_graphicsPipeline);
	_device.destroyPipelineLayout(_pipelineLayout);
	_device.destroyRenderPass(_renderPass);

	for (vk::ImageView& iv : _swapChainImageViews)
		_device.destroyImageView(iv);

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
void VulkanRenderer::submit(Mesh* mesh) { _drawList[mesh->technique].push_back(mesh); }
void VulkanRenderer::frame() {
	// TODO: Create queues, if needed
}
void VulkanRenderer::present() {
	// TODO: Submit queues
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
		printf("\t%s Version %d.%d.%d\n", e.extensionName,
					 VK_VERSION_MAJOR(e.specVersion),
					 VK_VERSION_MINOR(e.specVersion),
					 VK_VERSION_PATCH(e.specVersion));

	printf("Available Layers:\n");
	auto availableLayers = vk::enumerateInstanceLayerProperties();
	for (const vk::LayerProperties& l : availableLayers)
		printf("\t%s: Version %d.%d.%d, ImplVersion %d.%d.%d\n\t\t%s\n", l.layerName,
					 VK_VERSION_MAJOR(l.specVersion),
					 VK_VERSION_MINOR(l.specVersion),
					 VK_VERSION_PATCH(l.specVersion),
					 VK_VERSION_MAJOR(l.implementationVersion),
					 VK_VERSION_MINOR(l.implementationVersion),
					 VK_VERSION_PATCH(l.implementationVersion),
					 l.description);

	std::vector<const char *> layers{
		DEBUG_LAYER
	};
	std::vector<const char *> extensions{
		DEBUG_EXTENSION
	};
	{
		uint32_t count;
		EXPECT(SDL_Vulkan_GetInstanceExtensions(_window, &count, nullptr), SDL_GetError());

		extensions.resize(count + extensions.size());

		EXPECT(SDL_Vulkan_GetInstanceExtensions(_window, &count, &extensions[1]), SDL_GetError());
	}

	vk::ApplicationInfo appinfo{ "Bogdan", VK_MAKE_VERSION(1, 0, 0), "Nilsong", VK_MAKE_VERSION(1, 33, 7), VK_API_VERSION_1_0 };
	vk::InstanceCreateInfo createInfo{ vk::InstanceCreateFlags(), &appinfo, (uint32_t)layers.size(), layers.data(), (uint32_t)extensions.size(), extensions.data() };

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

			if (deviceProperties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu || !deviceFeatures.geometryShader)
				return false;

			auto queueFamilies = device.getQueueFamilyProperties();

			for (uint32_t i = 0; i < queueFamilies.size(); i++) {
				// TODO: Rate queues
				const vk::QueueFamilyProperties & q = queueFamilies[i];
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
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos {
		{ vk::DeviceQueueCreateFlags(), _queueInformation.graphics, 1, &queuePriority },
		{ vk::DeviceQueueCreateFlags(), _queueInformation.present, 1, &queuePriority },
	};
	std::vector<const char *> layers{
		DEBUG_LAYER
	};
	std::vector<const char *> extensions{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	vk::DeviceCreateInfo deviceCreateInfo{ vk::DeviceCreateFlags(), (uint32_t)queueCreateInfos.size(), queueCreateInfos.data(), (uint32_t)layers.size(), layers.data(), (uint32_t)extensions.size(), extensions.data() };

	_device = _physicalDevice.createDevice(deviceCreateInfo);
	EXPECT(_device, "Create device failed!");
	_graphicsQueue =	_device.getQueue(_queueInformation.graphics, 0);
	EXPECT(_graphicsQueue, "Graphic queue is null!");
	_presentQueue =	_device.getQueue(_queueInformation.present, 0);
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
			vk::Extent2D actualExtent = { _width, _height };

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
    }
	}(_swapChainInformation.capabilities);
	_swapChainExtent = extent;

	uint32_t imageCount = _swapChainInformation.capabilities.minImageCount + 1;
	if (_swapChainInformation.capabilities.maxImageCount > 0 && imageCount > _swapChainInformation.capabilities.maxImageCount)
    imageCount = _swapChainInformation.capabilities.maxImageCount;

	vk::SwapchainCreateInfoKHR createInfo{ vk::SwapchainCreateFlagsKHR(), _surface, imageCount, surfaceFormat.format, surfaceFormat.colorSpace, extent, 1, vk::ImageUsageFlagBits::eColorAttachment };

	uint32_t queueFamilyIndices[] = {_queueInformation.graphics, _queueInformation.present};

	if (_queueInformation.graphics != _queueInformation.present) {
    createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
    createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    createInfo.queueFamilyIndexCount = 0; // Optional
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

	return true;
}

bool VulkanRenderer::_createVulkanImageViews() {
	_swapChainImageViews.resize(_swapChainImages.size());

	for (size_t i = 0; i < _swapChainImages.size(); i++) {
		vk::ImageViewCreateInfo createInfo{vk::ImageViewCreateFlags(), _swapChainImages[i], vk::ImageViewType::e2D, _swapChainImageFormat, vk::ComponentMapping{}, vk::ImageSubresourceRange{vk::ImageAspectFlags(vk::ImageAspectFlagBits::eColor), 0, 1, 0, 1}};
		_swapChainImageViews[i] = _device.createImageView(createInfo);
	}
	return true;
}

bool VulkanRenderer::_createVulkanRenderPass() {
	vk::AttachmentDescription colorAttachment{{}, _swapChainImageFormat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR };

	vk::AttachmentReference colorAttachmentRef{0, vk::ImageLayout::eColorAttachmentOptimal};

	vk::SubpassDescription subpass{{}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef};

	vk::RenderPassCreateInfo renderPassInfo{{}, 1, &colorAttachment, 1, &subpass};

	_renderPass = _device.createRenderPass(renderPassInfo);
	EXPECT(_renderPass, "Create renderpass failed!");
	return true;
}

bool VulkanRenderer::_createVulkanPipeline() {
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{{}, /* Binding */0, nullptr, /* Attribute */ 0, nullptr };

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly{{}, vk::PrimitiveTopology::eTriangleList, false };

	vk::Viewport viewport{0, 0, _swapChainExtent.width, _swapChainExtent.height, 0, 1};
  vk::Rect2D scissor{{0, 0}, _swapChainExtent};

	vk::PipelineViewportStateCreateInfo viewportState{{}, 1, &viewport, 1, &scissor};

	vk::PipelineRasterizationStateCreateInfo rasterizer{{}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise, false, 0, 0, 0, 1};

	vk::PipelineMultisampleStateCreateInfo multisampling{{}, vk::SampleCountFlagBits::e1, false};

	vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

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

	vk::GraphicsPipelineCreateInfo pipelineInfo{{}, (uint32_t)m._program.size(), m._program.data(), &vertexInputInfo, &inputAssembly, nullptr, &viewportState, &rasterizer, &multisampling, nullptr, &colorBlending, nullptr, _pipelineLayout, _renderPass, 0, vk::Pipeline(), -1};

	_graphicsPipeline = _device.createGraphicsPipeline(vk::PipelineCache(), pipelineInfo);
	EXPECT(_graphicsPipeline, "Can't create Graphics pipeline!");


	return true;
}
