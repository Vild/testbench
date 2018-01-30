#pragma once

#include <GL/glew.h>

#include "../Texture2D.h"
#include "Sampler2DVK.h"


class Texture2DVK :
	public Texture2D
{
public:
	Texture2DVK();
	~Texture2DVK();

	int loadFromFile(std::string filename);
	void bind(unsigned int slot);

	// OPENVK HANDLE
	VKuint textureHandle = 0;
};

