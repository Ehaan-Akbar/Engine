#pragma once
#include <set>
#include <unordered_set>
#include "RenderPass.h"
#include "Pipeline.h"
#include "Framebuffer.h"
#include "Image.h"



class FrameGraph
{
public:
	friend class VulkanApp;
	friend class Renderer;

	


	FrameGraph(VulkanResources& vulkanResources);
	void initFrameGraph();
	void execute(VkCommandBuffer commandBuffer);
	void destroyFrameGraph();
	~FrameGraph();

private:
	VulkanResources& vulkanResources;
	



};

