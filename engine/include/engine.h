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
//    File: Neutrino\engine\include\engine.h
//  Author: B. Kidalka
//    Date: 2026-08-07
//
//    Lang: C++
//
// Descrip: Neutrino Engine declarations.
//
//------------------------------------------------------------------------------------------------------
#pragma once

#include "../platform/window.h"
#include "../renderer/renderer.h"

namespace Neutrino 
{
    class Engine 
    {
    public:
        Engine() = default;
        ~Engine();

        bool Initialize(const std::string& appName, int windowWidth, int windowHeight);
        void Run();
        void Shutdown();
        bool IsInitialized() const;

    private:
        void handleResize(int width, int height) const;
        void handleMouseInput(float x, float y, uint32_t buttons);
        void handleKeyInput(uint32_t key, bool pressed);

        // engine state ...
        bool initialized_ { false };
        bool running_{ false };
        // sub-systems ...
        std::unique_ptr<Window>     platformWindow_;
        std::unique_ptr<Renderer>   renderer_;
    };

} // namespace Neutrino
