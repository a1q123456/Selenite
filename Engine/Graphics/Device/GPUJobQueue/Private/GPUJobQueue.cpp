module Engine.Graphics.Device.GPUJobQueue;

namespace Engine::Graphics::Device
{
    auto GPUJobQueue::PushCommand(GraphicsCommandList&& commandList) -> void
    {
        m_renderNodeQueues[commandList.QueueType()].Push(std::move(commandList));
    }

    auto GPUJobQueue::TryPullJob(
        D3D12_COMMAND_LIST_TYPE type, 
        int commandListsCount, 
        Private::RenderNode& node) -> bool
    {
        if (m_renderNodeQueues[type].IsClosed())
        {
            if (!m_renderNodeQueues[type].CanPop())
            {
                throw Core::Containers::BlockingQueueClosed{};
            }
        }
        else if (m_renderNodeQueues[type].Size() < commandListsCount)
        {
            return false;
        }

        node.steps.clear();
        for (int i = 0; i < commandListsCount; i++)
        {
            GraphicsCommandList job;
            try
            {
                job = m_renderNodeQueues[type].Pop();
            }
            catch (Core::Containers::BlockingQueueClosed)
            {
                break;
            }
            bool stop = job.IsPresentFrame();
            node.steps.emplace_back(job);
            if (stop)
            {
                break;
            }
        }
        return true;
    }

    auto GPUJobQueue::CloseAll() -> void
    {
        for (auto type : CommandListPool::COMMAND_QUEUE_TYPES)
        {
            m_renderNodeQueues[type].Close();
            m_renderNodeQueues[type].Clear();
        }
    }

    auto GPUJobQueue::ResetAll() -> void
    {
        for (auto type : CommandListPool::COMMAND_QUEUE_TYPES)
        {
            m_renderNodeQueues[type].Reset();
        }
    }
}

