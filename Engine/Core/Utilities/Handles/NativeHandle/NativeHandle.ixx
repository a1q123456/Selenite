export module Engine.Core.NativeHandle;
import std;

import Engine.Core.RawNativeHandle;

namespace Engine::Core
{
    export struct NativeHandle : RawNativeHandle<void*>
    {
        using Super = RawNativeHandle;
        using HandleType = void*;
        NativeHandle() = default;
        NativeHandle(std::nullptr_t) : Super(nullptr) {}
        template <typename TPointer>
        NativeHandle(TPointer* pointer) : Super(reinterpret_cast<HandleType>(pointer)) {}

        template<typename TType>
        auto As() -> TType
        {
            return reinterpret_cast<TType>(m_nativeHandle);
        }

        template <typename TPointer>
        auto operator=(TPointer* pointer) -> auto&
        {
            m_nativeHandle = reinterpret_cast<HandleType>(pointer);
            return *this;
        }

        auto operator=(std::nullptr_t) -> auto&
        {
            m_nativeHandle = nullptr;
            return *this;
        }

        auto operator==(NativeHandle another) const noexcept -> bool
        {
            return m_nativeHandle == another.m_nativeHandle;
        }
    };
}

