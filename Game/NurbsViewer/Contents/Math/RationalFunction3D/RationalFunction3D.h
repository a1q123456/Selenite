#pragma once
#include "Game/NurbsViewer/Contents/Math/Polynomial3D/Polynomial3D.h"

namespace Engine
{
    namespace Math
    {
        struct RationalFunction3D
        {
            Polynomial3D numerator;
            Polynomial denominator;
            bool isRational;

            float3 EvaluateRationalFunction(float u, float v)
            {
                float3 result = numerator.EvaluatePolynomial(u, v);
                if (isRational)
                {
                    result /= denominator.EvaluatePolynomial(u, v);
                }
                return result;
            }
        };
    }

}