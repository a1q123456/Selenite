export module Engine.Core.Utilities.Overloaded;

namespace Engine::Core::Utilities
{
    // helper type for the visitor #4
    export template<typename... Ts>
    struct Overloaded : Ts... { using Ts::operator()...; };
}
