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
//    Date: 2026-08-03
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
    Engine::~Engine()
    {
        Shutdown();
    }

    bool Engine::Initialize(const std::string& appName, int windowWidth, int windowHeight)
    {
        if (initialized_) 
        {
            return true;
        }
        
        Logger::Init();

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
} // namespace Neutrino
