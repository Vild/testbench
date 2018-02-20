#include "Texture2DVK.h"
#include "VulkanRenderer.h"

#include <stb_image.h>

Texture2DVK::Texture2DVK(VulkanRenderer* renderer) : _renderer(renderer), _device(renderer->_device) {}
Texture2DVK::~Texture2DVK() {
	_device.destroyImageView(_imageView);
	_device.destroyImage(_image);
	_device.freeMemory(_imageMemory);
}

int Texture2DVK::loadFromFile(std::string filename) {
	int texWidth, texHeight, texChannelsq;
	uint8_t* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannelsq, STBI_rgb_alpha);
	vk::DeviceSize imageSize = texWidth * texHeight * 4 /* Channels? */;

	EXPECT_ASSERT(pixels, "Failed to load texture image!");
	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingBufferMemory;

	_renderer->createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
	                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

	void* data = _device.mapMemory(stagingBufferMemory, 0, imageSize);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	_device.unmapMemory(stagingBufferMemory);
	stbi_image_free(pixels);

	_renderer->createImage(texWidth, texHeight, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal,
	                       vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, _image,
	                       _imageMemory);


	{
		EasyCommandQueue ecq = _renderer->acquireEasyCommandQueue();
		ecq.transitionImageLayout(_image, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		ecq.copyBufferToImage(stagingBuffer, _image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		ecq.transitionImageLayout(_image, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
		ecq.run();
	}

	_device.destroyBuffer(stagingBuffer);
	_device.freeMemory(stagingBufferMemory);

	{ // Image view for fatboy.
		vk::ImageViewCreateInfo viewInfo;
		viewInfo.image = _image;
		viewInfo.viewType = vk::ImageViewType::e2D;
		viewInfo.format = vk::Format::eR8G8B8A8Unorm;
		viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		_imageView = _device.createImageView(viewInfo);
	}

	return 0;
}
void Texture2DVK::bind(unsigned int slot) {

}

void Texture2DVK::updateSampler(MeshVK* mesh, unsigned int slot) {
	vk::DescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	imageInfo.imageView = _imageView;
	if (this->sampler != nullptr) {
		Sampler2DVK* s = (Sampler2DVK*)this->sampler;
		imageInfo.sampler = s->getSampler();
	vk::WriteDescriptorSet descriptorWrite = {};
	descriptorWrite.dstSet = mesh->getDescriptorSet(DescriptorType::diffuseSlot);
	descriptorWrite.dstBinding = DIFFUSE_SLOT;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = vk::DescriptorType::eSampledImage;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	_device.updateDescriptorSets(descriptorWrite, nullptr);
	}
}
