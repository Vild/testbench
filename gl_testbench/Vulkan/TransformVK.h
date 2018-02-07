#pragma once

#include "../Transform.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class VulkanRenderer;

class TransformVK : public Transform {
public:
	glm::mat4 transform;

	TransformVK(VulkanRenderer* renderer);
	virtual ~TransformVK();

	void translate(float x, float y, float z);
	void rotate(float radians, float x, float y, float z);
};
