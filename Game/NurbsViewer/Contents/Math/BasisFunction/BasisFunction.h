#pragma once
#include "Game/NurbsViewer/Contents/Math/Polynomial/Polynomial.h"

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
