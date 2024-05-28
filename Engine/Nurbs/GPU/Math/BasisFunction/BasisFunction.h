#pragma once
#include "Engine/Nurbs/GPU/Math/Polynomial/Polynomial.h"

namespace Engine
{
    namespace Math
    {
        struct BasisFunction
        {
            Polynomial basisFunctions[4][4];
        };
    }
}
