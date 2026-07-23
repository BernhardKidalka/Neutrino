#include "engine.h"
#include <iostream>
#include <string_view>

// define dynamic dispatch loader for Vulkan
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace Neutrino 
{
    Engine::Engine() 
        : initialized_(false), window_(nullptr), physical_device_(nullptr)
    {
    }

    Engine::~Engine() 
    {
    }

    bool Engine::Initialize() 
    {
        if (initialized_) 
        {
            return true;
        }

        if (!InitializeWindow())
        {
            std::cerr << "Failed to initialize window\n";
            return false;
        }

        if (!InitializeVulkan())
        {
            std::cerr << "Failed to initialize Vulkan\n";
            ShutdownWindow();
            return false;
        }

        initialized_ = true;
        return true;
    }

    void Engine::Shutdown() 
    {
        if (!initialized_) 
        {
            return;
        }

        ShutdownVulkan();
        ShutdownWindow();

        initialized_ = false;
    }

    bool Engine::IsInitialized() const 
    {
        return initialized_;
    }

    bool Engine::IsWindowOpen() const
    {
        return window_ != nullptr && !glfwWindowShouldClose(window_);
    }

    void Engine::PollEvents()
    {
        if (window_ != nullptr)
        {
            glfwPollEvents();
        }
    }

    bool Engine::InitializeWindow()
    {
        if (!glfwInit())
        {
            std::cerr << "Failed to initialize GLFW\n";
            return false;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window_ = glfwCreateWindow(
            WINDOW_WIDTH, 
            WINDOW_HEIGHT, 
            "Neutrino Engine", 
            nullptr, 
            nullptr
        );

        if (!window_)
        {
            std::cerr << "Failed to create GLFW window\n";
            glfwTerminate();
            return false;
        }

        return true;
    }

    bool Engine::InitializeVulkan()
    {
        try
        {
            if (!CreateInstance())
            {
                std::cerr << "Failed to create Vulkan instance\n";
                return false;
            }

            // Initialize function pointers for instance
            VULKAN_HPP_DEFAULT_DISPATCHER.init(vulkan_instance_.get());

            if (!CreateSurface())
            {
                std::cerr << "Failed to create Vulkan surface\n";
                return false;
            }

            if (!SelectPhysicalDevice())
            {
                std::cerr << "Failed to select physical device\n";
                return false;
            }

            if (!CreateLogicalDevice())
            {
                std::cerr << "Failed to create logical device\n";
                return false;
            }

            // Initialize function pointers for device (keeps instance dispatcher intact)
            VULKAN_HPP_DEFAULT_DISPATCHER.init(logical_device_.get());

            return true;
        }
        catch (const vk::SystemError& err)
        {
            std::cerr << "Vulkan error: " << err.what() << "\n";
            return false;
        }
        catch (const std::exception& err)
        {
            std::cerr << "Exception: " << err.what() << "\n";
            return false;
        }
    }

    bool Engine::CreateInstance()
    {
        // Initialize the dynamic dispatcher with global-level functions
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        // Get required extensions from GLFW
        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

        vk::ApplicationInfo app_info{
            .pApplicationName = "Neutrino Engine",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "Neutrino",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_3
        };

        vk::InstanceCreateInfo create_info{
            .pApplicationInfo = &app_info,
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()
        };

        vulkan_instance_ = vk::createInstanceUnique(create_info);
        return true;
    }

    bool Engine::CreateSurface()
    {
        VkSurfaceKHR c_surface = nullptr;
        if (glfwCreateWindowSurface(vulkan_instance_.get(), window_, nullptr, &c_surface) != VK_SUCCESS)
        {
            std::cerr << "Failed to create window surface\n";
            return false;
        }

        // Store the raw surface handle (will be manually destroyed)
        surface_ = vk::SurfaceKHR(c_surface);

        return true;
    }

    QueueFamilyIndices Engine::FindQueueFamilies(vk::PhysicalDevice device) const
    {
        QueueFamilyIndices indices;

        auto queue_families = device.getQueueFamilyProperties();

        int i = 0;
        for (const auto& queue_family : queue_families)
        {
            if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics)
            {
                indices.graphics_family = i;
            }

            if (device.getSurfaceSupportKHR(i, surface_))
            {
                // For now, we use the same queue family for graphics and presentation
                if (indices.graphics_family.has_value())
                {
                    break;
                }
            }

            i++;
        }

        return indices;
    }

    bool Engine::IsDeviceSuitable(vk::PhysicalDevice device) const
    {
        auto indices = FindQueueFamilies(device);

        auto extensions = device.enumerateDeviceExtensionProperties();
        bool extensions_supported = false;

        for (const auto& ext : extensions)
        {
            if (std::string_view(ext.extensionName) == VK_KHR_SWAPCHAIN_EXTENSION_NAME)
            {
                extensions_supported = true;
                break;
            }
        }

        return indices.IsComplete() && extensions_supported;
    }

    bool Engine::SelectPhysicalDevice()
    {
        auto devices = vulkan_instance_->enumeratePhysicalDevices();

        if (devices.empty())
        {
            std::cerr << "No physical devices found\n";
            return false;
        }

        auto it = std::find_if(devices.begin(), devices.end(),
            [this](vk::PhysicalDevice device) { return IsDeviceSuitable(device); });

        if (it == devices.end())
        {
            std::cerr << "No suitable physical device found\n";
            return false;
        }

        physical_device_ = *it;

        auto properties = physical_device_.getProperties();
        std::cout << "Selected GPU: " << properties.deviceName << "\n";

        return true;
    }

    bool Engine::CreateLogicalDevice()
    {
        auto indices = FindQueueFamilies(physical_device_);

        if (!indices.IsComplete())
        {
            std::cerr << "Queue families not complete\n";
            return false;
        }

        float queue_priority = 1.0f;
        vk::DeviceQueueCreateInfo queue_create_info{
            .queueFamilyIndex = indices.graphics_family.value(),
            .queueCount = 1,
            .pQueuePriorities = &queue_priority
        };

        const std::vector<const char*> device_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        vk::PhysicalDeviceFeatures device_features{};

        vk::DeviceCreateInfo create_info{
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queue_create_info,
            .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
            .ppEnabledExtensionNames = device_extensions.data(),
            .pEnabledFeatures = &device_features
        };

        logical_device_ = physical_device_.createDeviceUnique(create_info);

        graphics_queue_ = logical_device_->getQueue(indices.graphics_family.value(), 0);

        return true;
    }

    void Engine::ShutdownVulkan()
    {
        if (logical_device_)
        {
            logical_device_->waitIdle();
            logical_device_.reset();
        }

        physical_device_ = nullptr;

        // manually destroy surface before instance is destroyed ...
        if (surface_)
        {
            // need to use instance dispatcher to destroy surface
            VULKAN_HPP_DEFAULT_DISPATCHER.init(vulkan_instance_.get());
            vulkan_instance_->destroySurfaceKHR(surface_);
            surface_ = nullptr;
        }

        // vulkan_instance_ is destroyed automatically at end of scope
    }

    void Engine::ShutdownWindow()
    {
        if (window_ != nullptr)
        {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }
        glfwTerminate();
    }

} // namespace Neutrino
