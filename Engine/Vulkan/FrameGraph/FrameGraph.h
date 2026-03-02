#pragma once
#include "../Helper/Helper.h"
#include <set>
#include <unordered_set>
#include "../Abstractions/RenderPass/RenderPass.h"
#include "../Abstractions/Pipeline/Pipeline.h"
#include "../Abstractions/Framebuffer/Framebuffer.h"
#include "../Abstractions/Image/Image.h"



class FrameGraph
{
public:
	
	struct Resource {
		std::string name;
		Image image;
	};

	struct Pass {
		std::string name;
		std::vector<Resource*> inputs;
		std::vector<Resource*> outputs;

		std::function<void(VkCommandBuffer commandBuffer)> executeFunction;
	};

	friend class VulkanApp;
	friend class Renderer;

	


	FrameGraph(VulkanResources& vulkanResources);
	void initFrameGraph();
	void addPass();
	void destroyFrameGraph();
	~FrameGraph();

private:
	VulkanResources& vulkanResources;
	



};

