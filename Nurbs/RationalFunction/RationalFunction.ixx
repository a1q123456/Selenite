module;
#include <DirectXMath.h>
export module Engine.Nurbs.RationalFunction;


namespace Engine::Nurbs
{
    struct RationalFunction
    {
        DirectX::XMMATRIX numeratorX;
        DirectX::XMMATRIX numeratorY;
        DirectX::XMMATRIX numeratorZ;

        DirectX::XMMATRIX denominator;
        bool isRational;

        // TODO: Rational function derivative
    };
}

