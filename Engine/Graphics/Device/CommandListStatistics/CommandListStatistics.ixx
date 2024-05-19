module;
#include "Engine/Graphics/Device/DirectX/DirectxHeaders.hpp"
export module Engine.Graphics.Device.CommandListStatistics;
import Engine.Graphics.Device.CommandListPool;
import std;

namespace Engine::Graphics::Device
{
    export class CommandListStatistics final
    {
    public:
        auto OnPresentFrame() noexcept
        {
            m_queuedFrames++;
            for (int i = 0; i < CommandListPool::MAX_COMMAND_LIST_TYPE; i++)
            {
                auto type = static_cast<D3D12_COMMAND_LIST_TYPE>(i);
                if (m_averageCommandListsPerFrame[type] == 0)
                {
                    m_averageCommandListsPerFrame[type] = m_commandListsCurrentFrame[type];
                }
                else
                {
                    m_averageCommandListsPerFrame[type] = (m_averageCommandListsPerFrame[type] + m_commandListsCurrentFrame[type]) / 2;
                }
                m_commandListsCurrentFrame[type] = 0;
            }
        }

        auto OnFrameReady() noexcept
        {
            m_queuedFrames--;
        }

        auto OnCommandList(D3D12_COMMAND_LIST_TYPE type) noexcept
        {
            m_commandListsCurrentFrame[type] += 1;
        }

        auto GetCommandListsPerFrame(D3D12_COMMAND_LIST_TYPE type) const noexcept
        {
            return std::max(0, m_averageCommandListsPerFrame[type]);
        }

        auto GetQueuedFrames() const noexcept -> int
        {
            return m_queuedFrames;
        }
    private:
        std::array<std::atomic_int, CommandListPool::MAX_COMMAND_LIST_TYPE> m_commandListsCurrentFrame{};
        std::array<int, CommandListPool::MAX_COMMAND_LIST_TYPE> m_averageCommandListsPerFrame{};
        std::atomic_int m_queuedFrames = 0;
    };

}
