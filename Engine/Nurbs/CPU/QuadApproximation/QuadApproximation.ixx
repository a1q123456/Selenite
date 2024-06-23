module;
#include <DirectXMath.h>
export module Engine.Nurbs.QuadApproximation;
import Engine.Nurbs.Plane;
import std;

namespace Engine::Nurbs
{
    export struct QuadApproximation
    {
        DirectX::XMFLOAT4 positions[3];
        Plane plane;

        DirectX::XMFLOAT2 uv[4];
    };
}

