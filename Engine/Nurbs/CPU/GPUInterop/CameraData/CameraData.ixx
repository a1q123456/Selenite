module;
#include <DirectXMath.h>
export module Engine.Nurbs.GPUInterop.CameraData;
import std;

namespace Engine::Nurbs::GPUInterop
{
    export struct CameraData
    {
        DirectX::XMFLOAT4X4 iProjMatrix;
        DirectX::XMFLOAT4 origin;
        DirectX::XMUINT2 outputSize;
        DirectX::XMUINT2 pad;
    };
}
