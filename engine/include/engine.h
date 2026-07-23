#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <memory>
#include <optional>
#include <vector>

namespace Neutrino 
{
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphics_family;

        bool IsComplete() const
        {
            return graphics_family.has_value();
        }
    };

    class Engine 
    {
    public:
        Engine();
        ~Engine();

        bool Initialize();
        void Shutdown();
        bool IsInitialized() const;
        bool IsWindowOpen() const;
        void PollEvents();

        static constexpr int WINDOW_WIDTH = 800;
        static constexpr int WINDOW_HEIGHT = 600;

    private:
        // initialization methods ...
        bool InitializeWindow();
        bool InitializeVulkan();
        void ShutdownWindow();
        void ShutdownVulkan();

        // Vulkan helper methods ...
        bool CreateInstance();
        bool SelectPhysicalDevice();
        bool CreateLogicalDevice();
        bool CreateSurface();
        QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice device) const;
        bool IsDeviceSuitable(vk::PhysicalDevice device) const;

        // state
        bool initialized_;
        
        GLFWwindow* window_;

        // Vulkan objects (using RAII wrappers)
        vk::UniqueInstance vulkan_instance_;
        vk::PhysicalDevice physical_device_;
        vk::UniqueDevice logical_device_;
        vk::SurfaceKHR surface_; // manually managed to avoid dispatcher conflicts
        vk::Queue graphics_queue_;
    };

} // namespace Neutrino
