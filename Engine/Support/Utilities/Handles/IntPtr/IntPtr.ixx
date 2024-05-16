module;
#include <cstdint>

export module Engine.Support.IntPtr;

import Engine.Support.RawNativeHandle;

namespace Engine::Support
{
    export class IntPtr : public RawNativeHandle<std::intptr_t>
    {
    public:
        using RawNativeHandle<std::intptr_t>::RawNativeHandle;
    };
}

