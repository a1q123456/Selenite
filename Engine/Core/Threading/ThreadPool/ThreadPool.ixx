export module Engine.Core.Threading.ThreadPool;
import Engine.Core.Containers.BlockingQueue;
import Engine.Core.Memory.FastLocalAllocator;
import Engine.Core.Memory.FastLocalHeap;
import std;

namespace Engine::Core::Threading
{
    export class ThreadPool
    {
    public:
        using WorkType = std::move_only_function<void()>;

        ThreadPool();
        ~ThreadPool() noexcept;
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&& another) = delete;

        auto operator=(const ThreadPool&) -> ThreadPool& = delete;
        auto operator=(ThreadPool&& another) -> ThreadPool& = delete;

        auto SubmitWork(WorkType&& work) -> void;

    private:
        auto WorkerThreadEntry() noexcept -> void;

        std::vector<std::jthread> m_workers;

        Memory::FastLocalHeap m_heap {1};
        Containers::BlockingQueue<WorkType, Memory::FastLocalAllocator<WorkType>> m_workQueue;
    };
}

