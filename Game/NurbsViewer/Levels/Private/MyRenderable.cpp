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

    void MyRenderable::Initialise()
    {
        auto rayTracerPath = std::filesystem::current_path() / "Output" / "Shaders" / "RayTracer.dxil";
        std::fstream rayTracerFileStream{ rayTracerPath, std::ios::in | std::ios::binary };

        std::tie(m_rayTracerPSO, m_rayTracerRS) = LoadComputeShader(rayTracerFileStream);

        auto rationaliserPath = std::filesystem::current_path() / "Output" / "Shaders" / "Rationaliser.dxil";
        std::fstream rationaliserFileStream{ rationaliserPath, std::ios::in | std::ios::binary };

        std::tie(m_rationaliserPSO, m_rationaliserRS) = LoadComputeShader(rationaliserFileStream);

        LoadNurbs();
    }

    auto MyRenderable::Render(float time) -> void
    {
        auto commandList = CreateDirectCommandList();

        // Transition the render target into the correct state to allow for drawing into it.
        const D3D12_RESOURCE_BARRIER barrierToRT = CD3DX12_RESOURCE_BARRIER::Transition(
            GetCurrentRTV().Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &barrierToRT);
        //commandList->SetPipelineState(m_pipelineState.Get());

        auto rtvHandle = GetRTVHandle();
        auto dsvHandle = GetDSVHandle();

        commandList->OMSetRenderTargets(
            1,
            &rtvHandle,
            FALSE,
            &dsvHandle);

        auto rgba = DirectX::Colors::CornflowerBlue;
        commandList->ClearRenderTargetView(
            rtvHandle,
            rgba,
            0,
            nullptr);
        commandList->ClearDepthStencilView(
            dsvHandle,
            D3D12_CLEAR_FLAG_DEPTH,
            1.0f,
            0,
            0,
            nullptr);

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
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandList->ResourceBarrier(1, &barrierToPresent);
        commandList->Close();
        PushCommandList(std::move(commandList));
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
    }
}
