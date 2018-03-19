#include "VulkanRenderer.h"

#include "ConstantBufferVK.h"
#include "MaterialVK.h"
#include "RenderStateVK.h"
#include "Sampler2DVK.h"
#include "Texture2DVK.h"
#include "TransformVK.h"
#include "VertexBufferVK.h"
#include "MeshVK.h"
#include "TechniqueVK.h"

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

#undef NDEBUG
#ifndef NDEBUG
#define VK_LAYER_LUNARG_standard_validation "VK_LAYER_LUNARG_standard_validation"

#define DEBUG_LAYER VK_LAYER_LUNARG_standard_validation,
#define DEBUG_EXTENSION VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#else
#define DEBUG_LAYER
#define DEBUG_EXTENSION
#endif

const std::vector<VulkanRenderer::InitFunction> VulkanRenderer::_inits = {{"initSDL", &VulkanRenderer::_initSDL, false},
                                                                          {"createVulkanInstance", &VulkanRenderer::_createVulkanInstance, false},
                                                                          {"createSDLSurface", &VulkanRenderer::_createSDLSurface, false},
                                                                          {"createVulkanPhysicalDevice", &VulkanRenderer::_createVulkanPhysicalDevice, false},
                                                                          {"createVulkanLogicalDevice", &VulkanRenderer::_createVulkanLogicalDevice, false},
                                                                          {"createVulkanSwapChain", &VulkanRenderer::_createVulkanSwapChain, true},
                                                                          {"createVulkanImageViews", &VulkanRenderer::_createVulkanImageViews, true},
                                                                          {"createVulkanRenderPass", &VulkanRenderer::_createVulkanRenderPass, true},
                                                                          {"createVulkanPipeline", &VulkanRenderer::_createVulkanPipeline, true},
                                                                          {"createVulkanCommandPool", &VulkanRenderer::_createVulkanCommandPool, false},
                                                                          {"createVulkanDepthResources", &VulkanRenderer::_createVulkanDepthResources, true},
                                                                          {"createVulkanFramebuffers", &VulkanRenderer::_createVulkanFramebuffers, true},
                                                                          {"createVulkanCommandBuffers", &VulkanRenderer::_createVulkanCommandBuffers, true},
                                                                          {"createVulkanSemaphores", &VulkanRenderer::_createVulkanSemaphores, false},
                                                                          {"createDescriptorPool", &VulkanRenderer::_createDescriptorPool, true}};

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

EasyCommandQueue::EasyCommandQueue(VulkanRenderer* renderer, vk::Queue queue) : _renderer(renderer), _device(renderer->_device), _queue(queue) {
	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandPool = renderer->_commandPool;
	allocInfo.commandBufferCount = 1;

	_commandBuffer = _device.allocateCommandBuffers(allocInfo)[0];

	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	_commandBuffer.begin(beginInfo);
}

EasyCommandQueue::~EasyCommandQueue() {
	_device.freeCommandBuffers(_renderer->_commandPool, 1, &_commandBuffer);
}

void EasyCommandQueue::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

		if (format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint)
			barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
	} else
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;

	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		barrier.srcAccessMask = vk::AccessFlags();
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	} else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	} else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		barrier.srcAccessMask = vk::AccessFlags();
		barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	} else
		EXPECT_ASSERT(0, "unsupported layout transition!");

	_commandBuffer.pipelineBarrier(sourceStage, destinationStage, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);
}

void EasyCommandQueue::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height) {
	vk::BufferImageCopy region;
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = vk::Offset3D{0, 0, 0};
	region.imageExtent = vk::Extent3D{width, height, 1};

	_commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);
}

void EasyCommandQueue::run() {
	_commandBuffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_commandBuffer;

	_queue.submit(1, &submitInfo, vk::Fence());
	_queue.waitIdle();
}

VulkanRenderer::VulkanRenderer() {}
VulkanRenderer::~VulkanRenderer() {}

Material* VulkanRenderer::makeMaterial(const std::string& name) {
	return new MaterialVK(this, name);
}
Mesh* VulkanRenderer::makeMesh() {
	return new MeshVK(this);
}
// VertexBuffer* VulkanRenderer::makeVertexBuffer();
VertexBuffer* VulkanRenderer::makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage) {
	return new VertexBufferVK(this, size /*, usage*/);
}
ConstantBuffer* VulkanRenderer::makeConstantBuffer(std::string name, unsigned int location) {
	return new ConstantBufferVK(this, /*name,*/ location);
}
//	ResourceBinding* VulkanRenderer::makeResourceBinding();
RenderState* VulkanRenderer::makeRenderState() {
	RenderStateVK* newRS = new RenderStateVK();
	newRS->setGlobalWireFrame(&_globalWireframeMode);
	newRS->setWireFrame(false);
	return (RenderState*)newRS;
}
Technique* VulkanRenderer::makeTechnique(Material* m, RenderState* r) {
	return new TechniqueVK(this, m, r);
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
	return 0;
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

	for (auto layout : _descriptorSetLayouts)
		_device.destroyDescriptorSetLayout(layout);

	_device.destroyDescriptorPool(_descriptorPool);

	_device.destroyPipelineLayout(_pipelineLayout);
	_device.destroyPipeline(_graphicsPipeline);

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

void VulkanRenderer::submitMap(EngineMap* map) {
	Renderer::submitMap(map);

	auto technique = static_cast<TechniqueVK*>(static_cast<MeshVK*>(map->meshes.begin()->second.mesh)->technique);
	auto material = static_cast<MaterialVK*>(technique->getMaterial());

	auto& bpi = _basePipelineInfo;
	auto& descriptorSetLayouts = _descriptorSetLayouts;

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.setLayoutCount = (uint32_t)descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

	_pipelineLayout = _device.createPipelineLayout(pipelineLayoutInfo);
	EXPECT_ASSERT(_pipelineLayout, "Pipeline Layout is null!");

	bpi.pipelineInfo.stageCount = (uint32_t) static_cast<MaterialVK*>(material)->_program.size();
	bpi.pipelineInfo.pStages = static_cast<MaterialVK*>(material)->_program.data();
	bpi.rasterizer.polygonMode = vk::PolygonMode::eFill;
	bpi.pipelineInfo.layout = _pipelineLayout;

	_graphicsPipeline = _device.createGraphicsPipeline(vk::PipelineCache(), bpi.pipelineInfo);
	EXPECT_ASSERT(_graphicsPipeline, "Can't create Graphics pipeline!");

	for (auto& it : map->meshes) {
		auto mesh = static_cast<MeshVK*>(it.second.mesh);

		auto technique = static_cast<TechniqueVK*>(mesh->technique);
		technique->enable();
		for (auto& cb : static_cast<MaterialVK*>(technique->getMaterial())->constantBuffers)
			cb.second->bindDescriptors(mesh);
		if (mesh->txBuffer)
			static_cast<ConstantBufferVK*>(mesh->txBuffer)->bindDescriptors(mesh);
		static_cast<ConstantBufferVK*>(mesh->cameraVPBuffer)->bindDescriptors(mesh);

		mesh->bindTextures();
		mesh->bindIAVertexBuffers();
	}

	// For each secondary commandbuffer
	for (int y = 0; y < ROOM_COUNT; y++)
		for (int x = 0; x < ROOM_COUNT; x++) {
			EngineRoom& r = map->rooms[y][x];
			vk::CommandBuffer& cb = _secondaryCommandBuffers[y * ROOM_COUNT + x];

			vk::CommandBufferInheritanceInfo inheritanceInfo{};
			inheritanceInfo.renderPass = _renderPass;
			//inheritanceInfo.subpass;
			//inheritanceInfo.framebuffer;
			inheritanceInfo.occlusionQueryEnable = false;
			//inheritanceInfo.queryFlags;
			//inheritanceInfo.pipelineStatistics;
			vk::CommandBufferBeginInfo beginInfo{vk::CommandBufferUsageFlagBits::eRenderPassContinue | vk::CommandBufferUsageFlagBits::eSimultaneousUse, &inheritanceInfo};
			cb.begin(beginInfo);
			cb.bindPipeline(vk::PipelineBindPoint::eGraphics, _graphicsPipeline);

			for (auto& m : r.meshes)
				for (int id : m.second) {
					auto mesh = static_cast<MeshVK*>(map->meshes[m.first].mesh);

					size_t numberOfElements = mesh->geometryBuffers[INDEX].numElements;
					cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, (uint32_t)mesh->descriptorSets.size(), mesh->descriptorSets.data(), 0, nullptr);
					cb.draw(numberOfElements, 1, 0, id);
				}

			cb.end();
		}

	for (size_t frame = 0; frame < _swapChainFramebuffers.size(); frame++)
		for (int y = 0; y < ROOM_COUNT; y++)
			for (int x = 0; x < ROOM_COUNT; x++) {
				EngineRoom& r = map->rooms[y][x];
				vk::CommandBuffer& cb = _primaryCommandBuffers[frame * ROOM_COUNT * ROOM_COUNT + y * ROOM_COUNT + x];

				vk::CommandBufferBeginInfo beginInfo{vk::CommandBufferUsageFlagBits::eSimultaneousUse};
				cb.begin(beginInfo);
				vk::ClearValue clearColor[2] = {vk::ClearColorValue{std::array<float, 4>{{_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]}}}, vk::ClearDepthStencilValue{1.0f, 0}};
				vk::RenderPassBeginInfo renderPassInfo = {_renderPass, _swapChainFramebuffers[frame], {{0, 0}, _swapChainExtent}, 2, clearColor};

				cb.beginRenderPass(renderPassInfo, vk::SubpassContents::eSecondaryCommandBuffers);

				technique->enable();
				//cb.bindPipeline(vk::PipelineBindPoint::eGraphics, _graphicsPipeline);

				std::vector<vk::CommandBuffer> toRender;
				for (auto& xy : r.canSee)
					toRender.push_back(_secondaryCommandBuffers[xy.y*ROOM_COUNT + xy.x]);

				cb.executeCommands(toRender);

				cb.endRenderPass();
				cb.end();
			}
}

void VulkanRenderer::submit() {
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
	vk::SubmitInfo submitInfo{1, waitSemaphores, waitStages, 1, &_primaryCommandBuffers[_currentImageIndex * ROOM_COUNT*ROOM_COUNT + roomY * ROOM_COUNT + roomX], 1, signalSemaphores};
	_graphicsQueue.submit(1, &submitInfo, vk::Fence());

	_drawList.clear();
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

EasyCommandQueue VulkanRenderer::acquireEasyCommandQueue() {
	return EasyCommandQueue(this, _graphicsQueue);
}

void VulkanRenderer::createBuffer(vk::DeviceSize size,
                                  vk::BufferUsageFlags usage,
                                  vk::MemoryPropertyFlags properties,
                                  vk::Buffer& buffer,
                                  vk::DeviceMemory& bufferMemory) {
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	buffer = _device.createBuffer(bufferInfo);
	EXPECT_ASSERT(buffer, "Failed to create buffer!");

	vk::MemoryRequirements memRequirements = _device.getBufferMemoryRequirements(buffer);

	vk::MemoryAllocateInfo allocInfo;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = _findMemoryType(memRequirements.memoryTypeBits, properties);

	bufferMemory = _device.allocateMemory(allocInfo);
	EXPECT_ASSERT(bufferMemory, "failed to allocate buffer memory!");

	_device.bindBufferMemory(buffer, bufferMemory, 0);
}

void VulkanRenderer::createImage(uint32_t width,
                                 uint32_t height,
                                 vk::Format format,
                                 vk::ImageTiling tiling,
                                 vk::ImageUsageFlags usage,
                                 vk::MemoryPropertyFlags properties,
                                 vk::Image& image,
                                 vk::DeviceMemory& imageMemory) {
	vk::ImageCreateInfo imageInfo;
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;

	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.usage = usage;
	imageInfo.sharingMode = vk::SharingMode::eExclusive;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	// imageInfo.flags = 0; // Optional

	image = _device.createImage(imageInfo);
	EXPECT_ASSERT(image, "Failed to create image!");

	vk::MemoryRequirements memRequirements = _device.getImageMemoryRequirements(image);

	vk::MemoryAllocateInfo allocInfo;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = _findMemoryType(memRequirements.memoryTypeBits, properties);

	imageMemory = _device.allocateMemory(allocInfo);
	EXPECT_ASSERT(imageMemory, "failed to allocate image memory!");
	_device.bindImageMemory(image, imageMemory, 0);
}

uint32_t VulkanRenderer::_findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
	vk::PhysicalDeviceMemoryProperties memProperties = _physicalDevice.getMemoryProperties();
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;

	EXPECT_ASSERT(0, "Failed to find memory type!");
}

void VulkanRenderer::_cleanupSwapChain() {
	_device.destroyImageView(_depthImageView);
	_device.destroyImage(_depthImage);
	_device.freeMemory(_depthImageMemory);

	for (auto framebuffer : _swapChainFramebuffers)
		_device.destroyFramebuffer(framebuffer);

	_device.freeCommandBuffers(_commandPool, (uint32_t)_secondaryCommandBuffers.size(), _secondaryCommandBuffers.data());
	_device.freeCommandBuffers(_commandPool, (uint32_t)_primaryCommandBuffers.size(), _primaryCommandBuffers.data());

	_device.destroyRenderPass(_renderPass);

	for (vk::ImageView& iv : _swapChainImageViews)
		_device.destroyImageView(iv);

	_device.destroySwapchainKHR(_swapChain);
}

void VulkanRenderer::_recreateSwapChain() {
	printf("\n\n\nRECREATEING\n\n\n");
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

		auto len = extensions.size();
		extensions.resize(count + len);

		EXPECT(SDL_Vulkan_GetInstanceExtensions(_window, &count, &extensions[len]), SDL_GetError());
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
			// vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
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

	vk::AttachmentDescription depthAttachment;
	depthAttachment.format = _findDepthFormat();
	depthAttachment.samples = vk::SampleCountFlagBits::e1;
	depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
	depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentReference colorAttachmentRef;
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference depthAttachmentRef;
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	vk::SubpassDependency dependency;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	// dependency.dstSubpass = vk::PipelineStageFlags();
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	// dependency.srcAccessMask = vk::PipelineStageFlags();
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

	std::vector<vk::AttachmentDescription> attachments{colorAttachment, depthAttachment};
	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	_renderPass = _device.createRenderPass(renderPassInfo);
	EXPECT(_renderPass, "Create renderpass failed!");
	return true;
}

bool VulkanRenderer::_createVulkanPipeline() {
	auto& bpi = _basePipelineInfo;
	bpi.vertexInputInfo.vertexBindingDescriptionCount = 0;
	bpi.vertexInputInfo.vertexAttributeDescriptionCount = 0;

	bpi.inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
	bpi.inputAssembly.primitiveRestartEnable = false;

	bpi.viewport.x = 0.0f;
	bpi.viewport.y = 0.0f;
	bpi.viewport.width = (float)_swapChainExtent.width;
	bpi.viewport.height = (float)_swapChainExtent.height;
	bpi.viewport.minDepth = 0.0f;
	bpi.viewport.maxDepth = 1.0f;

	bpi.scissor.offset = vk::Offset2D{0, 0};
	bpi.scissor.extent = _swapChainExtent;

	bpi.viewportState.viewportCount = 1;
	bpi.viewportState.pViewports = &bpi.viewport;
	bpi.viewportState.scissorCount = 1;
	bpi.viewportState.pScissors = &bpi.scissor;

	bpi.rasterizer.depthClampEnable = false;
	bpi.rasterizer.rasterizerDiscardEnable = false;
	bpi.rasterizer.polygonMode = vk::PolygonMode::eFill;
	bpi.rasterizer.lineWidth = 1.0f;
	bpi.rasterizer.cullMode = vk::CullModeFlagBits::eNone;
	bpi.rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
	bpi.rasterizer.depthBiasEnable = false;

	bpi.multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
	bpi.multisampling.sampleShadingEnable = false;

	bpi.colorBlendAttachment.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
	                                           vk::ColorComponentFlagBits::eA);

	bpi.colorBlending.logicOpEnable = false;
	bpi.colorBlending.logicOp = vk::LogicOp::eCopy;
	bpi.colorBlending.attachmentCount = 1;
	bpi.colorBlending.pAttachments = &bpi.colorBlendAttachment;
	bpi.colorBlending.blendConstants[0] = 0.0f;
	bpi.colorBlending.blendConstants[1] = 0.0f;
	bpi.colorBlending.blendConstants[2] = 0.0f;
	bpi.colorBlending.blendConstants[3] = 0.0f;

	bpi.depthStencil.depthTestEnable = true;
	bpi.depthStencil.depthWriteEnable = true;
	bpi.depthStencil.depthCompareOp = vk::CompareOp::eLessOrEqual;
	bpi.depthStencil.depthBoundsTestEnable = false;
	bpi.depthStencil.stencilTestEnable = false;
	bpi.depthStencil.minDepthBounds = 0.0f;
	bpi.depthStencil.maxDepthBounds = 1.0f;

	{ // Descriptor sets for SSBOs and constant buffers.
		// I guess we have to hardcode the bindings in here to match up with the design? I have no clue what i'm talking about.
		vk::DescriptorSetLayoutBinding layoutBinding;
		vk::DescriptorSetLayoutCreateInfo layoutInfo;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &layoutBinding;

		{ // Position
			layoutBinding.binding = POSITION;
			layoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
			layoutBinding.descriptorCount = 1;
			layoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
			layoutBinding.pImmutableSamplers = nullptr;

			_descriptorSetLayouts.push_back(_device.createDescriptorSetLayout(layoutInfo));
			EXPECT(_descriptorSetLayouts.back(), "Descriptor set layout is invalid!\n");
		}

		{ // Index
			layoutBinding.binding = INDEX;
			layoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
			layoutBinding.descriptorCount = 1;
			layoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
			layoutBinding.pImmutableSamplers = nullptr;

			_descriptorSetLayouts.push_back(_device.createDescriptorSetLayout(layoutInfo));
			EXPECT(_descriptorSetLayouts.back(), "Descriptor set layout is invalid!\n");
		}

		{ // Model.
			layoutBinding.binding = MODEL;
			layoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
			layoutBinding.descriptorCount = 1;
			layoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
			layoutBinding.pImmutableSamplers = nullptr;

			_descriptorSetLayouts.push_back(_device.createDescriptorSetLayout(layoutInfo));
			EXPECT(_descriptorSetLayouts.back(), "Descriptor set layout is invalid!\n");
		}

		{ // VIEW PROJECTION
			layoutBinding.binding = CAMERA_VIEW_PROJECTION;
			layoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
			layoutBinding.descriptorCount = 1;
			layoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
			layoutBinding.pImmutableSamplers = nullptr;

			_descriptorSetLayouts.push_back(_device.createDescriptorSetLayout(layoutInfo));
			EXPECT(_descriptorSetLayouts.back(), "Descriptor set layout is invalid!\n");
		}
	}

	// bpi.pipelineInfo.stageCount = (uint32_t)m._program.size();
	// bpi.pipelineInfo.pStages = m._program.data();
	bpi.pipelineInfo.pVertexInputState = &bpi.vertexInputInfo;
	bpi.pipelineInfo.pInputAssemblyState = &bpi.inputAssembly;
	bpi.pipelineInfo.pTessellationState = nullptr;
	bpi.pipelineInfo.pViewportState = &bpi.viewportState;
	bpi.pipelineInfo.pRasterizationState = &bpi.rasterizer;
	bpi.pipelineInfo.pMultisampleState = &bpi.multisampling;
	bpi.pipelineInfo.pDepthStencilState = &bpi.depthStencil;
	bpi.pipelineInfo.pColorBlendState = &bpi.colorBlending;
	bpi.pipelineInfo.pDynamicState = nullptr;
	// bpi.pipelineInfo.layout = _pipelineLayout;
	bpi.pipelineInfo.renderPass = _renderPass;
	bpi.pipelineInfo.subpass = 0;
	bpi.pipelineInfo.basePipelineHandle = vk::Pipeline();
	bpi.pipelineInfo.basePipelineIndex = -1;

	return true;
}

bool VulkanRenderer::_createDescriptorPool() {
	// Creates two pool sizes, one for the uniform buffers and one for the SSBOs.
	constexpr int MAX_AMOUNT = 1024; // TODO: Get better multiplier, idk how to do this. Let's hope no-one breaks it!!!!

	vk::DescriptorPoolSize poolSize[2];
	poolSize[0].type = vk::DescriptorType::eUniformBuffer;
	poolSize[0].descriptorCount = 2 * MAX_AMOUNT; // Three uniform buffer descriptor sets.
	poolSize[1].type = vk::DescriptorType::eStorageBuffer;
	poolSize[1].descriptorCount = 2 * MAX_AMOUNT; // Three storage buffer descriptor sets.

	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.poolSizeCount = sizeof(poolSize) / sizeof(*poolSize);
	poolInfo.pPoolSizes = poolSize;
	poolInfo.maxSets = 2 * MAX_AMOUNT; //(uint32_t)_descriptorSetLayouts.size() * 1000;

	_descriptorPool = _device.createDescriptorPool(poolInfo);
	EXPECT(_descriptorPool, "Failed to create descriptor pool!\n");
	return true;
}

bool VulkanRenderer::_createVulkanFramebuffers() {
	_swapChainFramebuffers.resize(_swapChainImageViews.size());
	for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
		std::vector<vk::ImageView> attachments{_swapChainImageViews[i], _depthImageView};

		vk::FramebufferCreateInfo framebufferInfo;
		framebufferInfo.renderPass = _renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = _swapChainExtent.width;
		framebufferInfo.height = _swapChainExtent.height;
		framebufferInfo.layers = 1;

		_swapChainFramebuffers[i] = _device.createFramebuffer(framebufferInfo);
		EXPECT(_swapChainFramebuffers[i], "Failed to create framebuffer!");
	}
	return true;
}

bool VulkanRenderer::_createVulkanCommandPool() {
	vk::CommandPoolCreateInfo createInfo{{}, _queueInformation.graphics};
	// Reset flag to be able to re-record each frame, this is needed since the meshes are created after the command buffers.
	createInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	_commandPool = _device.createCommandPool(createInfo);
	EXPECT(_commandPool, "Failed to create command pool!");
	return true;
}

vk::Format VulkanRenderer::_findDepthFormat() {
	auto findSupportedFormat = [this](const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) -> vk::Format {
		for (auto format : candidates) {
			vk::FormatProperties props = _physicalDevice.getFormatProperties(format);

			if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
				return format;
			else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
				return format;
		}

		EXPECT_ASSERT(0, "Failed to find supported format!");
	};
	return findSupportedFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint}, vk::ImageTiling::eOptimal,
	                           vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

bool VulkanRenderer::_createVulkanDepthResources() {
	vk::Format depthFormat = _findDepthFormat();

	createImage(_swapChainExtent.width, _swapChainExtent.height, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment,
	            vk::MemoryPropertyFlagBits::eDeviceLocal, _depthImage, _depthImageMemory);

	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = _depthImage;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = depthFormat;
	viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	_depthImageView = _device.createImageView(viewInfo);
	EXPECT(_depthImageView, "Failed to create texture image view!");

	EasyCommandQueue ecq = acquireEasyCommandQueue();
	ecq.transitionImageLayout(_depthImage, depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
	ecq.run();

	return true;
}

bool VulkanRenderer::_createVulkanCommandBuffers() {
	// [P ] swapChainFramebuffers.size() * 64² + [S] 64² = 36,864
	vk::CommandBufferAllocateInfo allocInfoPrimary{_commandPool, vk::CommandBufferLevel::ePrimary, (uint32_t)(_swapChainFramebuffers.size() * 64 * 64)};
	vk::CommandBufferAllocateInfo allocInfoSecondary{_commandPool, vk::CommandBufferLevel::eSecondary, 64 * 64};

	_primaryCommandBuffers = _device.allocateCommandBuffers(allocInfoPrimary);
	EXPECT(_primaryCommandBuffers.size(), "Failed to allocate command buffers");
	_secondaryCommandBuffers = _device.allocateCommandBuffers(allocInfoSecondary);
	EXPECT(_secondaryCommandBuffers.size(), "Failed to allocate command buffers");

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
