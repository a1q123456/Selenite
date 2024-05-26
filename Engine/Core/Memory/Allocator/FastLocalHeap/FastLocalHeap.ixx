export module Engine.Core.Memory.FastLocalHeap;
import std;

namespace Engine::Core::Memory
{
    export class FastLocalHeap
    {
    public:
        static constexpr std::size_t POOL_GROWTH_FACTOR = 8;

        FastLocalHeap(int poolSizePages = 0);
        FastLocalHeap(const FastLocalHeap&) = delete;
        FastLocalHeap(FastLocalHeap&& another) noexcept
        {
            MoveFrom(std::move(another));
        }

        auto operator=(const FastLocalHeap&) = delete;
        auto operator=(FastLocalHeap&& another) noexcept
        {
            MoveFrom(std::move(another));
        }

        auto Allocate(std::size_t sizeInBytes) noexcept -> void*;

        static auto Free(void* ptr) noexcept -> void;

        ~FastLocalHeap() noexcept;
    private:
        struct MemoryNode;
        struct RangeNode;

        auto MoveFrom(FastLocalHeap&& another) noexcept -> void;

        auto FindFreeMemory(std::size_t bytesNeeded) const noexcept -> MemoryNode*;

        auto AllocateMemory(std::size_t size) noexcept -> MemoryNode*;

        auto AllocatePages(std::size_t pagesCount) noexcept -> MemoryNode*;

        static auto TryMergeWithFreeNodes(MemoryNode* node) noexcept -> MemoryNode*;

        static auto GetNextAlignedAddress(std::intptr_t address) noexcept -> std::intptr_t;

        static auto ProtectMemory(std::intptr_t address, std::size_t size, int access) -> void;

        static auto ReleaseMemory(std::intptr_t address) noexcept -> void;

        RangeNode* m_firstRange = nullptr;
        RangeNode* m_lastRange = nullptr;
        std::size_t m_poolSizePages = 0;
    };

    export template <typename T, typename... TArgs>
    auto New(FastLocalHeap& allocator, TArgs&&... args)
    {
        auto address = allocator.Allocate(sizeof(T));
        return std::construct_at(address, std::forward<TArgs>(args)...);
    }

    export template <typename T>
    auto Delete(FastLocalHeap& allocator, T* ptr) noexcept
    {
        std::destroy_at(ptr);
        allocator.Free(ptr);
    }

    export struct FastLocalHeapDeleter
    {
        FastLocalHeapDeleter(FastLocalHeap& allocator) noexcept : allocator(allocator)
        {
        }

        template <typename T>
        auto operator()(T* ptr) noexcept
        {
            Delete(allocator, ptr);
        }

    private:
        FastLocalHeap& allocator;
    };

    export template <typename T>
    using FastLocalUniquePtr = std::unique_ptr<T, FastLocalHeapDeleter>;

    export template <typename T, typename... TArgs>
    auto MakeUnique(FastLocalHeap& allocator, TArgs&&... args) -> FastLocalUniquePtr<T>
    {
        return std::unique_ptr<T, FastLocalHeapDeleter>(New(allocator, std::forward<TArgs>(args)...), allocator);
    }

}
