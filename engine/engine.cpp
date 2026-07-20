#include "engine.h"

namespace Neutrino 
{

    Engine::Engine() : initialized_(false) 
    {
    }

    Engine::~Engine() 
    {
        shutdown();
    }

    bool Engine::initialize() 
    {
        if (initialized_) 
        {
            return true;
        }

        // initialize subsystems here (e.g., graphics, audio, input, etc.) ...

        initialized_ = true;
        return true;
    }

    void Engine::shutdown() 
    {
        if (!initialized_) 
        {
            return;
        }

        // shutdown code here (e.g., release resources, stop subsystems, etc.) ...

        initialized_ = false;
    }

    bool Engine::isInitialized() const 
    {
        return initialized_;
    }

} // namespace Neutrino
