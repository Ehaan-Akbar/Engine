#include "App.h"

App::App()
{
	window = new Window();
	window->initWindow(800, 800, "asd");

	context = new VulkanContext(window->getWindow());

	renderer = new Renderer(*context);
	renderer->init();

	camera = new Camera();

	controller = new Controller(window->getWindow(), camera);

	
	ecs = new ECS();
	ecs->addEntity(entity1)->addComponent<transform>(entity1, transformComponent1)->addComponent<mesh>(entity1, meshComponent1);
	ecs->addEntity(entity2)->addComponent<transform>(entity2, transformComponent2)->addComponent<mesh>(entity2, meshComponent2);

}

void App::run()
{
	meshComponent1->loadModel("sphere.obj");
	meshComponent2->loadModel("suzanne.obj");


	
	transformComponent1->position = { 0.0f, 0.0f, 0.0f };
	transformComponent2->position = { 8.0f, -6.0f, -8.0f };

	//camera->setViewTarget(glm::vec3(0.0f, 0.0f, 0.0f), transformComponent2->position);


	glfwSetInputMode(window->getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	while (!glfwWindowShouldClose(window->getWindow())) {

		//std::cout << "Camera Position: " << camera->position.x << ", " << camera->position.y << ", " << camera->position.z << std::endl;

		glfwPollEvents();

		auto dt = updateTiming();

		//Set Camera
		float aspect = renderer->getAspectRatio();
		camera->setPerspective(glm::radians(90.0f), aspect, 0.1f, 10.0f);


		//Controller
		controller->handleKeyboardInputs(dt);
		controller->handleMouseInputs();

		
		//Rotation Testing
		transformComponent1->rotation.x = glm::pi<float>();
		transformComponent2->rotation.x = glm::pi<float>();
		/*transformComponent1->rotation.y += 0.0002f * glm::pi<float>();
		transformComponent1->rotation.x += 0.0002f * glm::pi<float>();

		transformComponent2->rotation.y -= 0.00002f * glm::pi<float>();
		transformComponent2->rotation.x -= 0.00002f * glm::pi<float>();*/
		

		//Render Loop
		if (!renderer->beginFrame()) continue;
		renderer->submit(*ecs, *camera);
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
}