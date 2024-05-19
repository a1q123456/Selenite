export module Engine.Core.IntPtr;
import std;

import Engine.Core.RawNativeHandle;

namespace Engine::Core
{
    export struct IntPtr : public RawNativeHandle<std::intptr_t>
    {
        using Super = RawNativeHandle<std::intptr_t>;
        IntPtr() = default;
        IntPtr(std::intptr_t value) : Super(value) {}

        template<typename TType>
        auto As() -> TType
        {
            return reinterpret_cast<TType>(m_nativeHandle);
        }
    };
}

