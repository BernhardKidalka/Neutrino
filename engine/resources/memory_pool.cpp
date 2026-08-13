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
// File          : Neutrino\engine\resources\memory_pool.cpp
// Modifications : B. Kidalka
// Date          : 2026-08-13
// Language      : C++
// Description   : Memory Pool implementation.
//
//------------------------------------------------------------------------------------------------------

#include "memory_pool.h"
#include "../core/logger.h"

#include <algorithm>
#include <iostream>
#include <numeric>

#include <vulkan/vulkan.hpp>

namespace Neutrino
{
    MemoryPool::MemoryPool(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice) : 
        device_(device), 
        physicalDevice_(physicalDevice)
    {
    }

    MemoryPool::~MemoryPool() 
    {
        // RAII will handle cleanup automatically
        std::lock_guard lock(poolMutex_);
        pools_.clear();
    }

    bool MemoryPool::Initialize() 
    {
        std::lock_guard lock(poolMutex_);

        try 
        {
            // configure default pool settings based on typical usage patterns ...

            // vertex buffer pool: large allocations, device-local (increased for large models like bistro)
            ConfigurePool(
                PoolType::VERTEX_BUFFER,
                128 * 1024 * 1024,
                // 128MB blocks (doubled)
                4096,
                // 4KB allocation units
                vk::MemoryPropertyFlagBits::eDeviceLocal);

            // index buffer pool: medium allocations, device-local (increased for large models like bistro)
            ConfigurePool(
                PoolType::INDEX_BUFFER,
                64 * 1024 * 1024,
                // 64MB blocks (doubled)
                2048,
                // 2KB allocation units
                vk::MemoryPropertyFlagBits::eDeviceLocal);

            // uniform buffer pool: small allocations, host-visible
            // use 64-byte alignment to match nonCoherentAtomSize and prevent validation errors
            ConfigurePool(
                PoolType::UNIFORM_BUFFER,
                4 * 1024 * 1024,
                // 4MB blocks
                64,
                // 64B allocation units (aligned to nonCoherentAtomSize)
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

            // staging buffer pool: variable allocations, host-visible
            // use 64-byte alignment to match nonCoherentAtomSize and prevent validation errors
            ConfigurePool(
                PoolType::STAGING_BUFFER,
                16 * 1024 * 1024,
                // 16MB blocks
                64,
                // 64B allocation units (aligned to nonCoherentAtomSize)
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

            // texture image pool: use moderate block sizes to reduce allocation failures on mid-range GPUs
            ConfigurePool(
                PoolType::TEXTURE_IMAGE,
                64 * 1024 * 1024,
                // 64MB blocks (smaller blocks reduce contiguous allocation pressure)
                4096,
                // 4KB allocation units
                vk::MemoryPropertyFlagBits::eDeviceLocal);

            return true;
        }
        catch (const std::exception& e) 
        {
            Logger::Error("Failed to initialize memory pool: " + std::string(e.what()));
            return false;
        }
    }

    void MemoryPool::ConfigurePool(
        const PoolType poolType,
        const vk::DeviceSize blockSize,
        const vk::DeviceSize allocationUnit,
        const vk::MemoryPropertyFlags properties) 
    {
        PoolConfig config;
        config.BlockSize = blockSize;
        config.AllocationUnit = allocationUnit;
        config.Properties = properties;

        poolConfigs_[poolType] = config;
    }

    uint32_t MemoryPool::findMemoryType(const uint32_t typeFilter, const vk::MemoryPropertyFlags properties) const 
    {
        const vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice_.getMemoryProperties();

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
        {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) 
            {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type");
    }

    std::unique_ptr<MemoryPool::MemoryBlock> MemoryPool::createMemoryBlock(
        PoolType poolType, 
        vk::DeviceSize size, 
        vk::MemoryAllocateFlags allocFlags) 
    {
        auto configIt = poolConfigs_.find(poolType);
        if (configIt == poolConfigs_.end()) 
        {
            throw std::runtime_error("Pool type not configured");
        }

        const PoolConfig& config = configIt->second;

        // use the larger of the requested size or configured block size
        const vk::DeviceSize blockSize = std::max(size, config.BlockSize);

        // create a dummy buffer to get memory requirements for the memory type
        vk::BufferCreateInfo bufferInfo
        {
            .size = blockSize,
            .usage = 
                vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer |
                vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eTransferDst,
            .sharingMode = vk::SharingMode::eExclusive
        };

        vk::raii::Buffer dummyBuffer(device_, bufferInfo);
        vk::MemoryRequirements memRequirements = dummyBuffer.getMemoryRequirements();

        uint32_t memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, config.Properties);

        // allocate the memory block using the device-required size
        vk::MemoryAllocateInfo allocInfo
        {
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = memoryTypeIndex
        };

        // add allocation flags (e.g., VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT) if needed
        vk::MemoryAllocateFlagsInfo flagsInfo{};
        if (allocFlags != vk::MemoryAllocateFlags{}) 
        {
            flagsInfo.flags = allocFlags;
            allocInfo.pNext = &flagsInfo;
        }

        // create MemoryBlock with proper initialization to avoid default constructor issues
        auto block = std::unique_ptr<MemoryBlock>(new MemoryBlock
        {
            .Memory = vk::raii::DeviceMemory(device_, allocInfo),
            .Size = memRequirements.size,
            .Used = 0,
            .MemoryTypeIndex = memoryTypeIndex,
            .MappedPtr = nullptr,
            .IsMapped = false,
            .FreeList = {},
            .AllocationUnit = config.AllocationUnit
        });

        // map memory if it's host-visible
        block->IsMapped = (config.Properties & vk::MemoryPropertyFlagBits::eHostVisible) != vk::MemoryPropertyFlags{};
        if (block->IsMapped) 
        {
            block->MappedPtr = block->Memory.mapMemory(0, memRequirements.size);
        }
        else 
        {
            block->MappedPtr = nullptr;
        }

        // initialize a free list based on the actual allocated size
        const size_t numUnits = static_cast<size_t>(block->Size / config.AllocationUnit);
        block->FreeList.resize(numUnits, true); // all units initially free

        return block;
    }

    std::unique_ptr<MemoryPool::MemoryBlock> MemoryPool::createMemoryBlockWithType(
        PoolType poolType, 
        vk::DeviceSize size, 
        uint32_t memoryTypeIndex, 
        vk::MemoryAllocateFlags allocFlags) 
    {
        auto configIt = poolConfigs_.find(poolType);
        if (configIt == poolConfigs_.end()) 
        {
            throw std::runtime_error("Pool type not configured");
        }
        const PoolConfig& config = configIt->second;

        // allocate the memory block with the exact requested size
        vk::MemoryAllocateInfo allocInfo
        {
            .allocationSize = size,
            .memoryTypeIndex = memoryTypeIndex
        };

        // add allocation flags (e.g., VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT) if needed
        vk::MemoryAllocateFlagsInfo flagsInfo{};
        if (allocFlags != vk::MemoryAllocateFlags{}) 
        {
            flagsInfo.flags = allocFlags;
            allocInfo.pNext = &flagsInfo;
        }

        // determine properties from the chosen memory type
        const auto memProps = physicalDevice_.getMemoryProperties();
        if (memoryTypeIndex >= memProps.memoryTypeCount) 
        {
            throw std::runtime_error("Invalid memoryTypeIndex for createMemoryBlockWithType");
        }
        const vk::MemoryPropertyFlags typeProps = memProps.memoryTypes[memoryTypeIndex].propertyFlags;

        auto block = std::unique_ptr<MemoryBlock>(new MemoryBlock
        {
            .Memory = vk::raii::DeviceMemory(device_, allocInfo),
            .Size = size,
            .Used = 0,
            .MemoryTypeIndex = memoryTypeIndex,
            .MappedPtr = nullptr,
            .IsMapped = false,
            .FreeList = {},
            .AllocationUnit = config.AllocationUnit
        });

        block->IsMapped = (typeProps & vk::MemoryPropertyFlagBits::eHostVisible) != vk::MemoryPropertyFlags{};
        if (block->IsMapped) 
        {
            block->MappedPtr = block->Memory.mapMemory(0, size);
        }

        const size_t numUnits = static_cast<size_t>(block->Size / config.AllocationUnit);
        block->FreeList.resize(numUnits, true);

        return block;
    }

    std::pair<MemoryPool::MemoryBlock*, size_t> MemoryPool::findSuitableBlock(PoolType poolType, vk::DeviceSize size, vk::DeviceSize alignment) 
    {
        auto poolIt = pools_.find(poolType);
        if (poolIt == pools_.end()) 
        {
            poolIt = pools_.try_emplace(poolType).first;
        }

        auto& poolBlocks = poolIt->second;
        const PoolConfig& config = poolConfigs_[poolType];

        // calculate required units (accounting for size alignment)
        const vk::DeviceSize alignedSize = ((size + alignment - 1) / alignment) * alignment;
        const size_t requiredUnits = static_cast<size_t>((alignedSize + config.AllocationUnit - 1) / config.AllocationUnit);

        // search existing blocks for sufficient free space with proper offset alignment
        for (const auto& block : poolBlocks) 
        {
            const vk::DeviceSize unit = config.AllocationUnit;
            const size_t totalUnits = block->FreeList.size();

            size_t i = 0;
            while (i < totalUnits) 
            {
                // ensure starting unit produces an offset aligned to 'alignment'
                vk::DeviceSize startOffset = static_cast<vk::DeviceSize>(i) * unit;
                if ((alignment > 0) && (startOffset % alignment != 0)) 
                {
                    // advance i to the next unit that aligns with 'alignment'
                    const vk::DeviceSize remainder = startOffset % alignment;
                    const vk::DeviceSize advanceBytes = alignment - remainder;
                    const size_t advanceUnits = static_cast<size_t>((advanceBytes + unit - 1) / unit);
                    i += std::max<size_t>(advanceUnits, 1);
                    continue;
                }

                // from aligned i, check for consecutive free units
                size_t consecutiveFree = 0;
                size_t j = i;
                while (j < totalUnits && block->FreeList[j] && consecutiveFree < requiredUnits) 
                {
                    ++consecutiveFree;
                    ++j;
                }

                if (consecutiveFree >= requiredUnits) 
                {
                    return { block.get(), i };
                }

                // move past the checked range
                i = (j > i) ? j : (i + 1);
            }
        }

        // no suitable block found; create a new one on demand (no hard limits, allowed during rendering)
        try 
        {
            auto newBlock = createMemoryBlock(poolType, alignedSize);
            poolBlocks.push_back(std::move(newBlock));
            Logger::Info("Created new memory block (pool type: " + std::to_string(static_cast<int>(poolType)) + ")");
            return { poolBlocks.back().get(), 0 };
        }
        catch (const std::exception& e) 
        {
            Logger::Error("Failed to create new memory block: " + std::string(e.what()));
            return { nullptr, 0 };
        }
    }

    std::unique_ptr<MemoryPool::Allocation> MemoryPool::Allocate(PoolType poolType, vk::DeviceSize size, vk::DeviceSize alignment) 
    {
        std::lock_guard<std::mutex> lock(poolMutex_);

        auto [block, startUnit] = findSuitableBlock(poolType, size, alignment);
        if (!block) 
        {
            return nullptr;
        }

        const PoolConfig& config = poolConfigs_[poolType];

        // calculate required units (accounting for alignment)
        const vk::DeviceSize alignedSize = ((size + alignment - 1) / alignment) * alignment;
        const size_t requiredUnits = (alignedSize + config.AllocationUnit - 1) / config.AllocationUnit;

        // mark units as used
        for (size_t i = startUnit; i < startUnit + requiredUnits; ++i) 
        {
            block->FreeList[i] = false;
        }

        // create allocation info
        auto allocation = std::make_unique<Allocation>();
        allocation->Memory = *block->Memory;
        allocation->Offset = startUnit * config.AllocationUnit;
        allocation->Size = alignedSize;
        allocation->MemoryTypeIndex = block->MemoryTypeIndex;
        allocation->IsMapped = block->IsMapped;
        allocation->MappedPtr = block->IsMapped ? static_cast<char*>(block->MappedPtr) + allocation->Offset : nullptr;

        block->Used += alignedSize;

        return allocation;
    }

    void MemoryPool::Deallocate(std::unique_ptr<Allocation> allocation) 
    {
        if (!allocation) 
        {
            return;
        }

        std::lock_guard<std::mutex> lock(poolMutex_);

        // find the block that contains this allocation
        for (auto& [poolType, poolBlocks] : pools_) 
        {
            const PoolConfig& config = poolConfigs_[poolType];

            for (auto& block : poolBlocks) 
            {
                if (*block->Memory == allocation->Memory) 
                {
                    // calculate which units to free
                    size_t startUnit = allocation->Offset / config.AllocationUnit;
                    size_t numUnits = (allocation->Size + config.AllocationUnit - 1) / config.AllocationUnit;

                    // mark units as free
                    for (size_t i = startUnit; i < startUnit + numUnits; ++i) 
                    {
                        block->FreeList[i] = true;
                    }

                    block->Used -= allocation->Size;
                    return;
                }
            }
        }

        Logger::Warning("Warning: Could not find memory block for deallocation");
    }

    std::pair<vk::raii::Buffer, std::unique_ptr<MemoryPool::Allocation>> MemoryPool::CreateBuffer(
        const vk::DeviceSize size,
        const vk::BufferUsageFlags usage,
        const vk::MemoryPropertyFlags properties) 
    {
        // determine a pool type based on usage and properties
        PoolType poolType = PoolType::VERTEX_BUFFER;

        // check for host-visible requirements first (for instance buffers and staging)
        if (properties & vk::MemoryPropertyFlagBits::eHostVisible) 
        {
            poolType = PoolType::STAGING_BUFFER;
        }
        else if (usage & vk::BufferUsageFlagBits::eVertexBuffer) 
        {
            poolType = PoolType::VERTEX_BUFFER;
        }
        else if (usage & vk::BufferUsageFlagBits::eIndexBuffer) 
        {
            poolType = PoolType::INDEX_BUFFER;
        }
        else if (usage & vk::BufferUsageFlagBits::eUniformBuffer) 
        {
            poolType = PoolType::UNIFORM_BUFFER;
        }

        // create the buffer
        const vk::BufferCreateInfo bufferInfo
        {
            .size = size,
            .usage = usage,
            .sharingMode = vk::SharingMode::eExclusive
        };

        vk::raii::Buffer buffer(device_, bufferInfo);

        // get memory requirements
        vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

        // check if buffer requires device address support (for ray tracing)
        const bool needsDeviceAddress = (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) != vk::BufferUsageFlags{};

        std::unique_ptr<Allocation> allocation;

        if (needsDeviceAddress) 
        {
            // buffers with device address usage require VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT flag
            // create a dedicated memory block for this buffer (similar to image allocation)
            uint32_t memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

            std::lock_guard<std::mutex> lock(poolMutex_);
            auto poolIt = pools_.find(poolType);
            if (poolIt == pools_.end()) 
            {
                poolIt = pools_.try_emplace(poolType).first;
            }
            auto& poolBlocks = poolIt->second;
            auto block = createMemoryBlockWithType(
                poolType,
                memRequirements.size,
                memoryTypeIndex,
                vk::MemoryAllocateFlagBits::eDeviceAddress);

            // prepare allocation that uses the new block from offset 0
            allocation = std::make_unique<Allocation>();
            allocation->Memory = *block->Memory;
            allocation->Offset = 0;
            allocation->Size = memRequirements.size;
            allocation->MemoryTypeIndex = memoryTypeIndex;
            allocation->MappedPtr = block->MappedPtr;
            allocation->IsMapped = block->IsMapped;

            // mark the entire block as used
            block->Used = memRequirements.size;
            const size_t units = block->FreeList.size();
            for (size_t i = 0; i < units; ++i) 
            {
                block->FreeList[i] = false;
            }

            // keep the block owned by the pool for lifetime management
            poolBlocks.push_back(std::move(block));
        }
        else 
        {
            // normal pooled allocation path
            allocation = Allocate(poolType, memRequirements.size, memRequirements.alignment);
            if (!allocation) 
            {
                throw std::runtime_error("Failed to allocate memory from pool");
            }
        }

        // bind memory to buffer
        buffer.bindMemory(allocation->Memory, allocation->Offset);

        return { std::move(buffer), std::move(allocation) };
    }

    std::pair<vk::raii::Image, std::unique_ptr<MemoryPool::Allocation>> MemoryPool::CreateImage(
        uint32_t width,
        uint32_t height,
        vk::Format format,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        uint32_t mipLevels,
        vk::SharingMode sharingMode,
        const std::vector<uint32_t>& queueFamilyIndices) 
    {
        // create the image ...
        vk::ImageCreateInfo imageInfo
        {
            .imageType = vk::ImageType::e2D,
            .format = format,
            .extent = {width, height, 1},
            .mipLevels = std::max(1u, mipLevels),
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = tiling,
            .usage = usage,
            .sharingMode = sharingMode,
            .initialLayout = vk::ImageLayout::eUndefined
        };

        // if concurrent sharing is requested, provide queue family indices
        std::vector<uint32_t> fam = queueFamilyIndices;
        if (sharingMode == vk::SharingMode::eConcurrent && !fam.empty()) 
        {
            imageInfo.queueFamilyIndexCount = static_cast<uint32_t>(fam.size());
            imageInfo.pQueueFamilyIndices = fam.data();
        }

        vk::raii::Image image(device_, imageInfo);

        // get memory requirements for this image
        vk::MemoryRequirements memRequirements = image.getMemoryRequirements();

        // pick a memory type compatible with this image
        uint32_t memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        // create a dedicated memory block for this image with the exact type and size
        std::unique_ptr<Allocation> allocation; 
        {
            std::lock_guard<std::mutex> lock(poolMutex_);
            auto poolIt = pools_.find(PoolType::TEXTURE_IMAGE);
            if (poolIt == pools_.end()) 
            {
                poolIt = pools_.try_emplace(PoolType::TEXTURE_IMAGE).first;
            }
            auto& poolBlocks = poolIt->second;
            auto block = createMemoryBlockWithType(PoolType::TEXTURE_IMAGE, memRequirements.size, memoryTypeIndex);

            // prepare allocation that uses the new block from offset 0
            allocation = std::make_unique<Allocation>();
            allocation->Memory = *block->Memory;
            allocation->Offset = 0;
            allocation->Size = memRequirements.size;
            allocation->MemoryTypeIndex = memoryTypeIndex;
            allocation->MappedPtr = block->MappedPtr;
            allocation->IsMapped = block->IsMapped;

            // mark the entire block as used
            block->Used = memRequirements.size;
            const size_t units = block->FreeList.size();
            for (size_t i = 0; i < units; ++i) 
            {
                block->FreeList[i] = false;
            }

            // keep the block owned by the pool for lifetime management and deallocation support
            poolBlocks.push_back(std::move(block));
        }

        // bind memory to image
        image.bindMemory(allocation->Memory, allocation->Offset);

        return { std::move(image), std::move(allocation) };
    }

    std::pair<vk::DeviceSize, vk::DeviceSize> MemoryPool::GetMemoryUsage(PoolType poolType) const 
    {
        std::lock_guard<std::mutex> lock(poolMutex_);

        auto poolIt = pools_.find(poolType);
        if (poolIt == pools_.end()) 
        {
            return { 0, 0 };
        }

        auto [used, total] = std::accumulate(
            poolIt->second.begin(),
            poolIt->second.end(),
            std::pair<vk::DeviceSize, vk::DeviceSize>{0, 0},
            [](const auto& acc, const auto& block) 
            {
                return std::pair<vk::DeviceSize, vk::DeviceSize>{acc.first + block->Used, acc.second + block->Size};
            });

        return { used, total };
    }

    std::pair<vk::DeviceSize, vk::DeviceSize> MemoryPool::GetTotalMemoryUsage() const 
    {
        std::lock_guard<std::mutex> lock(poolMutex_);

        vk::DeviceSize totalUsed = 0;
        vk::DeviceSize totalAllocated = 0;

        for (const auto& [poolType, poolBlocks] : pools_) 
        {
            for (const auto& block : poolBlocks) 
            {
                totalUsed += block->Used;
                totalAllocated += block->Size;
            }
        }

        return { totalUsed, totalAllocated };
    }

    bool MemoryPool::PreAllocatePools() 
    {
        std::lock_guard<std::mutex> lock(poolMutex_);

        try 
        {
            Logger::Info("Pre-allocating initial memory blocks for pools ...");

            // pre-allocate at least one block for each pool type
            for (const auto& [poolType, config] : poolConfigs_) 
            {
                auto poolIt = pools_.find(poolType);
                if (poolIt == pools_.end()) 
                {
                    poolIt = pools_.try_emplace(poolType).first;
                }

                auto& poolBlocks = poolIt->second;
                if (poolBlocks.empty()) 
                {
                    // create initial block for this pool type
                    auto newBlock = createMemoryBlock(poolType, config.BlockSize);
                    poolBlocks.push_back(std::move(newBlock));
                    Logger::Info("  Pre-allocated block for pool type " + std::to_string(static_cast<int>(poolType)));
                }
            }

            Logger::Info("Memory pool pre-allocation completed successfully");
            return true;
        }
        catch (const std::exception& e) 
        {
            Logger::Error("Failed to pre-allocate memory pools: " + std::string(e.what()));
            return false;
        }
    }

    void MemoryPool::SetRenderingActive(bool active) 
    {
        std::lock_guard lock(poolMutex_);
        renderingActive_ = active;
    }

    bool MemoryPool::IsRenderingActive() const 
    {
        std::lock_guard<std::mutex> lock(poolMutex_);
        return renderingActive_;
    }
}
