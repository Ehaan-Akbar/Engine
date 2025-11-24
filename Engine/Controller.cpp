#include "Controller.h"

Controller::Controller(GLFWwindow* window, Camera* camera)
{
	this->window = window;
	this->camera = camera;

}

void Controller::handleKeyboardInputs(float dt)
{
	float speed = 3.0f * dt;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		camera->move({ 0.0f, 0.0f, speed });
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		camera->move({ -speed, 0.0f, 0.0f });
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		camera->move({ 0.0f, 0.0f, -speed });
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		camera->move({ speed, 0.0f, 0.0f });
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		camera->move({ 0.0f, speed, 0.0f });
	}
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		camera->move({ 0.0f, -speed, 0.0f });
	}
}

void Controller::handleMouseInputs()
{
	double mX, mY;
	glfwGetCursorPos(window, &mX, &mY);

	static double lastX = 400.0;
	static double lastY = 400.0;
	static bool firstFrame = true;

	if (firstFrame) {
		lastX = mX;
		lastY = mY;
		firstFrame = false;
	}

	double deltaX = (mX - lastX) * 0.001;
	double deltaY = (mY - lastY) * 0.001;

	lastX = mX;
	lastY = mY;


	camera->rotate({ -deltaX, deltaY, 0.0f });
}

Controller::~Controller()
{
}
