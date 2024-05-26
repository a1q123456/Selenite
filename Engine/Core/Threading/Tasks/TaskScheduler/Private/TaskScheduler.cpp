module Engine.Core.Threading.Tasks;
import :TaskScheduler;
import std;

namespace
{
    thread_local Engine::Core::Threading::TaskScheduler* threadLocalScheduler = nullptr;
}

namespace Engine::Core::Threading
{

    auto TaskScheduler::QueueContinuation(std::coroutine_handle<> continuation) noexcept
    {
        m_threadPool.SubmitWork([=] { continuation(); });
    }

    auto TaskScheduler::GetCurrentScheduler() noexcept -> TaskScheduler*
    {
        static TaskScheduler globalScheduler;
        if (threadLocalScheduler != nullptr)
        {
            return threadLocalScheduler;
        }
        return &globalScheduler;
    }

    auto TaskScheduler::SetTaskSchedulerForCurrentThread(TaskScheduler* taskScheduler) noexcept
    {
        threadLocalScheduler = taskScheduler;
    }
}


