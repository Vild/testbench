#include "Camera.h"

#include <algorithm>

Camera::Camera() {}

Camera::~Camera() {}

void Camera::updatePosition(glm::vec3 newPos, glm::vec2 newRot) {
	_position += newPos;

	_pitch = std::max(std::min(_pitch + newRot.x, (float)M_PI/2), (float)-M_PI/2);
	_yaw = fmod(_yaw + newRot.y, 2 * M_PI);


	glm::quat qPitch = glm::angleAxis(_pitch, glm::vec3(1, 0, 0));
	glm::quat qYaw = glm::angleAxis(_yaw, glm::vec3(0, 1, 0));
	glm::quat qRoll = glm::angleAxis(glm::radians(0.f), glm::vec3(0, 0, 1));

	_rotation = glm::normalize(qPitch * qYaw * qRoll);
}
