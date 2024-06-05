module;
#include "Engine/Graphics/Device/DirectX/DirectxHeaders.hpp"
#include <DirectXMath.h>
module NurbsViewer.MyRenderable;
import Engine.Nurbs.SurfacePatch;
import Engine.Graphics.Device.Utils;
import std;

using namespace Microsoft::WRL;
using namespace Engine::Graphics::Device;

namespace NurbsViewer
{
    MyRenderable::MyRenderable() :
        m_controlPoints(&m_resourceHeap),
        m_U(&m_resourceHeap),
        m_V(&m_resourceHeap),
        m_surfacePatches(&m_resourceHeap),
        m_basisFunctions(&m_resourceHeap),
        m_derivatives(&m_resourceHeap),
        m_indices(&m_resourceHeap)
    {
    }

    auto MyRenderable::LoadComputeShader(std::istream& shaderStream) const noexcept
        -> std::pair<ComPtr<ID3D12PipelineState>, ComPtr<ID3D12RootSignature>>
    {
        ComPtr<ID3D12PipelineState> pipelineState;
        ComPtr<ID3D12RootSignature> rootSignature;

        std::vector<std::uint8_t> buffer(std::istreambuf_iterator(shaderStream), {});

        ThrowIfFailed(GetDevice()->CreateRootSignature(
            0,
            buffer.data(),
            buffer.size(),
            IID_PPV_ARGS(rootSignature.ReleaseAndGetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineStateDesc{};
        pipelineStateDesc.CS.BytecodeLength = buffer.size();
        pipelineStateDesc.CS.pShaderBytecode = buffer.data();
        pipelineStateDesc.pRootSignature = rootSignature.Get();

        ThrowIfFailed(GetDevice()->CreateComputePipelineState(
            &pipelineStateDesc,
            IID_PPV_ARGS(pipelineState.ReleaseAndGetAddressOf()))
        );

        return { pipelineState, rootSignature };
    }

    auto MyRenderable::AllocateWritibleTexture(
        int width, int height, DXGI_FORMAT format
    ) noexcept -> ComPtr<ID3D12Resource>
    {
        ComPtr<ID3D12Resource> resource;
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            format,
            width,
            height,
            1,
            1,
            1,
            0,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
        );
        ThrowIfFailed(GetDevice()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            nullptr,
            IID_PPV_ARGS(resource.ReleaseAndGetAddressOf())));

        return resource;
    }

    template <typename T>
    auto MyRenderable::AllocateStructuredBuffer(
        const Memory::FastHeapVector<T>& arr,
        D3D12_HEAP_TYPE heapType,
        D3D12_RESOURCE_FLAGS resourceFlags,
        D3D12_RESOURCE_STATES defaultState
    ) noexcept -> ComPtr<ID3D12Resource>
    {
        ComPtr<ID3D12Resource> resource;
        std::span resourceView{ arr };
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(heapType);
        auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(resourceView.size_bytes(), resourceFlags);
        ThrowIfFailed(GetDevice()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            defaultState,
            nullptr,
            IID_PPV_ARGS(resource.ReleaseAndGetAddressOf())));

        if ((heapType & D3D12_HEAP_TYPE_UPLOAD) != 0)
        {
            CD3DX12_RANGE range{ 0, 0 };
            void* data = nullptr;
            ThrowIfFailed(resource->Map(0, &range, &data));
            std::memcpy(data, resourceView.data(), resourceView.size_bytes());
            resource->Unmap(0, nullptr);
        }
        return resource;
    }

    auto MyRenderable::CreateOutputTextureUAV(
        const ComPtr<ID3D12Resource>& buffer,
        DXGI_FORMAT format,
        CD3DX12_CPU_DESCRIPTOR_HANDLE& descriptorHandle) noexcept
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = format;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

        GetDevice()->CreateUnorderedAccessView(buffer.Get(), nullptr, &uavDesc, descriptorHandle);
        descriptorHandle.Offset(1, GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    }

    template <typename T>
    auto MyRenderable::CreateUAV(
        const ComPtr<ID3D12Resource>& buffer,
        const Memory::FastHeapVector<T>& arr,
        CD3DX12_CPU_DESCRIPTOR_HANDLE& descriptorHandle) noexcept
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = static_cast<UINT>(arr.size());
        uavDesc.Buffer.StructureByteStride = sizeof(T);
        uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

        GetDevice()->CreateUnorderedAccessView(buffer.Get(), nullptr, &uavDesc, descriptorHandle);
        descriptorHandle.Offset(1, GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    }

    Task<void> MyRenderable::Initialise()
    {
        auto rayTracerPath = std::filesystem::current_path() / "Output" / "Shaders" / "RayTracer.dxil";
        std::fstream rayTracerFileStream{ rayTracerPath, std::ios::in | std::ios::binary };

        std::tie(m_rayTracerPSO, m_rayTracerRS) = LoadComputeShader(rayTracerFileStream);

        auto rationaliserPath = std::filesystem::current_path() / "Output" / "Shaders" / "Rationaliser.dxil";
        std::fstream rationaliserFileStream{ rationaliserPath, std::ios::in | std::ios::binary };

        std::tie(m_rationaliserPSO, m_rationaliserRS) = LoadComputeShader(rationaliserFileStream);

        LoadNurbs();

        m_basisFunctionsResource = AllocateStructuredBuffer(
            m_basisFunctions, 
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_RESOURCE_FLAG_NONE,
            D3D12_RESOURCE_STATE_COPY_SOURCE | D3D12_RESOURCE_STATE_GENERIC_READ);
        m_derivativesResource = AllocateStructuredBuffer(
            m_derivatives,
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_RESOURCE_FLAG_NONE,
            D3D12_RESOURCE_STATE_COPY_SOURCE | D3D12_RESOURCE_STATE_GENERIC_READ);
        m_indicesResource = AllocateStructuredBuffer(
            m_indices, 
            D3D12_HEAP_TYPE_UPLOAD, 
            D3D12_RESOURCE_FLAG_NONE, 
            D3D12_RESOURCE_STATE_COPY_SOURCE | D3D12_RESOURCE_STATE_GENERIC_READ);
        m_controlPointsResource = AllocateStructuredBuffer(
            m_controlPoints,
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_RESOURCE_FLAG_NONE,
            D3D12_RESOURCE_STATE_COPY_SOURCE | D3D12_RESOURCE_STATE_GENERIC_READ);

        m_surfacePatches.resize(m_basisFunctions.size());

        m_surfacePatchesResource = AllocateStructuredBuffer(
            m_surfacePatches, 
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        m_surfacePatchesResource->SetName(L"Surface Patches Resource");

        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 1;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(m_rationaliserDescriptorHeap.ReleaseAndGetAddressOf())));

        CD3DX12_CPU_DESCRIPTOR_HANDLE descriptorHandle(m_rationaliserDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        CreateUAV(m_surfacePatchesResource, m_surfacePatches, descriptorHandle);

        m_outputTexture = AllocateWritibleTexture(
            1024,
            768,
            DXGI_FORMAT_B8G8R8A8_UNORM
        );

        D3D12_DESCRIPTOR_HEAP_DESC outputTextureHeapDesc = {};
        outputTextureHeapDesc.NumDescriptors = 1;
        outputTextureHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        outputTextureHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(GetDevice()->CreateDescriptorHeap(
            &outputTextureHeapDesc, IID_PPV_ARGS(m_rayTracerDescriptorHeap.ReleaseAndGetAddressOf())));

        CD3DX12_CPU_DESCRIPTOR_HANDLE outputDescriptorHandle(m_rayTracerDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        CreateOutputTextureUAV(m_outputTexture, DXGI_FORMAT_B8G8R8A8_UNORM, outputDescriptorHandle);

        // Pre-computation
        auto commandList = CreateDirectCommandList();

        commandList->SetPipelineState(m_rationaliserPSO.Get());
        commandList->SetComputeRootSignature(m_rationaliserRS.Get());
        commandList->SetComputeRoot32BitConstants(0, sizeof(m_rationaliserData) / sizeof(UINT), &m_rationaliserData, 0);
        commandList->SetComputeRootShaderResourceView(1, m_basisFunctionsResource->GetGPUVirtualAddress());
        commandList->SetComputeRootShaderResourceView(2, m_derivativesResource->GetGPUVirtualAddress());
        commandList->SetComputeRootShaderResourceView(3, m_indicesResource->GetGPUVirtualAddress());
        commandList->SetComputeRootShaderResourceView(4, m_controlPointsResource->GetGPUVirtualAddress());
        commandList->SetDescriptorHeaps(1, m_rationaliserDescriptorHeap.GetAddressOf());
        commandList->SetComputeRootDescriptorTable(5, m_rationaliserDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

        commandList->Dispatch(1, 1, 1);

        auto barrierToUA = CD3DX12_RESOURCE_BARRIER::Transition(
            m_surfacePatchesResource.Get(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS
        );

        commandList->ResourceBarrier(1, &barrierToUA);

        auto task = commandList.GetTask();

        commandList->Close();
        PushCommandList(std::move(commandList));

        co_await task;

        m_cameraData.outputSize = DirectX::XMUINT2(1024, 768);
        auto projectionMatrix = DirectX::XMMatrixPerspectiveFovRH(
            DirectX::XMConvertToRadians(70), 
            4.0f / 3.0f, 
            0.01f, 
            1000);

        m_iprojectionMatrix = XMMatrixInverse(nullptr, projectionMatrix);


        m_nurbsTracingConfiguration.errorThreshold = 0.0000001f;
        m_nurbsTracingConfiguration.maxIteration = 5;
        m_nurbsTracingConfiguration.seed = 3541;
        m_nurbsTracingConfiguration.patchesCount = m_rationaliserData.patchesCount;
        m_sphericalCoordinates = DirectX::XMVECTOR{ 0.1, 0.1, 0, 0 };
        MoveView();
    }

    auto MyRenderable::Render(float time) -> void
    {
        DirectX::XMFLOAT3 upAxis{ 0, 0, 1 };

        auto viewMatrix = DirectX::XMMatrixLookAtRH(
            XMLoadFloat3(&m_eyeLocation),
            m_lookAt,
            DirectX::XMLoadFloat3(&upAxis));

        auto rayTransform = XMMatrixMultiplyTranspose(viewMatrix, m_iprojectionMatrix);
        XMStoreFloat4x4(&m_cameraData.iProjMatrix, rayTransform);

        m_cameraData.origin = DirectX::XMFLOAT4{ m_eyeLocation.x, m_eyeLocation.y, m_eyeLocation.z, 1 };


        auto commandList = CreateDirectCommandList();

        // Transition the render target into the correct state to allow for drawing into it.
        const D3D12_RESOURCE_BARRIER barrierToCopyDest = CD3DX12_RESOURCE_BARRIER::Transition(
            GetCurrentRTV().Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->ResourceBarrier(1, &barrierToCopyDest);

        auto rtvHandle = GetRTVHandle();
        auto dsvHandle = GetDSVHandle();


        const D3D12_RESOURCE_BARRIER barrierCopySourceToUA = CD3DX12_RESOURCE_BARRIER::Transition(
            m_outputTexture.Get(),
            D3D12_RESOURCE_STATE_COPY_SOURCE, 
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        commandList->ResourceBarrier(1, &barrierCopySourceToUA);

        commandList->SetPipelineState(m_rayTracerPSO.Get());
        commandList->SetComputeRootSignature(m_rayTracerRS.Get());
        commandList->SetComputeRoot32BitConstants(
            0,
            sizeof(m_cameraData) / sizeof(UINT),
            &m_cameraData,
            0);
        commandList->SetComputeRoot32BitConstants(
            1,
            sizeof(m_nurbsTracingConfiguration) / sizeof(UINT), 
            &m_nurbsTracingConfiguration,
            0);
        commandList->SetComputeRootShaderResourceView(2, m_surfacePatchesResource->GetGPUVirtualAddress());
        commandList->SetDescriptorHeaps(1, m_rayTracerDescriptorHeap.GetAddressOf());
        commandList->SetComputeRootDescriptorTable(3, m_rayTracerDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        commandList->Dispatch(128, 96, 1);


        const D3D12_RESOURCE_BARRIER barrierUAToCopySource = CD3DX12_RESOURCE_BARRIER::Transition(
            m_outputTexture.Get(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        commandList->ResourceBarrier(1, &barrierUAToCopySource);

        commandList->CopyResource(GetCurrentRTV().Get(), m_outputTexture.Get());

        commandList->OMSetRenderTargets(
            1,
            &rtvHandle,
            FALSE,
            &dsvHandle);

        //auto rgba = DirectX::Colors::CornflowerBlue;
        //commandList->ClearRenderTargetView(
        //    rtvHandle,
        //    rgba,
        //    0,
        //    nullptr);
        //commandList->ClearDepthStencilView(
        //    dsvHandle,
        //    D3D12_CLEAR_FLAG_DEPTH,
        //    1.0f,
        //    0,
        //    0,
        //    nullptr);

        // Set the viewport and scissor rect.
        const D3D12_VIEWPORT viewport = {
            0.0f,
            0.0f,
            static_cast<float>(GetOutputWidth()),
            static_cast<float>(GetOutputHeight()),
            D3D12_MIN_DEPTH,
            D3D12_MAX_DEPTH
        };
        const D3D12_RECT scissorRect = {
            0,
            0,
            GetOutputWidth(),
            GetOutputHeight()
        };
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);

        // Transition the render target to the state that allows it to be presented to the display.
        const D3D12_RESOURCE_BARRIER barrierToPresent = CD3DX12_RESOURCE_BARRIER::Transition(
            GetCurrentRTV().Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
        commandList->ResourceBarrier(1, &barrierToPresent);
        commandList->Close();
        PushCommandList(std::move(commandList));
    }

    void MyRenderable::OnMouseMove(int x, int y)
    {
        if (m_mouseX != -1 && m_mouseY != -1)
        {
            float deltaX = x - m_mouseX;
            float deltaY = y - m_mouseY;

            DirectX::XMVECTOR delta{ deltaX, -deltaY, 0, 0 };

            delta = DirectX::XMVectorScale(delta, 0.001);

            if (m_mouseLeftDown)
            {
                m_sphericalCoordinates = DirectX::XMVectorAdd(m_sphericalCoordinates, delta);
                MoveView();
            }
            else if (m_mouseRightDown)
            {
                auto origin = XMLoadFloat3(&m_eyeLocation);

                origin = DirectX::XMVectorAdd(origin, delta);
                XMStoreFloat3(&m_eyeLocation, origin);
            }

        }
        m_mouseX = x;
        m_mouseY = y;
    }

    auto MyRenderable::OnMouseDown(Engine::Graphics::MouseButton button) -> void
    {
        if (button == Engine::Graphics::MouseButton::Left)
        {
            m_mouseLeftDown = true;
        }
        else
        {
            m_mouseRightDown = true;
        }
    }

    void MyRenderable::OnMouseUp(Engine::Graphics::MouseButton button)
    {
        if (button == Engine::Graphics::MouseButton::Left)
        {
            m_mouseLeftDown = false;
        }
        else
        {
            m_mouseRightDown = false;
        }

        m_mouseX = -1;
        m_mouseY = -1;
    }

    auto MyRenderable::MoveView() noexcept -> void
    {
        constexpr float r = 7.0f;

        float theta = DirectX::XMVectorGetY(m_sphericalCoordinates);
        float phi = DirectX::XMVectorGetX(m_sphericalCoordinates);

        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);

        m_eyeLocation = DirectX::XMFLOAT3{
                r * sinTheta * cosPhi,
                r * sinTheta * sinPhi,
                r * cosTheta
        };

        auto log = std::format(L"eye location at: [{}, {}, {}]\n",
            m_eyeLocation.x,
            m_eyeLocation.y,
            m_eyeLocation.z);
        OutputDebugStringW(log.c_str());
    }

    auto MyRenderable::LoadNurbs() -> void
    {
        m_controlPoints = Memory::FastHeapVector<DirectX::XMFLOAT4>
        {
            {
                {-0.93291452f, -0.93430338f, 0.14326487f, 1.0f},
                {-0.68768425f, -0.97456006f, 0.30977371f, 1.0f},
                {-0.41209381f, -0.92453847f, 0.32241873f, 1.0f},
                {-0.0864766f, -0.96633274f, 0.44503924f, 1.0f},
                {0.20201994f, -0.95380414f, 0.41399979f, 1.0f},
                {0.43186679f, -0.92170206f, 0.36094711f, 1.0f},
                {0.72297477f, -0.95352048f, 0.24821446f, 1.0f},
                {1.04450366f, -0.93186066f, 0.13898588f, 1.0f},

                {-0.92644383f, -0.7002879f, 0.29730489f, 1.0f},
                {-0.68073838f, -0.61854084f, 0.36402818f, 1.0f},
                {-0.41684517f, -0.68445711f, 0.56137545f, 1.0f},
                {-0.08436259f, -0.6555478f, 0.61118509f, 1.0f},
                {0.1798359f, -0.68918318f, 0.65185885f, 1.0f},
                {0.51233759f, -0.64787096f, 0.59706573f, 1.0f},
                {0.79844759f, -0.70484298f, 0.43804917f, 1.0f},
                {1.08881932f, -0.67544605f, 0.28947723f, 1.0f},

                {-0.93477197f, -0.39194272f, 0.33645722f, 1.0f},
                {-0.62016354f, -0.36137967f, 0.53672039f, 1.0f},
                {-0.36714725f, -0.3476956f, 0.69999225f, 1.0f},
                {-0.13858675f, -0.358004f, 0.84261268f, 1.0f},
                {0.15023435f, -0.36050667f, 0.8594896f, 1.0f},
                {0.44546691f, -0.34132279f, 0.7039371f, 1.0f},
                {0.77368522f, -0.34638587f, 0.51653783f, 1.0f},
                {1.03072222f, -0.39234082f, 0.38027884f, 1.0f},

                {-0.93454494f, -0.08673622f, 0.37742731f, 1.0f},
                {-0.69034872f, -0.10284034f, 0.68543352f, 1.0f},
                {-0.38823846f, -0.06705597f, 0.83909516f, 1.0f},
                {-0.09738017f, -0.08731263f, 1.03509977f, 1.0f},
                {0.15767586f, -0.12909964f, 1.01156471f, 1.0f},
                {0.4501853f, -0.10825462f, 0.85848629f, 1.0f},
                {0.71817041f, -0.13277442f, 0.64492184f, 1.0f},
                {1.04990956f, -0.06272838f, 0.3988145f, 1.0f},

                {-0.9006392f, 0.14410562f, 0.39964478f, 1.0f},
                {-0.66168363f, 0.17033357f, 0.64099629f, 1.0f},
                {-0.33687257f, 0.19113046f, 0.8987504f, 1.0f},
                {-0.07494838f, 0.1670956f, 0.99977181f, 1.0f},
                {0.15910702f, 0.15941868f, 0.99194508f, 1.0f},
                {0.4581425f, 0.19124508f, 0.81975132f, 1.0f},
                {0.79061466f, 0.22361131f, 0.59876834f, 1.0f},
                {1.07353537f, 0.23722724f, 0.3884255f, 1.0f},

                {-0.99084662f, 0.48866894f, 0.31137011f, 1.0f},
                {-0.71102244f, 0.49004658f, 0.59453765f, 1.0f},
                {-0.33619677f, 0.43512469f, 0.69957478f, 1.0f},
                {-0.10605608f, 0.47622879f, 0.90978684f, 1.0f},
                {0.16415563f, 0.44056081f, 0.83828485f, 1.0f},
                {0.4903296f, 0.4770386f, 0.75079742f, 1.0f},
                {0.73492056f, 0.50170628f, 0.52761081f, 1.0f},
                {1.04187412f, 0.44008164f, 0.33899424f, 1.0f},

                {-0.9584278f, 0.78125743f, 0.29688123f, 1.0f},
                {-0.6923073f, 0.80653583f, 0.41451641f, 1.0f},
                {-0.42553136f, 0.71759269f, 0.52133049f, 1.0f},
                {-0.07316131f, 0.77468592f, 0.64405221f, 1.0f},
                {0.21554181f, 0.79147267f, 0.65174102f, 1.0f},
                {0.48912608f, 0.72872011f, 0.55785635f, 1.0f},
                {0.77361853f, 0.75510084f, 0.38295214f, 1.0f},
                {1.06123459f, 0.79792089f, 0.27573957f, 1.0f},

                {-0.98982972f, 1.04789363f, 0.21430235f, 1.0f},
                {-0.62402395f, 1.04616068f, 0.3090965f, 1.0f},
                {-0.42247229f, 1.03794002f, 0.34079746f, 1.0f},
                {-0.13032245f, 1.02932545f, 0.37435833f, 1.0f},
                {0.20658805f, 1.02340641f, 0.42468165f, 1.0f},
                {0.47149695f, 1.04693984f, 0.37188286f, 1.0f},
                {0.79917367f, 1.09614281f, 0.29983599f, 1.0f},
                {1.08046113f, 1.04320828f, 0.18316995f, 1.0f}
            },
            &m_resourceHeap
        };

        m_rationaliserData.controlPointsStride = 8;

        m_U = { 0, 0, 0, 0, 1, 2, 3, 4, 5, 5, 5, 5 };
        m_V = { 0, 0, 0, 0, 1, 2, 3, 4, 5, 5, 5, 5 };
        
        m_surfacePatches = Engine::Nurbs::GetNurbsSurfacePatches<Memory::FastLocalAllocator>(
            std::mdspan(m_controlPoints.data(), 8, 8), 
            m_U, 
            m_V, 
            &m_resourceHeap);

        std::tie(m_basisFunctions, m_derivatives, m_indices) = GetNurbsSurfaceFunctions(
            m_U,
            m_V,
            Engine::Core::Memory::FastLocalAllocator<int>{&m_resourceHeap}
        );
        m_rationaliserData.patchesCount = static_cast<UINT>(m_basisFunctions.size());
    }
}
