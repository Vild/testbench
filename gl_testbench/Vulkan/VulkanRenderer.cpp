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

#ifdef DEBUG
#define VK_LAYER_LUNARG_standard_validation "VK_LAYER_LUNARG_standard_validation"

#define DEBUG_LAYER VK_LAYER_LUNARG_standard_validation,
#define DEBUG_EXTENSION VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#else
#define DEBUG_LAYER
#define DEBUG_EXTENSION
#endif

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
VertexBuffer* VulkanRenderer::makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage) { return new VertexBufferVK(size, usage); }
ConstantBuffer* VulkanRenderer::makeConstantBuffer(std::string NAME, unsigned int location) { return new ConstantBufferVK(NAME, location); }
//	ResourceBinding* VulkanRenderer::makeResourceBinding();
RenderState* VulkanRenderer::makeRenderState() { return new RenderStateVK(); }
Technique* VulkanRenderer::makeTechnique(Material* m, RenderState* r) { return new Technique(m, r); }
Texture2D* VulkanRenderer::makeTexture2D() { return new Texture2DVK(); }
Sampler2D* VulkanRenderer::makeSampler2D() { return new Sampler2DVK(); }
std::string VulkanRenderer::getShaderPath() { return ASSETS_FOLDER "/VK/"; }
std::string VulkanRenderer::getShaderExtension() { return ".glsl"; }

int VulkanRenderer::initialize(unsigned int width, unsigned int height) {
	_width = width;
	_height = height;
	for (auto init : _inits) {
		printf("Running %s():\n", init.name.c_str());
		const auto r = (this->*(init.function))();
		assert(r);
		if (!r)
			return false;
	}
	return true;
}
void VulkanRenderer::setWinTitle(const char* title) {
	SDL_SetWindowTitle(_window, title);
}
int VulkanRenderer::shutdown() {
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

void VulkanRenderer::setClearColor(float, float, float, float) {}
void VulkanRenderer::clearBuffer(unsigned int) {}
//	void VulkanRenderer::setRenderTarget(RenderTarget* rt); // complete parameters
void VulkanRenderer::setRenderState(RenderState* ps) {}
void VulkanRenderer::submit(Mesh* mesh) {}
void VulkanRenderer::frame() {}
void VulkanRenderer::present() {}


bool VulkanRenderer::_initSDL() {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		fprintf(stderr, "%s\n", SDL_GetError());
		return false;
	}
	SDL_Vulkan_LoadLibrary(nullptr);

	_window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _width, _height, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
	assert(_window);

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
		if (!SDL_Vulkan_GetInstanceExtensions(_window, &count, nullptr)) {
			fprintf(stderr, "%s\n", SDL_GetError());
			return false;
		}

		extensions.resize(count + extensions.size());

		if (!SDL_Vulkan_GetInstanceExtensions(_window, &count, &extensions[1])) {
			fprintf(stderr, "%s\n", SDL_GetError());
			return false;
		}
	}

	vk::ApplicationInfo appinfo{ "Bogdan", VK_MAKE_VERSION(1, 0, 0), "Nilsong", VK_MAKE_VERSION(1, 33, 7), VK_API_VERSION_1_0 };
	vk::InstanceCreateInfo createInfo{ vk::InstanceCreateFlags(), &appinfo, (uint32_t)layers.size(), layers.data(), (uint32_t)extensions.size(), extensions.data() };

	_instance = vk::createInstance(createInfo);
	assert(_instance);

#ifdef DEBUG
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
	assert(_physicalDevice);
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
	assert(_device);
	_graphicsQueue =	_device.getQueue(_queueInformation.graphics, 0);
	assert(_graphicsQueue);
	_presentQueue =	_device.getQueue(_queueInformation.present, 0);
	assert(_presentQueue);

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
	assert(_swapChain);

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
