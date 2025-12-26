#pragma once
#include "Window.h"
#include "Renderer.h"
#include "System.h"
#include "Controller.h"
#include "ResourceManager.h"

//TODO Make images from g buffer pass be used in lighting pass


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <chrono>

class App
{
public:

	App();
	void run();
	float updateTiming();
	~App();

private:
	Window* window;
	VulkanContext* context;
	Renderer* renderer;
	Camera* camera;
	Controller* controller;
	ResourceManager* resourceManager;


	ECS* ecs;

	std::shared_ptr<Entity> entity1 = std::make_shared<Entity>();
	std::shared_ptr<transform> transformComponent1 = std::make_shared<transform>();
	std::shared_ptr<mesh> meshComponent1 = std::make_shared<mesh>();
	std::shared_ptr<material> materialComponent1 = std::make_shared<material>();

	std::shared_ptr<Entity> entity2 = std::make_shared<Entity>();
	std::shared_ptr<transform> transformComponent2 = std::make_shared<transform>();
	std::shared_ptr<mesh> meshComponent2 = std::make_shared<mesh>();
	std::shared_ptr<material> materialComponent2 = std::make_shared<material>();

	std::shared_ptr<Entity> light1 = std::make_shared<Entity>();
	std::shared_ptr<light> lightComponent1 = std::make_shared<light>();


	std::chrono::time_point<std::chrono::high_resolution_clock> currentTime = std::chrono::high_resolution_clock::now();


};

