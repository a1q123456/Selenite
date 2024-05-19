module;
#include "Engine/Graphics/Device/DirectX/DirectxHeaders.hpp"
export module Engine.Graphics.Device.GPUJobQueue;
import Engine.Graphics.Device.GPUScheduler.RenderNode;
import Engine.Graphics.Device.CommandListPool;
import Engine.Core.Containers.BlockingQueue;
import std;

namespace Engine::Graphics::Device
{
    export class GPUJobQueue final
    {
    public:
        auto PushCommand(GraphicsCommandList&& commandList) -> void;
        auto TryPullJob(
            D3D12_COMMAND_LIST_TYPE type, 
            int commandListsCount, 
            Private::RenderNode& node) -> bool;
        auto CloseAll() -> void;
        auto ResetAll() -> void;
    private:

        std::array<
            Core::Containers::BlockingQueue<GraphicsCommandList>,
            CommandListPool::MAX_COMMAND_LIST_TYPE
        > m_renderNodeQueues;
    };
}

