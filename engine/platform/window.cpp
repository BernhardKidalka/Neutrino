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
//    File: Neutrino\engine\platform\window.cpp
//  Author: B. Kidalka
//    Date: 2026-07-31
//
//    Lang: C++
//
// Descrip: Platform-specific window implementation.
//
//------------------------------------------------------------------------------------------------------

#include "window.h"

#include <stdexcept>

namespace Neutrino
{
    bool Window::Initialize(const Desc& desc)
    {
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

        return true;
    }
    
    void Window::Shutdown() 
    {
        if (window_) 
        {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }
        glfwTerminate();
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
