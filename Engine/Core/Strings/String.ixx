module;

#include <string>
#include <string_view>

export module Engine.Core.String;
import Engine.Core.Char;

namespace Engine::Core
{
    export using String = std::basic_string<Char>;
    export using StringView = std::basic_string_view<Char>;
    export using ZString = Char*;
    export using CZString = const Char*;
}

