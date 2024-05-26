export module Engine.Core.Memory.FastLocalAllocator;
import Engine.Core.Memory.FastLocalHeap;
import std;

namespace Engine::Core::Memory
{
    export template <typename T>
    class FastLocalAllocator
    {
        static_assert(std::is_same_v<std::remove_cv_t<T>, T>, "T must be cv-unqualified");
    public:
        using value_type = T;

        FastLocalAllocator(FastLocalHeap* fastLocalHeap) noexcept : m_fastLocalHeap(fastLocalHeap)
        {
        }

        template<typename TValue>
        FastLocalAllocator(const FastLocalAllocator<TValue>& another) noexcept : m_fastLocalHeap(another.m_fastLocalHeap)
        {
        }

        FastLocalAllocator& operator=(const FastLocalAllocator& another) noexcept
        {
            m_fastLocalHeap = another.m_fastLocalHeap;
        }

        auto allocate(std::size_t n) -> T*
        {
            return static_cast<T*>(m_fastLocalHeap->Allocate(n * sizeof(T)));
        }

        auto deallocate(void* ptr, std::size_t) const noexcept -> void
        {
            m_fastLocalHeap->Free(ptr);
        }


    private:
        FastLocalHeap* m_fastLocalHeap;

        template <typename>
        friend class FastLocalAllocator;

        template <typename T1, typename T2>
        friend constexpr auto operator==(const FastLocalAllocator<T1>& lhs, const FastLocalAllocator<T2>& rhs) noexcept -> bool;
    };

    export template< class T1, class T2 >
    constexpr auto operator==(const FastLocalAllocator<T1>& lhs, const FastLocalAllocator<T2>& rhs) noexcept -> bool
    {
        return lhs.m_fastLocalHeap == rhs.m_fastLocalHeap;
    }

    export template< class T1, class T2 >
    constexpr auto operator!=(const FastLocalAllocator<T1>& lhs, const FastLocalAllocator<T2>& rhs) noexcept -> bool
    {
        return !(lhs == rhs);
    }
}

