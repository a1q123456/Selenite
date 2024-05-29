module;
#include "Engine/Graphics/Device/DirectX/DirectxHeaders.hpp"
export module NurbsViewer.MyRenderable;
import Engine.Graphics.Device.D3DX12;
import Engine.Graphics.Renderable;
import Engine.Core.Memory.FastLocalHeap;
import Engine.Core.Memory.FastLocalAllocator;
import Engine.Nurbs.SurfacePatch;
import Engine.Nurbs.NurbsCalculations;
import Engine.Nurbs.BasisFunction;
import std;

using namespace Microsoft::WRL;
using namespace Engine::Core;
using namespace Engine::Nurbs;

namespace NurbsViewer
{
    export class MyRenderable : public Engine::Graphics::Renderable
    {
    public:
        MyRenderable();
        auto Initialise() -> void override;
        auto Render(float time) -> void override;
    private:
        auto LoadNurbs() -> void;
        auto LoadComputeShader(std::istream& shaderStream) const noexcept
            -> std::pair<ComPtr<ID3D12PipelineState>, ComPtr<ID3D12RootSignature>>;

        ComPtr<ID3D12PipelineState> m_rationaliserPSO;
        ComPtr<ID3D12RootSignature> m_rationaliserRS;

        ComPtr<ID3D12PipelineState> m_rayTracerPSO;
        ComPtr<ID3D12RootSignature> m_rayTracerRS;

        Memory::FastLocalHeap m_resourceHeap{10};
        Memory::FastHeapVector<DirectX::XMFLOAT4> m_controlPoints;
        Memory::FastHeapVector<float> m_U;
        Memory::FastHeapVector<float> m_V;
        Memory::FastHeapVector<SurfacePatch> m_surfacePatches;
        SurfaceBasisFunctionsArray<Memory::FastLocalAllocator> m_basisFunctions;
        SurfaceBasisDerivativesArray<Memory::FastLocalAllocator> m_derivatives;
        SurfacePatchIndexArray<Memory::FastLocalAllocator> m_indices;
    };
}

