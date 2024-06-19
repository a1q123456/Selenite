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

            float3 Evaluate(float u, float v)
            {
                return float3(
                    x.Evaluate(u, v),
                    y.Evaluate(u, v),
                    z.Evaluate(u, v)
                );
            }

            void Initialise()
            {
                x.Initialise();
                y.Initialise();
                z.Initialise();
            }
        };
    }
}
