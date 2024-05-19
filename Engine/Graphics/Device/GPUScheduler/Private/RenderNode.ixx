module;
#include <cassert>
#include "Engine/Graphics/Device/DirectX/DirectxHeaders.hpp"
export module Engine.Graphics.Device.GPUScheduler.RenderNode;
import Engine.Core.Containers.BlockingQueue;
import Engine.Graphics.Device.CommandListPool;
import Engine.Core.Utilities.Overloaded;
import std;

namespace Engine::Graphics::Device::Private
{
    export struct RenderNode
    {
        std::vector<GraphicsCommandList> steps;

        D3D12_COMMAND_LIST_TYPE queueType = D3D12_COMMAND_LIST_TYPE_DIRECT;
    };
}

