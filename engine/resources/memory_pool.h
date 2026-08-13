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
// File          : Neutrino\engine\resources\memory_pool.h
// Modifications : B. Kidalka
// Date          : 2026-08-13
// Language      : C++
// Description   : Memory Pool declarations.
//
//------------------------------------------------------------------------------------------------------
#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace Neutrino
{
    /**
     * @brief Memory pool allocator for Vulkan resources
     *
     * This class implements a memory pool system to reduce memory fragmentation
     * and improve allocation performance by pre-allocating large chunks of memory
     * and sub-allocating from them.
     */
    class MemoryPool
    {
    public:
        /**
         * @brief Types of memory pools based on usage patterns
         */
        enum class PoolType
        {
	        VERTEX_BUFFER,         // Device-local memory for vertex data
	        INDEX_BUFFER,          // Device-local memory for index data
	        UNIFORM_BUFFER,        // Host-visible memory for uniform data
	        STAGING_BUFFER,        // Host-visible memory for staging operations
	        TEXTURE_IMAGE          // Device-local memory for texture images
        };

        /**
         * @brief Allocation information for a memory block
         */
        struct Allocation
        {
	        vk::DeviceMemory Memory;             // the underlying device memory
	        vk::DeviceSize   Offset;             // offset within the memory block
	        vk::DeviceSize   Size;               // size of the allocation
	        uint32_t         MemoryTypeIndex;    // memory type index
	        void*            MappedPtr;          // mapped pointer (if applicable)
            bool             IsMapped;           // whether the memory is persistently mapped
        };

        /**
        * @brief Memory block within a pool
        */
        struct MemoryBlock
        {
            vk::raii::DeviceMemory Memory;          // RAII wrapper for device memory
            vk::DeviceSize         Size;            // total size of the block
            vk::DeviceSize         Used;            // currently used bytes
            uint32_t               MemoryTypeIndex; // memory type index
            void*                  MappedPtr;       // mapped pointer (if applicable)
            bool                   IsMapped;        // whether the block is mapped
            std::vector<bool>      FreeList;        // free list for sub-allocations
            vk::DeviceSize         AllocationUnit;  // size of each allocation unit
        };

        /**
         * @brief Constructor
         * @param device Vulkan device
         * @param physicalDevice Vulkan physical device
         */
        MemoryPool(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice);

        /**
         * @brief Destructor
         */
        ~MemoryPool();

        /**
         * @brief Initialize the memory pool with default configurations
         * @return True if initialization was successful
         */
        bool Initialize();

        /**
         * @brief Allocate memory from a specific pool
         * @param poolType Type of pool to allocate from
         * @param size Size of the allocation
         * @param alignment Required alignment
         * @return Allocation information, or nullptr if allocation failed
         */
        std::unique_ptr<Allocation> Allocate(PoolType poolType, vk::DeviceSize size, vk::DeviceSize alignment = 1);

        /**
         * @brief Free a previously allocated memory block
         * @param allocation The allocation to free
         */
        void Deallocate(std::unique_ptr<Allocation> allocation);

        /**
        * @brief Create a buffer using pooled memory
        * @param size Size of the buffer
        * @param usage Buffer usage flags
        * @param properties Memory properties
        * @return Pair of buffer and allocation info
        */
        std::pair<vk::raii::Buffer, std::unique_ptr<Allocation>> CreateBuffer(
            vk::DeviceSize          size,
            vk::BufferUsageFlags    usage,
            vk::MemoryPropertyFlags properties
        );

        /**
        * @brief Create an image using pooled memory
        * @param width Image width
        * @param height Image height
        * @param format Image format
        * @param tiling Image tiling
        * @param usage Image usage flags
        * @param properties Memory properties
        * @param mipLevels Number of mip levels
        * @param sharingMode Image sharing mode
        * @param queueFamilyIndices Queue family indices for shared images
        *
        * @return Pair of image and allocation info
        */
        std::pair<vk::raii::Image, std::unique_ptr<Allocation>> CreateImage(
            uint32_t                     width,
            uint32_t                     height,
            vk::Format                   format,
            vk::ImageTiling              tiling,
            vk::ImageUsageFlags          usage,
            vk::MemoryPropertyFlags      properties,
            uint32_t                     mipLevels = 1,
            vk::SharingMode              sharingMode = vk::SharingMode::eExclusive,
            const std::vector<uint32_t>& queueFamilyIndices = {}
        );

        /**
         * @brief Get memory usage statistics
         * @param poolType Type of pool to query
         * @return Pair of (used bytes, total bytes)
         */
        std::pair<vk::DeviceSize, vk::DeviceSize> GetMemoryUsage(PoolType poolType) const;

        /**
         * @brief Get total memory usage across all pools
         * @return Pair of (used bytes, total bytes)
         */
        std::pair<vk::DeviceSize, vk::DeviceSize> GetTotalMemoryUsage() const;

        /**
        * @brief Configure a specific pool type
        * @param poolType Type of pool to configure
        * @param blockSize Size of each memory block
        * @param allocationUnit Minimum allocation unit
        * @param properties Memory properties
        */
        void ConfigurePool(
            PoolType                poolType,
            vk::DeviceSize          blockSize,
            vk::DeviceSize          allocationUnit,
            vk::MemoryPropertyFlags properties
		);

        /**
         * @brief Pre-allocate initial memory blocks for configured pools
         * @return True if pre-allocation was successful
         */
        bool PreAllocatePools();

        /**
         * @brief Set rendering active state flag (informational only)
         * @param active Whether rendering is currently active
         */
        void SetRenderingActive(bool active);

        /**
         * @brief Check if rendering is currently active (informational only)
         * @return True if rendering is active
         */
        bool IsRenderingActive() const;

    private:
		// helper methods ...
		uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
		std::unique_ptr<MemoryBlock> createMemoryBlock(PoolType poolType, vk::DeviceSize size, vk::MemoryAllocateFlags allocFlags = {});
		// create a memory block with an explicit memory type index (used for images requiring a specific type)
		std::unique_ptr<MemoryBlock> createMemoryBlockWithType(
			PoolType poolType, 
			vk::DeviceSize size, 
			uint32_t memoryTypeIndex, 
			vk::MemoryAllocateFlags allocFlags = {}
		);
		std::pair<MemoryBlock*, size_t> findSuitableBlock(PoolType poolType, vk::DeviceSize size, vk::DeviceSize alignment);

        
        const vk::raii::Device&            device_;
        const vk::raii::PhysicalDevice&    physicalDevice_;
        vk::PhysicalDeviceMemoryProperties memPropsCache_ {};

        // pool configurations
        struct PoolConfig
        {
	        vk::DeviceSize          BlockSize;      // size of each memory block
	        vk::DeviceSize          AllocationUnit; // minimum allocation unit
	        vk::MemoryPropertyFlags Properties;     // memory properties
        };

        // memory pools for different types ...
        std::unordered_map<PoolType, std::vector<std::unique_ptr<MemoryBlock>>> pools_;
        std::unordered_map<PoolType, PoolConfig>                                poolConfigs_;

        // thread safety
        mutable std::mutex poolMutex_;

        // optional rendering state flag (no allocation restrictions enforced)
        bool renderingActive_ = false;
	};
}
