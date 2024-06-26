module;
#include "Engine/Graphics/Device/DirectX/DirectxHeaders.hpp"
export module NurbsViewer.MyRenderable;
import Engine.Graphics.Device.D3DX12;
import Engine.Graphics.Renderable;
import Engine.Core.Threading.Tasks;
import Engine.Core.Memory.FastLocalHeap;
import Engine.Core.Memory.FastLocalAllocator;
import Engine.Nurbs.SurfacePatch;
import Engine.Nurbs.NurbsCalculations;
import Engine.Nurbs.BasisFunction;
import Engine.Nurbs.GPUInterop.RationaliserData;
import Engine.Nurbs.GPUInterop.CameraData;
import Engine.Nurbs.GPUInterop.NurbsTracingConfiguration;
import Engine.Nurbs.TraceableSurface;
import std;

using namespace Microsoft::WRL;
using namespace Engine::Core;
using namespace Engine::Core::Threading;
using namespace Engine::Nurbs;

namespace NurbsViewer
{
    export class MyRenderable : public Engine::Graphics::Renderable
    {
    public:
        MyRenderable();
        Task<void> Initialise() override;
        auto Render(float time) -> void override;
        auto OnMouseMove(int x, int y) -> void override;
        auto OnMouseDown(Engine::Graphics::MouseButton button) -> void override;
        auto OnMouseUp(Engine::Graphics::MouseButton button) -> void override;
    private:
        auto MoveView() noexcept -> void;
        auto LoadNurbs() -> void;
        auto LoadComputeShader(std::istream& shaderStream) const noexcept
            -> std::pair<ComPtr<ID3D12PipelineState>, ComPtr<ID3D12RootSignature>>;

        template <typename T>
        auto AllocateStructuredBuffer(const Memory::FastHeapVector<T>& arr, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS resourceFlags, D3D12_RESOURCE_STATES defaultState) noexcept
            -> ComPtr<ID3D12Resource>;

        auto AllocateWritableTexture(int width, int height, DXGI_FORMAT format) const noexcept
            -> ComPtr<ID3D12Resource>;
        
        auto CreateOutputTextureUAV(
            const ComPtr<ID3D12Resource>& buffer,
            DXGI_FORMAT format,
            CD3DX12_CPU_DESCRIPTOR_HANDLE& descriptorHandle) const noexcept;

        template <typename T>
        auto CreateUAV(
            const ComPtr<ID3D12Resource>& buffer, 
            const Memory::FastHeapVector<T>& arr,
            CD3DX12_CPU_DESCRIPTOR_HANDLE& descriptorHandle) noexcept;

        ComPtr<ID3D12PipelineState> m_rationaliserPSO;
        ComPtr<ID3D12RootSignature> m_rationaliserRS;

        ComPtr<ID3D12PipelineState> m_rayTracerPSO;
        ComPtr<ID3D12RootSignature> m_rayTracerRS;

        ComPtr<ID3D12Resource> m_basisFunctionsResource;
        ComPtr<ID3D12Resource> m_derivativesResource;
        ComPtr<ID3D12Resource> m_indicesResource;
        ComPtr<ID3D12Resource> m_controlPointsResource;
        ComPtr<ID3D12Resource> m_surfacePatchesResource;
        ComPtr<ID3D12Resource> m_outputTexture;

        ComPtr<ID3D12DescriptorHeap> m_rationaliserDescriptorHeap;
        ComPtr<ID3D12DescriptorHeap> m_rayTracerDescriptorHeap;

        Memory::FastLocalHeap m_resourceHeap{10};
        Memory::FastHeapVector<DirectX::XMFLOAT4> m_controlPoints;
        GPUInterop::RationaliserData m_rationaliserData{};
        GPUInterop::CameraData m_cameraData{};
        GPUInterop::NurbsTracingConfiguration m_nurbsTracingConfiguration{};

        Memory::FastHeapVector<float> m_U;
        Memory::FastHeapVector<float> m_V;
        Memory::FastHeapVector<TraceableSurface> m_traceablePatches;
        SurfaceBasisFunctionsArray<Memory::FastLocalAllocator> m_basisFunctions;
        SurfaceBasisDerivativesArray<Memory::FastLocalAllocator> m_derivatives;
        SurfacePatchIndicesArray<Memory::FastLocalAllocator> m_indices;
        DirectX::XMMATRIX m_iprojectionMatrix{};
        DirectX::XMVECTOR m_lookAt{0, 0, 0};
        DirectX::XMFLOAT3 m_eyeLocation{ 0, -2, 0 };
        DirectX::XMVECTOR m_sphericalCoordinates{ 0, 0 };
        float m_cameraDistance = 7.0f;

        int m_mouseX = -1;
        int m_mouseY = -1;
        bool m_mouseLeftDown = false;
        bool m_mouseRightDown = false;

        bool m_done = false;
        bool m_dispatched = false;
    };
}

