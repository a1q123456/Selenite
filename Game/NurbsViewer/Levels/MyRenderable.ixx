module;
#include "Engine/Graphics/Device/DirectX/DirectxHeaders.hpp"
export module NurbsViewer.MyRenderable;
import Engine.Graphics.Device.D3DX12;
import Engine.Graphics.Renderable;
import std;

using namespace Microsoft::WRL;

namespace NurbsViewer
{
    export class MyRenderable : public Engine::Graphics::Renderable
    {
    public:
        auto Initialise() -> void override;
        auto Render(float time) -> void override;
    private:
        auto LoadNurbs();

        ComPtr<ID3D12PipelineState> m_pipelineState;
        ComPtr<ID3D12RootSignature> m_rootSignature;
    };
}

