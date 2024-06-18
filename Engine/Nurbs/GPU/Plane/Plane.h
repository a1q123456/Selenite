#pragma once

namespace Engine
{
    namespace NurbsRayTracer
    {
        struct Plane
        {
            float3 normal;
            float offset;

            // a*x + b*y + c*z + d = 0
            static Plane FromCoefficients(float a, float b, float c, float d)
            {
                Plane result;
                result.normal = float3(a, b, c);
                result.offset = d;
                return result;
            }
        };
    }
}
