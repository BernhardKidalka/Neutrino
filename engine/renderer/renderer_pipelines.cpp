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
// File          : Neutrino\engine\renderer\renderer_pipelines.cpp
// Modifications : B. Kidalka
// Date          : 2026-08-19
// Language      : C++
// Description   : Renderer pipelines implementation.
//
//------------------------------------------------------------------------------------------------------

#include "renderer.h"
#include "../core/logger.h"

#include <string>

namespace Neutrino
{
    bool Renderer::createDescriptorSetLayout() 
    {
        try 
        {
            // create binding for a uniform buffer ...
            vk::DescriptorSetLayoutBinding uboLayoutBinding
            {
                .binding = static_cast<uint32_t>(0),
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .descriptorCount = static_cast<uint32_t>(1),
                .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                .pImmutableSamplers = nullptr
            };

            // create binding for texture sampler ...
            vk::DescriptorSetLayoutBinding samplerLayoutBinding
            {
                .binding = static_cast<uint32_t>(1),
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .descriptorCount = static_cast<uint32_t>(1),
                .stageFlags = vk::ShaderStageFlagBits::eFragment,
                .pImmutableSamplers = nullptr
            };

            // create descriptor set layout ...
            std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

            // descriptor indexing: set per-binding flags for UPDATE_AFTER_BIND if enabled ...
            vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
            std::array<vk::DescriptorBindingFlags, 2> bindingFlags{};
            if (descriptorIndexingEnabled_) 
            {
                if (descriptorBindingUniformBufferUpdateAfterBindEnabled_) 
                {
                    bindingFlags[0] = vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending;
                }
                if (descriptorBindingSampledImageUpdateAfterBindEnabled_) 
                {
                    bindingFlags[1] = vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending;
                }
                bindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
                bindingFlagsInfo.pBindingFlags = bindingFlags.data();
            }

            vk::DescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            layoutInfo.pBindings = bindings.data();
            if (descriptorIndexingEnabled_) 
            {
                layoutInfo.flags |= vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
                layoutInfo.pNext = &bindingFlagsInfo;
            }

            descriptorSetLayout_ = vk::raii::DescriptorSetLayout(device_, layoutInfo);
            return true;
        }
        catch (const std::exception& e) 
        {
            Logger::Error("Failed to create descriptor set layout: " + std::string(e.what()));
            return false;
        }
    }

}
