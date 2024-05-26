module;
#include <Windows.h>
export module Engine.Core.SystemInfo;

namespace Engine::Core
{
    export struct SystemInfo
    {
        static auto Get() noexcept -> SYSTEM_INFO&;
    private:
        SystemInfo();
        SYSTEM_INFO info;
    };
}

