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
//    Date: 2026-08-09
//
//    Lang: C++
//
// Descrip: Renderer core implementation.
//
//------------------------------------------------------------------------------------------------------

#include "renderer.h"
#include "../core/logger.h"

#include <map>
#include <set>

// define dynamic dispatch loader for Vulkan - define only once in a .cpp file
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan.h> // for PFN_vkGetInstanceProcAddr and C types

// debug callback for vk::raii - uses Vulkan-Hpp C++ types
static vk::Bool32 debugCallbackWrapper(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]] vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    [[maybe_unused]] void* pUserData)
{
    if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
    {
        Neutrino::Logger::Error("Validation layer: " + std::string(pCallbackData->pMessage));
    }
    if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
    {
        Neutrino::Logger::Warning("Validation layer: " + std::string(pCallbackData->pMessage));
    }
    if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
    {
        Neutrino::Logger::Info("Validation layer: " + std::string(pCallbackData->pMessage));
    }
    
    Neutrino::Logger::Flush();

    // note: return VK_FALSE to tell Vulkan not to abort the API call causing the latest validation message, otherwise return VK_TRUE
    return VK_FALSE;
}

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
        
        // setup debug messenger
        if (!setupDebugMessenger(enableValidationLayers_)) 
        {
            Logger::Error("Failed to setup debug messenger");
            return false;
        }

        // create surface
        if (!createSurface()) 
        {
            Logger::Error("Failed to create surface");
            return false;
        }
        
        // select the physical device (GPU)
        if (!selectPhysicalDevice()) 
        {
            Logger::Error("Failed to select physical device");
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

    bool Renderer::setupDebugMessenger(bool enableValidationLayers) 
    {
        if (!enableValidationLayers) 
        {
            return true;
        }
        try 
        {
            // setup debug messenger create info ...
            vk::DebugUtilsMessengerCreateInfoEXT createInfo
            {
                .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
                .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
#ifdef _WIN32
                .pfnUserCallback = static_cast<vk::PFN_DebugUtilsMessengerCallbackEXT>(&debugCallbackWrapper)
#else
                .pfnUserCallback = reinterpret_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(&debugCallbackWrapper)
#endif
            };

            // create debug messenger
            debugMessenger_ = vk::raii::DebugUtilsMessengerEXT(instance_, createInfo);
            return true;
        }
        catch (const std::exception& e) 
        {
            Logger::Error("Failed to set up debug messenger: " + std::string(e.what()));
            return false;
        }
    }

    bool Renderer::createSurface() 
    {
        try 
        {
            // create surface ...
            VkSurfaceKHR khrSurface;
            if (!platformWindow_->CreateVulkanSurface(*instance_, &khrSurface)) 
            {
                Logger::Error("Failed to create window surface");
                return false;
            }

            surface_ = vk::raii::SurfaceKHR(instance_, khrSurface);
            return true;
        }
        catch (const std::exception& e) 
        {
            Logger::Error("Failed to create surface: " + std::string(e.what()));
            return false;
        }
    }

    void Renderer::addSupportedOptionalExtensions() 
    {
        try 
        {
            // get available extensions
            auto availableExtensions = physicalDevice_.enumerateDeviceExtensionProperties();

            // build a set of available extension names for quick lookup ...
            std::set<std::string> avail;
            for (const auto& e : availableExtensions) 
            {
                avail.insert(e.extensionName);
            }

            for (const auto& optionalExt : optionalDeviceExtensions_) 
            {
                if (avail.contains(optionalExt)) 
                {
                    deviceExtensions_.push_back(optionalExt);
                    Logger::Info("Adding optional extension: " + std::string(optionalExt));
                }
            }
        }
        catch (const std::exception& e) 
        {
            Logger::Warning("Failed to add optional extensions: " + std::string(e.what()));
        }
    }

    bool Renderer::selectPhysicalDevice() 
    {
        try 
        {
            // get available physical devices
            std::vector<vk::raii::PhysicalDevice> devices = instance_.enumeratePhysicalDevices();

            if (devices.empty()) 
            {
                Logger::Error("Failed to find GPUs with Vulkan support");
                return false;
            }

            // prioritize discrete GPUs over integrated GPUs
            // first, collect all suitable devices with their suitability scores ...
            std::multimap<int, vk::raii::PhysicalDevice> suitableDevices;

            for (auto& gpu : devices) 
            {
                // print device properties for debugging
                vk::PhysicalDeviceProperties deviceProperties = gpu.getProperties();
                Logger::Info("Checking device: " + std::string((const char*)deviceProperties.deviceName) +
                    " (Type: " + vk::to_string(deviceProperties.deviceType) + ")");

                // check if the device supports Vulkan 1.3
                bool supportsVulkan1_3 = deviceProperties.apiVersion >= VK_API_VERSION_1_3;
                if (!supportsVulkan1_3) 
                {
                    Logger::Info("  - Does not support Vulkan 1.3");
                    continue;
                }

                // check queue families ...
                QueueFamilyIndices indices = findQueueFamilies(gpu);
                bool supportsGraphics = indices.isComplete();
                if (!supportsGraphics) 
                {
                    Logger::Info("  - Missing required queue families");
                    continue;
                }

                // check device extensions ...
                bool supportsAllRequiredExtensions = checkDeviceExtensionSupport(gpu);
                if (!supportsAllRequiredExtensions) 
                {
                    Logger::Info("  - Missing required extensions");
                    continue;
                }

                // check swap chain support ...
                SwapChainSupportDetails swapChainSupport = querySwapChainSupport(gpu);
                bool swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
                if (!swapChainAdequate) 
                {
                    Logger::Info("  - Inadequate swap chain support");
                    continue;
                }

                // check for required features ...
                auto features = gpu.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features>();
                bool supportsRequiredFeatures = features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering;
                if (!supportsRequiredFeatures) 
                {
                    Logger::Info("  - Does not support required features (dynamicRendering)");
                    continue;
                }

                // calculate suitability score - prioritize discrete GPUs
                int score = 0;

                // discrete GPUs get the highest priority
                if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) 
                {
                    score += 1000;
                    Logger::Info("  - Discrete GPU: +1000 points");
                }
                // integrated GPUs get lower priority
                else if (deviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) 
                {
                    score += 100;
                    Logger::Info("  - Integrated GPU: +100 points");
                }

                // add points for memory size (more VRAM is better)
                vk::PhysicalDeviceMemoryProperties memProperties = gpu.getMemoryProperties();
                for (uint32_t i = 0; i < memProperties.memoryHeapCount; i++) 
                {
                    if (memProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal) 
                    {
                        // add 1 point per GB of VRAM
                        score += static_cast<int>(memProperties.memoryHeaps[i].size / (1024 * 1024 * 1024));
                        break;
                    }
                }

                Logger::Info("  - Device is suitable with score: " + std::to_string(score));
                suitableDevices.emplace(score, gpu);
            }

            if (!suitableDevices.empty()) 
            {
                // select the device with the highest score (discrete GPU with most VRAM)
                physicalDevice_ = suitableDevices.rbegin()->second;
                vk::PhysicalDeviceProperties deviceProperties = physicalDevice_.getProperties();
                Logger::Info("Selected device: " + std::string((const char*)deviceProperties.deviceName)
                    + " (Type: " + vk::to_string(deviceProperties.deviceType)
                    + ", Score: " + std::to_string(suitableDevices.rbegin()->first) + ")");

                // store queue family indices for the selected device
                queueFamilyIndices_ = findQueueFamilies(physicalDevice_);

                // add supported optional extensions
                addSupportedOptionalExtensions();

                return true;
            }
            Logger::Error("Failed to find a suitable GPU. Make sure your GPU supports Vulkan and has the required extensions.");
            return false;
        }
        catch (const std::exception& e) 
        {
            Logger::Error("Failed to pick physical device: " + std::string(e.what()));
            return false;
        }
    }

}
