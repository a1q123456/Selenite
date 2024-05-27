module;
#include <DirectXMath.h>
export module Engine.Nurbs.Polynomial;
import std;

namespace Engine::Nurbs
{
    export auto PolynomialMultiplication(const DirectX::XMFLOAT4X4& A, const DirectX::XMFLOAT4X4& B) noexcept
        -> DirectX::XMFLOAT4X4;

    export auto DifferentiatePolynomial(const DirectX::XMMATRIX& A) noexcept
        -> std::pair<DirectX::XMMATRIX, DirectX::XMMATRIX>;
}
