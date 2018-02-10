#include "TransformVK.h"

#include <glm/gtc/matrix_transform.hpp>

void TransformVK::translate(float x, float y, float z) {
	transform[3][0] += x;
	transform[3][1] += y;
	transform[3][2] += z;
}

void TransformVK::rotate(float radians, float x, float y, float z) {
	transform = glm::rotate(transform, radians, glm::vec3(x, y, z));
}
