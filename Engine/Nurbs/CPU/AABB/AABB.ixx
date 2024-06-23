module;
#include <DirectXMath.h>
export module Engine.Nurbs.AABB;
import std;

namespace Engine::Nurbs
{
    export struct AABB
    {
        DirectX::XMFLOAT4 minPosition;
        DirectX::XMFLOAT4 maxPosition;
    };
}

