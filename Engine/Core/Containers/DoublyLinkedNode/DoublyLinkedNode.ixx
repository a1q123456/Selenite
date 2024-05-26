export module Engine.Core.Containers.DoublyLinkedNode;

namespace Engine::Core::Containers
{
    export template <typename T>
    struct DoublyLinkedNode
    {
        DoublyLinkedNode* prev = nullptr;
        DoublyLinkedNode* next = nullptr;

        T* data = nullptr;

        auto InsertBefore(DoublyLinkedNode* node) noexcept -> void
        {
            RemoveFromList();
            prev = node->prev;
            next = node;
            if (prev != nullptr)
            {
                prev->next = this;
            }
            node->prev = this;
        }

        auto InsertAfter(DoublyLinkedNode* node) noexcept -> void
        {
            RemoveFromList();
            next = node->next;
            prev = node;
            prev->next = this;
            if (next != nullptr)
            {
                next->prev = this;
            }
        }

        auto RemoveFromList() noexcept -> void
        {
            if (prev != nullptr)
            {
                prev->next = next;
            }
            if (next != nullptr)
            {
                next->prev = prev;
            }
            prev = nullptr;
            next = nullptr;
        }

        auto ReplaceNode(DoublyLinkedNode* node) noexcept -> void
        {
            auto previousNode = node->prev;
            auto nextNode = node->next;
            RemoveFromList();
            node->RemoveFromList();
            if (previousNode != nullptr)
            {
                previousNode->next = this;
            }
            if (nextNode != nullptr)
            {
                nextNode->prev = this;
            }
            prev = previousNode;
            next = nextNode;

        }
    };
}


