#include "App.h"

App::App()
{
	window = new Window();
	window->initWindow(2000, 1200, "asd");

	context = new VulkanContext(window->getWindow());

	renderer = new Renderer(*context);
	renderer->init();

	camera = new Camera();

	controller = new Controller(window->getWindow(), camera);

	
	ecs = new ECS();
	ecs->addEntity(entity1)->addComponent<transform>(entity1, transformComponent1)->addComponent<mesh>(entity1, meshComponent1)->addComponent<material>(entity1, materialComponent1);
	//ecs->addEntity(entity2)->addComponent<transform>(entity2, transformComponent2)->addComponent<mesh>(entity2, meshComponent2)->addComponent<material>(entity2, materialComponent2);
	ecs->addEntity(light1)->addComponent<light>(light1, lightComponent1);

	resourceManager = new ResourceManager();

	


}

void App::run()
{

	auto def = resourceManager->createImage("default.png", ResourceManager::TEXTURES);
	resourceManager->loadImage(def);

	auto t1 = resourceManager->createImage("viking_room.png", ResourceManager::TEXTURES);
	resourceManager->loadImage(t1);

	meshComponent1->textureIndex = t1->getID();

	std::cout << t1->cpuState << std::endl;

	meshComponent1->loadModel("viking_room.obj.txt");
	meshComponent2->loadModel("plane.obj");

	transformComponent1->position = { 0.0f, 0.0f, 0.0f };
	transformComponent2->position = { 0.0f, 0.0f, 0.0f };

	lightComponent1->type = light::DIRECTIONAL;
	lightComponent1->direction = glm::normalize(glm::vec4(-1.0f, -1.0f, -1.0f, 0.0f));
	lightComponent1->position = glm::vec4(4.0f, -3.0f, -4.0f, 0.0f);
	lightComponent1->color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	

	//camera->setViewTarget(glm::vec3(0.0f, 0.0f, 0.0f), transformComponent2->position);


	glfwSetInputMode(window->getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	while (!glfwWindowShouldClose(window->getWindow())) {


		glfwPollEvents();

		auto dt = updateTiming();

		//Set Camera
		float aspect = renderer->getAspectRatio();
		camera->setPerspective(glm::radians(90.0f), aspect, 0.1f, 10.0f);


		//Controller
		controller->handleKeyboardInputs(dt);
		controller->handleMouseInputs();

		
		//Rotation Testing
		transformComponent1->rotation.x = glm::pi<float>()/2;
		transformComponent2->rotation.x = glm::pi<float>();
		transformComponent1->rotation.y += 0.0002f * glm::pi<float>();
		/*transformComponent1->rotation.x += 0.0002f * glm::pi<float>();

		transformComponent2->rotation.y -= 0.00002f * glm::pi<float>();
		transformComponent2->rotation.x -= 0.00002f * glm::pi<float>();*/
		
		/*static float t = 0;
		t = t + 1 % 1000;
		lightComponent1->color.x = glm::cos(t / 1000) / 2 + 0.5;
		lightComponent1->color.y = glm::sin(t / 1000) / 2 + 0.5;
		lightComponent1->color.z = glm::cos(t / 1000) / 2 + 0.5;*/

		//Render Loop
		if (!renderer->beginFrame()) continue;
		renderer->submit(*ecs, *camera, *resourceManager);
		renderer->endFrame();

	}
}

float App::updateTiming()
{
	auto newTime = std::chrono::high_resolution_clock::now();
	auto dt = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
	currentTime = newTime;
	return dt;
}

App::~App()
{
	delete camera;
	delete renderer;
	delete context;
	delete window;
	delete ecs;
	delete controller;
	delete resourceManager;
}