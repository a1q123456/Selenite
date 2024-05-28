#pragma once
#include "Engine/Nurbs/GPU/Math/Polynomial/Polynomial.h"

namespace Engine
{
    namespace Math
    {
        struct Polynomial3D
        {
            Polynomial x;
            Polynomial y;
            Polynomial z;

            float3 EvaluatePolynomial(float u, float v)
            {
                return float3(
                    x.EvaluatePolynomial(u, v),
                    y.EvaluatePolynomial(u, v),
                    z.EvaluatePolynomial(u, v)
                );
            }
        };
    }
}
