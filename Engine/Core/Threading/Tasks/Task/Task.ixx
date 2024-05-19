export module Engine.Core.Threading:Task;
import std;

namespace Engine::Core::Threading
{
    export template<typename TResult>
    class Task
    {
    public:
        static auto Start(std::function<TResult()>&& entry) -> Task;
    };

    template<typename TResult>
    auto Task<TResult>::Start(std::function<TResult()>&& entry) -> Task
    {
        return Task();
    }
}
