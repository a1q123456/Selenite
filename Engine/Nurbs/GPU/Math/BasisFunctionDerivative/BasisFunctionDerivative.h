#pragma once
#include "Engine/Nurbs/GPU/Math/Polynomial/Polynomial.h"

namespace Engine
{
    namespace Math
    {
        struct BasisFunctionDerivative
        {
            Polynomial derivativeU[4][4];
            Polynomial derivativeV[4][4];
        };
    }
}
