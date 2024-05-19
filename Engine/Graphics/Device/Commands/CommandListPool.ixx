module;
#include "Engine/Graphics/Device/DirectX/DirectxHeaders.hpp"
export module Engine.Graphics.Device.CommandListPool;
import Engine.Graphics.Device.Context;
import std;

using namespace Microsoft::WRL;

namespace Engine::Graphics::Device
{
    export class GraphicsCommandList
    {
    public:
        using ContinuationFunction = void(void*);
        using ContinuationType = std::pair<void*, ContinuationFunction*>;

        GraphicsCommandList() = default;

        auto operator->() const noexcept -> ID3D12GraphicsCommandList*
        {
            return m_commandList.Get();
        }

        auto IsPresentFrame() const noexcept -> bool
        {
            return !m_commandList;
        }

        auto Type() const noexcept
        {
            return m_type;
        }

        auto QueueType() const noexcept
        {
            if (m_type == D3D12_COMMAND_LIST_TYPE_BUNDLE)
            {
                return D3D12_COMMAND_LIST_TYPE_DIRECT;
            }
            return m_type;
        }

        auto GetAddressOf() noexcept
        {
            return m_commandList.GetAddressOf();
        }

        auto Get() noexcept
        {
            return m_commandList.Get();
        }

        auto SetContinuation(ContinuationType continuation) noexcept
        {
            m_continuation = continuation;
        }

        auto GetContinuation() const noexcept
        {
            return m_continuation;
        }
        auto GetAllocator() const noexcept
        {
            return m_allocator;
        }
    private:
        GraphicsCommandList(
            D3D12_COMMAND_LIST_TYPE type,
            const ComPtr<ID3D12CommandAllocator>& allocator,
            const ComPtr<ID3D12GraphicsCommandList>& commandList)
                : m_type(type), m_allocator(allocator), m_commandList(commandList)
        {
        }

        std::optional<ContinuationType> m_continuation = {};
        D3D12_COMMAND_LIST_TYPE m_type;
        ComPtr<ID3D12CommandAllocator> m_allocator;
        ComPtr<ID3D12GraphicsCommandList> m_commandList;

        friend class CommandListPool;
    };

    export class CommandListPool
    {
    public:
        auto Initialise(Context* context) -> void;

        auto Rent(D3D12_COMMAND_LIST_TYPE type) -> GraphicsCommandList;
        auto Return(GraphicsCommandList&& commandList) -> void;

        auto GetCommandQueue(D3D12_COMMAND_LIST_TYPE type)
        {
            return m_commandQueues[type];
        }

        auto Teardown() -> void;

        static constexpr int MAX_COMMAND_LIST_TYPE = D3D12_COMMAND_LIST_TYPE_COPY + 1;
        static constexpr D3D12_COMMAND_LIST_TYPE COMMAND_QUEUE_TYPES[] = {
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            D3D12_COMMAND_LIST_TYPE_COMPUTE,
            D3D12_COMMAND_LIST_TYPE_COPY,
        };
        static constexpr D3D12_COMMAND_LIST_TYPE COMMAND_LIST_TYPES[] = {
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            D3D12_COMMAND_LIST_TYPE_BUNDLE,
            D3D12_COMMAND_LIST_TYPE_COMPUTE,
            D3D12_COMMAND_LIST_TYPE_COPY,
        };
    private:
        auto CreateCommandList(D3D12_COMMAND_LIST_TYPE type, std::size_t count = 1) -> void;

        using PoolType = std::array<std::vector<GraphicsCommandList>, MAX_COMMAND_LIST_TYPE>;

        Context* m_context = nullptr;
        std::array<ComPtr<ID3D12CommandQueue>, MAX_COMMAND_LIST_TYPE> m_commandQueues;
        PoolType m_pool;
        std::mutex m_mutex;
    };
}

