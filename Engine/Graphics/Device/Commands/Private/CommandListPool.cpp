module;
#include "Engine/Graphics/Device/DirectX/DirectxHeaders.hpp"
module Engine.Graphics.Device.CommandListPool;
import Engine.Graphics.Device.D3DX12;
import Engine.Graphics.Device.Utils;

namespace Engine::Graphics::Device
{
    auto CommandListPool::Initialise(Context* context) -> void
    {
        std::lock_guard lock{ m_mutex };
        m_context = context;

        for (int i = 0; i < m_commandQueues.size(); i++)
        {
            auto type = static_cast<D3D12_COMMAND_LIST_TYPE>(i);
            if (type != D3D12_COMMAND_LIST_TYPE_BUNDLE)
            {
                // Create the command queue.
                D3D12_COMMAND_QUEUE_DESC queueDesc = {};
                queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
                queueDesc.Type = type;

                ThrowIfFailed(m_context->GetDevice()->CreateCommandQueue(
                    &queueDesc,
                    IID_PPV_ARGS(m_commandQueues[i].ReleaseAndGetAddressOf())));
                auto name = std::format(L"CommandQueue {}", i);
                m_commandQueues[i]->SetName(name.c_str());
            }
            CreateCommandList(type);
        }
    }

    auto CommandListPool::Rent(D3D12_COMMAND_LIST_TYPE type)
        -> GraphicsCommandList
    {
        std::lock_guard lock{ m_mutex };
        if (std::empty(m_pool[type]))
        {
            CreateCommandList(type);
        }
        auto commandList = m_pool[type].back();
        m_pool[type].pop_back();
        commandList.GetAllocator()->Reset();
        commandList->Reset(commandList.GetAllocator().Get(), nullptr);
        return commandList;
    }

    auto CommandListPool::Return(
        GraphicsCommandList&& commandList) -> void
    {
        std::lock_guard lock{ m_mutex };
        commandList.SetContinuation({});
        m_pool[commandList.Type()].emplace_back(std::move(commandList));
    }

    auto CommandListPool::Teardown() -> void
    {
        std::lock_guard lock{ m_mutex };
        for (auto& commandLists: m_pool)
        {
            commandLists.clear();
        }

        for (auto& commandQueue : m_commandQueues)
        {
            commandQueue.Reset();
        }
    }

    auto CommandListPool::CreateCommandList(D3D12_COMMAND_LIST_TYPE type, std::size_t count) -> void
    {
        for (std::size_t i = 0; i < count; i++)
        {
            ComPtr<ID3D12CommandAllocator> commandAllocator;

            ThrowIfFailed(m_context
                ->GetDevice()
                ->CreateCommandAllocator(
                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                    IID_PPV_ARGS(commandAllocator.ReleaseAndGetAddressOf())));

            ComPtr<ID3D12GraphicsCommandList> commandList;
            ThrowIfFailed(m_context->GetDevice()->CreateCommandList(
                0, 
                D3D12_COMMAND_LIST_TYPE_DIRECT, 
                commandAllocator.Get(),
                nullptr, IID_PPV_ARGS(commandList.ReleaseAndGetAddressOf()))
            );
            ThrowIfFailed(commandList->Close());
            m_pool[type].emplace_back(
                GraphicsCommandList{ type, std::move(commandAllocator), std::move(commandList) }
            );
        }
    }
}

