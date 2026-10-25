#pragma once

#include "VulkanContext.h"

// Swapchain + image views. Se recrée quand la fenêtre est redimensionnée.
class SwapChain
{
public:
	void Create(VulkanContext& context, GLFWwindow* window);

	// Recrée la swapchain (attend que la fenêtre ait une taille non nulle).
	// Renvoie false si la fenêtre a été fermée pendant l'attente.
	bool Recreate();

	// Détruit les image views et la swapchain
	void Cleanup();

	vk::raii::SwapchainKHR& GetHandle() { return swapChain; }
	vk::Format                              GetFormat() const { return surfaceFormat.format; }
	vk::Extent2D                            GetExtent() const { return extent; }
	const std::vector<vk::Image>& GetImages() const { return images; }
	vk::Image                               GetImage(uint32_t index) const { return images[index]; }
	vk::ImageView                           GetImageView(uint32_t index) const { return *imageViews[index]; }
	size_t                                  GetImageCount() const { return images.size(); }

private:
	void create();
	void createImageViews();

	vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
	vk::PresentModeKHR   choosePresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes);
	vk::Extent2D         chooseExtent(vk::SurfaceCapabilitiesKHR const& capabilities);
	uint32_t             chooseMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities);

	VulkanContext* ctx = nullptr;
	GLFWwindow* window = nullptr;

	vk::SurfaceFormatKHR             surfaceFormat;
	vk::Extent2D                     extent;
	vk::raii::SwapchainKHR           swapChain = nullptr;
	std::vector<vk::Image>           images;
	std::vector<vk::raii::ImageView> imageViews;
};