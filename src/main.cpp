#include <iostream>
#include "engine.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) 
{
    std::cout << "================================\n";
    std::cout << "Neutrino Engine - Version 0.0.1\n";
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

        // main loop
        while (engine.IsWindowOpen())
        {
            engine.PollEvents();
        }

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
