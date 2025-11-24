#pragma once
#include "Window.h"
#include "Camera.h"


class Controller {
public:

	Controller(GLFWwindow* window, Camera* camera);
	void handleKeyboardInputs(float dt);
	void handleMouseInputs();
	~Controller();
private:

	GLFWwindow* window;
	Camera* camera;

};
