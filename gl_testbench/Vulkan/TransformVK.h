#pragma once

#include "../Transform.h"

#include <glm/glm.hpp>

class TransformVK : public Transform {
public:
	glm::mat4 transform;

	void translate(float x, float y, float z);
	void rotate(float radians, float x, float y, float z);
};
