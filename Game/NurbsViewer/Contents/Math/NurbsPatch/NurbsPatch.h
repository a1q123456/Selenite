#pragma once
#include "Game/NurbsViewer/Contents/Math/RationalFunction3D/RationalFunction3D.h"

namespace Engine
{
    namespace Math
    {
        struct NurbsPatch
        {
            RationalFunction3D nurbsFunction;
            RationalFunction3D partialDerivativeU;
            RationalFunction3D partialDerivativeV;

            float2 initialGuess;
        };
    }

}
