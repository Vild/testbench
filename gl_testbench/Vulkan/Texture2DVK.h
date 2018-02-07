#pragma once

#include "../Texture2D.h"
#include "Sampler2DVK.h"

#include <cstdint>

class VulkanRenderer;

class Texture2DVK : public Texture2D {
public:
	Texture2DVK(VulkanRenderer* renderer);
	virtual ~Texture2DVK();

	int loadFromFile(std::string filename) final;
	void bind(unsigned int slot) final;

	uint32_t textureHandle = 0;
};
