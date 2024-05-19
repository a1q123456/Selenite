export module Engine.Core.Logging;
import Engine.Core.String;
import std;

namespace Engine::Core
{
    export auto Log(CZString message) noexcept;

    export template<typename ...TArgs>
    auto Log(std::basic_format_string<Char, TArgs...> message, TArgs&&... args)
    {
        auto str = std::format(message, std::forward<TArgs>(args)...);
        Log(str.c_str());
    }
}

