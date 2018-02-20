#include "Sampler2DVK.h"
#include "VulkanRenderer.h"

Sampler2DVK::Sampler2DVK(VulkanRenderer* renderer) : _renderer(renderer), _device(renderer->_device) {
	vk::SamplerCreateInfo samplerInfo = {};
	samplerInfo.magFilter = vk::Filter::eNearest;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	_sampler = _device.createSampler(samplerInfo);
}
Sampler2DVK::~Sampler2DVK() {
	_device.destroySampler(_sampler);
}

void Sampler2DVK::setMagFilter(FILTER filter) {
	STUB();
}
void Sampler2DVK::setMinFilter(FILTER filter) {
	STUB();
}
void Sampler2DVK::setWrap(WRAPPING s, WRAPPING t) {
	STUB();
}
