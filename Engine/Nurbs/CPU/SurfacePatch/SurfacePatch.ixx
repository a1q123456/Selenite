module;
#include <DirectXMath.h>
export module Engine.Nurbs.SurfacePatch;
import Engine.Nurbs.RationalFunction;
import Engine.Nurbs.BasisFunction;
import Engine.Nurbs.Polynomial;
import std;

namespace Engine::Nurbs
{
    export struct SurfacePatch
    {
        RationalFunction nurbsFunction;
        RationalFunction partialDerivativeU;
        RationalFunction partialDerivativeV;

        DirectX::XMFLOAT2 minUV;
        DirectX::XMFLOAT2 maxUV;
    };

    export struct SurfacePatchIndex
    {
        DirectX::XMUINT2 uvIndex;
        DirectX::XMFLOAT2 minUV;
        DirectX::XMFLOAT2 maxUV;
    };
}

