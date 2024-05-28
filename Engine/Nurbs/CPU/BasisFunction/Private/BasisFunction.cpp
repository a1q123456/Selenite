module;
#include <DirectXMath.h>
module Engine.Nurbs.BasisFunction;
import Engine.Nurbs.Polynomial;


namespace Engine::Nurbs
{
    auto GetBasisFunctions(int k, std::span<float> knots) noexcept -> std::array<DirectX::XMMATRIX, 4>
    {
        constexpr int degree = 3;

        std::array<DirectX::XMMATRIX, degree + 1> N{};
        N[0] = DirectX::XMMATRIX{
            1, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0
        };
        for (int i = 0; i <= degree; i++)
        {
            for (int j = i - 1; j >= 0; j--)
            {
                auto b = (knots[k + i - j] - knots[k - j]);
                DirectX::XMFLOAT4X4 A{};
                if (b != 0)
                {
                    A = DirectX::XMFLOAT4X4(
                        -knots[k - j] / b, 0, 0, 0,
                        1 / b, 0, 0, 0,
                        0, 0, 0, 0,
                        0, 0, 0, 0);
                }

                DirectX::XMFLOAT4X4 nj{};
                XMStoreFloat4x4(&nj, N[j]);

                auto tmp = PolynomialMultiplication(nj, A);
                auto tmpMatrix = XMLoadFloat4x4(&tmp);
                N[j + 1] += N[j] - tmpMatrix;
                N[j] = tmpMatrix;
            }
        }
        return N;
    }
}

