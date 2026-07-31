//------------------------------------------------------------------------------------------------------
// Copyright (c) 2026 Bernhard Kidalka
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//------------------------------------------------------------------------------------------------------
//
// Project: Neutrino Engine
//    File: Neutrino\engine\include\engine.h
//  Author: B. Kidalka
//    Date: 2026-07-31
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

        bool Initialize(const std::string& appName, int windowWidth, int windowHeight);
        void Run();
        void Shutdown();
        bool IsInitialized() const;
        bool IsWindowOpen() const;

    private:
        // initialization & shutdown methods ...
        bool initializeWindow(const std::string& title, int width, int height);
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
