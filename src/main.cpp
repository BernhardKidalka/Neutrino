//------------------------------------------------------------------------------------------------------
// Copyright(C) Bernhard Kidalka     (2026) 
//------------------------------------------------------------------------------------------------------
//
// Project: Neutrino Engine
//    File: Neutrino\src\main.cpp
//  Author: B. Kidalka
//    Date: 2026-07-25
//
//    Lang: C++
//
// Descrip: Neutrino Engine main entry point.
//
//          Neutrino is a Real-Time Rendering and Compute Engine written in C++20, 
//          based on Vulkan and GLFW for graphics and window management.
//
//------------------------------------------------------------------------------------------------------

#include <iostream>
#include "engine.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) 
{
    std::cout << "================================\n";
    std::cout << "Neutrino Engine - Version 0.0.1 \n";
    std::cout << "================================\n\n";

    try 
    {
        // create and initialize the engine ...
        Neutrino::Engine engine;

        if (!engine.Initialize()) 
        {
            std::cerr << "Error: Unable to initialize engine.\n";
            return 1;
        }

        std::cout << "Engine successfully initialized.\n";
        std::cout << "Application is running ...\n";

        // engine main loop
        while (engine.IsWindowOpen())
        {
            engine.PollEvents();
        }

        // shutdown the engine ...
        engine.Shutdown();
        std::cout << "Engine was shut down.\n";

        return 0;
    }
    catch (const std::exception& ex) 
    {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
}
