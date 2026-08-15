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
// File          : Neutrino\engine\renderer\renderer_utils.cpp
// Modifications : B. Kidalka
// Date          : 2026-08-15
// Language      : C++
// Description   : Renderer utils implementation.
//
//------------------------------------------------------------------------------------------------------

#include "renderer.h"
#include "core/logger.h"

#include <set>

namespace Neutrino
{
    void Renderer::WaitIdle() 
    {
        device_.waitIdle();
    }

    QueueFamilyIndices Renderer::findQueueFamilies(const vk::raii::PhysicalDevice& device) 
    {
        QueueFamilyIndices indices;

        // get queue family properties
        std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

        // find queue families that support graphics, compute, present, and (optionally) a dedicated transfer queue
        for (uint32_t i = 0; i < queueFamilies.size(); i++) 
        {
            const auto& qf = queueFamilies[i];
            // check for graphics support
            if ((qf.queueFlags & vk::QueueFlagBits::eGraphics) && !indices.GraphicsFamily.has_value()) 
            {
                indices.GraphicsFamily = i;
            }
            // check for compute support
            if ((qf.queueFlags & vk::QueueFlagBits::eCompute) && !indices.ComputeFamily.has_value()) 
            {
                indices.ComputeFamily = i;
            }
            // check for present support
            if (!indices.PresentFamily.has_value() && device.getSurfaceSupportKHR(i, *surface_)) 
            {
                indices.PresentFamily = i;
            }
            // prefer a dedicated transfer queue (transfer bit set, but NOT graphics) if available
            if ((qf.queueFlags & vk::QueueFlagBits::eTransfer) && !(qf.queueFlags & vk::QueueFlagBits::eGraphics)) 
            {
                if (!indices.TransferFamily.has_value()) 
                {
                    indices.TransferFamily = i;
                }
            }
            // if all required queue families are found, we can still continue to try to find a dedicated transfer queue
            if (indices.isComplete() && indices.TransferFamily.has_value()) 
            {
                // found everything including dedicated transfer
                break;
            }
        }

        // fallback: if no dedicated transfer queue, reuse graphics queue for transfer
        if (!indices.TransferFamily.has_value() && indices.GraphicsFamily.has_value()) 
        {
            indices.TransferFamily = indices.GraphicsFamily;
        }

        return indices;
    }

    bool Renderer::checkDeviceExtensionSupport(vk::raii::PhysicalDevice& device) 
    {
        auto availableDeviceExtensions = device.enumerateDeviceExtensionProperties();

        // check if all required extensions are supported ...
        std::set<std::string> requiredExtensionsSet(requiredDeviceExtensions_.begin(), requiredDeviceExtensions_.end());

        for (const auto& extension : availableDeviceExtensions) 
        {
            requiredExtensionsSet.erase(extension.extensionName);
        }

        // print missing required extensions ...
        if (!requiredExtensionsSet.empty()) 
        {
            Logger::Info("Missing required extensions:");
            for (const auto& extension : requiredExtensionsSet) 
            {
                Logger::Info("  " + std::string(extension));
            }
            return false;
        }

        return true;
    }
    
    SwapChainSupportDetails Renderer::querySwapChainSupport(const vk::raii::PhysicalDevice& device) 
    {
        SwapChainSupportDetails details;

        // get surface capabilities
        details.Capabilities = device.getSurfaceCapabilitiesKHR(*surface_);

        // get surface formats
        details.Formats = device.getSurfaceFormatsKHR(*surface_);

        // get present modes
        details.PresentModes = device.getSurfacePresentModesKHR(*surface_);

        return details;
    }
    
    vk::SurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) 
    {
        // look for SRGB format ...
        for (const auto& availableFormat : availableFormats) 
        {
            if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) 
            {
                return availableFormat;
            }
        }
        // if not found, return first available format
        return availableFormats[0];
    }
    
    vk::PresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) 
    {
        // look for mailbox mode (triple buffering) ...
        for (const auto& availablePresentMode : availablePresentModes) 
        {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox) 
            {
                return availablePresentMode;
            }
        }
        // if not found, return FIFO mode (guaranteed to be available on every platform)
        return vk::PresentModeKHR::eFifo;
    }
    
    vk::Extent2D Renderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) 
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) 
        {
            Logger::Info("Renderer: Using surface currentExtent: " 
                + std::to_string(capabilities.currentExtent.width) + " x " 
                + std::to_string(capabilities.currentExtent.height));
            return capabilities.currentExtent;
        }
        else 
        {
            // get framebuffer size
            int width, height;
            platformWindow_->GetWindowSize(&width, &height);

            Logger::Info("Renderer: surface extent is undefined. Using platform window size: " + std::to_string(width) + " x " + std::to_string(height));

            // create extent
            vk::Extent2D actualExtent = 
            {
              static_cast<uint32_t>(width),
              static_cast<uint32_t>(height)
            };

            // clamp to min/max extent ...
            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            Logger::Info("Renderer: Clamped extent: " + std::to_string(actualExtent.width) + " x " + std::to_string(actualExtent.height));

            return actualExtent;
        }
    }

}
