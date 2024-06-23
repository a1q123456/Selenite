module;
#include <DirectXMath.h>
export module Engine.Nurbs.NurbsCalculations;
import Engine.Nurbs.Polynomial;
import Engine.Nurbs.RationalFunction;
import Engine.Nurbs.BasisFunction;
import Engine.Nurbs.SurfacePatch;
import std;


namespace Engine::Nurbs
{
    export template <template<typename> typename TAllocator>
        using SurfaceBasisFunctionsArray = std::vector<SurfaceBasisFunctions, TAllocator<SurfaceBasisFunctions>>;

    export template <template<typename> typename TAllocator>
        using SurfaceBasisDerivativesArray = std::vector<SurfaceBasisFunctionsDerivatives, TAllocator<SurfaceBasisFunctionsDerivatives>>;

    export template <template<typename> typename TAllocator>
    using SurfacePatchesArray = std::vector<SurfacePatch, TAllocator<SurfacePatch>>;

    export template <template<typename> typename TAllocator>
        using SurfacePatchIndicesArray = std::vector<SurfacePatchIndex, TAllocator<SurfacePatchIndex>>;

    export using SurfaceBasisFunctionsView = std::span<SurfaceBasisFunctions>;
    export using SurfaceBasisDerivativesView = std::span<SurfaceBasisFunctions>;
    export using SurfacePatchIndicesView = std::span<SurfacePatchIndex>;

    export using KnotVector = std::span<float>;

    export using ControlPointsView = std::mdspan<DirectX::XMFLOAT4, std::extents<std::size_t, std::dynamic_extent, std::dynamic_extent>>;
    export using SurfacePatchesView = std::span<SurfacePatch>;


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

    export template <template<typename> typename TAllocator = std::allocator>
        auto GetNurbsSurfacePatches(
            ControlPointsView P,
            std::span<float> U,
            std::span<float> V,
            TAllocator<SurfacePatch> allocator
        ) noexcept -> SurfacePatchesArray<TAllocator>
    {
        constexpr int degree = 3;
        SurfacePatchesArray<TAllocator> result{ allocator };

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

                DirectX::XMMATRIX denominator{};
                DirectX::XMMATRIX polynomialX{};
                DirectX::XMMATRIX polynomialY{};
                DirectX::XMMATRIX polynomialZ{};

                bool isRational = false;
                std::optional<float> previousW{};
                for (int i = u - degree; i <= u; i++)
                {
                    DirectX::XMFLOAT4X4 NuMatrix{};

                    auto nUIndex = i - (u - degree);
                    XMStoreFloat4x4(&NuMatrix, Nu[Nu.size() - nUIndex - 1]);

                    for (int j = v - degree; j <= v; j++)
                    {
                        auto nVIndex = j - (v - degree);

                        DirectX::XMMATRIX weighted{};
                        DirectX::XMFLOAT4X4 NvMatrix{};
                        XMStoreFloat4x4(&NvMatrix, XMMatrixTranspose(Nv[Nv.size() - nVIndex - 1]));

                        auto N = PolynomialMultiplication(NuMatrix, NvMatrix);
                        auto NMatrix = XMLoadFloat4x4(&N);

                        auto Puv = P[std::array{ i, j }];

                        auto w = Puv.w;
                        weighted = w * NMatrix;
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

                        denominator += weighted;
                    }
                }

                RationalFunction nurbsFunction
                {
                    polynomialX,
                    polynomialY,
                    polynomialZ,
                    denominator,
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

    export template <template<typename> typename TAllocator = std::allocator>
    auto GetNurbsSurfaceFunctions(
            KnotVector U,
            KnotVector V,
            TAllocator<int> allocator
        ) noexcept -> std::tuple<
                                SurfaceBasisFunctionsArray<TAllocator>,
                                SurfaceBasisDerivativesArray<TAllocator>,
                                SurfacePatchIndicesArray<TAllocator>
                      >
    
    {
        constexpr int degree = 3;
        using SurfaceBasisFunctionsAllocator = typename std::allocator_traits<TAllocator<int>>::template rebind_alloc<SurfaceBasisFunctions>;
        using SurfaceBasisFunctionsDerivativesAllocator = typename std::allocator_traits<TAllocator<int>>::template rebind_alloc<SurfaceBasisFunctionsDerivatives>;
        using SurfacePatchIndexAllocator = typename std::allocator_traits<TAllocator<int>>::template rebind_alloc<SurfacePatchIndex>;

        SurfaceBasisFunctionsArray<TAllocator> basisFunctions{ static_cast<SurfaceBasisFunctionsAllocator>(allocator) };
        SurfaceBasisDerivativesArray<TAllocator> derivatives{ static_cast<SurfaceBasisFunctionsDerivativesAllocator>(allocator) };
        SurfacePatchIndicesArray<TAllocator> indices{ static_cast<SurfacePatchIndexAllocator>(allocator) };

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

                SurfaceBasisFunctions patchBasisFunctions{};
                SurfaceBasisFunctionsDerivatives patchDerivatives{};
                for (int i = u - degree; i <= u; i++)
                {
                    DirectX::XMFLOAT4X4 NuMatrix{};

                    auto nUIndex = i - (u - degree);
                    XMStoreFloat4x4(&NuMatrix, Nu[Nu.size() - nUIndex - 1]);

                    for (int j = v - degree; j <= v; j++)
                    {
                        auto nVIndex = j - (v - degree);

                        DirectX::XMFLOAT4X4 NvMatrix{};
                        XMStoreFloat4x4(&NvMatrix, XMMatrixTranspose(Nv[Nv.size() - nVIndex - 1]));

                        auto N = PolynomialMultiplication(NuMatrix, NvMatrix);
                        auto NMatrix = XMLoadFloat4x4(&N);

                        patchBasisFunctions[nUIndex][nVIndex] = NMatrix;

                        auto& uDerivative = patchDerivatives.first[nUIndex][nVIndex];
                        auto& vDerivative = patchDerivatives.second[nUIndex][nVIndex];

                        std::tie(uDerivative, vDerivative) = DifferentiatePolynomial(NMatrix);
                    }
                }

                DirectX::XMFLOAT2 minUV{ U[u], V[v] };
                DirectX::XMFLOAT2 maxUV{ U[nextU], V[nextV] };
                DirectX::XMUINT2 index{ static_cast<std::uint32_t>(u), static_cast<std::uint32_t>(v) };
                SurfacePatchIndex patchIndex{ index, minUV, maxUV};

                basisFunctions.emplace_back(patchBasisFunctions);
                derivatives.emplace_back(patchDerivatives);
                indices.emplace_back(patchIndex);


                v = nextV;
                nextV = GetNextKnot(V, v);
            }
            u = nextU;
            nextU = GetNextKnot(U, u);
        }

        return { basisFunctions, derivatives, indices };
    }
}


