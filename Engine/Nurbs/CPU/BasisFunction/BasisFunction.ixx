module;
#include <DirectXMath.h>
export module Engine.Nurbs.BasisFunction;
import std;

namespace Engine::Nurbs
{
    export auto GetBasisFunctions(int k, std::span<float> knots) noexcept -> std::array<DirectX::XMMATRIX, 4>;
}
