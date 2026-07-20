#ifndef NEUTRINO_ENGINE_H
#define NEUTRINO_ENGINE_H

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

#endif // NEUTRINO_ENGINE_H
