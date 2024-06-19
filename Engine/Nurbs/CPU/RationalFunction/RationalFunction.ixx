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
        DirectX::XMINT3 pad;

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

        auto Evaluate(float u, float v) const noexcept -> DirectX::XMVECTOR
        {
            using namespace DirectX;
            auto [num, den] = EvaluateRational(u, v);

            return num / den;
        }

        auto EvaluateRational(float u, float v) const noexcept -> std::pair<DirectX::XMVECTOR, float>
        {
            auto x = EvaluatePolynomial(u, v, numeratorX);
            auto y = EvaluatePolynomial(u, v, numeratorY);
            auto z = EvaluatePolynomial(u, v, numeratorZ);

            DirectX::XMVECTOR num{ x, y, z };
            float den = 1.0f;
            if (isRational)
            {
                den = EvaluatePolynomial(u, v, denominator);
            }

            return {num, den};
        }

        // Apply the quotient rule and evaluate the first derivative
        auto EvaluateFirstDerivative(const RationalFunction& P_u, const RationalFunction& P_v, float u, float v)
            const noexcept -> std::pair<DirectX::XMVECTOR, DirectX::XMVECTOR>
        {
            using namespace DirectX;
            // Evaluate f, g, f_u, f_v, g_u, g_v at(u, v)
            auto [fVal, gVal] = EvaluateRational(u, v);
            auto [fVal_u, gVal_u] = P_u.EvaluateRational(u, v);
            auto [fVal_v, gVal_v] = P_v.EvaluateRational(u, v);

            // Calculate the first derivatives using the quotient rule
            auto gVal2 = gVal * gVal;

            auto R_u = (fVal_u * gVal - fVal * gVal_u) / gVal2;
            auto R_v = (fVal_v * gVal - fVal * gVal_v) / gVal2;

            return { R_u, R_v };
        }

        // Apply the quotient rule and evaluate the second derivative
        auto EvaluateSecondDerivative(
            const RationalFunction& P_u, 
            const RationalFunction& P_v, 
            const RationalFunction& P_uu, 
            const RationalFunction& P_uv, 
            const RationalFunction& P_vv, 
            float u, float v) const noexcept -> std::tuple<DirectX::XMVECTOR, DirectX::XMVECTOR, DirectX::XMVECTOR>
        {
            using namespace DirectX;
            // Evaluate all required derivatives at (u, v)
            auto [fVal, gVal] = EvaluateRational(u, v);

            auto [fVal_u, gVal_u] = P_u.EvaluateRational(u, v);
            auto [fVal_v, gVal_v] = P_v.EvaluateRational(u, v);

            auto [fVal_uu, gVal_uu] = P_uu.EvaluateRational(u, v);
            auto [fVal_uv, gVal_uv] = P_uv.EvaluateRational(u, v);
            auto [fVal_vv, gVal_vv] = P_vv.EvaluateRational(u, v);

            auto gVal2 = gVal * gVal;
            auto gVal3 = gVal2 * gVal;

            auto uSubVal = fVal_u * gVal - fVal * gVal_u;
            auto vSubVal = fVal_v * gVal - fVal * gVal_v;

            // Compute second derivatives
            auto R_uu = (fVal_uu * gVal - 2 * fVal_u * gVal_u + fVal * gVal_uu) * gVal2 - uSubVal * uSubVal;
            R_uu /= gVal3;
            
            auto R_uv = (fVal_uv * gVal - fVal_u * gVal_v - fVal_v * gVal_u + fVal * gVal_uv) * gVal2 - uSubVal * vSubVal;
            R_uv /= gVal3;
            
            auto R_vv = (fVal_vv * gVal - 2 * fVal_v * gVal_v + fVal * gVal_vv) * gVal2 - vSubVal * vSubVal;
            R_vv /= gVal3;

            return { R_uu, R_uv, R_vv };
        }
    };
}

