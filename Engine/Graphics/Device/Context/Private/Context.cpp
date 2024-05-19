module;
#include "Engine/Graphics/Device/DirectX/DirectxHeaders.hpp"
module Engine.Graphics.Device.Context;
import Engine.Graphics.Device.D3DX12;
import Engine.Graphics.Device.Utils;
import Engine.Graphics.Device.CommandListPool;
import Engine.Graphics.Device.GPUScheduler;
import Engine.Graphics.Renderable;
import Engine.Core.IntPtr;

using namespace Microsoft::WRL;
namespace Engine::Graphics::Device
{
    auto Context::Initialise(IntPtr hwnd, int width, int height) -> void
    {
        m_featureLevel = D3D_FEATURE_LEVEL_12_0;
        m_window = hwnd;
        m_outputWidth = std::max(width, 1);
        m_outputHeight = std::max(height, 1);

        CreateDevice();
        m_commandListPool = std::make_unique<CommandListPool>();
        m_commandListPool->Initialise(this);
        m_gpuScheduler = std::make_unique<GPUScheduler>();
        m_gpuScheduler->Initialise(this);
        CreateResources();
        
    }
    
    auto Context::Teardown() -> void
    {
        m_gpuScheduler->Teardown();
        m_commandListPool->Teardown();
        m_rtvDescriptorHeap.Reset();
        m_dsvDescriptorHeap.Reset();
        m_swapChain.Reset();
        m_d3dDevice.Reset();
        m_dxgiFactory.Reset();
    }

    Context::~Context() noexcept
    {
    }

    auto Context::GetRTVHandle() const noexcept -> CD3DX12_CPU_DESCRIPTOR_HANDLE
    {
        auto cpuHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(
            cpuHandle, 
            static_cast<INT>(m_backBufferIndex), 
            m_rtvDescriptorSize);
    }

    auto Context::GetDSVHandle() const noexcept -> CD3DX12_CPU_DESCRIPTOR_HANDLE
    {
        auto cpuHandleDSV = m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuHandleDSV);
    }

    auto Context::CreateDevice() -> void
    {
        DWORD dxgiFactoryFlags = 0;

#if defined(_DEBUG)
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        //
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        {
            ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
            {
                debugController->EnableDebugLayer();
            }

            ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
            {
                dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

                dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
            }
        }
#endif

        ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf())));

        ComPtr<IDXGIAdapter1> adapter;
        GetAdapter(adapter.GetAddressOf());

        // Create the DX12 API device object.
        ThrowIfFailed(D3D12CreateDevice(
            adapter.Get(),
            m_featureLevel,
            IID_PPV_ARGS(m_d3dDevice.ReleaseAndGetAddressOf())
        ));

#ifndef NDEBUG
        // Configure debug device (if active).
        ComPtr<ID3D12InfoQueue> d3dInfoQueue;
        if (SUCCEEDED(m_d3dDevice.As(&d3dInfoQueue)))
        {
#ifdef _DEBUG
            d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
            D3D12_MESSAGE_ID hide[] =
            {
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
                // Workarounds for debug layer issues on hybrid-graphics systems
                D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
                D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
            };
            D3D12_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);
        }
#endif

        // Create descriptor heaps for render target views and depth stencil views.
        D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
        rtvDescriptorHeapDesc.NumDescriptors = SWAP_BUFFER_COUNT;
        rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

        D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
        dsvDescriptorHeapDesc.NumDescriptors = 1;
        dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

        ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(m_rtvDescriptorHeap.ReleaseAndGetAddressOf())));
        ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(m_dsvDescriptorHeap.ReleaseAndGetAddressOf())));

        m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // Check Shader Model 6 support
        D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
        if (FAILED(m_d3dDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
            || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
        {
#ifdef _DEBUG
            OutputDebugStringA("ERROR: Shader Model 6.0 is not supported!\n");
#endif
            throw std::runtime_error("Shader Model 6.0 is not supported!");
        }

        // TODO: Initialize device dependent objects here (independent of window size).
    }

    auto Context::GetAdapter(IDXGIAdapter1** ppAdapter) -> void
    {
        *ppAdapter = nullptr;

        ComPtr<IDXGIAdapter1> adapter;
        for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_dxgiFactory->EnumAdapters1(adapterIndex, adapter.ReleaseAndGetAddressOf()); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            ThrowIfFailed(adapter->GetDesc1(&desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                continue;
            }

            // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), m_featureLevel, __uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }

#if !defined(NDEBUG)
        if (!adapter)
        {
            if (FAILED(m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()))))
            {
                throw std::runtime_error("WARP12 not available. Enable the 'Graphics Tools' optional feature");
            }
        }
#endif

        if (!adapter)
        {
            throw std::runtime_error("No Direct3D 12 device found");
        }

        *ppAdapter = adapter.Detach();
    }

    auto Context::OnDeviceLost() -> void
    {
        Teardown();

        CreateDevice();
        m_commandListPool->Initialise(this);
        m_gpuScheduler->Initialise(this);
        CreateResources();
    }

    auto Context::PresentFrame() -> void
    {
        // The first argument instructs DXGI to block until VSync, putting the application
        // to sleep until the next VSync. This ensures we don't waste any cycles rendering
        // frames that will never be displayed to the screen.
        HRESULT hr = m_swapChain->Present(1, 0);
        m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

        // If the device was reset we must completely reinitialize the renderer.
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            OnDeviceLost();
        }
        else
        {
            ThrowIfFailed(hr);
        }
    }

    auto Context::CreateResources() -> void
    {
        constexpr DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
        constexpr DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;
        const UINT backBufferWidth = static_cast<UINT>(GetOutputWidth());
        const UINT backBufferHeight = static_cast<UINT>(GetOutputHeight());

        // If the swap chain already exists, resize it, otherwise create one.
        if (m_swapChain)
        {
            HRESULT hr = m_swapChain->ResizeBuffers(Context::SWAP_BUFFER_COUNT, backBufferWidth, backBufferHeight, backBufferFormat, 0);

            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                // If the device was removed for any reason, a new device and swap chain will need to be created.
                OnDeviceLost();

                // Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method
                // and correctly set up the new device.
                return;
            }
            ThrowIfFailed(hr);
        }
        else
        {
            // Create a descriptor for the swap chain.
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
            swapChainDesc.Width = backBufferWidth;
            swapChainDesc.Height = backBufferHeight;
            swapChainDesc.Format = backBufferFormat;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = Context::SWAP_BUFFER_COUNT;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
            fsSwapChainDesc.Windowed = TRUE;

            // Create a swap chain for the window.
            ComPtr<IDXGISwapChain1> swapChain;
            ThrowIfFailed(GetDXGIFactory()->CreateSwapChainForHwnd(
                GetCommandListPool()
                ->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT).Get(),
                GetWindow().As<HWND>(),
                &swapChainDesc,
                &fsSwapChainDesc,
                nullptr,
                swapChain.GetAddressOf()
            ));

            ThrowIfFailed(swapChain.As(&m_swapChain));

            // This template does not support exclusive fullscreen mode and prevents DXGI from responding to the ALT+ENTER shortcut
            ThrowIfFailed(GetDXGIFactory()->MakeWindowAssociation(GetWindow().As<HWND>(), DXGI_MWA_NO_ALT_ENTER));
        }

        // Obtain the back buffers for this window which will be the final render targets
        // and create render target views for each of them.
        for (UINT n = 0; n < SWAP_BUFFER_COUNT; n++)
        {
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(m_renderTargets[n].GetAddressOf())));

            auto name = std::format(L"Render target {}", n);
            m_renderTargets[n]->SetName(name.c_str());

            auto cpuHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

            const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(cpuHandle, static_cast<INT>(n), m_rtvDescriptorSize);
            m_d3dDevice->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvDescriptor);
        }

        // Reset the index to the current back buffer.
        m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

        // Allocate a 2-D surface as the depth/stencil buffer and create a depth/stencil view
        // on this surface.
        const CD3DX12_HEAP_PROPERTIES depthHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            depthBufferFormat,
            backBufferWidth,
            backBufferHeight,
            1, // This depth stencil view has only one texture.
            1  // Use a single mipmap level.
        );
        depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        const CD3DX12_CLEAR_VALUE depthOptimizedClearValue(depthBufferFormat, 1.0f, 0u);

        ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
            &depthHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_PPV_ARGS(m_depthStencil.ReleaseAndGetAddressOf())
        ));

        m_depthStencil->SetName(L"Depth stencil");

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = depthBufferFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

        auto cpuHandle = m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

        m_d3dDevice->CreateDepthStencilView(m_depthStencil.Get(), &dsvDesc, cpuHandle);
    }
}

