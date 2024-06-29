#pragma once
#include "Engine/Nurbs/GPU/Math/Polynomial3D/Polynomial3D.h"

namespace Engine
{
    namespace Math
    {
        struct RationalFunction3D
        {
            Polynomial3D numerator;
            Polynomial denominator;
            bool isRational;
            bool3 unused;

            float3 Evaluate(float u, float v)
            {
                float3 num;
                float den;
                EvaluateRational(u, v, num, den);
                return num / den;
            }

            void EvaluateRational(float u, float v, out float3 num, out float den)
            {
                num = numerator.Evaluate(u, v);
                den = 1;
                if (isRational)
                {
                    den = denominator.Evaluate(u, v);
                }
            }

            // Apply the quotient rule and evaluate the first derivative
            void EvaluateFirstDerivative(
                in RationalFunction3D P_u, 
                in RationalFunction3D P_v, 
                float u, float v,
                out float3 R_u, out float3 R_v
                )
            {
                R_u = float3(0, 0, 0);
                R_v = float3(0, 0, 0);
                if (!isRational)
                {
                    R_u = P_u.Evaluate(u, v);
                    R_v = P_v.Evaluate(u, v);
                    return;
                }
                float3 fVal, fVal_u, fVal_v;
                float gVal, gVal_u, gVal_v;

                // Evaluate f, g, f_u, f_v, g_u, g_v at(u, v)
                EvaluateRational(u, v, fVal, gVal);
                P_u.EvaluateRational(u, v, fVal_u, gVal_u);
                P_v.EvaluateRational(u, v, fVal_v, gVal_v);

                // Calculate the first derivatives using the quotient rule
                float gVal2 = gVal * gVal;

                R_u = (fVal_u * gVal - fVal * gVal_u) / gVal2;
                R_v = (fVal_v * gVal - fVal * gVal_v) / gVal2;
            }

            // Apply the quotient rule and evaluate the second derivative
            void EvaluateSecondDerivative(
                in RationalFunction3D P_u,
                in RationalFunction3D P_v,
                in RationalFunction3D P_uu,
                in RationalFunction3D P_uv,
                in RationalFunction3D P_vv,
                float u, float v,
                out float3 R_uu,
                out float3 R_vv,
                out float3 R_uv
                )
            {
                if (!isRational)
                {
                    R_uu = P_uu.Evaluate(u, v);
                    R_uv = P_uv.Evaluate(u, v);
                    R_vv = P_vv.Evaluate(u, v);
                    return;
                }

                float3 fVal, fVal_u, fVal_v, fVal_uu, fVal_uv, fVal_vv;
                float gVal, gVal_u, gVal_v, gVal_uu, gVal_uv, gVal_vv;

                // Evaluate all required derivatives at (u, v)
                EvaluateRational(u, v, fVal, gVal);
                P_u.EvaluateRational(u, v, fVal_u, gVal_u);
                P_v.EvaluateRational(u, v, fVal_v, gVal_v);

                P_uu.EvaluateRational(u, v, fVal_uu, gVal_uu);
                P_uv.EvaluateRational(u, v, fVal_uv, gVal_uv);
                P_vv.EvaluateRational(u, v, fVal_vv, gVal_vv);

                float gVal2 = gVal * gVal;
                float gVal3 = gVal2 * gVal;

                float3 uSubVal = fVal_u * gVal - fVal * gVal_u;
                float3 vSubVal = fVal_v * gVal - fVal * gVal_v;

                // Compute second derivatives
                R_uu = (fVal_uu * gVal - 2 * fVal_u * gVal_u + fVal * gVal_uu) * gVal2 - uSubVal * uSubVal;
                R_uu /= gVal3;

                R_uv = (fVal_uv * gVal - fVal_u * gVal_v - fVal_v * gVal_u + fVal * gVal_uv) * gVal2 - uSubVal * vSubVal;
                R_uv /= gVal3;

                R_vv = (fVal_vv * gVal - 2 * fVal_v * gVal_v + fVal * gVal_vv) * gVal2 - vSubVal * vSubVal;
                R_vv /= gVal3;
            }

            void Initialise()
            {
                numerator.Initialise();
                denominator.Initialise();
                isRational = false;
            }
        };
    }

}