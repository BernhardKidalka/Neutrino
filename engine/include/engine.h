#pragma once

namespace Neutrino 
{

    class Engine 
    {
    public:
        Engine();
        ~Engine();

        bool initialize();
        void shutdown();
        bool isInitialized() const;

    private:
        bool initialized_;
    };

} // namespace Neutrino
