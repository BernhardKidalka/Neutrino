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
// Date          : 2026-11-13
// Language      : C++
// Description   : Renderer pipelines implementation.
//
//------------------------------------------------------------------------------------------------------

#include "renderer.h"
#include "../core/logger.h"
#include "../components/mesh_component.h"

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

    // Create a graphics pipeline
    bool Renderer::createGraphicsPipeline() 
    {
        try 
        {
            // read shader code (reverted to use the Slang-compiled texturedMesh shader)
            auto shaderCode = ReadFile("shaders/texturedMesh.spv");

            // create shader module
            vk::raii::ShaderModule shaderModule = createShaderModule(shaderCode);

            // create shader stage info ...
            vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
            vertShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
            vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
            vertShaderStageInfo.module = *shaderModule;
            vertShaderStageInfo.pName = "VSMain";

            vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
            fragShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
            fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
            fragShaderStageInfo.module = *shaderModule;
            fragShaderStageInfo.pName = "PSMain";

            vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

            // create vertex input info with instancing support ...
            auto vertexBindingDescription = Vertex::getBindingDescription();
            auto instanceBindingDescription = InstanceData::getBindingDescription();
            std::array<vk::VertexInputBindingDescription, 2> bindingDescriptions = 
            {
                vertexBindingDescription,
                instanceBindingDescription
            };

            auto vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
            auto instanceAttributeDescriptions = InstanceData::getAttributeDescriptions();

            // combine all attribute descriptions (no duplicates) ...
            std::vector<vk::VertexInputAttributeDescription> allAttributeDescriptions;
            allAttributeDescriptions.insert(allAttributeDescriptions.end(), vertexAttributeDescriptions.begin(), vertexAttributeDescriptions.end());
            allAttributeDescriptions.insert(allAttributeDescriptions.end(), instanceAttributeDescriptions.begin(), instanceAttributeDescriptions.end());

            // note: materialIndex attribute (location 11) is not used by current shaders

            vk::PipelineVertexInputStateCreateInfo vertexInputInfo
            {
                .vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size()),
                .pVertexBindingDescriptions = bindingDescriptions.data(),
                .vertexAttributeDescriptionCount = static_cast<uint32_t>(allAttributeDescriptions.size()),
                .pVertexAttributeDescriptions = allAttributeDescriptions.data()
            };

            // create input assembly info ...
            vk::PipelineInputAssemblyStateCreateInfo inputAssembly
            {
                .topology = vk::PrimitiveTopology::eTriangleList,
                .primitiveRestartEnable = VK_FALSE
            };

            // create viewport state info ...
            vk::PipelineViewportStateCreateInfo viewportState
            {
                .viewportCount = 1,
                .scissorCount = 1
            };

            // create rasterization state info ...
            vk::PipelineRasterizationStateCreateInfo rasterizer
            {
                .depthClampEnable = VK_FALSE,
                .rasterizerDiscardEnable = VK_FALSE,
                .polygonMode = vk::PolygonMode::eFill,
                .cullMode = vk::CullModeFlagBits::eNone,
                .frontFace = vk::FrontFace::eCounterClockwise,
                .depthBiasEnable = VK_FALSE,
                .lineWidth = 1.0f
            };

            // create multisample state info ...
            vk::PipelineMultisampleStateCreateInfo multisampling
            {
                .rasterizationSamples = vk::SampleCountFlagBits::e1,
                .sampleShadingEnable = VK_FALSE
            };

            // create depth stencil state info ...
            vk::PipelineDepthStencilStateCreateInfo depthStencil
            {
                .depthTestEnable = VK_TRUE,
                .depthWriteEnable = VK_TRUE,
                // use LessOrEqual so that the main shading pass works after a depth pre-pass
                .depthCompareOp = vk::CompareOp::eLessOrEqual,
                .depthBoundsTestEnable = VK_FALSE,
                .stencilTestEnable = VK_FALSE
            };

            // create a color blend attachment state ...
            vk::PipelineColorBlendAttachmentState colorBlendAttachment
            {
                .blendEnable = VK_FALSE,
                .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
            };

            // create color blend state info ...
            vk::PipelineColorBlendStateCreateInfo colorBlending
            {
                .logicOpEnable = VK_FALSE,
                .logicOp = vk::LogicOp::eCopy,
                .attachmentCount = 1,
                .pAttachments = &colorBlendAttachment
            };

            // create dynamic state info ...
            std::vector dynamicStates = 
            {
                vk::DynamicState::eViewport,
                vk::DynamicState::eScissor
            };

            vk::PipelineDynamicStateCreateInfo dynamicState
            {
                .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
                .pDynamicStates = dynamicStates.data()
            };

            // create pipeline layout ...
            vk::PipelineLayoutCreateInfo pipelineLayoutInfo
            {
                .setLayoutCount = 1,
                .pSetLayouts = &*descriptorSetLayout_,
                .pushConstantRangeCount = 0,
                .pPushConstantRanges = nullptr
            };

            pipelineLayout_ = vk::raii::PipelineLayout(device_, pipelineLayoutInfo);

            // create pipeline rendering info ...
            vk::Format depthFormat = findDepthFormat();
            Logger::Info("Creating main graphics pipeline with depth format: " + std::to_string(static_cast<int>(depthFormat)));

            vk::PipelineRenderingCreateInfo pipelineRenderingInfo{};
            pipelineRenderingInfo.sType = vk::StructureType::ePipelineRenderingCreateInfo;
            pipelineRenderingInfo.colorAttachmentCount = 1;
            pipelineRenderingInfo.pColorAttachmentFormats = &swapChainImageFormat_;
            pipelineRenderingInfo.depthAttachmentFormat = depthFormat;
            pipelineRenderingInfo.stencilAttachmentFormat = vk::Format::eUndefined;

            // create the graphics pipeline ...
            vk::PipelineRasterizationStateCreateInfo rasterizerBack = rasterizer;
            
            // disable back-face culling for opaque PBR to avoid disappearing geometry when
            // instance/model transforms flip winding (ensures PASS 1 actually shades pixels)
            rasterizerBack.cullMode = vk::CullModeFlagBits::eNone;

            vk::GraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
            pipelineInfo.pNext = &pipelineRenderingInfo;
            pipelineInfo.stageCount = 2;
            pipelineInfo.pStages = shaderStages;
            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizerBack;
            pipelineInfo.pMultisampleState = &multisampling;
            pipelineInfo.pDepthStencilState = &depthStencil;
            pipelineInfo.pColorBlendState = &colorBlending;
            pipelineInfo.pDynamicState = &dynamicState;
            pipelineInfo.layout = *pipelineLayout_;

            graphicsPipeline_ = vk::raii::Pipeline(device_, nullptr, pipelineInfo);
            
            return true;
        }
        catch (const std::exception& e) 
        {
            Logger::Error("Failed to create graphics pipeline: " + std::string(e.what()));
            return false;
        }
    }

}
