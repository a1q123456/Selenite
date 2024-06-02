module;
#include "Engine/Graphics/Device/DirectX/DirectxHeaders.hpp"
export module Engine.Graphics.Device.GPUScheduler;
import Engine.Core.Threading.Tasks;
import Engine.Core.Memory.FastLocalAllocator;
import Engine.Core.Memory.FastLocalHeap;
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
        GPUScheduler();
        auto Initialise(Context* context) -> void;
        auto Teardown() -> void;
        Core::Threading::Task<void> SetRootRenderable(std::unique_ptr<Renderable> renderable);

        auto Tick() -> void;
        ~GPUScheduler() noexcept;

    private:
        template <typename T>
        using Vector = std::vector<T, Core::Memory::FastLocalAllocator<T>>;

        struct JobContext
        {
            Vector<GraphicsCommandList::ContinuationType> callbacks;
            Private::RenderNode node;
            bool presentsFrame = false;

            JobContext() = delete;
            JobContext(const JobContext&) = delete;
            JobContext(JobContext&& another) noexcept
                : callbacks(std::move(another.callbacks))
            {
                node = std::move(another.node);
                presentsFrame = another.presentsFrame;
            }

            auto operator=(const JobContext&) ->JobContext & = delete;
            auto operator=(JobContext&& another) noexcept -> JobContext&
            {
                callbacks = std::move(callbacks);
                node = std::move(another.node);
                presentsFrame = another.presentsFrame;
                return *this;
            }

            JobContext(Core::Memory::FastLocalHeap* heap) : callbacks(heap) {}
        };

        auto CreateEvents() -> void;
        auto WaitForGpu(bool waitAll, std::optional<std::chrono::milliseconds> waitDuration = {}) noexcept -> bool;
        auto WaitForQueues(std::chrono::milliseconds duration) -> std::array<GPUJobExecutor::JobID, CommandListPool::MAX_COMMAND_LIST_TYPE>;

        auto GPUSchedulerThread() noexcept -> void;
        auto GetIdealCommandListsCount(D3D12_COMMAND_LIST_TYPE type) const noexcept -> int;

        auto PushCommands(GraphicsCommandList&& commandList) -> void;
        auto CheckIfNodePresentsFrame(const Private::RenderNode& node) -> bool;

        auto UpdateStatisticDataWhenFrameFinished() -> void;
        auto GetJobFinishStatus(const std::array<GPUJobExecutor::JobID, CommandListPool::MAX_COMMAND_LIST_TYPE>& currentJobIDs)
            const noexcept -> std::array<bool, CommandListPool::MAX_COMMAND_LIST_TYPE>;
        auto PopJobContexts(D3D12_COMMAND_LIST_TYPE queueType, GPUJobExecutor::JobID finishedJobID)
            -> Vector<JobContext>;
        auto CleanupJobs(Vector<JobContext>&& jobs) -> void;
        auto SetupJobContext(GPUJobExecutor::JobID jobID, D3D12_COMMAND_LIST_TYPE queueType, Private::RenderNode&& node) -> void;

        auto CanExitRendererThread(const std::array<bool, CommandListPool::MAX_COMMAND_LIST_TYPE>& queueClosedState) const noexcept -> bool;

        static constexpr int MAX_FRAME_DELAY = 3;
        static constexpr int MAX_QUEUED_COMMAND_EXECUTIONS = 3;

        Core::Memory::FastLocalHeap m_heap { 1 };

        Context* m_context = nullptr;
        std::unique_ptr<Renderable> m_rootRenderable;
        bool m_gpuIdle = true;

        std::array<Wrappers::Event, CommandListPool::MAX_COMMAND_LIST_TYPE> m_fenceEvents;

        std::jthread m_gpuSchedulerThread;

        CommandListStatistics m_statistics;
        GPUJobQueue m_gpuJobQueue;
        GPUJobExecutor m_gpuJobExecutor;

        std::array<
            Vector<std::pair<GPUJobExecutor::JobID, JobContext>>,
            CommandListPool::MAX_COMMAND_LIST_TYPE
        > m_jobContexts;

        Core::Memory::FastHeapList<GraphicsCommandList::ContinuationType> m_delayedCleanups;
        std::atomic_size_t m_unblockedCleanupIndex;

        friend class Renderable;
    };
}
