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
//    File: Neutrino\engine\core\engine.cpp
//  Author: B. Kidalka
//    Date: 2026-07-31
//
//    Lang: C++
//
// Descrip: Neutrino Engine implementation.
//
//------------------------------------------------------------------------------------------------------

#include "engine.h"
#include "logger.h"

#include <iostream>
#include <string_view>

// define dynamic dispatch loader for Vulkan
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace Neutrino 
{
    Engine::Engine() : 
        initialized_(false), 
        window_(nullptr), 
        physicalDevice_(nullptr)
    {
    }

    Engine::~Engine()
    {
        Shutdown();
    }

    bool Engine::Initialize() 
    {
        if (initialized_) 
        {
            return true;
        }
        
        Logger::Init();

        if (!initializeWindow())
        {
            Logger::Error("Failed to initialize window");
            return false;
        }

        if (!initializeVulkan())
        {
            Logger::Error("Failed to initialize Vulkan");
            shutdownWindow();
            return false;
        }

        initialized_ = true;
        return true;
    }
    
    void Engine::Run()
    {
        if (!initialized_)
        {
            throw std::runtime_error("Neutrino Engine is not initialized!");
        }

        while (IsWindowOpen())
        {
            glfwPollEvents();
        }
    }

    void Engine::Shutdown() 
    {
        if (!initialized_) 
        {
            return;
        }

        shutdownVulkan();
        shutdownWindow();

        initialized_ = false;
    }

    bool Engine::IsInitialized() const 
    {
        return initialized_;
    }

    bool Engine::IsWindowOpen() const
    {
        return window_ != nullptr && !glfwWindowShouldClose(window_);
    }

    bool Engine::initializeWindow()
    {
        if (!glfwInit())
        {
            Logger::Error("Failed to initialize GLFW");
            return false;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window_ = glfwCreateWindow(
            WINDOW_WIDTH, 
            WINDOW_HEIGHT, 
            "Neutrino Engine", 
            nullptr, 
            nullptr
        );

        if (!window_)
        {
            Logger::Error("Failed to create GLFW window");
            glfwTerminate();
            return false;
        }

        Logger::Info("GLFW window created successfully.");

        return true;
    }

    bool Engine::initializeVulkan()
    {
        try
        {
            if (!createInstance())
            {
                Logger::Error("Failed to create Vulkan instance");
                return false;
            }

            // initialize function pointers for instance
            VULKAN_HPP_DEFAULT_DISPATCHER.init(vulkanInstance_.get());

            if (!createSurface())
            {
                Logger::Error("Failed to create Vulkan surface");
                return false;
            }

            if (!selectPhysicalDevice())
            {
                Logger::Error("Failed to select physical device (GPU)");
                return false;
            }

            if (!createLogicalDevice())
            {
                Logger::Error("Failed to create logical device");
                return false;
            }

            // initialize function pointers for device (keeps instance dispatcher intact)
            VULKAN_HPP_DEFAULT_DISPATCHER.init(logicalDevice_.get());

            Logger::Info("Vulkan initialized successfully.");

            return true;
        }
        catch (const vk::SystemError& err)
        {
            Logger::Error("Vulkan error: " + std::string(err.what()));
            return false;
        }
        catch (const std::exception& err)
        {
            Logger::Error("Exception: " + std::string(err.what()));
            return false;
        }
    }

    bool Engine::createInstance()
    {
        // initialize the dynamic dispatcher with global-level functions
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        // get required extensions from GLFW ...
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        vk::ApplicationInfo appInfo
        {
            .pApplicationName = "Neutrino Engine",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "Neutrino",
            .engineVersion = VK_MAKE_VERSION(0, 0, 1),
            .apiVersion = VK_API_VERSION_1_3 // restricted to Vulkan 1.3 to ensure compatibility with MoltenVK (macOS)
        };

        vk::InstanceCreateInfo instCreateInfo
        {
            .pApplicationInfo = &appInfo,
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()
        };

        vulkanInstance_ = vk::createInstanceUnique(instCreateInfo);
        
        Logger::Info("Vulkan instance created successfully.");

        return true;
    }

    bool Engine::createSurface()
    {
        VkSurfaceKHR surfaceKHR = nullptr;
        if (glfwCreateWindowSurface(vulkanInstance_.get(), window_, nullptr, &surfaceKHR) != VK_SUCCESS)
        {
            Logger::Error("Failed to create window surface");
            return false;
        }

        // store the raw surface handle (will be manually destroyed)
        surface_ = vk::SurfaceKHR(surfaceKHR);

        Logger::Info("Vulkan surface created successfully.");

        return true;
    }

    QueueFamilyIndices Engine::findQueueFamilies(vk::PhysicalDevice device) const
    {
        QueueFamilyIndices indices;

        auto queueFamilies = device.getQueueFamilyProperties();

        int i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
            {
                indices.graphicsFamily = i;
            }

            if (device.getSurfaceSupportKHR(i, surface_))
            {
                // for now, we use the same queue family for graphics and presentation
                if (indices.graphicsFamily.has_value())
                {
                    break;
                }
            }

            i++;
        }

        return indices;
    }

    bool Engine::isDeviceSuitable(vk::PhysicalDevice device) const
    {
        auto indices = findQueueFamilies(device);

        auto extensions = device.enumerateDeviceExtensionProperties();
        bool extensionsSupported = false;

        for (const auto& ext : extensions)
        {
            if (std::string_view(ext.extensionName) == VK_KHR_SWAPCHAIN_EXTENSION_NAME)
            {
                extensionsSupported = true;
                break;
            }
        }

        return indices.IsComplete() && extensionsSupported;
    }

    bool Engine::selectPhysicalDevice()
    {
        auto devices = vulkanInstance_->enumeratePhysicalDevices();

        if (devices.empty())
        {
            Logger::Error("No physical devices found");
            return false;
        }

        auto it = std::find_if(devices.begin(), devices.end(),
            [this](vk::PhysicalDevice device) { return isDeviceSuitable(device); });

        if (it == devices.end())
        {
            Logger::Error("No suitable physical device found");
            return false;
        }

        physicalDevice_ = *it;

        auto properties = physicalDevice_.getProperties();
        Logger::Info("Selected GPU: " + std::string(properties.deviceName.data()));

        return true;
    }

    bool Engine::createLogicalDevice()
    {
        auto indices = findQueueFamilies(physicalDevice_);

        if (!indices.IsComplete())
        {
            Logger::Error("Queue families not complete");
            return false;
        }

        float queuePriority = 1.0f;
        vk::DeviceQueueCreateInfo queueCreateInfo
        {
            .queueFamilyIndex = indices.graphicsFamily.value(),
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        };

        const std::vector<const char*> deviceExtensions = 
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        vk::PhysicalDeviceFeatures device_features{};

        vk::DeviceCreateInfo deviceCreateInfo
        {
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data(),
            .pEnabledFeatures = &device_features
        };

        logicalDevice_ = physicalDevice_.createDeviceUnique(deviceCreateInfo);

        graphicsQueue_ = logicalDevice_->getQueue(indices.graphicsFamily.value(), 0);

        Logger::Info("Vulkan logical device created successfully.");

        return true;
    }

    void Engine::shutdownVulkan()
    {
        Logger::Info("Shutting down Vulkan...");

        if (logicalDevice_)
        {
            logicalDevice_->waitIdle();
            logicalDevice_.reset();
        }

        physicalDevice_ = nullptr;

        // manually destroy surface before instance is destroyed ...
        if (surface_)
        {
            // need to use instance dispatcher to destroy surface
            VULKAN_HPP_DEFAULT_DISPATCHER.init(vulkanInstance_.get());
            vulkanInstance_->destroySurfaceKHR(surface_);
            surface_ = nullptr;
        }

        Logger::Info("Vulkan instance destroyed.");
        // vulkan_instance_ is destroyed automatically at end of scope
    }

    void Engine::shutdownWindow()
    {
        Logger::Info("Shutting down GLFW window...");

        if (window_ != nullptr)
        {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }
        
        Logger::Info("GLFW window destroyed.");

        glfwTerminate();
    }

} // namespace Neutrino
