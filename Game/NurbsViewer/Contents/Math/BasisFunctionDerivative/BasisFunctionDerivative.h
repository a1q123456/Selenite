#pragma once
#include "Game/NurbsViewer/Contents/Math/Polynomial/Polynomial.h"

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
