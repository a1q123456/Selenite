module;
#include <cassert>
#define NOMINMAX
#include <Windows.h>
module Engine.Core.Memory.FastLocalHeap;
import Engine.Core.Containers.DoublyLinkedNode;
import Engine.Core.SystemInfo;
import std;

namespace Engine::Core::Memory
{
    enum class MemoryNodeRole : std::uint8_t
    {
        Unused,
        Used,
        Tail,
    };

    struct FastLocalHeap::MemoryNode
    {
        MemoryNodeRole role = MemoryNodeRole::Unused;
        Containers::DoublyLinkedNode<MemoryNode> memoryNodeList;
        Containers::DoublyLinkedNode<MemoryNode> freeNodeList;
        RangeNode* rangeNode = nullptr;

        MemoryNode()
        {
            static_assert(std::is_trivially_destructible_v<MemoryNode>);
            memoryNodeList.data = this;
            freeNodeList.data = this;
        }

        auto GetMemory() const noexcept -> std::intptr_t
        {
            return reinterpret_cast<std::intptr_t>(this) + sizeof(MemoryNode);
        }

        auto Size() const noexcept -> std::size_t
        {
            assert(role != MemoryNodeRole::Tail);
            assert(memoryNodeList.next != nullptr);

            auto nextNodeAddress = reinterpret_cast<std::intptr_t>(memoryNodeList.next->data);
            auto currentNodeAddress = GetMemory();
            return nextNodeAddress - currentNodeAddress;
        }
    };

    struct FastLocalHeap::RangeNode
    {
        Containers::DoublyLinkedNode<RangeNode> rangeNodeList;
        MemoryNode* firstNode = nullptr;
        MemoryNode* firstFreeNode = nullptr;

        RangeNode()
        {
            static_assert(std::is_trivially_destructible_v<RangeNode>);
            rangeNodeList.data = this;
        }
    };

    FastLocalHeap::FastLocalHeap(int poolSizePages) : m_poolSizePages(poolSizePages)
    {
        AllocatePages(poolSizePages);
    }

    auto FastLocalHeap::Allocate(std::size_t sizeInBytes) noexcept -> void*
    {
        auto memoryNode = FindFreeMemory(sizeInBytes);
        if (memoryNode == nullptr)
        {
            auto& systemInfo = SystemInfo::Get();
            memoryNode = AllocateMemory(std::max<std::size_t>(m_poolSizePages * systemInfo.dwPageSize, sizeInBytes));
            m_poolSizePages *= POOL_GROWTH_FACTOR;
        }
        assert(memoryNode->role == MemoryNodeRole::Unused);
        auto nextMemoryNode = memoryNode->memoryNodeList.next->data;
        auto nextMemoryNodeAddress = reinterpret_cast<std::intptr_t>(nextMemoryNode);
        memoryNode->role = MemoryNodeRole::Used;

        auto newMemoryNodeAddress = memoryNode->GetMemory() + sizeInBytes;
        newMemoryNodeAddress = GetNextAlignedAddress(newMemoryNodeAddress);
        if (newMemoryNodeAddress + sizeof(MemoryNode) < nextMemoryNodeAddress)
        {
            auto freeMemoryNode = std::construct_at(reinterpret_cast<MemoryNode*>(newMemoryNodeAddress));
            freeMemoryNode->rangeNode = memoryNode->rangeNode;
            freeMemoryNode->role = MemoryNodeRole::Unused;

            freeMemoryNode->memoryNodeList.InsertAfter(&memoryNode->memoryNodeList);
            freeMemoryNode->freeNodeList.InsertAfter(&memoryNode->freeNodeList);
        }
        if (memoryNode->freeNodeList.prev == nullptr)
        {
            memoryNode->rangeNode->firstFreeNode = memoryNode->freeNodeList.next->data;
        }
        memoryNode->freeNodeList.RemoveFromList();
        assert(memoryNode->rangeNode->firstFreeNode != memoryNode);
        auto pointer = reinterpret_cast<void*>(memoryNode->GetMemory());
#ifdef _DEBUG
        std::memset(pointer, 0xCD, sizeInBytes);
#endif
        return pointer;
    }

    auto FastLocalHeap::Free(void* ptr) noexcept -> void
    {
        auto address = reinterpret_cast<std::intptr_t>(ptr);
        auto memoryNode = reinterpret_cast<MemoryNode*>(address - sizeof(MemoryNode));
        memoryNode->role = MemoryNodeRole::Unused;
        memoryNode = TryMergeWithFreeNodes(memoryNode);

#ifdef _DEBUG
        auto memory = memoryNode->GetMemory();
        auto size = memoryNode->Size();
        std::memset(reinterpret_cast<void*>(memory), 0xDD, size);
#endif
    }

    FastLocalHeap::~FastLocalHeap() noexcept
    {
        auto rangeNode = &m_firstRange->rangeNodeList;
        while (rangeNode != nullptr)
        {
            auto nextRangeNode = rangeNode->next;
            auto rangeNodeAddress = reinterpret_cast<std::intptr_t>(rangeNode->data);
            ReleaseMemory(rangeNodeAddress);
            rangeNode = nextRangeNode;
        }
    }

    auto FastLocalHeap::MoveFrom(FastLocalHeap&& another) noexcept -> void
    {
        m_firstRange = another.m_firstRange;
        m_lastRange = another.m_lastRange;
        m_poolSizePages = another.m_poolSizePages;

        another.m_firstRange = nullptr;
        another.m_lastRange = nullptr;
        another.m_poolSizePages = 0;
    }

    auto FastLocalHeap::GetNextAlignedAddress(std::intptr_t address) noexcept -> std::intptr_t
    {
        auto alignmentValue = alignof(std::max_align_t);
        auto mask = alignmentValue - 1;
        auto alignedAddress = (address + mask) & ~mask;
        return alignedAddress;
    }

    auto FastLocalHeap::FindFreeMemory(std::size_t bytesNeeded) const noexcept -> MemoryNode*
    {
        auto rangeNode = m_firstRange;
        while (rangeNode != nullptr)
        {
            auto memoryNode = rangeNode->firstFreeNode;
            while (memoryNode != nullptr && memoryNode->role != MemoryNodeRole::Tail)
            {
                assert(memoryNode->role == MemoryNodeRole::Unused);
                auto bytesAvailable = memoryNode->Size();
                if (bytesAvailable >= bytesNeeded)
                {
                    return memoryNode;
                }
                memoryNode = memoryNode->freeNodeList.next->data;
            }
            if (rangeNode->rangeNodeList.next == nullptr)
            {
                break;
            }
            rangeNode = rangeNode->rangeNodeList.next->data;
        }
        return nullptr;
    }

    auto FastLocalHeap::AllocateMemory(std::size_t size) noexcept -> MemoryNode*
    {
        auto& systemInfo = SystemInfo::Get();
        size += sizeof(RangeNode) + sizeof(MemoryNode) * 2;
        auto pagesCount = (size + systemInfo.dwPageSize - 1) / systemInfo.dwPageSize;
        return AllocatePages(pagesCount);
    }

    auto FastLocalHeap::AllocatePages(std::size_t pagesCount) noexcept -> MemoryNode*
    {
        auto& systemInfo = SystemInfo::Get();
        auto bytes = pagesCount * systemInfo.dwPageSize;

        auto address = VirtualAlloc(
            nullptr,
            bytes,
            MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE);
        auto rangeNodeAddress = reinterpret_cast<std::intptr_t>(address);

        auto rangeNode = std::construct_at(reinterpret_cast<RangeNode*>(rangeNodeAddress));

        if (m_firstRange == nullptr)
        {
            m_firstRange = rangeNode;
        }
        if (m_lastRange != nullptr)
        {
            rangeNode->rangeNodeList.InsertAfter(&m_lastRange->rangeNodeList);
        }
        m_lastRange = rangeNode;

        auto firstMemoryNodeAddress = rangeNodeAddress + sizeof(RangeNode);

        auto firstMemoryNode = std::construct_at(reinterpret_cast<MemoryNode*>(firstMemoryNodeAddress));
        firstMemoryNode->rangeNode = rangeNode;
        rangeNode->firstNode = firstMemoryNode;
        rangeNode->firstFreeNode = firstMemoryNode;

        auto lastMemoryNodeAddress = rangeNodeAddress + bytes - sizeof(MemoryNode);

        auto lastMemoryNode = std::construct_at(reinterpret_cast<MemoryNode*>(lastMemoryNodeAddress));
        lastMemoryNode->rangeNode = rangeNode;
        firstMemoryNode->role = MemoryNodeRole::Unused;
        firstMemoryNode->memoryNodeList.prev = nullptr;
        firstMemoryNode->freeNodeList.prev = nullptr;

        firstMemoryNode->memoryNodeList.next = &lastMemoryNode->memoryNodeList;
        firstMemoryNode->freeNodeList.next = &lastMemoryNode->freeNodeList;

        lastMemoryNode->role = MemoryNodeRole::Tail;
        lastMemoryNode->memoryNodeList.next = nullptr;
        lastMemoryNode->freeNodeList.next = nullptr;

        lastMemoryNode->memoryNodeList.prev = &firstMemoryNode->memoryNodeList;
        lastMemoryNode->freeNodeList.prev = &firstMemoryNode->freeNodeList;

        return firstMemoryNode;
    }

    auto FastLocalHeap::TryMergeWithFreeNodes(MemoryNode* node) noexcept -> MemoryNode*
    {

        MemoryNode* previousNode = nullptr;
        if (node->memoryNodeList.prev != nullptr)
        {
            node->memoryNodeList.prev->data;
        }

        MemoryNode* nextNode = nullptr;
        if (node->memoryNodeList.next != nullptr)
        {
            nextNode = node->memoryNodeList.next->data;
        }

        bool merged = false;
        auto result = node;
        if (previousNode != nullptr && previousNode->role == MemoryNodeRole::Unused)
        {
            merged = true;
            result = previousNode;
            node->memoryNodeList.RemoveFromList();
            node->freeNodeList.RemoveFromList();
            if (node->rangeNode->firstFreeNode == node)
            {
                node->rangeNode->firstFreeNode = result;
            }
        }
        if (nextNode != nullptr && nextNode->role == MemoryNodeRole::Unused)
        {
            merged = true;
            nextNode->memoryNodeList.RemoveFromList();

            result->freeNodeList.ReplaceNode(&nextNode->freeNodeList);
            if (nextNode->rangeNode->firstFreeNode == nextNode)
            {
                nextNode->rangeNode->firstFreeNode = result;
            }
        }

        if (!merged)
        {
            node->role = MemoryNodeRole::Unused;
            node->freeNodeList.InsertBefore(&node->rangeNode->firstFreeNode->freeNodeList);
            node->rangeNode->firstFreeNode = node;
        }
        return result;
    }

    auto FastLocalHeap::ProtectMemory(std::intptr_t address, std::size_t size, int access) -> void
    {
        DWORD previousAccess = 0;
        VirtualProtect(reinterpret_cast<LPVOID>(address), size, access, &previousAccess);
    }

    auto FastLocalHeap::ReleaseMemory(std::intptr_t address) noexcept -> void
    {
        VirtualFree(reinterpret_cast<LPVOID>(address), 0, MEM_RELEASE);
    }
}
