module;
#include <DirectXMath.h>
export module Engine.Nurbs.RationalFunction;
import Engine.Nurbs.Polynomial;
import std;

namespace Engine::Nurbs
{
    export struct RationalFunction
    {
        DirectX::XMMATRIX numeratorX;
        DirectX::XMMATRIX numeratorY;
        DirectX::XMMATRIX numeratorZ;

        DirectX::XMMATRIX denominator;
        bool isRational;

        auto Differentiate() const noexcept -> std::pair<RationalFunction, RationalFunction>
        {
            RationalFunction U{};
            RationalFunction V{};
            std::tie(U.numeratorX, V.numeratorX) = DifferentiatePolynomial(numeratorX);
            std::tie(U.numeratorY, V.numeratorY) = DifferentiatePolynomial(numeratorY);
            std::tie(U.numeratorZ, V.numeratorZ) = DifferentiatePolynomial(numeratorZ);

            if (isRational)
            {
                std::tie(U.denominator, V.denominator) = DifferentiatePolynomial(denominator);
                U.isRational = true;
                V.isRational = true;
            }

            return { U, V };

        }
    };
}

