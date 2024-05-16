
export module Engine.Support.RawNativeHandle;

namespace Engine::Support
{
    export template<typename TType>
    struct RawNativeHandle
    {
        using HandleType = TType;
        RawNativeHandle() = default;
        RawNativeHandle(HandleType value) : m_nativeHandle(value) {}

        HandleType GetValue()
        {
            return m_nativeHandle;
        }
    private:
        HandleType m_nativeHandle = {};
    };
}

