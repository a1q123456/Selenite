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

            float3 EvaluateRationalFunction(float u, float v)
            {
                float3 result = numerator.EvaluatePolynomial(u, v);
                if (isRational)
                {
                    result /= denominator.EvaluatePolynomial(u, v);
                }
                return result;
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