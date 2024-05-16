module;
#include <concepts>
export module Engine.Support.Handle;
import Engine.Support.NativeHandleTraits;

namespace Engine::Support::Handle
{

    export template <typename TNativeHandle, std::invocable<TNativeHandle> TDeleter>
    class Handle
    {
    public:
        Handle() = default;
        Handle(Handle&& handle)
        {
            MoveFrom(std::move(handle));
        }
        Handle(const Handle&) = delete;

        Handle(TNativeHandle&& handle): m_handle(handle) { }

        Handle& operator=(const Handle&) = delete;
        Handle& operator=(Handle&& handle)
        {
            MoveFrom(handle);
            return *this;
        }

        operator bool() const
        {
            return m_handle;
        }

        operator TNativeHandle() const
        {
            return m_handle;
        }

        TNativeHandle& operator&()
        {
            return ReleaseAndGetAddressOf();
        }

        void Release()
        {
            if (m_handle == NativeHandleTraits<TNativeHandle>::InvalidValue)
            {
                TDeleter{}(m_handle);
                m_handle = NativeHandleTraits<TNativeHandle>::InvalidValue;
            }
        }

        TNativeHandle& ReleaseAndGetAddressOf()
        {
            Release();
            return m_handle;
        }

        TNativeHandle Get() const
        {
            return m_handle;
        }
    private:
        void MoveFrom(Handle&& handle)
        {
            Release();
            std::swap(m_handle, handle.m_handle);
        }
    private:
        TNativeHandle m_handle;
    };
}
