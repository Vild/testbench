#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>

class Camera
{
public:
	struct ViewProjection {
		glm::mat4 view;
		glm::mat4 proj;
	};
	Camera();
	virtual ~Camera();
	ViewProjection& getMatrices() {
		_matrices.view = glm::translate(glm::mat4_cast(_rotation), -_position);
		_matrices.proj = glm::perspective(90.f, 4/3.0f, 0.1f, 1000.f);
		return _matrices;
	}

	void updatePosition(glm::vec3 dir, glm::vec2 newRot);

	ViewProjection _matrices;
	glm::vec3 _position;
	glm::quat _rotation;

	float _pitch = 0;
	float _yaw = 0;
};
