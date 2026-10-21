#pragma once

#include "Dependancies.h"
#include "Renderer.h"

class Application
{
public:

	int width = 800;
	int height = 600;
	GLFWwindow* window = nullptr;
	Renderer* renderer = nullptr; 
	bool framebufferResized = false;

	void Init();
	void Run();
	void Destroy();

private:

	void InitWindow();
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	void InitVulkan();	

	void DestroyWindow();
	void DestroyVulkan();
};

