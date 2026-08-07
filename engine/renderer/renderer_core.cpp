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
//    File: Neutrino\engine\renderer\renderer_core.cpp
//  Author: B. Kidalka
//    Date: 2026-08-07
//
//    Lang: C++
//
// Descrip: Renderer core implementation.
//
//------------------------------------------------------------------------------------------------------

#include "renderer.h"
#include "../core/logger.h"

// define dynamic dispatch loader for Vulkan - define only once in a .cpp file
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan.h> // for PFN_vkGetInstanceProcAddr and C types

namespace Neutrino
{
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // construction and destruction ...
    
    Renderer::Renderer(Window* platformWindow) : 
        platformWindow_(platformWindow) 
    {
        // initialize deviceExtensions with required extensions only
        // optional extensions will be added later after checking device support
        deviceExtensions_ = requiredDeviceExtensions_;
    }

    Renderer::~Renderer() 
    {
        Shutdown();
    }
    
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // public methods ...
    
    bool Renderer::Initialize(const std::string& appName)
    {
        Logger::Info("Starting renderer initialization ...");

#ifdef _DEBUG
        enableValidationLayers_ = true; // enable validation layers in debug builds
#endif
        
        // initialize the Vulkan-Hpp default dispatcher using the global symbol directly
        // this avoids differences across Vulkan-Hpp versions for DynamicLoader placement
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
        
        // create the Vulkan instance
        if (!createInstance(appName)) 
        {
            Logger::Error("Failed to create Vulkan instance!");
            return false;
        }
        
        initialized_ = true;
        Logger::Info("Renderer initialized successfully.");
        return true;
    }

    void Renderer::Shutdown()
    {
        if (!initialized_) 
        {
            return;
        }

        Logger::Info("Starting renderer shutdown ...");


        Logger::Info("Renderer shutdown completed.");
        initialized_ = false;
    }

    void Renderer::RenderFrame()
    {
        // TODO: implementation for rendering a frame
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // private methods ...
    
    bool Renderer::checkValidationLayerSupport() const
    {
        // get available layers
        std::vector<vk::LayerProperties> availableLayers = context_.enumerateInstanceLayerProperties();

        // check if all requested layers are available ...
        for (const char* layerName : validationLayers_) 
        {
            bool layerFound { false };
            for (const auto& layerProperties : availableLayers) 
            {
                if (strcmp(layerName, layerProperties.layerName) == 0) 
                {
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound) 
            {
                return false;
            }
        }
        return true;
    }

    bool Renderer::createInstance(const std::string& appName)
    {
        try 
        {
            // create application info ...
            vk::ApplicationInfo appInfo
            {
                .pApplicationName = appName.c_str(),
                .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                .pEngineName = "Neutrino Engine",
                .engineVersion = VK_MAKE_VERSION(0, 0, 1),
                .apiVersion = VK_API_VERSION_1_3
            };

            // get required extensions ...
            std::vector<const char*> extensions;

            // add required extensions for GLFW ...
            uint32_t glfwExtensionCount { 0 };
            const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            extensions.insert(extensions.end(), glfwExtensions, glfwExtensions + glfwExtensionCount);

            // add debug extension if validation layers are enabled
            if (enableValidationLayers_) 
            {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

            // create instance info ...
            vk::InstanceCreateInfo instanceInfo
            {
                .pApplicationInfo = &appInfo,
                .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
                .ppEnabledExtensionNames = extensions.data()
            };

            // enable validation layers if requested ...
            vk::ValidationFeaturesEXT validationFeatures{};
            std::vector<vk::ValidationFeatureEnableEXT> enabledValidationFeatures;

            if (enableValidationLayers_) 
            {
                if (!checkValidationLayerSupport()) 
                {
                    Logger::Error("Vulkan validation layers requested, but not available");
                    return false;
                }

                instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers_.size());
                instanceInfo.ppEnabledLayerNames = validationLayers_.data();

                validationFeatures.enabledValidationFeatureCount = static_cast<uint32_t>(enabledValidationFeatures.size());
                validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures.data();

                instanceInfo.pNext = &validationFeatures;
            }

            // finally create Vulkan instance
            instance_ = vk::raii::Instance(context_, instanceInfo);
            
            return true;
        }
        catch (const std::exception& e) 
        {
            Logger::Error("Vulkan instance creation failed: " + std::string(e.what()));
            return false;
        }
    }

}
