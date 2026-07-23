#pragma once

namespace Neutrino 
{

    class Engine 
    {
    public:
        Engine();
        ~Engine();

        bool Initialize();
        void Shutdown();
        bool IsInitialized() const;

    private:
        bool initialized_;
    };

} // namespace Neutrino
