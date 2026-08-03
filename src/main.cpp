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
//    File: Neutrino\src\main.cpp
//  Author: B. Kidalka
//    Date: 2026-08-03
//
//    Lang: C++
//
// Descrip: Neutrino Engine main entry point.
//
//          Neutrino is a Real-Time Rendering and Compute Engine implemented with C++20, 
//          using Vulkan and GLFW for graphics and window management.
//
//          Neutrino is based on the engine example implementation from the Khronos Vulkan Tutorial 
//          (https://docs.vulkan.org/tutorial/latest/00_Introduction.html).
//
//------------------------------------------------------------------------------------------------------

#include "engine.h"

#include <iostream>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) 
{
    try 
    {
        const int WINDOW_WIDTH { 1024 };
        const int WINDOW_HEIGHT { 768 };
        const std::string appName{ "Neutrino Engine" };
        
        std::cout << "================================\n";
        std::cout << appName << " - Version 0.0.1 \n";
        std::cout << "================================\n\n";

        // create and initialize the engine ...
        Neutrino::Engine engine;

        if (!engine.Initialize(appName, WINDOW_WIDTH, WINDOW_HEIGHT)) 
        {
            std::cerr << "Error: Unable to initialize engine.\n";
            return 1;
        }

        std::cout << "Engine successfully initialized.\n";

        // enter engine main loop
        engine.Run();

        // shutdown the engine ...
        engine.Shutdown();

        return 0;
    }
    catch (const std::exception& ex) 
    {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
}
