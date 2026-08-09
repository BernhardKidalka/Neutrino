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
//    File: Neutrino\engine\renderer\renderer_utils.cpp
//  Author: B. Kidalka
//    Date: 2026-08-09
//
//    Lang: C++
//
// Descrip: Renderer utils implementation.
//
//------------------------------------------------------------------------------------------------------

#include "renderer.h"
#include "core/logger.h"

#include <set>

namespace Neutrino
{
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

}
