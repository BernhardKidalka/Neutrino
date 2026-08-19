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
// File          : Neutrino\engine\renderer\renderer.cpp
// Modifications : B. Kidalka
// Date          : 2026-08-19
// Language      : C++
// Description   : Renderer implementation.
//
//------------------------------------------------------------------------------------------------------

#include "renderer.h"
#include "../core/logger.h"

namespace Neutrino
{
    bool Renderer::createSwapChain() 
    {
        try 
        {
            // query swap chain support
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice_);

            // choose swap chain surface format, present mode, and extent ...
            vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.Formats);
            vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.PresentModes);
            vk::Extent2D extent = chooseSwapExtent(swapChainSupport.Capabilities);

            // choose image count ...
            uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
            if (swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount) 
            {
                imageCount = swapChainSupport.Capabilities.maxImageCount;
            }

            // choose preTransform ...
            vk::SurfaceTransformFlagBitsKHR preTransform;
            if (swapChainSupport.Capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) 
            {
                preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
            }
            else 
            {
                preTransform = swapChainSupport.Capabilities.currentTransform;
            }

            // create swap chain info ...
            vk::SwapchainCreateInfoKHR createInfo
            {
                .surface = *surface_,
                .minImageCount = imageCount,
                .imageFormat = surfaceFormat.format,
                .imageColorSpace = surfaceFormat.colorSpace,
                .imageExtent = extent,
                .imageArrayLayers = 1,
                .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
                .preTransform = preTransform,
                .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
                .presentMode = presentMode,
                .clipped = VK_TRUE,
                .oldSwapchain = nullptr
            };

            // find queue families ...
            QueueFamilyIndices indices = findQueueFamilies(physicalDevice_);
            std::array<uint32_t, 2> queueFamilyIndicesLoc = 
            {
                indices.GraphicsFamily.value(), 
                indices.PresentFamily.value()
            };

            // set sharing mode ...
            if (indices.GraphicsFamily != indices.PresentFamily) 
            {
                createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
                createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndicesLoc.size());
                createInfo.pQueueFamilyIndices = queueFamilyIndicesLoc.data();
            }
            else 
            {
                createInfo.imageSharingMode = vk::SharingMode::eExclusive;
                createInfo.queueFamilyIndexCount = 0;
                createInfo.pQueueFamilyIndices = nullptr;
            }

            // create swap chain
            swapChain_ = vk::raii::SwapchainKHR(device_, createInfo);

            // get swap chain images
            swapChainImages_ = swapChain_.getImages();

            // swap chain images start in UNDEFINED layout; track per-image layout for correct barriers
            swapChainImageLayouts_.assign(swapChainImages_.size(), vk::ImageLayout::eUndefined);

            // store swap chain format and extent ...
            swapChainImageFormat_ = surfaceFormat.format;
            swapChainExtent_ = extent;

            return true;
        }
        catch (const std::exception& e) 
        {
            Logger::Error("Failed to create swap chain: " + std::string(e.what()));
            return false;
        }
    }
    
    void Renderer::cleanupSwapChain() 
    {
        // clean up swap chain image views
        swapChainImageViews_.clear();
        // clean up swap chain
        swapChain_ = vk::raii::SwapchainKHR(nullptr);
    }
    
    bool Renderer::createImageViews() 
    {
        try 
        {
            opaqueSceneColorImages_.clear();
            opaqueSceneColorImageAllocations_.clear();
            opaqueSceneColorImageViews_.clear();
            opaqueSceneColorImageLayouts_.clear();
            opaqueSceneColorSampler_ = nullptr;
            
            // resize image views vector
            swapChainImageViews_.clear();
            swapChainImageViews_.reserve(swapChainImages_.size());

            // create image view info ...
            vk::ImageViewCreateInfo createInfo
            {
                .viewType = vk::ImageViewType::e2D,
                .format = swapChainImageFormat_,
                .components = 
                {
                    .r = vk::ComponentSwizzle::eIdentity,
                    .g = vk::ComponentSwizzle::eIdentity,
                    .b = vk::ComponentSwizzle::eIdentity,
                    .a = vk::ComponentSwizzle::eIdentity
                },
                .subresourceRange = 
                {
                    .aspectMask = vk::ImageAspectFlagBits::eColor, 
                    .baseMipLevel = 0, 
                    .levelCount = 1, 
                    .baseArrayLayer = 0, 
                    .layerCount = 1
                }
            };

            // create image view for each swap chain image ...
            for (const auto& image : swapChainImages_) 
            {
                createInfo.image = image;
                swapChainImageViews_.emplace_back(device_, createInfo);
            }

            return true;
        }
        catch (const std::exception& e) 
        {
            Logger::Error("Failed to create image views: " + std::string(e.what()));
            return false;
        }
    }

    bool Renderer::setupDynamicRendering() 
    {
        try 
        {
            // create color attachment ...
            vk::RenderingAttachmentInfo colorAttach;
            colorAttach.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
            colorAttach.loadOp = vk::AttachmentLoadOp::eClear;
            colorAttach.storeOp = vk::AttachmentStoreOp::eStore;
            colorAttach.clearValue.color = vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f });
            colorAttachments_ = { colorAttach };

            // create depth attachment ...
            depthAttachment_.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            depthAttachment_.loadOp = vk::AttachmentLoadOp::eClear;
            depthAttachment_.storeOp = vk::AttachmentStoreOp::eStore;
            depthAttachment_.clearValue.depthStencil = vk::ClearDepthStencilValue{1.0f, 0};

            // create rendering info ...
            dynamicRenderingInfo_.renderArea = vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent_);
            dynamicRenderingInfo_.layerCount = 1;
            dynamicRenderingInfo_.colorAttachmentCount = static_cast<uint32_t>(colorAttachments_.size());
            dynamicRenderingInfo_.pColorAttachments = colorAttachments_.data();
            dynamicRenderingInfo_.pDepthAttachment = &depthAttachment_;

            return true;
        }
        catch (const std::exception& e) 
        {
            Logger::Error("Failed to setup dynamic rendering: " + std::string(e.what()));
            return false;
        }
    }

}
