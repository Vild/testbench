#include "Camera.h"

Camera::Camera(){
	_position = glm::vec3(0);
	_direction = glm::vec3(0, 0, -1);
}

Camera::~Camera(){

}

void Camera::updatePosition(glm::vec3 newPos) {
	_position += newPos;
}