#pragma once

#include <cstdint>
#include <memory>
#include <vulkan/vulkan_raii.hpp>

// Forward declaration
struct GLFWwindow;

namespace Neutrino 
{

class Engine 
{
public:
    static constexpr int32_t WINDOW_WIDTH = 800;
    static constexpr int32_t WINDOW_HEIGHT = 600;

    Engine();
    ~Engine();

    /// Initialize the engine (window and Vulkan)
    bool Initialize();

    /// Shutdown the engine (cleanup Vulkan and window)
    void Shutdown();

    /// Check if engine is initialized
    bool IsInitialized() const;

    /// Process events (returns false when window should close)
    bool ProcessEvents();

    /// Get GLFW window handle
    GLFWwindow* GetWindowHandle() const;

private:
    /// Initialize GLFW and create window
    bool InitializeWindow();

    /// Initialize Vulkan
    bool InitializeVulkan();

    /// Cleanup window resources
    void ShutdownWindow();

    /// Cleanup Vulkan resources
    void ShutdownVulkan();

private:
    bool initialized_;
    GLFWwindow* window_;

    // Vulkan objects using RAII
    std::unique_ptr<vk::raii::Context> vulkan_context_;
    std::unique_ptr<vk::raii::Instance> vulkan_instance_;
};

} // namespace Neutrino
