//------------------------------------------------------------------------------------------------------
// Copyright (c) 2025 Holochip Corporation
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
// Project       : Neutrino Engine
// File          : Neutrino\engine\renderer\renderer.h
// Modifications : B. Kidalka
// Date          : 2026-08-21
// Language      : C++
// Description   : Renderer declarations.
//
//------------------------------------------------------------------------------------------------------
#pragma once

#include "../platform/window.h"
#include "../resources/memory_pool.h"

#include <vector>
#include <string>
#include <optional>
#include <atomic>
#include <memory>

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_hpp_macros.hpp>
#include <vulkan/vulkan_raii.hpp>

// fallback defines for optional extension names (allow compiling against older headers)
#ifndef VK_EXT_ROBUSTNESS_2_EXTENSION_NAME
#	define VK_EXT_ROBUSTNESS_2_EXTENSION_NAME "VK_EXT_robustness2"
#endif
#ifndef VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME
#	define VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME "VK_KHR_dynamic_rendering_local_read"
#endif
#ifndef VK_EXT_SHADER_TILE_IMAGE_EXTENSION_NAME
#	define VK_EXT_SHADER_TILE_IMAGE_EXTENSION_NAME "VK_EXT_shader_tile_image"
#endif

namespace Neutrino
{

    struct QueueFamilyIndices 
    {
        std::optional<uint32_t> GraphicsFamily;
        std::optional<uint32_t> PresentFamily;
        std::optional<uint32_t> ComputeFamily;
        std::optional<uint32_t> TransferFamily; // optional dedicated transfer queue family

        [[nodiscard]] bool isComplete() const 
        {
            return GraphicsFamily.has_value() && PresentFamily.has_value() && ComputeFamily.has_value();
        }
    };
    
    struct SwapChainSupportDetails 
    {
        vk::SurfaceCapabilitiesKHR Capabilities;
        std::vector<vk::SurfaceFormatKHR> Formats;
        std::vector<vk::PresentModeKHR> PresentModes;
    };
    
    class Renderer
    {
    public:
        explicit Renderer(Window* platformWindow);
        ~Renderer();
        
        bool Initialize(const std::string& appName);
        void Shutdown();
        void RenderFrame();
        void WaitIdle();

    private:
        ///////////////////////////////////////////////////////////////////////////////////////////////
        // private methods ...

        // check if all required Vulkan validation layers are available
        bool checkValidationLayerSupport() const;
        // create Vulkan instance
        bool createInstance(const std::string& appName);
        // setup Vulkan debug messenger for validation layer messages
        bool setupDebugMessenger(bool enableValidationLayers);
        // create Vulkan surface for rendering
        bool createSurface();
        // add supported optional device extensions to the list of device extensions
        void addSupportedOptionalExtensions();
        // select a suitable physical device (GPU) for rendering
        bool selectPhysicalDevice();
        // create a logical device from the selected physical device
        bool createLogicalDevice_v1();
        bool createLogicalDevice();
        // create a swap chain for presenting rendered images to the surface
        bool createSwapChain();
        // cleanup and destroy the swap chain and its associated resources
        void cleanupSwapChain();
        // create image views for the swap chain images
        bool createImageViews();
        // setup dynamic rendering
        bool setupDynamicRendering();
        // create descriptor set layout for resource bindings
        bool createDescriptorSetLayout();

        // renderer utils ...
        // find queue family indices for the given physical device
        QueueFamilyIndices findQueueFamilies(const vk::raii::PhysicalDevice& device);
        // check if the given physical device supports all required device extensions
        bool checkDeviceExtensionSupport(vk::raii::PhysicalDevice& device);
        // query swap chain support details for the given physical device
        SwapChainSupportDetails querySwapChainSupport(const vk::raii::PhysicalDevice& device);
        // choose the best swap chain surface format from the available formats
        vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
        // choose the best swap chain present mode from the available present modes
        vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
        // choose the best swap chain extent (resolution) based on the surface capabilities
        vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // consts ...
        
        // Vulkan validation layers ...
        const std::vector<const char*> validationLayers_ = 
        {
            "VK_LAYER_KHRONOS_validation"
        };
        
        // required device extensions ...
        const std::vector<const char*> requiredDeviceExtensions_ = 
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        // optional device extensions ...
        const std::vector<const char*> optionalDeviceExtensions_ = 
        {
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
            VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
            // robustness and safety
            VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
            // tile/local memory friendly dynamic rendering readback
            VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME,
            // shader tile image for fast tile access
            VK_EXT_SHADER_TILE_IMAGE_EXTENSION_NAME,
            // ray query support for ray-traced rendering
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_RAY_QUERY_EXTENSION_NAME
        };

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // member variables ...
        
        // pointer to the platform window (GLFW window)
        Window* platformWindow_{ nullptr };
        // flag indicating whether the renderer has been initialized
        bool    initialized_{ false };
        // flag to control Vulkan validation layers (enabled in debug builds, disabled in release builds)
        bool    enableValidationLayers_{ false };
        // flag indicating whether the VK_EXT_descriptor_indexing (update-after-bind) path is enabled
        bool descriptorIndexingEnabled_ { false };
        bool descriptorBindingUniformBufferUpdateAfterBindEnabled_ { false };
        bool descriptorBindingSampledImageUpdateAfterBindEnabled_ { false };
        bool storageAfterBindEnabled_ { false };
        // feature toggles detected/enabled at device creation ...
        bool robustness2Enabled_ { false };
        bool dynamicRenderingLocalReadEnabled_ { false };
        bool shaderTileImageEnabled_ { false };
        bool rayQueryEnabled_ { false };
        bool accelerationStructureEnabled_ { false };
#ifdef ENABLE_OPACITY_MICROMAPS
        bool opacityMicromapEnabled_ = false; // VK_KHR_opacity_micromap
#endif

        // all device extensions (required + optional)
        std::vector<const char*> deviceExtensions_;
        
        // Vulkan RAII context
        vk::raii::Context context_;
        // Vulkan instance and debug messenger
        vk::raii::Instance instance_ { nullptr };
        vk::raii::DebugUtilsMessengerEXT debugMessenger_ { nullptr };
        
        // Vulkan surface
        vk::raii::SurfaceKHR surface_ = nullptr;
        // swap chain and images related ...
        vk::raii::SwapchainKHR swapChain_ = nullptr;
        std::vector<vk::Image> swapChainImages_;
        std::vector<vk::raii::ImageView> swapChainImageViews_;
        vk::Format swapChainImageFormat_ { vk::Format::eUndefined };
        vk::Extent2D swapChainExtent_ = { 0, 0 };
        // tracked layouts for swap chain images, initialized at swap chain creation and updated as we transition
        std::vector<vk::ImageLayout> swapChainImageLayouts_;
        
        // Vulkan device
        vk::raii::PhysicalDevice physicalDevice_ { nullptr };
        vk::raii::Device device_ { nullptr };
        // name of the selected physical device
        std::string renderDeviceName_;
        // memory pool for efficient memory management
        std::unique_ptr<MemoryPool> memoryPool_ { nullptr };

        // queue family indices
        QueueFamilyIndices queueFamilyIndices_;
        // Vulkan queues ...
        vk::raii::Queue graphicsQueue_ { nullptr };
        vk::raii::Queue presentQueue_ { nullptr };
        vk::raii::Queue computeQueue_ { nullptr };
        // dedicated transfer queue (falls back to graphics if unavailable)
        vk::raii::Queue transferQueue_ { nullptr };

        // upload timeline semaphore for transfer -> graphics handoff (signaled per upload)
        vk::raii::Semaphore uploadsTimeline_ { nullptr };
        // tracks last timeline value that has been submitted for signaling on uploadsTimeline
        std::atomic<uint64_t> uploadTimelineLastSubmitted_ { 0 };

        // the texture that will hold a snapshot of the opaque scene
        // one off-screen color image per frame-in-flight to avoid cross-frame read/write hazards
        std::vector<vk::raii::Image> opaqueSceneColorImages_;
        std::vector<std::unique_ptr<MemoryPool::Allocation>> opaqueSceneColorImageAllocations_;
        std::vector<vk::raii::ImageView> opaqueSceneColorImageViews_;
        // track the current layout per frame (initialized to eUndefined at creation)
        std::vector<vk::ImageLayout> opaqueSceneColorImageLayouts_;
        vk::raii::Sampler opaqueSceneColorSampler_{ nullptr };
        
        // dynamic rendering setup ...
        vk::RenderingInfo dynamicRenderingInfo_;
        std::vector<vk::RenderingAttachmentInfo> colorAttachments_;
        vk::RenderingAttachmentInfo depthAttachment_;
        
        // descriptor set layouts (declared before pools and sets)
        vk::raii::DescriptorSetLayout descriptorSetLayout_ { nullptr };

    };
}
