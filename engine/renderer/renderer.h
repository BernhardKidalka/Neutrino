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
//    File: Neutrino\engine\renderer\renderer.h
//  Author: B. Kidalka
//    Date: 2026-08-01
//
//    Lang: C++
//
// Descrip: Renderer declarations.
//
//------------------------------------------------------------------------------------------------------
#pragma once

#include "platform/window.h"

namespace Neutrino
{
    class Renderer
    {
    public:
        Renderer() = default;
        ~Renderer() = default;
        
        bool Initialize();
        void Shutdown();
        void RenderFrame();

    private:
        Window* platformWindow_{ nullptr };

    };
}
