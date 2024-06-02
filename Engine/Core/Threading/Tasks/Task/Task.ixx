module;
#include <cassert>
export module Engine.Core.Threading.Tasks:Task;
import :TaskScheduler;
import std;

namespace Engine::Core::Threading
{
    export template <typename TResult>
    class TaskCompletionSource;

    export template <typename TResult, typename TPromise>
    class TaskCompletionSourceBase;

    export template<typename TResult>
    class Task
    {
    public:
        static auto Start(std::function<TResult()>&& entry) -> Task;

        using ResultType = TResult;
        using PromiseType = TaskCompletionSource<ResultType>;
        using promise_type = PromiseType;

        Task() noexcept;

        template <typename T>
        Task(T&& value) noexcept
            requires(!std::is_void_v<TResult>&& std::convertible_to<T, TResult>);

        Task(const Task& another) = delete;
        auto operator=(const Task& another)-> Task& = delete;

        Task(Task&& another) noexcept;
        auto operator=(Task&& another) noexcept -> Task&;

        auto GetResult() const noexcept -> std::optional<ResultType>;

        auto GetException() const noexcept -> std::optional<std::exception_ptr>;

        auto Wait() const noexcept -> void;

        auto await_ready() const noexcept -> bool;

        auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool;

        auto await_resume() -> TResult;

        virtual ~Task() noexcept;
    private:
        Task(PromiseType* promise) noexcept;

        using InnerResultType = std::conditional_t<std::is_void_v<TResult>, int, TResult>;

        PromiseType* m_promise = nullptr;
        mutable std::condition_variable m_cv;
        mutable std::mutex m_mutex;
        std::variant<InnerResultType, std::exception_ptr, std::monostate> m_result = std::monostate{};
        std::coroutine_handle<> m_continuation{};

        friend PromiseType;

        friend TaskCompletionSourceBase;
    };

    export template <typename TResult, typename TPromise>
    class TaskCompletionSourceBase
    {
    public:

        using ResultType = TResult;
        using FutureType = Task<ResultType>;
        using InnerResultType = std::conditional_t<std::is_void_v<TResult>, int, TResult>;

        TaskCompletionSourceBase() = default;
        //TaskCompletionSourceBase(const TaskCompletionSourceBase&);
        //auto operator=(const TaskCompletionSourceBase&) ->TaskCompletionSourceBase &;

        TaskCompletionSourceBase(TaskCompletionSourceBase&& another) noexcept;
        auto operator=(TaskCompletionSourceBase&& another) noexcept -> TaskCompletionSourceBase&;

        auto GetTask() noexcept -> FutureType;

        template <typename T>
        auto SetResult(T&& result) noexcept -> void
            requires(std::convertible_to<T, TResult> && !std::is_void_v<TResult>);

        auto SetDone() noexcept -> void
            requires(std::is_void_v<TResult>);

        auto SetException(std::exception_ptr exception) noexcept -> void;

        virtual ~TaskCompletionSourceBase() noexcept;
    private:

        Task<TResult>* m_task = nullptr;

        friend FutureType;
    };

    export template <typename TResult>
    class TaskCompletionSource : public TaskCompletionSourceBase<TResult, TaskCompletionSource<TResult>>
    {
    public:

        using Super = TaskCompletionSourceBase<TResult, TaskCompletionSource>;

        using Super::TaskCompletionSourceBase;

        auto get_return_object() noexcept -> typename Super::FutureType;

        auto initial_suspend() const noexcept -> std::suspend_never;

        auto unhandled_exception() noexcept -> void;

        template <typename T>
        auto return_value(T&& value) noexcept -> void
            requires(std::convertible_to<T, TResult>);

        auto final_suspend() const noexcept -> std::suspend_never;
    };

    export template <>
    class TaskCompletionSource<void> : public TaskCompletionSourceBase<void, TaskCompletionSource<void>>
    {
    public:
        using Super = TaskCompletionSourceBase;

        using Super::TaskCompletionSourceBase;

        auto get_return_object() noexcept -> FutureType;

        auto initial_suspend() const noexcept -> std::suspend_never;

        auto unhandled_exception() noexcept -> void;

        auto return_void() noexcept -> void;

        auto final_suspend() const noexcept -> std::suspend_never;
    };

    template<typename TResult>
    auto Task<TResult>::Start(std::function<TResult()>&& entry) -> Task
    {
        PromiseType promise{};
        auto result = promise.GetTask();
        TaskScheduler::GetCurrentScheduler()->GetThreadPool().SubmitWork([
            entry = std::move(entry), 
            promise = std::move(promise)] () mutable
        {
            if constexpr (std::is_void_v<TResult>)
            {
                entry();
                promise.SetDone();
            }
            else
            {
                promise.SetResult(entry());
            }
        });

        return result;
    }

    template <typename TResult>
    Task<TResult>::Task() noexcept
    {
        if constexpr (std::is_void_v<TResult>)
        {
            m_result = 1;
        }
    }

    template <typename TResult>
    template <typename T>
    Task<TResult>::Task(T&& value) noexcept
        requires (!std::is_void_v<TResult> && std::convertible_to<T, TResult>)
    {
        m_result = std::forward<T>(value);
    }

    template <typename TResult>
    Task<TResult>::Task(Task&& another) noexcept
    {
        m_promise = another.m_promise;
        another.m_promise = nullptr;
        if (m_promise != nullptr)
        {
            m_promise->m_task = this;
        }

        m_result = std::move(another.m_result);
        m_continuation = std::move(m_continuation);
    }

    template <typename TResult>
    auto Task<TResult>::operator=(Task&& another) noexcept -> Task&
    {
        m_promise = another.m_promise;
        another.m_promise = nullptr;
        if (m_promise != nullptr)
        {
            m_promise->m_task = this;
        }
        m_result = std::move(another.m_result);
        m_continuation = std::move(m_continuation);

        return *this;
    }

    template <typename TResult>
    auto Task<TResult>::GetResult() const noexcept -> std::optional<ResultType>
    {
        //if (m_promise == nullptr)
        //{
        //    return std::nullopt;
        //}
        std::unique_lock lock{ m_mutex };

        if (std::holds_alternative<std::monostate>(m_result))
        {
            m_cv.wait(lock);
        }

        if (std::holds_alternative<ResultType>(m_result))
        {
            return std::get<ResultType>(m_result);
        }
        return std::nullopt;
    }

    template <typename TResult>
    auto Task<TResult>::GetException() const noexcept -> std::optional<std::exception_ptr>
    {
        //if (m_promise == nullptr)
        //{
        //    return std::nullopt;
        //}
        std::unique_lock lock{ m_mutex };

        if (std::holds_alternative<std::monostate>(m_result))
        {
            m_cv.wait(lock);
        }

        if (std::holds_alternative<std::exception_ptr>(m_result))
        {
            return std::get<std::exception_ptr>(m_result);
        }
        return std::nullopt;
    }

    template <typename TResult>
    auto Task<TResult>::Wait() const noexcept -> void
    {
        //if (m_promise == nullptr)
        //{
        //    return;
        //}
        std::unique_lock lock{ m_mutex };
        if (std::holds_alternative<std::monostate>(m_result))
        {
            m_cv.wait(lock);
        }
    }

    template <typename TResult>
    auto Task<TResult>::await_ready() const noexcept -> bool
    {
        //if (m_promise == nullptr)
        //{
        //    return true;
        //}

        std::unique_lock lock{ m_mutex };
        return !std::holds_alternative<std::monostate>(m_result);
    }

    template <typename TResult>
    auto Task<TResult>::await_suspend(std::coroutine_handle<> handle) noexcept -> bool
    {
        //if (m_promise == nullptr)
        //{
        //    return false;
        //}
        std::unique_lock lock{ m_mutex };
        if (std::holds_alternative<std::monostate>(m_result))
        {
            m_continuation = handle;
            //assert(m_promise->m_task);
            return true;
        }
        return false;
    }

    template <typename TResult>
    auto Task<TResult>::await_resume() -> TResult
    {
        //if constexpr (!std::is_void_v<TResult>)
        //{
        //    if (m_promise == nullptr)
        //    {
        //        return {};
        //    }
        //}

        std::unique_lock lock{ m_mutex };
        assert(!std::holds_alternative<std::monostate>(m_result));

        if (std::holds_alternative<std::exception_ptr>(m_result))
        {
            std::rethrow_exception(std::get<std::exception_ptr>(m_result));
        }

        if constexpr (!std::is_void_v<TResult>)
        {
            return std::get<TResult>(m_result);
        }
    }

    template <typename TResult>
    Task<TResult>::~Task() noexcept
    {
        //if (m_promise == nullptr)
        //{
        //    return;
        //}

        std::unique_lock lock{ m_mutex };
        //m_promise->m_task = nullptr;
        //m_promise->m_continuation.destroy();
    }

    template <typename TResult>
    Task<TResult>::Task(PromiseType* promise) noexcept
        : m_promise(promise)
    {
        m_promise->m_task = this;
    }

    template <typename TResult, typename TPromise>
    TaskCompletionSourceBase<TResult, TPromise>::TaskCompletionSourceBase(TaskCompletionSourceBase&& another) noexcept
    {
        //std::unique_lock lock{ another.m_mutex };
        //assert(std::holds_alternative<std::monostate>(m_result));
        //assert(!m_continuation);

        m_task = another.m_task;
        another.m_task = nullptr;

        if (m_task != nullptr)
        {
            m_task->m_promise = static_cast<TPromise*>(this);
        }
    }

    template <typename TResult, typename TPromise>
    auto TaskCompletionSourceBase<TResult, TPromise>::operator=(
        TaskCompletionSourceBase&& another) noexcept -> TaskCompletionSourceBase&
    {
        //std::unique_lock lock{ another.m_mutex };
        //assert(std::holds_alternative<std::monostate>(m_result));
        //assert(!m_continuation);

        m_task = another.m_task;
        another.m_task = nullptr;

        if (m_task != nullptr)
        {
            m_task->m_promise = static_cast<TPromise*>(this);
        }
        return *this;
    }

    template <typename TResult, typename TPromise>
    auto TaskCompletionSourceBase<TResult, TPromise>::GetTask() noexcept -> FutureType
    {
        assert(m_task == nullptr);
        return FutureType(static_cast<TPromise*>(this));
    }

    template <typename TResult, typename TPromise>
    template <typename T>
    auto TaskCompletionSourceBase<TResult, TPromise>::SetResult(T&& result) noexcept -> void
        requires (std::convertible_to<T, TResult> && !std::is_void_v<TResult>)
    {
        std::lock_guard lock{ m_task->m_mutex };
        if (!std::holds_alternative<std::monostate>(m_task->m_result))
        {
            return;
        }

        m_task->m_result.emplace(std::forward<T>(result));
        m_task->m_cv.notify_all();

        if (m_task->m_continuation)
        {
            TaskScheduler::GetCurrentScheduler()->QueueContinuation(m_task->m_continuation);
        }
    }

    template <typename TResult, typename TPromise>
    auto TaskCompletionSourceBase<TResult, TPromise>::SetDone() noexcept -> void
        requires (std::is_void_v<TResult>)
    {
        std::lock_guard lock{ m_task->m_mutex };
        if (!std::holds_alternative<std::monostate>(m_task->m_result))
        {
            return;
        }

        m_task->m_result = 1;
        m_task->m_cv.notify_all();

        if (m_task->m_continuation)
        {
            TaskScheduler::GetCurrentScheduler()->QueueContinuation(m_task->m_continuation);
        }
    }

    template <typename TResult, typename TPromise>
    auto TaskCompletionSourceBase<TResult, TPromise>::SetException(std::exception_ptr exception) noexcept -> void
    {
        std::lock_guard lock{ m_task->m_mutex };
        if (!std::holds_alternative<std::monostate>(m_task->m_result))
        {
            return;
        }

        m_task->m_result = exception;
        m_task->m_cv.notify_all();
        TaskScheduler::GetCurrentScheduler()->QueueContinuation(m_task->m_continuation);
    }

    template <typename TResult, typename TPromise>
    TaskCompletionSourceBase<TResult, TPromise>::~TaskCompletionSourceBase() noexcept
    {
        //std::unique_lock lock{ m_task->m_mutex };
        //assert(m_task == nullptr);
    }

    template <typename TResult>
    auto TaskCompletionSource<TResult>::get_return_object() noexcept -> typename Super::FutureType
    {
        return Super::GetTask();
    }

    template <typename TResult>
    auto TaskCompletionSource<TResult>::initial_suspend() const noexcept -> std::suspend_never
    {
        return {};
    }

    template <typename TResult>
    auto TaskCompletionSource<TResult>::unhandled_exception() noexcept -> void
    {
        Super::SetException(std::current_exception());
    }

    template <typename TResult>
    template <typename T>
    auto TaskCompletionSource<TResult>::return_value(T&& value) noexcept -> void
        requires(std::convertible_to<T, TResult>)
    {
        Super::SetResult(std::forward<T>(value));
    }

    template <typename TResult>
    auto TaskCompletionSource<TResult>::final_suspend() const noexcept -> std::suspend_never
    {
        return {};
    }

    auto TaskCompletionSource<void>::get_return_object() noexcept -> FutureType
    {
        return GetTask();
    }

    auto TaskCompletionSource<void>::initial_suspend() const noexcept -> std::suspend_never
    {
        return {};
    }

    auto TaskCompletionSource<void>::unhandled_exception() noexcept -> void
    {
        SetException(std::current_exception());
    }

    auto TaskCompletionSource<void>::final_suspend() const noexcept -> std::suspend_never
    {
        return {};
    }

    auto TaskCompletionSource<void>::return_void() noexcept -> void
    {
        SetDone();
    }
}

export namespace std
{
    template <typename TResult, typename... TArgs>
    struct coroutine_traits<Engine::Core::Threading::Task<TResult>, TArgs...>
    {
        using promise_type = Engine::Core::Threading::TaskCompletionSource<TResult>;
    };
}

