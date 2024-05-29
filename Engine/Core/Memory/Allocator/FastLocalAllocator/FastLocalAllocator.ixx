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

    export template <typename T>
    using FastHeapVector = std::vector<T, FastLocalAllocator<T>>;

    export template <typename T>
    using FastHeapList = std::list<T, FastLocalAllocator<T>>;

    export template <typename T>
    using FastHeapSet = std::set<T, FastLocalAllocator<T>>;

    export template <typename T>
    using FastHeapMultiSet = std::multiset<T, FastLocalAllocator<T>>;

    export template <typename T>
    using FastHeapUnorderedSet = std::unordered_set<T, FastLocalAllocator<T>>;

    export template <typename T>
    using FastHeapUnorderedMultiSet = std::unordered_multiset<T, FastLocalAllocator<T>>;

    export template <typename T>
    using FastHeapMap = std::map<T, FastLocalAllocator<T>>;

    export template <typename T>
    using FastHeapUnorderedMap = std::unordered_map<T, FastLocalAllocator<T>>;

    export template <typename T>
    using FastHeapMultiMap = std::multimap<T, FastLocalAllocator<T>>;

    export template <typename T>
    using FastHeapUnorderedMultiMap = std::unordered_multimap<T, FastLocalAllocator<T>>;
}

