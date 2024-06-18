module;
#include <DirectXMath.h>
module Engine.Nurbs.Polynomial;


namespace Engine::Nurbs
{
    auto PolynomialMultiplication(const DirectX::XMFLOAT4X4& A,
        const DirectX::XMFLOAT4X4& B) noexcept -> DirectX::XMFLOAT4X4
    {
        constexpr int degree = 3;

        DirectX::XMFLOAT4X4 result{};


        // Iterate over each element in the resultant matrix to calculate the polynomial coefficients
        for (int i = 0; i <= degree; i++)
        {
            for (int j = 0; j <= degree; j++)
            {
                // Calculate the coefficient for u^i * v^j in the result
                float sumCoeff = 0;
                for (int k = 0; k <= degree; k++)
                {
                    if (i - k >= 0)
                    {
                        for (int l = 0; l <= degree; l++)
                        {
                            if (j - l >= 0)
                            {
                                sumCoeff += A(k, l) * B(i - k, j - l);
                            }
                        }
                    }
                }
                result(i, j) = sumCoeff;
            }
        }

        return result;
    }

    auto DifferentiatePolynomial(const DirectX::XMMATRIX& A) noexcept
        -> std::pair<DirectX::XMMATRIX, DirectX::XMMATRIX>
    {
        constexpr int degree = 3;
        DirectX::XMMATRIX derivative_u{};
        DirectX::XMMATRIX derivative_v{};

        auto AT = XMMatrixTranspose(A);

        // Compute derivative with respect to u
        for (int i = 1; i <= degree; i++)
        {
            derivative_u.r[i - 1] = DirectX::XMVectorScale(A.r[i], static_cast<float>(i));
        }

        // Compute derivative with respect to v
        for (int j = 1; j <= degree; j++)
        {
            derivative_v.r[j - 1] = DirectX::XMVectorScale(AT.r[j], static_cast<float>(j));
        }

        derivative_v = XMMatrixTranspose(derivative_v);
        return { derivative_u, derivative_v };
    }

    auto EvaluatePolynomial(float u, float v, const DirectX::XMMATRIX& A) noexcept -> float
    {
        DirectX::XMFLOAT4X4 coefficients;
        XMStoreFloat4x4(&coefficients, A);

        const int degree = 3;
        float result = 0;

        float uToI = 1;
        for (int i = 0; i <= degree; i++)
        {
            float vToJ = 1;
            for (int j = 0; j <= degree; j++)
            {
                result += coefficients.m[j][i] * uToI * vToJ;
                vToJ *= v;
            }
            uToI *= u;
        }

        return result;
    }
}

