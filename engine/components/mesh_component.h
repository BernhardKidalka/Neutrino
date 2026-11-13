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
// File          : Neutrino\engine\components\mesh_component.h
// Modifications : B. Kidalka
// Date          : 2026-11-13
// Language      : C++
// Description   : Mesh component declarations.
//
//------------------------------------------------------------------------------------------------------
#pragma once

#include <array>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace Neutrino
{
    // per-instance data for instanced rendering
    // note: using explicit float vectors instead of matrices for better control over GPU data layout
    struct InstanceData 
    {
        // model matrix as glm::mat4 (4x4)
        glm::mat4 modelMatrix{};

        // normal matrix as glm::mat3x4 (3 columns of vec4: xyz = normal matrix columns, w unused)
        glm::mat3x4 normalMatrix{};

        // material index for this instance
        uint32_t materialIndex{ 0 };

        InstanceData() 
        {
            // initialize as identity matrices
            modelMatrix = glm::mat4(1.0f);
            normalMatrix[0] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            normalMatrix[1] = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
            normalMatrix[2] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
            materialIndex = 0;
        }

        explicit InstanceData(const glm::mat4& transform, uint32_t matIndex = 0) 
        {
            // store model matrix directly
            modelMatrix = transform;

            // calculate normal matrix (inverse transpose of upper-left 3x3)
            glm::mat3 normalMat3 = glm::transpose(glm::inverse(glm::mat3(transform)));
            normalMatrix[0] = glm::vec4(normalMat3[0], 0.0f);
            normalMatrix[1] = glm::vec4(normalMat3[1], 0.0f);
            normalMatrix[2] = glm::vec4(normalMat3[2], 0.0f);

            materialIndex = matIndex;
        }

        // helper methods for backward compatibility ...
        [[nodiscard]] glm::mat4 getModelMatrix() const 
        {
            return modelMatrix;
        }

        void setModelMatrix(const glm::mat4& matrix) 
        {
            modelMatrix = matrix;

            // also update normal matrix when model matrix changes
            glm::mat3 normalMat3 = glm::transpose(glm::inverse(glm::mat3(matrix)));
            normalMatrix[0] = glm::vec4(normalMat3[0], 0.0f);
            normalMatrix[1] = glm::vec4(normalMat3[1], 0.0f);
            normalMatrix[2] = glm::vec4(normalMat3[2], 0.0f);
        }

        [[nodiscard]] glm::mat3 getNormalMatrix() const 
        {
            return
            {
                glm::vec3(normalMatrix[0]),
                glm::vec3(normalMatrix[1]),
                glm::vec3(normalMatrix[2])
            };
        }

        static vk::VertexInputBindingDescription getBindingDescription() 
        {
            constexpr vk::VertexInputBindingDescription bindingDescription(
                1,                              // binding (binding 1 for instance data)
                sizeof(InstanceData),           // stride
                vk::VertexInputRate::eInstance  // inputRate
            );
            return bindingDescription;
        }

        static std::array<vk::VertexInputAttributeDescription, 7> getAttributeDescriptions() 
        {
            constexpr uint32_t modelBase = offsetof(InstanceData, modelMatrix);
            constexpr uint32_t normalBase = offsetof(InstanceData, normalMatrix);
            constexpr uint32_t vec4Size = sizeof(glm::vec4);
            constexpr std::array<vk::VertexInputAttributeDescription, 7> attributeDescriptions = 
            {
                // model matrix columns (locations 4-7)
                vk::VertexInputAttributeDescription
                {
                    .location = 4,
                    .binding = 1,
                    .format = vk::Format::eR32G32B32A32Sfloat,
                    .offset = modelBase + 0u * vec4Size
                },
                vk::VertexInputAttributeDescription
                {
                    .location = 5,
                    .binding = 1,
                    .format = vk::Format::eR32G32B32A32Sfloat,
                    .offset = modelBase + 1u * vec4Size
                },
                vk::VertexInputAttributeDescription
                {
                    .location = 6,
                    .binding = 1,
                    .format = vk::Format::eR32G32B32A32Sfloat,
                    .offset = modelBase + 2u * vec4Size
                },
                vk::VertexInputAttributeDescription
                {
                    .location = 7,
                    .binding = 1,
                    .format = vk::Format::eR32G32B32A32Sfloat,
                    .offset = modelBase + 3u * vec4Size
                },
                
                // normal matrix columns (locations 8-10)
                vk::VertexInputAttributeDescription
                {
                    .location = 8,
                    .binding = 1,
                    .format = vk::Format::eR32G32B32A32Sfloat,
                    .offset = normalBase + 0u * vec4Size
                },
                vk::VertexInputAttributeDescription
                {
                    .location = 9,
                    .binding = 1,
                    .format = vk::Format::eR32G32B32A32Sfloat,
                    .offset = normalBase + 1u * vec4Size
                },
                vk::VertexInputAttributeDescription
                {
                    .location = 10,
                    .binding = 1,
                    .format = vk::Format::eR32G32B32A32Sfloat,
                    .offset = normalBase + 2u * vec4Size
                }
            };
            return attributeDescriptions;
        }

        // get all attribute descriptions for model matrix (4 vec4's)
        static std::array<vk::VertexInputAttributeDescription, 4> getModelMatrixAttributeDescriptions() 
        {
            constexpr uint32_t modelBase = offsetof(InstanceData, modelMatrix);
            constexpr uint32_t vec4Size = sizeof(glm::vec4);
            constexpr std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions = 
            {
                vk::VertexInputAttributeDescription
                {
                    .location = 4,
                    .binding = 1,
                    .format = vk::Format::eR32G32B32A32Sfloat,
                    .offset = modelBase + 0u * vec4Size
                },
                vk::VertexInputAttributeDescription
                {
                    .location = 5,
                    .binding = 1,
                    .format = vk::Format::eR32G32B32A32Sfloat,
                    .offset = modelBase + 1u * vec4Size
                },
                vk::VertexInputAttributeDescription
                {
                    .location = 6,
                    .binding = 1,
                    .format = vk::Format::eR32G32B32A32Sfloat,
                    .offset = modelBase + 2u * vec4Size
                },
                vk::VertexInputAttributeDescription
                {
                    .location = 7,
                    .binding = 1,
                    .format = vk::Format::eR32G32B32A32Sfloat,
                    .offset = modelBase + 3u * vec4Size
                }
            };
            return attributeDescriptions;
        }

        // get all attribute descriptions for normal matrix (3 vec4's)
        static std::array<vk::VertexInputAttributeDescription, 3> getNormalMatrixAttributeDescriptions() 
        {
            constexpr uint32_t normalBase = offsetof(InstanceData, normalMatrix);
            constexpr uint32_t vec4Size = sizeof(glm::vec4);
            constexpr std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions = 
            {
                vk::VertexInputAttributeDescription
                {
                    .location = 8,
                    .binding = 1,
                    .format = vk::Format::eR32G32B32A32Sfloat,
                    .offset = normalBase + 0u * vec4Size
                },
                vk::VertexInputAttributeDescription
                {
                    .location = 9,
                    .binding = 1,
                    .format = vk::Format::eR32G32B32A32Sfloat,
                    .offset = normalBase + 1u * vec4Size
                },
                vk::VertexInputAttributeDescription
                {
                    .location = 10,
                    .binding = 1,
                    .format = vk::Format::eR32G32B32A32Sfloat,
                    .offset = normalBase + 2u * vec4Size
                }
            };
            return attributeDescriptions;
        }
    };
    
    // mesh vertex
    struct Vertex 
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
        glm::vec4 tangent;

        bool operator==(const Vertex& other) const 
        {
            return position == other.position &&
                normal == other.normal &&
                texCoord == other.texCoord &&
                tangent == other.tangent;
        }

        static vk::VertexInputBindingDescription getBindingDescription() 
        {
            constexpr vk::VertexInputBindingDescription bindingDescription(
                0,                              // binding
                sizeof(Vertex),                 // stride
                vk::VertexInputRate::eVertex    // inputRate
            );
            return bindingDescription;
        }

        static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions() 
        {
            constexpr std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions = 
            {
                vk::VertexInputAttributeDescription
                {
                    .location = 0,
                    .binding = 0,
                    .format = vk::Format::eR32G32B32Sfloat,
                    .offset = offsetof(Vertex, position)
                },
                vk::VertexInputAttributeDescription
                {
                    .location = 1,
                    .binding = 0,
                    .format = vk::Format::eR32G32B32Sfloat,
                    .offset = offsetof(Vertex, normal)
                },
                vk::VertexInputAttributeDescription
                {
                    .location = 2,
                    .binding = 0,
                    .format = vk::Format::eR32G32Sfloat,
                    .offset = offsetof(Vertex, texCoord)
                },
                vk::VertexInputAttributeDescription
                {
                    .location = 3,
                    .binding = 0,
                    .format = vk::Format::eR32G32B32A32Sfloat,
                    .offset = offsetof(Vertex, tangent)
                }
            };
            return attributeDescriptions;
        }
    };

}
