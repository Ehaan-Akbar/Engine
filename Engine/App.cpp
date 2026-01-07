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
	//ecs->addEntity(entity1)->addComponent<Transform>(entity1, transformComponent1)->addComponent<Mesh>(entity1, meshComponent1)->addComponent<Material>(entity1, materialComponent1);
	ecs->addEntity(entity2)->addComponent<Transform>(entity2, transformComponent2)->addComponent<Mesh>(entity2, meshComponent2)->addComponent<Material>(entity2, materialComponent2);
	ecs->addEntity(light1)->addComponent<Light>(light1, lightComponent1);

	resourceManager = new ResourceManager();

	


}

void App::run()
{
	auto vikingRoom = resourceManager->loadOBJ("viking_room.obj.txt");
	auto plane = resourceManager->loadOBJ("plane.obj");
	auto damagedHelmet = resourceManager->loadGLTF("../Assets/Sponza/scene.gltf");


	auto t1 = resourceManager->createImage("viking_room.png", ResourceManager::TEXTURES);
	resourceManager->loadTexture(t1);

	auto skybox = resourceManager->createImage("../Assets/CityBox/CityBox", ResourceManager::CUBE_MAP);
	resourceManager->loadCubeMap(skybox);

	//auto skybox2 = resourceManager->createImage("../Assets/HouseBox/HouseBox", ResourceManager::CUBE_MAP);
	//resourceManager->loadCubeMap(skybox2);



	//std::cout << skybox->cpuState;
	
	for (size_t i = 0; i < damagedHelmet.size(); ++i) {
		std::shared_ptr<Entity> entity = std::make_shared<Entity>();
		std::shared_ptr<Transform> transform = std::make_shared<Transform>();
		std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
		std::shared_ptr<Material> material = std::make_shared<Material>();

		ecs->addEntity(entity)->addComponent<Transform>(entity, transform)->addComponent<Mesh>(entity, mesh)->addComponent(entity, material);

		transform->position = { 0.0f, 0.0f, 0.0f };
		//transform->scale = { 0.1f, 0.1f, 0.1f };
		transform->rotation.x = 0*glm::pi<float>() / 2;
		transform->rotation.y = glm::pi<float>();

		mesh->vertices = damagedHelmet[i]->vertices;
		mesh->indices = damagedHelmet[i]->indices;

		material->albedoIndex = damagedHelmet[i]->albedoIndex;
		material->roughnessIndex = damagedHelmet[i]->roughnessIndex;
		material->normalIndex = damagedHelmet[i]->normalIndex;
		material->occlusionIndex = damagedHelmet[i]->occlusionIndex;
		material->emissiveIndex = damagedHelmet[i]->emissiveIndex;

		std::cout << material->roughnessIndex;
		std::cout << material->occlusionIndex;
	}

	materialComponent1->albedoIndex = t1->getID();
	meshComponent1->vertices = vikingRoom->vertices;
	meshComponent1->indices = vikingRoom->indices;


	//meshComponent2->vertices = plane->vertices;
	//meshComponent2->indices = plane->indices;

	transformComponent1->position = { 5.0f, 0.0f, 0.0f };
	

	lightComponent1->type = Light::POINT;
	lightComponent1->direction = glm::normalize(glm::vec4(0, 1, 0, 0));//glm::normalize(glm::vec4(-1.0f, -1.0f, -1.0f, 0.0f));
	//lightComponent1->position = glm::vec4(4.0f, 3.0f, -4.0f, 0.0f);
	lightComponent1->position = glm::vec4(-100.0f, 30.0f, 0.0f, 1.0f);
	lightComponent1->color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);



	std::vector<glm::vec3> lightPositions = {
	{ -1300.0f,  420.0f, -520.0f },
	{ -1150.0f,  680.0f, -180.0f },
	{ -980.0f,   360.0f,  260.0f },
	{ -820.0f,   740.0f,  520.0f },

	{ -650.0f,   300.0f, -430.0f },
	{ -480.0f,   560.0f, -120.0f },
	{ -300.0f,   420.0f,  390.0f },

	{ -120.0f,   820.0f, -500.0f },
	{    0.0f,   500.0f,    0.0f },
	{  140.0f,   760.0f,  480.0f },

	{  320.0f,   340.0f, -350.0f },
	{  520.0f,   620.0f,  110.0f },
	{  700.0f,   450.0f,  560.0f },

	{  900.0f,   780.0f, -260.0f },
	{ 1080.0f,   400.0f,  320.0f },
	{ 1240.0f,   680.0f, -540.0f },

	{  1400.0f,  520.0f,  140.0f },
	{  1120.0f,  860.0f,  500.0f },
	{  820.0f,   300.0f, -100.0f },
	{ -1400.0f,  600.0f,  200.0f }
	};

	std::vector<glm::vec3> lightColors = {
	{ 1.00f, 0.25f, 0.25f }, // red
	{ 0.25f, 1.00f, 0.30f }, // green
	{ 0.30f, 0.45f, 1.00f }, // blue
	{ 1.00f, 0.85f, 0.30f }, // warm yellow
	{ 0.80f, 0.30f, 1.00f }, // purple

	{ 0.30f, 1.00f, 0.90f }, // cyan
	{ 1.00f, 0.55f, 0.20f }, // orange
	{ 0.60f, 1.00f, 0.40f }, // lime
	{ 0.40f, 0.70f, 1.00f }, // sky blue
	{ 1.00f, 0.40f, 0.70f }, // pink

	{ 0.90f, 0.90f, 0.90f }, // near-white
	{ 0.60f, 0.60f, 0.20f }, // olive
	{ 0.20f, 0.80f, 0.60f }, // teal
	{ 0.70f, 0.30f, 0.20f }, // brick
	{ 0.45f, 0.20f, 0.80f }, // deep violet

	{ 0.95f, 0.70f, 0.35f }, // warm amber
	{ 0.35f, 0.95f, 0.55f }, // mint
	{ 0.55f, 0.35f, 0.95f }, // indigo
	{ 0.80f, 0.80f, 0.30f }, // chartreuse
	{ 0.30f, 0.30f, 0.30f }  // dim neutral
	};

	for (int i = 0; i < 20; ++i) {
		std::shared_ptr<Entity> lightEntity = std::make_shared<Entity>();
		std::shared_ptr<Light> lightComp = std::make_shared<Light>();
		ecs->addEntity(lightEntity)->addComponent<Light>(lightEntity, lightComp);

		lightComp->type = Light::POINT;
		lightComp->color = glm::vec4(lightColors[i], 2.0);
		lightComp->position = glm::vec4(lightPositions[i] * glm::vec3(-1.0, 1.0, -1.0), 1.0f);
	}


	transformComponent2->position = lightComponent1->position;
	

	//camera->setViewTarget(glm::vec3(0.0f, 0.0f, 0.0f), transformComponent2->position);


	glfwSetInputMode(window->getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	static bool preprocessing = renderer->preprocess(*resourceManager);
	//while preprocessing is not done,
	while (!preprocessing) {
		//keep doing it
		preprocessing = renderer->preprocess(*resourceManager);
	}
	//so when its done, loop stops and we go to main loop

	while (!glfwWindowShouldClose(window->getWindow())) {

		glfwPollEvents();

		auto dt = updateTiming();
		//std::cout << camera->position.x << " " << camera->position.y << " " << camera->position.z << '\n';
		//lightComponent1->position.x += 10.0f * dt;
		//transformComponent2->position = lightComponent1->position;

		//Set Camera
		float aspect = renderer->getAspectRatio();
		camera->setPerspective(glm::radians(90.0f), aspect, 0.1f);


		//Controller
		controller->handleKeyboardInputs(dt);
		controller->handleMouseInputs();

		
		//Rotation Testing
		transformComponent1->rotation.x = glm::pi<float>()/2;
		//transformComponent2->rotation.x = glm::pi<float>();
		//transformComponent1->rotation.y += dt * glm::pi<float>();
		/*transformComponent1->rotation.x += 0.0002f * glm::pi<float>();

		transformComponent2->rotation.y -= 0.00002f * glm::pi<float>();
		transformComponent2->rotation.x -= 0.00002f * glm::pi<float>();*/
		
		/*static float t = 0;
		t = t + 1 % 1000;
		lightComponent1->color.x = glm::cos(t / 1000) / 2 + 0.5;
		lightComponent1->color.y = glm::sin(t / 1000) / 2 + 0.5;
		lightComponent1->color.z = glm::cos(t / 1000) / 2 + 0.5;*/

		
		//vkDeviceWaitIdle(context->vulkanResources.device);
		//Render Loop
		if (!renderer->beginFrame()) continue;
		renderer->submit(*ecs, *camera);
		renderer->endFrame();

	}
	//std::cout << skybox->gpuState;
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