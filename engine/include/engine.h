//------------------------------------------------------------------------------------------------------
// Copyright(C) Bernhard Kidalka     (2026) 
//------------------------------------------------------------------------------------------------------
//
// Project: Neutrino Engine
//    File: Neutrino\engine\include\engine.h
//  Author: B. Kidalka
//    Date: 2026-07-25
//
//    Lang: C++
//
// Descrip: Neutrino Engine declarations.
//
//------------------------------------------------------------------------------------------------------
#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include <optional>

namespace Neutrino 
{
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;

        bool IsComplete() const
        {
            return graphicsFamily.has_value();
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

        static constexpr int WINDOW_WIDTH = 1024;
        static constexpr int WINDOW_HEIGHT = 768;

    private:
        // initialization & shutdown methods ...
        bool initializeWindow();
        bool initializeVulkan();
        void shutdownWindow();
        void shutdownVulkan();

        // Vulkan helper methods ...
        bool createInstance();
        bool selectPhysicalDevice();
        bool createLogicalDevice();
        bool createSurface();
        QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device) const;
        bool isDeviceSuitable(vk::PhysicalDevice device) const;

        // state
        bool initialized_;
        
        // platform-specific window handle (GLFW)
        GLFWwindow* window_;

        // Vulkan objects (using RAII wrappers)
        vk::UniqueInstance  vulkanInstance_;
        vk::PhysicalDevice  physicalDevice_;
        vk::UniqueDevice    logicalDevice_;
        vk::SurfaceKHR      surface_; // manually managed to avoid dispatcher conflicts
        vk::Queue           graphicsQueue_;
    };

} // namespace Neutrino
