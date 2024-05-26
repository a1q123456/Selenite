module Engine.Core.Threading.ThreadPool;


namespace Engine::Core::Threading
{
    ThreadPool::ThreadPool()
        : m_workQueue(&m_heap)
    {
        for (unsigned int i = 0; i < std::thread::hardware_concurrency(); i++)
        {
            m_workers.emplace_back(std::jthread{ [this] { WorkerThreadEntry(); } });
        }
    }

    ThreadPool::~ThreadPool() noexcept
    {
        m_workQueue.Close();
        for (auto& worker : m_workers)
        {
            try
            {
                worker.join();
            }
            catch (std::system_error) {}
        }
    }

    auto ThreadPool::SubmitWork(WorkType&& work) -> void
    {
        m_workQueue.Push(std::move(work));
    }

    auto ThreadPool::WorkerThreadEntry() noexcept -> void
    {
        while (true)
        {
            WorkType job;
            try
            {
                job = m_workQueue.Pop();
            }
            catch (Containers::BlockingQueueClosed)
            {
                break;
            }
            try
            {
                job();
            }
            catch (...) {}
        }
    }
    
}


