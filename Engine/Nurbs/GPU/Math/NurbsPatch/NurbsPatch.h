#pragma once
#include "Engine/Nurbs/GPU/Math/RationalFunction3D/RationalFunction3D.h"

namespace Engine
{
    namespace Math
    {
        struct NurbsPatch
        {
            RationalFunction3D nurbsFunction;
            RationalFunction3D partialDerivativeU;
            RationalFunction3D partialDerivativeV;

            uint2 uvIndex;
            float2 unused;

            void Initialise()
            {
                nurbsFunction.Initialise();
                partialDerivativeU.Initialise();
                partialDerivativeV.Initialise();
            }
        };
    }

}
