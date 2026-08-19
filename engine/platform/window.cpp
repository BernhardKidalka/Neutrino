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
// File          : Neutrino\engine\platform\window.cpp
// Modifications : B. Kidalka
// Date          : 2026-08-19
// Language      : C++
// Description   : Platform-specific window implementation.
//
//------------------------------------------------------------------------------------------------------

#include "window.h"

#include <stdexcept>

#include "../core/logger.h"

namespace Neutrino
{
    bool Window::Initialize(const Desc& desc)
    {
        Logger::Info("Initializing platform window ...");
        
        desc_ = desc;
        
        if (!glfwInit()) 
        {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window_ = glfwCreateWindow(desc_.Width, desc_.Height, desc_.Title.c_str(), nullptr, nullptr);
        if (!window_) 
        {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwSetWindowUserPointer(window_, this);

        glfwSetFramebufferSizeCallback(window_, windowResizeCallback);
        glfwSetCursorPosCallback(window_, mousePositionCallback);
        glfwSetMouseButtonCallback(window_, mouseButtonCallback);
        glfwSetKeyCallback(window_, keyCallback);
        glfwSetCharCallback(window_, charCallback);

        glfwGetFramebufferSize(window_, &desc_.Width, &desc_.Height);

        Logger::Info("Platform window initialized successfully.");
        return true;
    }
    
    void Window::Shutdown() 
    {
        Logger::Info("Shutting down platform window ...");
        if (window_) 
        {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }
        glfwTerminate();
        Logger::Info("Platform window shut down successfully.");
    }
    
    bool Window::ProcessEvents() 
    {
        glfwPollEvents();
        return !glfwWindowShouldClose(window_);
    }

    bool Window::HasWindowResized() 
    {
        bool resized = windowResized_;
        windowResized_ = false;
        return resized;
    }
    
    bool Window::CreateVulkanSurface(VkInstance instance, VkSurfaceKHR* surface) 
    {
        if (glfwCreateWindowSurface(instance, window_, nullptr, surface) != VK_SUCCESS) 
        {
            return false;
        }
        return true;
    }
    
    void Window::SetResizeCallback(std::function<void(int, int)> callback) 
    {
        resizeCallback = std::move(callback);
    }

    void Window::SetMouseCallback(std::function < void(float, float, uint32_t) > callback) 
    {
        mouseCallback = std::move(callback);
    }

    void Window::SetKeyboardCallback(std::function < void(uint32_t, bool) > callback) 
    {
        keyboardCallback = std::move(callback);
    }

    void Window::SetCharCallback(std::function<void(uint32_t)> callback) 
    {
        characterCallback = std::move(callback);
    }

    std::string Window::GetWindowTitle() const
    {
        return std::string(glfwGetWindowTitle(window_));
    }
    
    void Window::SetWindowTitle(const std::string& title) 
    {
        if (window_) 
        {
            desc_.Title = title;
            glfwSetWindowTitle(window_, title.c_str());
        }
    }
    
    void Window::windowResizeCallback(GLFWwindow* glfwWindow, int width, int height) 
    {
        auto* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        window->desc_.Width = width;
        window->desc_.Height = height;
        window->windowResized_ = true;

        if (window->resizeCallback) 
        {
            window->resizeCallback(width, height);
        }
    }
    
    void Window::mousePositionCallback(GLFWwindow* glfwWindow, double xpos, double ypos) 
    {
        auto* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        if (window->mouseCallback) 
        {
            uint32_t buttons = 0;
            if (glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) 
            {
                buttons |= 0x01;
            }
            if (glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) 
            {
                buttons |= 0x02;
            }
            if (glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) 
            {
                buttons |= 0x04;
            }
            window->mouseCallback(static_cast<float>(xpos), static_cast<float>(ypos), buttons);
        }
    }
    
    void Window::mouseButtonCallback(
        GLFWwindow* glfwWindow, 
        [[maybe_unused]] int button, 
        [[maybe_unused]] int action, 
        [[maybe_unused]] int mods)
    {
        auto* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        if (window->mouseCallback) 
        {
            double xpos, ypos;
            glfwGetCursorPos(glfwWindow, &xpos, &ypos);
            uint32_t buttons = 0;
            if (glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) 
            {
                buttons |= 0x01;
            }
            if (glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) 
            {
                buttons |= 0x02;
            }
            if (glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) 
            {
                buttons |= 0x04;
            }
            window->mouseCallback(static_cast<float>(xpos), static_cast<float>(ypos), buttons);
        }
    }
    
    void Window::keyCallback(
        GLFWwindow* glfwWindow, 
        int key, 
        [[maybe_unused]] int scancode, 
        int action, 
        [[maybe_unused]] int mods)
    {
        auto* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        if (window->keyboardCallback) 
        {
            window->keyboardCallback(key, action != GLFW_RELEASE);
        }
    }
    
    void Window::charCallback(GLFWwindow* glfwWindow, unsigned int codepoint) 
    {
        auto* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        if (window->characterCallback) 
        {
            window->characterCallback(codepoint);
        }
    }

}
