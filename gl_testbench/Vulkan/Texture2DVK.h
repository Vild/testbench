#pragma once

#include "../Texture2D.h"
#include "Sampler2DVK.h"

#include <cstdint>

class Texture2DVK : public Texture2D {
public:
	Texture2DVK();
	virtual ~Texture2DVK();

	int loadFromFile(std::string filename) final;
	void bind(unsigned int slot) final;

	uint32_t textureHandle = 0;
};

