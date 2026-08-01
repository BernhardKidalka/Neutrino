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
//    File: Neutrino\engine\platform\window.h
//  Author: B. Kidalka
//    Date: 2026-08-01
//
//    Lang: C++
//
// Descrip: Platform-specific window declarations.
//
//------------------------------------------------------------------------------------------------------
#pragma once

#include <string>
#include <memory>
#include <functional>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Neutrino
{
    class Window 
    {
    public:
        struct Desc 
        {
            int Width{ 800 };
            int Height{ 600 };
            std::string Title;
        };

        Window() = default;

        bool Initialize(const Desc& desc);
        void Shutdown();
        bool ProcessEvents();
        bool HasWindowResized();
        [[nodiscard]] int GetWindowWidth() const { return desc_.Width; }
        [[nodiscard]] int GetWindowHeight() const { return desc_.Height; }
        bool CreateVulkanSurface(VkInstance instance, VkSurfaceKHR* surface);
        void SetResizeCallback(std::function<void(int, int)> callback);
        void SetMouseCallback(std::function<void(float, float, uint32_t)> callback);
        void SetKeyboardCallback(std::function<void(uint32_t, bool)> callback);
        void SetCharCallback(std::function<void(uint32_t)> callback);
        void SetWindowTitle(const std::string& title);
        GLFWwindow* GetWindow() const { return window_; }

    private:
        // GLFW window handle
        GLFWwindow* window_{ nullptr };
        // window description
        Desc        desc_;
        bool        windowResized_{ false };

        // callback functions for window events ...
        std::function<void(int, int)> resizeCallback;
        std::function<void(float, float, uint32_t)> mouseCallback;
        std::function<void(uint32_t, bool)> keyboardCallback;
        std::function<void(uint32_t)> characterCallback;

        // GLFW callback functions (static member functions) ...
        static void windowResizeCallback(GLFWwindow* glfwWindow, int width, int height);
        static void mousePositionCallback(GLFWwindow* glfwWindow, double xpos, double ypos);
        static void mouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, int mods);
        static void keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods);
        static void charCallback(GLFWwindow* glfwWindow, unsigned int codepoint);

    };

    inline std::unique_ptr<Window> CreateWindow()
    {
        return std::make_unique<Window>();
    }

}
