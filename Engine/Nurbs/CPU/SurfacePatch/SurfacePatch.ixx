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

    int GetNextKnot(std::span<float> U, int k = -1)
    {
        constexpr int degree = 3;
        k += 1;
        if (k > U.size() - 1 - degree)
        {
            return -1;
        }
        while (U[k] == U[k + 1] || k < degree)
        {
            k += 1;
            if (k >= U.size() - 1)
            {
                return k;
            }
        }

        return k;
    }

    export using ControlPointsArray = std::mdspan<DirectX::XMFLOAT4, std::extents<std::size_t, std::dynamic_extent, std::dynamic_extent>>;

    export template <template<typename> typename TAllocator = std::allocator>
    auto GetNurbsSurfacePatches(
        ControlPointsArray P,
        std::span<float> U,
        std::span<float> V,
        TAllocator<SurfacePatch> allocator
    ) noexcept -> std::vector<SurfacePatch, TAllocator<SurfacePatch>>
    {
        constexpr int degree = 3;
        std::vector<SurfacePatch, TAllocator<SurfacePatch>> result{ allocator };

        auto u = GetNextKnot(U);
        auto nextU = GetNextKnot(U, u);

        while (nextU != -1)
        {
            auto v = GetNextKnot(V);
            auto nextV = GetNextKnot(V, v);
            auto Nu = GetBasisFunctions(u, U);
            while (nextV != -1)
            {
                auto Nv = GetBasisFunctions(v, V);

                DirectX::XMMATRIX weighted{};
                DirectX::XMMATRIX polynomialX{};
                DirectX::XMMATRIX polynomialY{};
                DirectX::XMMATRIX polynomialZ{};

                bool isRational = false;
                std::optional<float> previousW = 0;
                for (int i = u - degree; i <= u; i++)
                {
                    for (int j = v - degree; j <= v; j++)
                    {
                        DirectX::XMFLOAT4X4 NuMatrix{};
                        DirectX::XMFLOAT4X4 NvMatrix{};

                        XMStoreFloat4x4(&NuMatrix, Nu[degree - i]);
                        XMStoreFloat4x4(&NvMatrix, Nv[degree - j]);

                        auto N = PolynomialMultiplication(NuMatrix, NvMatrix);
                        auto NMatrix = XMLoadFloat4x4(&N);

                        auto Puv = P[std::array{ i, j }];

                        auto w = Puv.w;
                        weighted += w * NMatrix;
                        if (!previousW.has_value())
                        {
                            previousW = w;
                        }
                        else if (previousW != w)
                        {
                            isRational = true;
                        }

                        polynomialX += Puv.x * weighted;
                        polynomialY += Puv.y * weighted;
                        polynomialZ += Puv.z * weighted;
                    }
                }

                RationalFunction nurbsFunction
                {
                    polynomialX,
                    polynomialY,
                    polynomialZ,
                    weighted,
                    isRational
                };
                DirectX::XMFLOAT2 minUV{ U[u], V[v] };
                DirectX::XMFLOAT2 maxUV{ U[nextU], V[nextV] };

                auto [derivativeU, derivativeV] = nurbsFunction.Differentiate();

                result.emplace_back(nurbsFunction, derivativeU, derivativeV, minUV, maxUV);

                v = nextV;
                nextV = GetNextKnot(V, v);
            }
            u = nextU;
            nextU = GetNextKnot(U, u);
        }

        return result;
    }
}

