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

            float2 minUV;
            float2 maxUV;

            void Initialise()
            {
                nurbsFunction.Initialise();
                partialDerivativeU.Initialise();
                partialDerivativeV.Initialise();

                minUV = float2(0, 0);
                maxUV = float2(0, 0);
            }
        };
    }

}
