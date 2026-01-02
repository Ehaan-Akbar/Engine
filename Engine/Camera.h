#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

class Camera
{
public:

	Camera();
	void setOrthographic(float left, float right, float top, float bottom, float near, float far);
	void setPerspective(float fovy, float aspect, float near, float far);
	void setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up = glm::vec3{ 0.0f, -1.0f, 0.0f });
	void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{ 0.0f, -1.0f, 0.0f });
	void setViewXYZ(glm::vec3 position, glm::vec3 rotation);

	void move(glm::vec3& delta);
	void move(glm::vec3&& delta);
	void rotate(glm::vec3& delta);
	void rotate(glm::vec3&& delta);


	~Camera();

	glm::mat4& getProjectionMatrix() { return projectionMatrix; }
	glm::mat4& getViewMatrix() {
		//viewMatrix = glm::yawPitchRoll(rotation.x, rotation.y, rotation.z) * glm::translate(glm::mat4(1.0f), -position);
		viewMatrix = glm::mat4(1.0f);
		viewMatrix = glm::rotate(viewMatrix, rotation.y, glm::vec3(1.0f, 0.0f, 0.0f));
		viewMatrix = glm::rotate(viewMatrix, rotation.x, glm::vec3(0.0f, 1.0f, 0.0f));
		viewMatrix = glm::translate(viewMatrix, -position);

		return viewMatrix;
	}
	glm::mat4& getInverseProjectionMatrix() {
		inverseProjectionMatrix = glm::inverse(projectionMatrix);
		return inverseProjectionMatrix;
	}

	glm::mat4& getInverseViewMatrix() {
		inverseViewMatrix = glm::inverse(getViewMatrix());
		return inverseViewMatrix;
	}

	glm::vec3 getForwardVector() {
		return glm::normalize(glm::vec3{ glm::cos(rotation.y) * glm::cos(rotation.x), glm::sin(rotation.y), glm::cos(rotation.y) * glm::sin(rotation.x) });
	}

	glm::vec3 getRightVector() {
		return glm::normalize(glm::cross(getForwardVector(), glm::vec3{ 0.0f, 1.0f, 0.0f }));
	}

	glm::vec3 getUpVector() {
		return glm::normalize(glm::cross(getRightVector(), getUpVector()));
	}


	glm::vec3 position = { 0.0f ,0.0f, 0.0f };
	glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };

private:

	glm::mat4 projectionMatrix{ 1.0f };
	glm::mat4 viewMatrix{ 1.0f };
	glm::mat4 inverseProjectionMatrix{ 1.0f };
	glm::mat4 inverseViewMatrix{ 1.0f };
};