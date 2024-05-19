export module Engine.Core.String;
import Engine.Core.Char;
import std;

namespace Engine::Core
{
    export using String = std::basic_string<Char>;
    export using StringView = std::basic_string_view<Char>;
    export using ZString = Char*;
    export using CZString = const Char*;
}

