module;
#include <DirectXMath.h>
export module Engine.Nurbs.Plane;

namespace Engine::Nurbs
{
    export struct Plane
    {
        DirectX::XMFLOAT3 normal;
        float offset;
    };
}
