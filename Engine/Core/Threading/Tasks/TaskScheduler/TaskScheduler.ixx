export module Engine.Core.Threading.Tasks:TaskScheduler;
import Engine.Core.Threading.ThreadPool;
import std;

namespace Engine::Core::Threading
{
    export class TaskScheduler
    {
    public:
        static auto GetCurrentScheduler() noexcept -> TaskScheduler*;

        static auto SetTaskSchedulerForCurrentThread(TaskScheduler* taskScheduler) noexcept;

        auto QueueContinuation(std::coroutine_handle<> continuation) noexcept;

    private:

        ThreadPool m_threadPool;
    };

    
}
