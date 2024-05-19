module;
#include "DirectXHeaders.hpp"

export module Engine.Graphics.Device.Utils;
import std;
namespace Engine::Graphics::Device
{
    export void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            // Set a breakpoint on this line to catch DirectX API errors
            throw std::exception();
        }
    }
}
