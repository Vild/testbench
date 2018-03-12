#pragma once
#include <glm/glm.hpp>
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
		_matrices.view = glm::lookAt(_position, _position + _direction, glm::vec3(0, 1, 0));
		_matrices.proj = glm::perspective(90.f, 4/3.0f, 0.1f, 100.f);
		return _matrices;
	}
	
	void updatePosition(glm::vec3 dir);

	ViewProjection _matrices;
	glm::vec3 _position;
	glm::vec3 _direction;
};
