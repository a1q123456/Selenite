module;
#include <Windows.h>
module Engine.Core.SystemInfo;

namespace Engine::Core
{
    SystemInfo::SystemInfo()
    {
        GetSystemInfo(&info);
    }
    auto SystemInfo::Get() noexcept -> SYSTEM_INFO&
    {
        static SystemInfo info;
        return info.info;
    }
}
