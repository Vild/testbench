#pragma once
#include "../Sampler2D.h"
#include <vulkan/vulkan.hpp>

class VulkanRenderer;

class Sampler2DVK : public Sampler2D {
public:
	Sampler2DVK(VulkanRenderer* renderer);
	virtual ~Sampler2DVK();
	void setMagFilter(FILTER filter) final;
	void setMinFilter(FILTER filter) final;
	void setWrap(WRAPPING s, WRAPPING t) final;
	vk::Sampler getSampler() { return _sampler; }
private:
	VulkanRenderer* _renderer;
	vk::Device _device;

	vk::Sampler _sampler;
};
