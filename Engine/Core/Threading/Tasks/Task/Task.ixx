export module Engine.Core.Threading.Tasks:Task;
import :TaskScheduler;
import std;

namespace Engine::Core::Threading
{
    export template <typename TResult>
    class TaskCompletionSource;

    export template<typename TResult>
    class Task
    {
    public:
        static auto Start(std::function<TResult()>&& entry) -> Task;

        using ResultType = TResult;
        using PromiseType = TaskCompletionSource<ResultType>;

        Task() = default;

        Task(const Task& another) noexcept;
        auto operator=(const Task& another) noexcept -> Task&;

        auto GetResult() const noexcept -> std::optional<ResultType>;

        auto GetException() const noexcept -> std::optional<std::exception_ptr>;

        auto Wait() const noexcept -> void;

        auto await_ready() const noexcept -> bool;

        auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool;

        auto await_resume() -> TResult;
    private:
        Task(PromiseType* promise) noexcept;

        PromiseType* m_promise = nullptr;

        friend PromiseType;
    };

    export template <typename TResult>
    class TaskCompletionSource
    {
    public:

        using ResultType = TResult;
        using FutureType = Task<ResultType>;

        template <typename T>
        static auto FromResult(T&& result) noexcept -> TaskCompletionSource
            requires(std::convertible_to<T, TResult>);

        auto GetTask() noexcept -> FutureType;

        template <typename T>
        auto SetResult(T&& result) noexcept -> void
            requires(std::convertible_to<T, TResult>);

        auto SetException(std::exception_ptr exception) noexcept -> void;

        auto get_return_object() const noexcept -> FutureType;

        auto initial_suspend() const noexcept -> std::suspend_never;

        template <typename T>
        auto return_value(T&& value) noexcept -> void
            requires(std::convertible_to<T, TResult>);

        auto final_suspend() const noexcept -> std::suspend_never;
    private:
        std::condition_variable m_cv;
        std::mutex m_mutex;
        std::variant<ResultType, std::exception_ptr, std::monostate> m_result = std::monostate{};
        std::coroutine_handle<> m_continuation{};
    };

    template<typename TResult>
    auto Task<TResult>::Start(std::function<TResult()>&& entry) -> Task
    {
        return Task();
    }
    template <typename TResult>
    Task<TResult>::Task(const Task& another) noexcept
        : m_promise(another.m_promise)
    {
    }

    template <typename TResult>
    auto Task<TResult>::operator=(const Task& another) noexcept -> Task&
    {
        m_promise = another.m_promise;
        return *this;
    }

    template <typename TResult>
    auto Task<TResult>::GetResult() const noexcept -> std::optional<ResultType>
    {
        if (m_promise == nullptr)
        {
            return std::nullopt;
        }
        std::unique_lock lock{ m_promise->m_mutex };

        if (std::holds_alternative<std::monostate>(m_promise->m_result))
        {
            m_promise->m_cv.wait(lock);
        }

        if (std::holds_alternative<ResultType>(m_promise->m_result))
        {
            return std::get<ResultType>(m_promise->m_result);
        }
        return std::nullopt;
    }

    template <typename TResult>
    auto Task<TResult>::GetException() const noexcept -> std::optional<std::exception_ptr>
    {
        if (m_promise == nullptr)
        {
            return std::nullopt;
        }
        std::unique_lock lock{ m_promise->m_mutex };

        if (std::holds_alternative<std::monostate>(m_promise->m_result))
        {
            m_promise->m_cv.wait(lock);
        }

        if (std::holds_alternative<std::exception_ptr>(m_promise->m_result))
        {
            return std::get<std::exception_ptr>(m_promise->m_result);
        }
        return std::nullopt;
    }

    template <typename TResult>
    auto Task<TResult>::Wait() const noexcept -> void
    {
        if (m_promise == nullptr)
        {
            return;
        }
        std::unique_lock lock{ m_promise->m_mutex };
        if (std::holds_alternative<std::monostate>(m_promise->m_result))
        {
            m_promise->m_cv.wait(lock);
        }
    }

    template <typename TResult>
    auto Task<TResult>::await_ready() const noexcept -> bool
    {
        if (m_promise == nullptr)
        {
            return true;
        }

        std::unique_lock lock{ m_promise->m_mutex };
        return !std::holds_alternative<std::monostate>(m_promise->m_result);
    }

    template <typename TResult>
    auto Task<TResult>::await_suspend(std::coroutine_handle<> handle) noexcept -> bool
    {
        if (m_promise == nullptr)
        {
            return false;
        }
        std::unique_lock lock{ m_promise->m_mutex };
        if (std::holds_alternative<std::monostate>(m_promise->m_result))
        {
            m_promise->m_continuation = handle;
            return true;
        }
        return false;
    }

    template <typename TResult>
    auto Task<TResult>::await_resume() -> TResult
    {
        if (m_promise == nullptr)
        {
            return {};
        }

        std::unique_lock lock{ m_promise->m_mutex };
        assert(!std::holds_alternative<std::monostate>(m_promise->m_result));

        if (std::holds_alternative<std::exception_ptr>(m_promise->m_result))
        {
            std::rethrow_exception(std::get<std::exception_ptr>(m_promise->m_result));
        }
        return std::get<TResult>(m_promise->m_result);
    }

    template <typename TResult>
    Task<TResult>::Task(PromiseType* promise) noexcept
        : m_promise(promise)
    {
    }

    template <typename TResult>
    template <typename T>
    auto TaskCompletionSource<TResult>::FromResult(T&& result) noexcept -> TaskCompletionSource
        requires (std::convertible_to<T, TResult>)
    {
        TaskCompletionSource tcs{};
        tcs.m_result = std::forward<T>(result);
        return tcs;
    }

    template <typename TResult>
    auto TaskCompletionSource<TResult>::GetTask() noexcept -> FutureType
    {
        return FutureType{ this };
    }

    template <typename TResult>
    template <typename T>
    auto TaskCompletionSource<TResult>::SetResult(T&& result) noexcept -> void
        requires (std::convertible_to<T, TResult>)
    {
        std::lock_guard lock{ m_mutex };
        if (!std::holds_alternative<std::monostate>(m_result))
        {
            return;
        }

        m_result.emplace(std::forward<T>(result));
        m_cv.notify_all();

        TaskScheduler::GetCurrentScheduler()->QueueContinuation(m_continuation);
    }

    template <typename TResult>
    auto TaskCompletionSource<TResult>::SetException(std::exception_ptr exception) noexcept -> void
    {
        std::lock_guard lock{ m_mutex };
        if (!std::holds_alternative<std::monostate>(m_result))
        {
            return;
        }

        m_result.emplace(exception);
        m_cv.notify_all();
        TaskScheduler::GetCurrentScheduler()->QueueContinuation(m_continuation);
    }

    template <typename TResult>
    auto TaskCompletionSource<TResult>::get_return_object() const noexcept -> FutureType
    {
        return GetTask();
    }

    template <typename TResult>
    auto TaskCompletionSource<TResult>::initial_suspend() const noexcept -> std::suspend_never
    {
        return {};
    }

    template <typename TResult>
    template <typename T>
    auto TaskCompletionSource<TResult>::return_value(T&& value) noexcept -> void
        requires (std::convertible_to<T, TResult>)
    {
        SetResult(std::forward<T>(value));
    }

    template <typename TResult>
    auto TaskCompletionSource<TResult>::final_suspend() const noexcept -> std::suspend_never
    {
        return {};
    }
}

namespace std
{
    export template <typename TResult, typename... TArgs>
        class coroutine_traits<Engine::Core::Threading::Task<TResult>, TArgs...>
    {
        using PromiseType = Engine::Core::Threading::TaskCompletionSource<TResult>;
    };
}
