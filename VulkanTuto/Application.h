#pragma once

#include <memory>
#include "Dependancies.h"
#include "Renderer.h"

class Application
{
public:
	void Init();
	void Run();
	void Destroy();

private:
	void InitWindow();
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

	void DestroyWindow();

	int width = 800;
	int height = 600;
	GLFWwindow* window = nullptr;
	std::unique_ptr<Renderer> renderer;
};