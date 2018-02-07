#pragma once

#include "../ConstantBuffer.h"

class VulkanRenderer;

class ConstantBufferVK : public ConstantBuffer {
public:
	ConstantBufferVK(VulkanRenderer* renderer, const std::string& name, unsigned int location);
	virtual ~ConstantBufferVK();
	void setData(const void* data, size_t size, Material* m, unsigned int location) final;
	void bind(Material* material) final;

private:
	std::string name;

	void* buff = nullptr;
	void* lastMat;
};
