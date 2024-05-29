module;
#include <DirectXMath.h>
export module Engine.Nurbs.BasisFunction;
import std;

namespace Engine::Nurbs
{
    export using CurveBasisFunctions = std::array<DirectX::XMMATRIX, 4>;
    export using SurfaceBasisFunctions = std::array<std::array<DirectX::XMMATRIX, 4>, 4>;
    export using SurfaceBasisFunctionsDerivatives = std::pair<SurfaceBasisFunctions, SurfaceBasisFunctions>;

    export auto GetBasisFunctions(int k, std::span<float> knots) noexcept -> CurveBasisFunctions;
}
