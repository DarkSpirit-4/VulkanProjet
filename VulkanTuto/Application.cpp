#include "Application.h"

void Application::Init()
{
    InitWindow();
    InitVulkan();
}

void Application::Run()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        renderer->drawFrame();
    }
}

void Application::Destroy()
{
    DestroyVulkan();
    DestroyWindow();
}

void Application::InitWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void Application::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void Application::InitVulkan()
{
    renderer = new Renderer();
    renderer->SetWindow(window);

    renderer->CreateInstance();
    renderer->SetupCallback();
    renderer->createSurface();
    renderer->pickPhysicalDevice();
    renderer->createLogicalDevice();
    renderer->createSwapChain();
    renderer->createImageViews();
    renderer->createGraphicsPipeline();
    renderer->createCommandPool();
    renderer->createVertexBuffer();
    renderer->createCommandBuffers();
    renderer->createSyncObjects();

}

void Application::DestroyWindow()
{
    renderer->cleanupSwapChain();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Application::DestroyVulkan()
{

}