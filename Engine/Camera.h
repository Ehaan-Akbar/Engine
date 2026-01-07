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
	
	void setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up = glm::vec3{ 0.0f, -1.0f, 0.0f });
	void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{ 0.0f, -1.0f, 0.0f });
	void setViewXYZ(glm::vec3 position, glm::vec3 rotation);

	void move(glm::vec3& delta);
	void move(glm::vec3&& delta);
	void rotate(glm::vec3& delta);
	void rotate(glm::vec3&& delta);


	~Camera();

	void setPerspective(float fovy, float aspect, float near) {
		assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
		projectionMatrix = glm::infinitePerspectiveLH_ZO(fovy, aspect, near);
		projectionMatrix[1][1] *= -1.0f;
	}

	glm::mat4 getProjectionMatrix() { return projectionMatrix; }
	glm::mat4 getViewMatrix() {
		viewMatrix = glm::lookAtLH(position, position + getForwardVector(), glm::vec3(0.0, 1.0, 0.0));
		return viewMatrix;
	}
	glm::mat4 getInverseProjectionMatrix() {
		inverseProjectionMatrix = glm::inverse(getProjectionMatrix());
		return inverseProjectionMatrix;
	}

	glm::mat4 getInverseViewMatrix() {
		inverseViewMatrix = glm::inverse(getViewMatrix());
		return inverseViewMatrix;
	}

	glm::vec3 getForwardVector() {
		return glm::normalize(glm::vec3{ glm::cos(pitch) * glm::sin(yaw), glm::sin(pitch), glm::cos(pitch) * glm::cos(yaw) });
	}

	glm::vec3 getRightVector() {
		return glm::normalize(glm::cross(glm::vec3{ 0.0f, 1.0f, 0.0f }, getForwardVector()));
	}


	glm::vec3 position = { 0.0f ,0.0f, 0.0f };
	float pitch = 0.0f;
	float yaw = 0.0f;

private:

	glm::mat4 projectionMatrix{ 1.0f };
	glm::mat4 viewMatrix{ 1.0f };
	glm::mat4 inverseProjectionMatrix{ 1.0f };
	glm::mat4 inverseViewMatrix{ 1.0f };
};