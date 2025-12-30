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
	~App();

	float updateTiming();

private:
	Window* window;
	VulkanContext* context;
	Renderer* renderer;
	Camera* camera;
	Controller* controller;
	ResourceManager* resourceManager;


	ECS* ecs;

	std::shared_ptr<Entity> entity1 = std::make_shared<Entity>();
	std::shared_ptr<Transform> transformComponent1 = std::make_shared<Transform>();
	std::shared_ptr<Mesh> meshComponent1 = std::make_shared<Mesh>();
	std::shared_ptr<Material> materialComponent1 = std::make_shared<Material>();

	std::shared_ptr<Entity> entity2 = std::make_shared<Entity>();
	std::shared_ptr<Transform> transformComponent2 = std::make_shared<Transform>();
	std::shared_ptr<Mesh> meshComponent2 = std::make_shared<Mesh>();
	std::shared_ptr<Material> materialComponent2 = std::make_shared<Material>();

	std::shared_ptr<Entity> light1 = std::make_shared<Entity>();
	std::shared_ptr<Light> lightComponent1 = std::make_shared<Light>();


	std::chrono::time_point<std::chrono::high_resolution_clock> currentTime = std::chrono::high_resolution_clock::now();


};

