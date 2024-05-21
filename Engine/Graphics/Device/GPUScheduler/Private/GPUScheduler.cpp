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
import std;

namespace Engine::Graphics::Device
{
    auto GPUScheduler::Initialise(Context* context) -> void
    {
        m_context = context;
        CreateEvents();
        m_gpuJobExecutor.Initialise(m_context);
        m_gpuSchedulerThread = std::jthread([this] { GPUSchedulerThread(); });

        for (auto& fenceEvent : m_fenceEvent)
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

    auto GPUScheduler::SetRootRenderable(std::unique_ptr<Renderable> renderable) -> void
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
        m_rootRenderable->SetContext(m_context);
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
        //Teardown();
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
        WaitForGpu(true);
        for (auto& fenceEvent : m_fenceEvent)
        {
            fenceEvent.Attach(CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_MODIFY_STATE | SYNCHRONIZE));
            if (!fenceEvent.IsValid())
            {
                throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "CreateEventEx");
            }
        }
    }

    auto GPUScheduler::WaitForGpu(
        D3D12_COMMAND_LIST_TYPE type, 
        std::optional<std::chrono::milliseconds> waitDuration) noexcept -> bool
    {
        DWORD waitResult = WAIT_TIMEOUT;
        DWORD milliseconds = INFINITE;
        if (waitDuration.has_value())
        {
            milliseconds = static_cast<DWORD>(waitDuration.value().count());
        }

        return WAIT_OBJECT_0 == WaitForSingleObjectEx(
            m_fenceEvent[type].Get(), 
            milliseconds,
            FALSE);
    }

    auto GPUScheduler::WaitForGpu(
        bool waitAll, 
        std::optional<std::chrono::milliseconds> waitDuration) noexcept -> bool
    {
        std::vector<HANDLE> eventsToWait;

        for (auto type : CommandListPool::COMMAND_QUEUE_TYPES)
        {
            eventsToWait.emplace_back(m_fenceEvent[type].Get());
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
            if (!WaitForGpu(true, 0ms))
            {
                m_gpuIdle = false;
            }
            if (!WaitForGpu(false, 1ms))
            {
                m_gpuIdle = false;
                continue;
            }
            auto jobExecuted = false;
            bool closedQueues[CommandListPool::MAX_COMMAND_LIST_TYPE]{};
            for (auto type : CommandListPool::COMMAND_QUEUE_TYPES)
            {
                // The queue is idle and there are enough command lists
                if (!WaitForGpu(type, 0ms))
                {
                    continue;
                }
                ReturnCommandListsWhenCommandsFinished(type);
                if (m_currentCommandPresentsFrame && type == D3D12_COMMAND_LIST_TYPE_DIRECT)
                {
                    m_currentCommandPresentsFrame = false;
                    UpdateStatisticDataWhenFrameFinished();
                }
                RunCommandContinuations(type);
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
                m_commandListsToReturn[type] = node;
                if (type == D3D12_COMMAND_LIST_TYPE_DIRECT)
                {
                    m_currentCommandPresentsFrame = CheckIfNodePresentsFrame(node);
                }
                SetupContinuations(node);
                ResetEvent(m_fenceEvent[type].Get());
                auto jobID = m_gpuJobExecutor.ExecuteGPUJob(std::move(node));
                ThrowIfFailed(
                    m_gpuJobExecutor.GetFence(type)->SetEventOnCompletion(jobID, m_fenceEvent[type].Get())
                );
                m_inFlightJobs[type].emplace_back(jobID);
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

    auto GPUScheduler::ReturnCommandListsWhenCommandsFinished(D3D12_COMMAND_LIST_TYPE queueType) -> void
    {
        for (auto& commandList : m_commandListsToReturn[queueType].steps)
        {
            if (!commandList.IsPresentFrame())
            {
                m_context->GetCommandListPool()->Return(std::move(commandList));
            }
        }
    }

    auto GPUScheduler::UpdateStatisticDataWhenFrameFinished() -> void
    {
        m_statistics.OnFrameReady();
    }

    auto GPUScheduler::SetupContinuations(const Private::RenderNode& node) -> void
    {
        m_commandListContinuations[node.queueType].clear();
        for (auto& commandList : node.steps)
        {
            auto continuation = commandList.GetContinuation();
            if (continuation.has_value())
            {
                m_commandListContinuations[node.queueType].emplace_back(continuation.value());
            }
        }
    }

    auto GPUScheduler::RunCommandContinuations(D3D12_COMMAND_LIST_TYPE queueType) -> void
    {
        auto continuations = m_commandListContinuations[queueType];
        m_commandListContinuations[queueType].clear();

        Core::Threading::Task<void>::Start([continuations]
            {
                for (auto [userData, func] : continuations)
                {
                    func(userData);
                }
            });
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

