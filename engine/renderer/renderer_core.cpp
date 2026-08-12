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
//    Date: 2026-08-12
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
        
        // create logical device
        if (!createLogicalDevice()) 
        {
            Logger::Error("Failed to create logical device");
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

    bool Renderer::createLogicalDevice() 
    {
        try 
        {
            // create queue create info for each unique queue family ...
            std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
            std::set uniqueQueueFamilies = 
            {
              queueFamilyIndices_.GraphicsFamily.value(),
              queueFamilyIndices_.PresentFamily.value(),
              queueFamilyIndices_.ComputeFamily.value(),
              queueFamilyIndices_.TransferFamily.value()
            };

            float queuePriority = 1.0f;
            for (uint32_t queueFamily : uniqueQueueFamilies) 
            {
                vk::DeviceQueueCreateInfo queueCreateInfo
                {
                  .queueFamilyIndex = queueFamily,
                  .queueCount = 1,
                  .pQueuePriorities = &queuePriority
                };
                queueCreateInfos.push_back(queueCreateInfo);
            }

            // query supported features before enabling them ...
            auto supportedFeatures = physicalDevice_.getFeatures2<
                vk::PhysicalDeviceFeatures2,
                vk::PhysicalDeviceTimelineSemaphoreFeatures,
                vk::PhysicalDeviceVulkanMemoryModelFeatures,
                vk::PhysicalDeviceBufferDeviceAddressFeatures,
                vk::PhysicalDevice8BitStorageFeatures,
                vk::PhysicalDeviceVulkan11Features,
                vk::PhysicalDeviceVulkan13Features>();

            // verify critical features are supported ...
            const auto& coreSupported = supportedFeatures.get<vk::PhysicalDeviceFeatures2>().features;
            const auto& timelineSupported = supportedFeatures.get<vk::PhysicalDeviceTimelineSemaphoreFeatures>();
            const auto& memoryModelSupported = supportedFeatures.get<vk::PhysicalDeviceVulkanMemoryModelFeatures>();
            const auto& bufferAddressSupported = supportedFeatures.get<vk::PhysicalDeviceBufferDeviceAddressFeatures>();
            const auto& storage8BitSupported = supportedFeatures.get<vk::PhysicalDevice8BitStorageFeatures>();
            const auto& vulkan11Supported = supportedFeatures.get<vk::PhysicalDeviceVulkan11Features>();
            const auto& vulkan13Supported = supportedFeatures.get<vk::PhysicalDeviceVulkan13Features>();

            // check for required features ...
            if (!coreSupported.samplerAnisotropy ||
                !timelineSupported.timelineSemaphore ||
                !memoryModelSupported.vulkanMemoryModel ||
                !bufferAddressSupported.bufferDeviceAddress ||
                !vulkan11Supported.shaderDrawParameters ||
                !vulkan13Supported.dynamicRendering ||
                !vulkan13Supported.synchronization2) {
                throw std::runtime_error("Required Vulkan features not supported by physical device");
            }

            // enable required features (now verified to be supported) ...
            auto features = physicalDevice_.getFeatures2();
            features.features.samplerAnisotropy = vk::True;
            features.features.depthBiasClamp = coreSupported.depthBiasClamp ? vk::True : vk::False;

            // explicitly configure device features to prevent validation layer warnings ...
            // these features are required by extensions or other features, so we enable them explicitly

            // timeline semaphore features (required for synchronization2)
            vk::PhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures;
            timelineSemaphoreFeatures.timelineSemaphore = vk::True;

            // Vulkan memory model features (required for some shader operations)
            vk::PhysicalDeviceVulkanMemoryModelFeatures memoryModelFeatures;
            memoryModelFeatures.vulkanMemoryModel = vk::True;
            memoryModelFeatures.vulkanMemoryModelDeviceScope = memoryModelSupported.vulkanMemoryModelDeviceScope ? vk::True : vk::False;

            // buffer device address features (required for some buffer operations)
            vk::PhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures;
            bufferDeviceAddressFeatures.bufferDeviceAddress = vk::True;

            // 8-bit storage features (required for some shader storage operations)
            vk::PhysicalDevice8BitStorageFeatures storage8BitFeatures;
            storage8BitFeatures.storageBuffer8BitAccess = storage8BitSupported.storageBuffer8BitAccess ? vk::True : vk::False;

            // enable Vulkan 1.3 features
            vk::PhysicalDeviceVulkan13Features vulkan13Features;
            vulkan13Features.dynamicRendering = vk::True;
            vulkan13Features.synchronization2 = vk::True;

            // Vulkan 1.1 features: shaderDrawParameters to satisfy SPIR-V DrawParameters capability
            vk::PhysicalDeviceVulkan11Features vulkan11Features{};
            vulkan11Features.shaderDrawParameters = vk::True;
            
            // query extended feature support ...
#ifdef _WIN32            
            auto featureChain = physicalDevice_.getFeatures2<
                vk::PhysicalDeviceFeatures2,
                vk::PhysicalDeviceDescriptorIndexingFeatures,
                vk::PhysicalDeviceRobustness2FeaturesEXT,
                vk::PhysicalDeviceDynamicRenderingLocalReadFeaturesKHR,
                vk::PhysicalDeviceShaderTileImageFeaturesEXT,
                vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
                vk::PhysicalDeviceRayQueryFeaturesKHR>();
            const auto& localReadSupported = featureChain.get<vk::PhysicalDeviceDynamicRenderingLocalReadFeaturesKHR>();
            const auto& tileImageSupported = featureChain.get<vk::PhysicalDeviceShaderTileImageFeaturesEXT>();
#else
            auto featureChain = physicalDevice_.getFeatures2<
                vk::PhysicalDeviceFeatures2,
                vk::PhysicalDeviceDescriptorIndexingFeatures,
                vk::PhysicalDeviceRobustness2FeaturesEXT,
                vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
                vk::PhysicalDeviceRayQueryFeaturesKHR>();
#endif
            const auto& coreFeaturesSupported = featureChain.get<vk::PhysicalDeviceFeatures2>().features;
            const auto& indexingFeaturesSupported = featureChain.get<vk::PhysicalDeviceDescriptorIndexingFeatures>();
            const auto& robust2Supported = featureChain.get<vk::PhysicalDeviceRobustness2FeaturesEXT>();
            const auto& accelerationStructureSupported = featureChain.get<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();
            const auto& rayQuerySupported = featureChain.get<vk::PhysicalDeviceRayQueryFeaturesKHR>();

            // ray query shader uses indexing into a (large) sampled-image array.
            // some drivers require this core feature to be explicitly enabled.
            if (coreFeaturesSupported.shaderSampledImageArrayDynamicIndexing) 
            {
                features.features.shaderSampledImageArrayDynamicIndexing = vk::True;
            }

            // prepare descriptor indexing features to enable if supported
            vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeaturesEnable{};
            descriptorIndexingEnabled_ = false;
            // enable non-uniform indexing of sampled image arrays when supported - required for
            // 'NonUniformResourceIndex()' in the ray-query shader to actually take effect
            if (indexingFeaturesSupported.shaderSampledImageArrayNonUniformIndexing) 
            {
                indexingFeaturesEnable.shaderSampledImageArrayNonUniformIndexing = vk::True;
                descriptorIndexingEnabled_ = true;
            }

            // these are not strictly required when writing a fully-populated descriptor array,
            // but enabling them when available avoids edge-case driver behavior for large arrays.
            if (descriptorIndexingEnabled_) 
            {
                if (indexingFeaturesSupported.descriptorBindingPartiallyBound) 
                {
                    indexingFeaturesEnable.descriptorBindingPartiallyBound = vk::True;
                }
                if (indexingFeaturesSupported.descriptorBindingUpdateUnusedWhilePending) 
                {
                    indexingFeaturesEnable.descriptorBindingUpdateUnusedWhilePending = vk::True;
                }
            }
            // optionally enable UpdateAfterBind flags when supported (not strictly required for RQ textures)
            if (indexingFeaturesSupported.descriptorBindingSampledImageUpdateAfterBind)
                indexingFeaturesEnable.descriptorBindingSampledImageUpdateAfterBind = vk::True;
            if (indexingFeaturesSupported.descriptorBindingUniformBufferUpdateAfterBind)
                indexingFeaturesEnable.descriptorBindingUniformBufferUpdateAfterBind = vk::True;
            if (indexingFeaturesSupported.descriptorBindingUpdateUnusedWhilePending)
                indexingFeaturesEnable.descriptorBindingUpdateUnusedWhilePending = vk::True;

            // helper to check if an extension is enabled (using string comparison) ...
            auto hasExtension = [&](const char* name) 
            {
                return std::find_if(deviceExtensions_.begin(),
                    deviceExtensions_.end(),
                    [&](const char* ext) 
                    {
                        return std::strcmp(ext, name) == 0;
                    }) != deviceExtensions_.end();
                };

            // prepare Robustness2 features if the extension is enabled and device supports ...
            auto hasRobust2 = hasExtension(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME);
            vk::PhysicalDeviceRobustness2FeaturesEXT robust2Enable{};
            if (hasRobust2) 
            {
                if (robust2Supported.robustBufferAccess2)
                    robust2Enable.robustBufferAccess2 = vk::True;
                if (robust2Supported.robustImageAccess2)
                    robust2Enable.robustImageAccess2 = vk::True;
                if (robust2Supported.nullDescriptor)
                    robust2Enable.nullDescriptor = vk::True;
            }

#ifdef _WIN32            
            // prepare 'Dynamic Rendering Local Read' features if extension is enabled and supported ...
            auto hasLocalRead = hasExtension(VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME);
            vk::PhysicalDeviceDynamicRenderingLocalReadFeaturesKHR localReadEnable{};
            if (hasLocalRead && localReadSupported.dynamicRenderingLocalRead) 
            {
                localReadEnable.dynamicRenderingLocalRead = vk::True;
            }

            // prepare 'Shader Tile Image' features if extension is enabled and supported ...
            auto hasTileImage = hasExtension(VK_EXT_SHADER_TILE_IMAGE_EXTENSION_NAME);
            vk::PhysicalDeviceShaderTileImageFeaturesEXT tileImageEnable{};
            if (hasTileImage) 
            {
                if (tileImageSupported.shaderTileImageColorReadAccess)
                    tileImageEnable.shaderTileImageColorReadAccess = vk::True;
                if (tileImageSupported.shaderTileImageDepthReadAccess)
                    tileImageEnable.shaderTileImageDepthReadAccess = vk::True;
                if (tileImageSupported.shaderTileImageStencilReadAccess)
                    tileImageEnable.shaderTileImageStencilReadAccess = vk::True;
            }
#else
            bool hasLocalRead = false;
            bool hasTileImage = false;
#endif

            // prepare 'Acceleration Structure' features if extension is enabled and supported ...
            auto hasAccelerationStructure = hasExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
            vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureEnable{};
            if (hasAccelerationStructure && accelerationStructureSupported.accelerationStructure) 
            {
                accelerationStructureEnable.accelerationStructure = vk::True;
            }

            // prepare 'Ray Query' features if extension is enabled and supported ...
            auto hasRayQuery = hasExtension(VK_KHR_RAY_QUERY_EXTENSION_NAME);
            vk::PhysicalDeviceRayQueryFeaturesKHR rayQueryEnable{};
            if (hasRayQuery && rayQuerySupported.rayQuery) 
            {
                rayQueryEnable.rayQuery = vk::True;
            }

            // chain the feature structures together (build pNext chain explicitly) ...
            // base
            features.pNext = &timelineSemaphoreFeatures;
            timelineSemaphoreFeatures.pNext = &memoryModelFeatures;
            memoryModelFeatures.pNext = &bufferDeviceAddressFeatures;
            bufferDeviceAddressFeatures.pNext = &storage8BitFeatures;
            storage8BitFeatures.pNext = &vulkan11Features; // link 1.1 first
            vulkan11Features.pNext = &vulkan13Features; // then 1.3 features

            // build tail chain starting at Vulkan 1.3 features
            void** tailNext = reinterpret_cast<void**>(&vulkan13Features.pNext);
            if (descriptorIndexingEnabled_) 
            {
                *tailNext = &indexingFeaturesEnable;
                tailNext = reinterpret_cast<void**>(&indexingFeaturesEnable.pNext);
            }
            if (hasRobust2) 
            {
                *tailNext = &robust2Enable;
                tailNext = reinterpret_cast<void**>(&robust2Enable.pNext);
            }
#ifdef _WIN32            
            if (hasLocalRead) 
            {
                *tailNext = &localReadEnable;
                tailNext = reinterpret_cast<void**>(&localReadEnable.pNext);
            }
            if (hasTileImage) 
            {
                *tailNext = &tileImageEnable;
                tailNext = reinterpret_cast<void**>(&tileImageEnable.pNext);
            }
#endif
            if (hasAccelerationStructure) 
            {
                *tailNext = &accelerationStructureEnable;
                tailNext = reinterpret_cast<void**>(&accelerationStructureEnable.pNext);
            }
            if (hasRayQuery) 
            {
                *tailNext = &rayQueryEnable;
                tailNext = reinterpret_cast<void**>(&rayQueryEnable.pNext);
            }

            // record which features ended up enabled (for runtime decisions/tutorial diagnostics) ...
            robustness2Enabled_ = hasRobust2 && (robust2Enable.robustBufferAccess2 == vk::True ||
                robust2Enable.robustImageAccess2 == vk::True ||
                robust2Enable.nullDescriptor == vk::True);
#ifdef _WIN32            
            dynamicRenderingLocalReadEnabled_ = hasLocalRead && (localReadEnable.dynamicRenderingLocalRead == vk::True);
            shaderTileImageEnabled_ = hasTileImage && (tileImageEnable.shaderTileImageColorReadAccess == vk::True ||
                tileImageEnable.shaderTileImageDepthReadAccess == vk::True ||
                tileImageEnable.shaderTileImageStencilReadAccess == vk::True);
#endif            
            accelerationStructureEnabled_ = hasAccelerationStructure && (accelerationStructureEnable.accelerationStructure == vk::True);
            rayQueryEnabled_ = hasRayQuery && (rayQueryEnable.rayQuery == vk::True);

            // one-time startup diagnostics (ray query + texture array indexing) ...
            static bool printedFeatureDiag = false;
            if (!printedFeatureDiag) 
            {
                printedFeatureDiag = true;
                Logger::Info("[DeviceFeatures] shaderSampledImageArrayDynamicIndexing="
                    + std::string(features.features.shaderSampledImageArrayDynamicIndexing == vk::True ? "ON" : "OFF")
                    + std::string(", shaderSampledImageArrayNonUniformIndexing=")
                    + std::string(indexingFeaturesEnable.shaderSampledImageArrayNonUniformIndexing == vk::True ? "ON" : "OFF")
                    + std::string(", descriptorIndexingEnabled=")
                    + std::string(descriptorIndexingEnabled_ ? "true" : "false")
                );
            }

            // device layers are deprecated and ignored, so we only configure extensions and features here
            // validation is enabled via instance layers.
            vk::DeviceCreateInfo createInfo
            {
              .pNext = &features,
              .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
              .pQueueCreateInfos = queueCreateInfos.data(),
              .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions_.size()),
              .ppEnabledExtensionNames = deviceExtensions_.data(),
              .pEnabledFeatures = nullptr // using pNext for features
            };

            // create the logical device
            device_ = vk::raii::Device(physicalDevice_, createInfo);

            // get queue handles ...
            graphicsQueue_ = vk::raii::Queue(device_, queueFamilyIndices_.GraphicsFamily.value(), 0);
            presentQueue_ = vk::raii::Queue(device_, queueFamilyIndices_.PresentFamily.value(), 0);
            computeQueue_ = vk::raii::Queue(device_, queueFamilyIndices_.ComputeFamily.value(), 0);
            transferQueue_ = vk::raii::Queue(device_, queueFamilyIndices_.TransferFamily.value(), 0);

            // create global timeline semaphore for uploads early (needed before default texture creation) ...
            vk::StructureChain<vk::SemaphoreCreateInfo, vk::SemaphoreTypeCreateInfo> timelineChain(
                {},
                { .semaphoreType = vk::SemaphoreType::eTimeline, .initialValue = 0 }
            );
            uploadsTimeline_ = vk::raii::Semaphore(device_, timelineChain.get<vk::SemaphoreCreateInfo>());
            uploadTimelineLastSubmitted_.store(0, std::memory_order_relaxed);

            return true;
        }
        catch (const std::exception& e) 
        {
            Logger::Error("Failed to create logical device: " + std::string(e.what()));
            return false;
        }
    }

}
