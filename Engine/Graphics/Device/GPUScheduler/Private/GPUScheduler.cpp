module;
#include <cassert>
#include "Engine/Graphics/Device/DirectX/DirectxHeaders.hpp"
module Engine.Graphics.Device.GPUScheduler;
import Engine.Graphics.Device.GPUScheduler.RenderNode;
import Engine.Graphics.Device.Utils;
import Engine.Graphics.Device.D3DX12;
import Engine.Graphics.Renderable;
import Engine.Graphics.Device.CommandListPool;
import Engine.Graphics.Renderable;
import Engine.Core.Containers.BlockingQueue;
import Engine.Core.Utilities.Overloaded;
import Engine.Core.Threading.Tasks;
import std;

namespace Engine::Graphics::Device
{
    GPUScheduler::GPUScheduler() :
        m_jobContexts{
            Vector<std::pair<GPUJobExecutor::JobID, JobContext>>{&m_heap},
            Vector<std::pair<GPUJobExecutor::JobID, JobContext>>{&m_heap},
            Vector<std::pair<GPUJobExecutor::JobID, JobContext>>{&m_heap},
            Vector<std::pair<GPUJobExecutor::JobID, JobContext>>{&m_heap},
        },
        m_delayedCleanups(&m_heap)
    {
    }

    auto GPUScheduler::Initialise(Context* context) -> void
    {
        m_context = context;
        CreateEvents();
        m_gpuJobExecutor.Initialise(m_context);
        m_gpuSchedulerThread = std::jthread([this] { GPUSchedulerThread(); });

        for (auto& fenceEvent : m_fenceEvents)
        {
            SetEvent(fenceEvent.Get());
        }
    }

    auto GPUScheduler::Teardown() -> void
    {
        m_gpuJobQueue.CloseAll();
        try
        {
            m_gpuSchedulerThread.join();
        } catch (std::system_error) {}

        m_gpuJobExecutor.Teardown();
    }

    Core::Threading::Task<void> GPUScheduler::SetRootRenderable(std::unique_ptr<Renderable> renderable)
    {
        m_gpuJobQueue.CloseAll();
        try
        {
            m_gpuSchedulerThread.join();
        }
        catch (std::system_error) {}
        m_gpuJobQueue.ResetAll();
        assert(m_gpuIdle);
        m_rootRenderable = std::move(renderable);
        m_gpuSchedulerThread = std::jthread([this] { GPUSchedulerThread(); });
        return m_rootRenderable->SetContext(m_context);
    }

    auto GPUScheduler::Tick() -> void
    {
        bool fireNewFrame = m_statistics.GetQueuedFrames() < MAX_FRAME_DELAY;
        
        if (!fireNewFrame)
        {
            return;
        }

        // TODO: Calculate the time
        m_rootRenderable->Render(0);
        PushCommands({});
        m_context->RollRenderTarget();
        if (m_gpuIdle)
        {
            // TODO: Use more threads (up to number of cores to record commands)
        }
    }

    GPUScheduler::~GPUScheduler() noexcept
    {
        Teardown();
    }

    auto GPUScheduler::PushCommands(GraphicsCommandList&& commandList) -> void
    {
        if (commandList.IsPresentFrame())
        {
            m_statistics.OnPresentFrame();
        }
        else
        {
            m_statistics.OnCommandList(commandList.QueueType());
        }
        m_gpuJobQueue.PushCommand(std::move(commandList));
    }

    auto GPUScheduler::CheckIfNodePresentsFrame(const Private::RenderNode& node) -> bool
    {
        return !std::empty(node.steps) && node.steps.back().IsPresentFrame();
    }

    auto GPUScheduler::CreateEvents() -> void
    {
        for (auto& fenceEvent : m_fenceEvents)
        {
            fenceEvent.Attach(CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_MODIFY_STATE | SYNCHRONIZE));
            if (!fenceEvent.IsValid())
            {
                throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "CreateEventEx");
            }
        }
    }

    auto GPUScheduler::WaitForGpu(
        bool waitAll, 
        std::optional<std::chrono::milliseconds> waitDuration) noexcept -> bool
    {
        std::vector<HANDLE> eventsToWait;

        for (auto type : CommandListPool::COMMAND_QUEUE_TYPES)
        {
            m_gpuJobExecutor.Signal(type);
            eventsToWait.emplace_back(m_fenceEvents[type].Get());
        }

        DWORD milliseconds = INFINITE;
        if (waitDuration.has_value())
        {
            milliseconds = static_cast<DWORD>(waitDuration.value().count());
        }
        auto waitResult = WaitForMultipleObjectsEx(
            static_cast<DWORD>(eventsToWait.size()), 
            eventsToWait.data(), 
            waitAll,
            milliseconds,
            FALSE);

        return waitResult >= WAIT_OBJECT_0 && (waitResult < WAIT_OBJECT_0 + eventsToWait.size());
    }

    auto GPUScheduler::GPUSchedulerThread() noexcept -> void
    {
        using namespace std::chrono_literals;
        Private::RenderNode node;
        while (true)
        {
            auto currentJobIDs = WaitForQueues(1ms);
            auto finishStatus = GetJobFinishStatus(currentJobIDs);
            if (!std::ranges::any_of(finishStatus, [](bool r) { return r; }))
            {
                continue;
            }

            auto jobExecuted = false;
            bool closedQueues[CommandListPool::MAX_COMMAND_LIST_TYPE]{};
            for (auto type : CommandListPool::COMMAND_QUEUE_TYPES)
            {
                // The queue is idle and there are enough command lists
                if (!finishStatus[type])
                {
                    continue;
                }

                auto finishedJobContexts = PopJobContexts(type, currentJobIDs[type]);
                CleanupJobs(std::move(finishedJobContexts));
                try
                {
                    if (!m_gpuJobQueue.TryPullJob(type, GetIdealCommandListsCount(type), node))
                    {
                        continue;
                    }
                }
                catch (Core::Containers::BlockingQueueClosed)
                {
                    closedQueues[type] = true;
                    continue;
                }
                jobExecuted = true;
                ResetEvent(m_fenceEvents[type].Get());
                auto jobID = m_gpuJobExecutor.ExecuteGPUJob(node);
                SetupJobContext(jobID, type, std::move(node));
                ThrowIfFailed(
                    m_gpuJobExecutor.GetFence(type)->SetEventOnCompletion(jobID, m_fenceEvents[type].Get())
                );
            }
            if (!jobExecuted)
            {
                m_gpuIdle = true;
            }
            if (closedQueues[D3D12_COMMAND_LIST_TYPE_COMPUTE] &&
                closedQueues[D3D12_COMMAND_LIST_TYPE_DIRECT] &&
                closedQueues[D3D12_COMMAND_LIST_TYPE_COPY])
            {
                return;
            }
        }
    }

    auto GPUScheduler::UpdateStatisticDataWhenFrameFinished() -> void
    {
        m_statistics.OnFrameReady();
    }

    auto GPUScheduler::WaitForQueues(std::chrono::milliseconds duration)
        -> std::array<GPUJobExecutor::JobID, CommandListPool::MAX_COMMAND_LIST_TYPE>
    {
        std::array<GPUJobExecutor::JobID, CommandListPool::MAX_COMMAND_LIST_TYPE> result{};
        std::array<HANDLE, CommandListPool::MAX_COMMAND_LIST_TYPE> eventsToWait{};

        std::ranges::transform(
            m_fenceEvents,
            std::begin(eventsToWait), 
            [](const Wrappers::Event& fenceEvent)
            {
                return fenceEvent.Get();
            });

        DWORD milliseconds = static_cast<DWORD>(duration.count());
        auto waitResult = WaitForMultipleObjectsEx(
            static_cast<DWORD>(eventsToWait.size()),
            eventsToWait.data(),
            FALSE,
            milliseconds,
            FALSE);

        auto success = waitResult >= WAIT_OBJECT_0 && (waitResult < WAIT_OBJECT_0 + eventsToWait.size());

        if (!success)
        {
            return result;
        }

        for (auto queueType : CommandListPool::COMMAND_QUEUE_TYPES)
        {
            auto completedValue = m_gpuJobExecutor.GetFence(queueType)->GetCompletedValue();

            result[queueType] = completedValue;
        }
        return result;
    }

    auto GPUScheduler::GetJobFinishStatus(
        const std::array<GPUJobExecutor::JobID, CommandListPool::MAX_COMMAND_LIST_TYPE>& currentJobIDs)
            const noexcept -> std::array<bool, CommandListPool::MAX_COMMAND_LIST_TYPE>
    {
        std::array<bool, CommandListPool::MAX_COMMAND_LIST_TYPE> result{};
        for (auto queueType : CommandListPool::COMMAND_QUEUE_TYPES)
        {
            auto completedValue = m_gpuJobExecutor.GetFence(queueType)->GetCompletedValue();
            auto latestValue = m_gpuJobExecutor.GetCurrentJobID(queueType);

            if (latestValue - completedValue <= MAX_QUEUED_COMMAND_EXECUTIONS)
            {
                result[queueType] = true;
            }
        }
        return result;
    }

    auto GPUScheduler::PopJobContexts(
        D3D12_COMMAND_LIST_TYPE queueType,
        GPUJobExecutor::JobID finishedJobID) -> Vector<JobContext>
    {
        Vector<JobContext> result(& m_heap);
        for (auto& [jobID, jobContext] : m_jobContexts[queueType])
        {
            if (finishedJobID - jobID <= MAX_QUEUED_COMMAND_EXECUTIONS)
            {
                result.emplace_back(std::move(jobContext));
            }
        }
        std::erase_if(m_jobContexts[queueType], [&](auto& pair)
            {
                return finishedJobID - pair.first <= MAX_QUEUED_COMMAND_EXECUTIONS;
            });
        return result;
    }

    auto GPUScheduler::CleanupJobs(Vector<JobContext>&& jobs) -> void
    {
        while (m_unblockedCleanupIndex != 0)
        {
            m_delayedCleanups.pop_front();

            --m_unblockedCleanupIndex;
        }
        for (auto& job : jobs)
        {
            for (auto& commandList : job.node.steps)
            {
                if (!commandList.IsPresentFrame())
                {
                    m_context->GetCommandListPool()->Return(std::move(commandList));
                }
                commandList.ReleaseCommandList();
            }
            if (job.presentsFrame)
            {
                UpdateStatisticDataWhenFrameFinished();
            }

            if (!std::empty(job.callbacks))
            {
                auto begin = m_delayedCleanups.insert(
                    std::end(m_delayedCleanups),
                    std::make_move_iterator(std::begin(job.callbacks)),
                    std::make_move_iterator(std::end(job.callbacks)));

                auto end = std::end(m_delayedCleanups);

                // TODO: Make the tread pool lock-free
                Core::Threading::TaskScheduler::GetCurrentScheduler()->GetThreadPool().SubmitWork([begin, end, this]
                {
                    for (auto iter = begin; iter != end; )
                    {
                        auto& func = *iter;
                        func();

                        ++iter;
                        ++m_unblockedCleanupIndex;
                    }
                });
            }
        }
    }

    auto GPUScheduler::SetupJobContext(
        GPUJobExecutor::JobID jobID,
        D3D12_COMMAND_LIST_TYPE queueType, 
        Private::RenderNode&& node) -> void
    {
        JobContext jobContext{ &m_heap };
        if (queueType == D3D12_COMMAND_LIST_TYPE_DIRECT)
        {
            jobContext.presentsFrame = CheckIfNodePresentsFrame(node);
        }
        for (auto& step : node.steps)
        {
            auto& continuation = step.GetContinuation();
            if (continuation.has_value())
            {
                jobContext.callbacks.emplace_back(
                    std::move(continuation.value())
                );
            }
        }
        jobContext.node = std::move(node);

        m_jobContexts[queueType].emplace_back(std::make_pair(jobID, std::move(jobContext)));
    }

    auto GPUScheduler::GetIdealCommandListsCount(D3D12_COMMAND_LIST_TYPE type) const noexcept -> int
    {
        return 1;
        // Number of ExecuteCommandLists calls per frame
        constexpr int OPTIMISATION_TARGET = 7;

        int numerator = 0;
        int denominator = 0;
        for (int i = 0; i < CommandListPool::MAX_COMMAND_LIST_TYPE; i++)
        {
            auto currentType = static_cast<D3D12_COMMAND_LIST_TYPE>(i);

            auto count = m_statistics.GetCommandListsPerFrame(currentType);
            if (currentType == type)
            {
                numerator = count;
            }

            denominator += count;
        }
        if (denominator == 0)
        {
            return true;
        }

        denominator *= OPTIMISATION_TARGET;

        return std::max(1, numerator / std::max(1, denominator));
    }
}

