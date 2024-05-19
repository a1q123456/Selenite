export module Engine.Core.RawNativeHandle;

namespace Engine::Core
{
    export template<typename TType>
    struct RawNativeHandle
    {
        using HandleType = TType;
        RawNativeHandle() = default;
        RawNativeHandle(HandleType value) : m_nativeHandle(value) {}

        HandleType GetValue() const
        {
            return m_nativeHandle;
        }

        virtual ~RawNativeHandle() noexcept = default;
    protected:
        HandleType m_nativeHandle = {};
    };
}

