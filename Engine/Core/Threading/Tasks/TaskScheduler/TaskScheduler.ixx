export module Engine.Core.Threading.Tasks:TaskScheduler;
import Engine.Core.Threading.ThreadPool;
import std;

namespace Engine::Core::Threading
{
    export class TaskScheduler
    {
    public:
        static auto GetCurrentScheduler() noexcept -> TaskScheduler*;

        static auto SetTaskSchedulerForCurrentThread(TaskScheduler* taskScheduler) noexcept -> void;

        auto QueueContinuation(std::coroutine_handle<> continuation) noexcept -> void;


        auto GetThreadPool() noexcept -> ThreadPool&
        {
            return m_threadPool;
        }


    private:
        ThreadPool m_threadPool;
    };

    
}
