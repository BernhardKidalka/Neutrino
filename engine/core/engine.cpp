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
//    File: Neutrino\engine\core\engine.cpp
//  Author: B. Kidalka
//    Date: 2026-08-08
//
//    Lang: C++
//
// Descrip: Neutrino Engine implementation.
//
//------------------------------------------------------------------------------------------------------

#include "engine.h"
#include "logger.h"

namespace Neutrino 
{
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // construction and destruction ...

    Engine::~Engine()
    {
        Shutdown();
    }
    
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // public methods ...

    bool Engine::Initialize(const std::string& appName, int windowWidth, int windowHeight)
    {
        if (initialized_) 
        {
            return true;
        }
        
        Logger::Init();
        Logger::Info("Starting engine initialization ...");

        // create platform window ...
        platformWindow_ = CreateWindow();
        
        Window::Desc windowDesc 
        {
            .Width = windowWidth,
            .Height = windowHeight,
            .Title = appName,
        };
        if (!platformWindow_->Initialize(windowDesc)) 
        {
            Logger::Error("Failed to initialize platform window!");
            return false;
        }
        
        // set resize callback
        platformWindow_->SetResizeCallback([this](int width, int height) 
        {
            handleResize(width, height);
        });

        // set mouse callback
        platformWindow_->SetMouseCallback([this](float x, float y, uint32_t buttons) 
        {
            handleMouseInput(x, y, buttons);
        });

        // set keyboard callback
        platformWindow_->SetKeyboardCallback([this](uint32_t key, bool pressed) 
        {
            handleKeyInput(key, pressed);
        });

        // set char callback
        platformWindow_->SetCharCallback([this]([[maybe_unused]] uint32_t c)
        {
            //if (imguiSystem) 
            //{
            //    imguiSystem->HandleChar(c);
            //}
        });

        // create renderer ...
        renderer_ = std::make_unique<Renderer>(platformWindow_.get());
        if (!renderer_->Initialize(appName)) 
        {
            Logger::Error("Failed to initialize renderer!");
            return false;
        }

        Logger::Info("Engine initialized successfully.");
        initialized_ = true;
        return true;
    }
    
    void Engine::Run()
    {
        if (!initialized_)
        {
            throw std::runtime_error("Neutrino Engine is not initialized!");
        }

        running_ = true;

        // enter engine main loop ...
        Logger::Info("Starting engine main loop ...");
        while (running_) 
        {
            // process platform events ...
            if (!platformWindow_->ProcessEvents()) 
            {
                running_ = false;
                break;
            }
        }
    }

    void Engine::Shutdown() 
    {
        if (!initialized_)
        {
            return;
        }
        
        Logger::Info("Shutting down engine ...");
        
        if (renderer_)
        {
            renderer_->Shutdown();
            renderer_.reset();
        }
        if (platformWindow_)
        {
            platformWindow_->Shutdown();
            platformWindow_.reset();
        }
        
        initialized_ = false;
        Logger::Info("Engine shut down successfully.");
    }

    bool Engine::IsInitialized() const 
    {
        return initialized_;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // private methods ...
    
    void Engine::handleResize(int width, int height) const 
    {
        if (height <= 0 || width <= 0) 
        {
            return;
        }
        // TODO:
        // update the active camera's aspect ratio
        // notify the renderer that the framebuffer has been resized
        // notify ImGui system about the resize
    }
    
    void Engine::handleMouseInput([[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] uint32_t buttons)
    {
        // TODO: mouse input handling logic ...    
    }

    void Engine::handleKeyInput([[maybe_unused]] uint32_t key, [[maybe_unused]] bool pressed)
    {
        // TODO: keyboard input handling logic ...
    }

} // namespace Neutrino
