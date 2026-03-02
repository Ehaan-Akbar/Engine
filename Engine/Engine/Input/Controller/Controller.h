#pragma once
#include "../../../Vulkan/Initialization/Window/Window.h"
#include "../../Camera/Camera.h"


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
