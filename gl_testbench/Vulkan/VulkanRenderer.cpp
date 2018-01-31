#include "VulkanRenderer.h"

#include "ConstantBufferVK.h"
#include "MaterialVK.h"
#include "RenderStateVK.h"
#include "ResourceBindingVK.h"
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

#ifdef NDEBUG
#define DEBUG
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

Material* VulkanRenderer::makeMaterial(const std::string& name) { return new MaterialVK(name); }
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
std::string VulkanRenderer::getShaderExtension() { return ".spv"; }

int VulkanRenderer::initialize(unsigned int width, unsigned int height) {
	_width = width;
	_height = height;
	for (auto init : _inits)
		if (!(this->*init)())
			return false;
	return true;
}
void VulkanRenderer::setWinTitle(const char* title) {
	SDL_SetWindowTitle(_window, title);
}
int VulkanRenderer::shutdown() {
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
		fprintf(stderr, "%s", SDL_GetError());
		exit(-1);
	}
	SDL_Vulkan_LoadLibrary(nullptr);

	_window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _width, _height, SDL_WINDOW_SHOWN);

	return true;
}

bool VulkanRenderer::_initVulkanInstance() {
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
		if (!SDL_Vulkan_GetInstanceExtensions(_window, &count, nullptr))
			return false;

		extensions.resize(count + extensions.size());

		if (!SDL_Vulkan_GetInstanceExtensions(_window, &count, &extensions[1]))
			return false;
	}

	vk::ApplicationInfo appinfo{ "Bogdan", VK_MAKE_VERSION(1, 0, 0), "Nilsong", VK_MAKE_VERSION(1, 33, 7), VK_API_VERSION_1_0 };
	vk::InstanceCreateInfo createInfo{ vk::InstanceCreateFlags(), &appinfo, (uint32_t)layers.size(), layers.data(), (uint32_t)extensions.size(), extensions.data() };

	_instance = vk::createInstance(createInfo);

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

bool VulkanRenderer::_initSDLSurface() {
	VkSurfaceKHR surface;
	bool r = !SDL_Vulkan_CreateSurface(_window, _instance, &surface);
	_surface = surface;
	return r;
}

bool VulkanRenderer::_initVulkanPhysicalDevice() {
	auto physicalDevices = _instance.enumeratePhysicalDevices();

	auto isDeviceSuitable = [](vk::PhysicalDevice device, QueueInformation & qi) -> bool {
		vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
		vk::PhysicalDeviceFeatures deviceFeatures = device.getFeatures();

		if (deviceProperties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu || deviceFeatures.geometryShader)
			return false;

		auto queueFamilies = device.getQueueFamilyProperties();

		for (uint32_t i = 0; i < queueFamilies.size(); i++) {
			// TODO: Rate queues
			const vk::QueueFamilyProperties & q = queueFamilies[i];
			if (!q.queueCount)
				continue;
			if (q.queueFlags & vk::QueueFlagBits::eGraphics)
				qi.graphics = i;
		}

		return qi.completed();
	};

	for (auto& device : physicalDevices) {
		QueueInformation qi;
		if (isDeviceSuitable(device, qi)) {
			_physicalDevice = device;
			_queueInformation = qi;
			break;
		}
	}
	assert(_physicalDevice);
	return true;
}

bool VulkanRenderer::_initVulkanLogicalDevice() {
	vk::DeviceQueueCreateInfo queueCreateInfo{ vk::DeviceQueueCreateFlags(), _queueInformation.graphics, 1 };
	std::vector<const char *> layers{
		DEBUG_LAYER
	};
	std::vector<const char *> extensions{};

	vk::DeviceCreateInfo deviceCreateInfo{ vk::DeviceCreateFlags(), 1, &queueCreateInfo, (uint32_t)layers.size(), layers.data(), (uint32_t)extensions.size(), extensions.data() };

	_device = _physicalDevice.createDevice(deviceCreateInfo);
	_graphicsQueue =	_device.getQueue(_queueInformation.graphics, 0);

	return true;
}
