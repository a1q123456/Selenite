export module Engine.Core.Containers.BlockingQueue;
import Engine.Core.Exceptions.OperationiCancelledError;
import std;

namespace Engine::Core::Containers
{
    export class BlockingQueueClosed : public std::exception {};

    export template<typename TItem, typename TAllocator = std::allocator<TItem>>
    class BlockingQueue final
    {
    public:
        BlockingQueue() = default;
        BlockingQueue(BlockingQueue&& another) noexcept(noexcept(MoveFrom(another)))
        {
            MoveFrom(std::move(another));
        }

        BlockingQueue& operator=(BlockingQueue&& another) noexcept(noexcept(MoveFrom(another)))
        {
            MoveFrom(std::move(another));
            return *this;
        }

        auto Reset()
        {
            m_closed = false;
        }

        template <typename Rep, typename Period, typename TPrediction>
        auto WaitFor(TPrediction&& pred, std::chrono::duration<Rep, Period> duration) -> bool;

        auto Pop(std::stop_token stopToken = {}) -> TItem;
        auto Size() const noexcept { return m_list.size(); }
        auto CanPop() const noexcept { return !std::ranges::empty(m_list); }
        auto Push(TItem& item) -> void
            requires(std::copyable<TItem>);
        auto Push(TItem&& item) -> void
            requires(std::movable<TItem>);
        auto Close() -> void;
        auto IsClosed() -> bool { return m_closed; }
    private:
        auto MoveFrom(BlockingQueue&& another) noexcept(noexcept(TItem(std::declval<TItem&&>()))) -> void
            requires(std::movable<TItem>);

        std::list<TItem, TAllocator> m_list;
        std::condition_variable m_cv;
        std::mutex m_mutex;
        bool m_closed = false;
    };

    template<typename TItem, typename TAllocator>
    auto BlockingQueue<TItem, TAllocator>::Pop(std::stop_token stopToken) -> TItem
    {
        std::unique_lock lock{ m_mutex };
        if (!std::ranges::empty(m_list))
        {
            auto item = m_list.front();
            m_list.pop_front();
            return item;
        }

        m_cv.wait(lock, 
            [=] { return m_closed || !std::ranges::empty(m_list) || stopToken.stop_requested(); });

        if (m_closed)
        {
            throw BlockingQueueClosed{};
        }
        if (stopToken.stop_requested())
        {
            throw Core::Exceptions::OperationCancelledError{};
        }
        auto item = m_list.front();
        m_list.pop_front();
        return item;
    }
    template <typename TItem, typename TAllocator>
    template <typename Rep, typename Period, typename TPrediction>
    auto BlockingQueue<TItem, TAllocator>::WaitFor(TPrediction&& pred,
        std::chrono::duration<Rep, Period> duration) -> bool
    {
        std::unique_lock lock{ m_mutex };

        m_cv.wait_for(lock, duration,
            [=] { return m_closed || pred(); });

        return !m_closed;
    }

    template <typename TItem, typename TAllocator>
    auto BlockingQueue<TItem, TAllocator>::Push(TItem& item) -> void
        requires (std::copyable<TItem>)
    {
        std::unique_lock lock{ m_mutex };
        if (m_closed)
        {
            throw BlockingQueueClosed{};
        }
        m_list.push_back(item);
        m_cv.notify_one();
    }

    template <typename TItem, typename TAllocator>
    auto BlockingQueue<TItem, TAllocator>::Push(TItem&& item) -> void
        requires (std::movable<TItem>)
    {
        std::unique_lock lock{ m_mutex };
        if (m_closed)
        {
            throw BlockingQueueClosed{};
        }
        m_list.push_back(std::move(item));
        m_cv.notify_one();
    }

    template <typename TItem, typename TAllocator>
    auto BlockingQueue<TItem, TAllocator>::Close() -> void
    {
        std::unique_lock lock{ m_mutex };
        m_closed = true;
        m_cv.notify_all();
    }

    template <typename TItem, typename TAllocator>
    auto BlockingQueue<TItem, TAllocator>::MoveFrom(BlockingQueue&& another)
        noexcept(noexcept(TItem(std::declval<TItem&&>()))) -> void
            requires (std::movable<TItem>)
    {
        m_list.clear();
        std::swap(m_list, another.m_list);
    }
}

