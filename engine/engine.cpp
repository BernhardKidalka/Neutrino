#include "engine.h"

namespace Neutrino 
{

    Engine::Engine() : initialized_(false) 
    {
    }

    Engine::~Engine() 
    {
        Shutdown();
    }

    bool Engine::Initialize() 
    {
        if (initialized_) 
        {
            return true;
        }

        // initialize subsystems here (e.g., graphics, audio, input, etc.) ...

        initialized_ = true;
        return true;
    }

    void Engine::Shutdown() 
    {
        if (!initialized_) 
        {
            return;
        }

        // shutdown code here (e.g., release resources, stop subsystems, etc.) ...

        initialized_ = false;
    }

    bool Engine::IsInitialized() const 
    {
        return initialized_;
    }

} // namespace Neutrino
