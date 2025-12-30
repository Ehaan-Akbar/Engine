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
	ecs->addEntity(entity1)->addComponent<Transform>(entity1, transformComponent1)->addComponent<Mesh>(entity1, meshComponent1)->addComponent<Material>(entity1, materialComponent1);
	ecs->addEntity(entity2)->addComponent<Transform>(entity2, transformComponent2)->addComponent<Mesh>(entity2, meshComponent2)->addComponent<Material>(entity2, materialComponent2);
	ecs->addEntity(light1)->addComponent<Light>(light1, lightComponent1);

	resourceManager = new ResourceManager();

	


}

void App::run()
{
	auto vikingRoom = resourceManager->loadOBJ("viking_room.obj.txt");
	auto plane = resourceManager->loadOBJ("plane.obj");
	auto damagedHelmet = resourceManager->loadGLTF("../Assets/DamagedHelmet/scene.gltf");


	auto t1 = resourceManager->createImage("viking_room.png", ResourceManager::TEXTURES);
	resourceManager->loadImage(t1);
	
	for (size_t i = 0; i < damagedHelmet.size(); ++i) {
		std::shared_ptr<Entity> entity = std::make_shared<Entity>();
		std::shared_ptr<Transform> transform = std::make_shared<Transform>();
		std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
		std::shared_ptr<Material> material = std::make_shared<Material>();

		ecs->addEntity(entity)->addComponent<Transform>(entity, transform)->addComponent<Mesh>(entity, mesh)->addComponent(entity, material);

		transform->position = { 0.0f, 0.0f, 0.0f };
		transform->rotation.x = -glm::pi<float>()/2;

		mesh->vertices = damagedHelmet[i]->vertices;
		mesh->indices = damagedHelmet[i]->indices;

		material->albedoIndex = damagedHelmet[i]->albedoIndex;
		material->roughnessIndex = damagedHelmet[i]->roughnessIndex;
		material->normalIndex = damagedHelmet[i]->normalIndex;
		material->occlusionIndex = damagedHelmet[i]->occlusionIndex;
		material->emissiveIndex = damagedHelmet[i]->emissiveIndex;
	}

	materialComponent1->albedoIndex = t1->getID();
	meshComponent1->vertices = vikingRoom->vertices;
	meshComponent1->indices = vikingRoom->indices;


	meshComponent2->vertices = plane->vertices;
	meshComponent2->indices = plane->indices;

	transformComponent1->position = { 5.0f, 0.0f, 0.0f };
	transformComponent2->position = { 0.0f, 0.0f, 0.0f };

	lightComponent1->type = Light::DIRECTIONAL;
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
		//transformComponent1->rotation.y += dt * glm::pi<float>();
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