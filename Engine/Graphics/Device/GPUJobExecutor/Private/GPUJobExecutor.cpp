module;
#include "Engine/Graphics/Device/DirectX/DirectxHeaders.hpp"
module Engine.Graphics.Device.GPUJobExecutor;
import Engine.Graphics.Device.Utils;
import Engine.Graphics.Device.D3DX12;
import Engine.Graphics.Device.GPUScheduler;

namespace Engine::Graphics::Device
{
    auto GPUJobExecutor::Initialise(Context* context) -> void
    {
        m_context = context;
        for (const auto& [fenceValue, fence] :
            std::views::zip(m_fenceValues, m_fences))
        {
            ThrowIfFailed(m_context
                ->GetDevice()
                ->CreateFence(
                    fenceValue,
                    D3D12_FENCE_FLAG_NONE,
                    IID_PPV_ARGS(fence.ReleaseAndGetAddressOf())));
            fenceValue++;
        }
    }

    auto GPUJobExecutor::Teardown() -> void
    {
        for (auto& fence :m_fences)
        {
            fence.Reset();
        }
    }

    auto GPUJobExecutor::ExecuteGPUJob(Private::RenderNode& job) -> JobID
    {
        m_fenceValues[job.queueType]++;
        std::vector<ID3D12GraphicsCommandList*> commandLists;

        // Present frame will always be at the end of the list
        bool presentFrame = false;
        for (auto& list : job.steps)
        {
            if (!list.IsPresentFrame())
            {
                commandLists.emplace_back(list.Get());
            }
            else
            {
                presentFrame = true;
            }
        }

        std::optional<JobID> result = std::nullopt;
        if (!std::empty(commandLists))
        {
            m_context->GetCommandListPool()
                ->GetCommandQueue(job.queueType)
                ->ExecuteCommandLists(
                    static_cast<UINT>(commandLists.size()),
                    CommandListCast(commandLists.data()));
        }
        if (presentFrame)
        {
            m_context->PresentFrame();
        }
        m_context->GetCommandListPool()
            ->GetCommandQueue(job.queueType)
            ->Signal(m_fences[job.queueType].Get(), m_fenceValues[job.queueType]);
        return m_fenceValues[job.queueType];
    }

    auto GPUJobExecutor::Signal(D3D12_COMMAND_LIST_TYPE type) noexcept -> JobID
    {
        m_fenceValues[type]++;
        m_context->GetCommandListPool()
            ->GetCommandQueue(type)
            ->Signal(m_fences[type].Get(), m_fenceValues[type]);
        return m_fenceValues[type];
    }
}

