module;
#include "Engine/Graphics/Device/DirectX/DirectxHeaders.hpp"
export module Engine.Graphics.Device.GPUScheduler;
import Engine.Core.Threading;
import Engine.Graphics.Device.Context;
import Engine.Graphics.Device.GPUScheduler.RenderNode;
import Engine.Graphics.Device.CommandListPool;
import Engine.Graphics.Device.CommandListStatistics;
import Engine.Graphics.Device.GPUJobQueue;
import Engine.Graphics.Device.GPUJobExecutor;
import std;

namespace Engine::Graphics
{
    export class Renderable;
}

using namespace Microsoft::WRL;

namespace Engine::Graphics::Device
{
    export class GPUScheduler final
    {
    public:
        auto Initialise(Context* context) -> void;
        auto Teardown() -> void;
        auto SetRootRenderable(std::unique_ptr<Renderable> renderable) -> void;

        auto Tick() -> void;
        ~GPUScheduler() noexcept;

    private:
        auto CreateEvents() -> void;
        auto WaitForGpu(D3D12_COMMAND_LIST_TYPE type, std::optional<std::chrono::milliseconds> waitDuration) noexcept -> bool;
        auto WaitForGpu(bool waitAll, std::optional<std::chrono::milliseconds> waitDuration = {}) noexcept -> bool;

        auto GPUSchedulerThread() noexcept -> void;
        auto GetIdealCommandListsCount(D3D12_COMMAND_LIST_TYPE type) const noexcept -> int;

        auto PushCommands(GraphicsCommandList&& commandList) -> void;
        auto CheckIfNodePresentsFrame(const Private::RenderNode& node) -> bool;

        auto ReturnCommandListsWhenCommandsFinished(D3D12_COMMAND_LIST_TYPE queueType) -> void;
        auto UpdateStatisticDataWhenFrameFinished() -> void;
        auto SetupContinuations(const Private::RenderNode& node) -> void;
        auto RunCommandContinuations(D3D12_COMMAND_LIST_TYPE queueType) -> void;

        static constexpr int MAX_FRAME_DELAY = 0;

        Context* m_context = nullptr;
        std::unique_ptr<Renderable> m_rootRenderable;
        bool m_gpuIdle = true;

        std::array<Wrappers::Event, CommandListPool::MAX_COMMAND_LIST_TYPE> m_fenceEvent;

        std::jthread m_gpuSchedulerThread;

        bool m_currentCommandPresentsFrame = false;
        CommandListStatistics m_statistics;
        GPUJobQueue m_gpuJobQueue;
        GPUJobExecutor m_gpuJobExecutor;

        std::array<
            Private::RenderNode,
            CommandListPool::MAX_COMMAND_LIST_TYPE
        > m_commandListsToReturn;

        std::array<
            std::vector<GraphicsCommandList::ContinuationType>,
            CommandListPool::MAX_COMMAND_LIST_TYPE
        > m_commandListContinuations;
            

        friend class Renderable;
    };
}
