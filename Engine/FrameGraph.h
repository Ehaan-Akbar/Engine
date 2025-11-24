#pragma once
#include "RenderPass.h"
#include "Pipeline.h"
#include "Framebuffer.h"
#include "Image.h"



class FrameGraph
{
public:
	friend class VulkanApp;
	friend class Renderer;

	struct Pass {
		std::string name;

		RenderPass renderPass;
		Pipeline pipeline;

		std::vector<VkImage> inputImages;
		std::vector<VkImage> outputImages;
		std::vector<Pass*> dependencies;
		void addDependency(Pass* node) {
			dependencies.push_back(node);
		}
		void addInputImage(VkImage image) {
			inputImages.push_back(image);
		}
		void addOutputImage(VkImage image) {
			outputImages.push_back(image);
		}
	};


	FrameGraph(VulkanResources& vulkanResources);
	void initFrameGraph();
	void destroyFrameGraph();
	~FrameGraph();

private:
	VulkanResources& vulkanResources;




};

