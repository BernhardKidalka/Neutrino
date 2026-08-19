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
// File          : Neutrino\engine\platform\window.h
// Modifications : B. Kidalka
// Date          : 2026-08-19
// Language      : C++
// Description   : Platform-specific window declarations.
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

        // default constructor
        Window() = default;

        // initialize the window with the given description
        bool Initialize(const Desc& desc);

        // shutdown and cleanup the window
        void Shutdown();

        // process window events and return false if window should close
        bool ProcessEvents();

        // check if the window has been resized since last frame
        bool HasWindowResized();

        // get the window width in pixels
        [[nodiscard]] int GetWindowWidth() const { return desc_.Width; }

        // get the window height in pixels
        [[nodiscard]] int GetWindowHeight() const { return desc_.Height; }

        // get the window size in pixels
        void GetWindowSize(int* width, int* height) const 
        {
            *width = GetWindowWidth();
            *height = GetWindowHeight();
        }

        // create a Vulkan surface for rendering
        bool CreateVulkanSurface(VkInstance instance, VkSurfaceKHR* surface);

        // set callback function for window resize events
        void SetResizeCallback(std::function<void(int, int)> callback);

        // set callback function for mouse movement and button events
        void SetMouseCallback(std::function<void(float, float, uint32_t)> callback);

        // set callback function for keyboard events
        void SetKeyboardCallback(std::function<void(uint32_t, bool)> callback);

        // set callback function for character input events
        void SetCharCallback(std::function<void(uint32_t)> callback);
        // get the current window title
        std::string GetWindowTitle() const;
        // set the window title
        void SetWindowTitle(const std::string& title);

        // get the underlying GLFW window handle
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
        // callback invoked when the window is resized
        static void windowResizeCallback(GLFWwindow* glfwWindow, int width, int height);

        // callback invoked when the mouse moves
        static void mousePositionCallback(GLFWwindow* glfwWindow, double xpos, double ypos);

        // callback invoked when a mouse button is pressed or released
        static void mouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, int mods);

        // callback invoked when a keyboard key is pressed or released
        static void keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods);

        // callback invoked when a character is input
        static void charCallback(GLFWwindow* glfwWindow, unsigned int codepoint);

    };

    inline std::unique_ptr<Window> CreateWindow()
    {
        return std::make_unique<Window>();
    }

}
