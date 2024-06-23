#pragma once

namespace Engine
{
    namespace Math
    {
        struct Ray
        {
            static Ray MakeRay(float3 O, float3 D)
            {
                Ray r;
                r.O = O;
                r.D = D;

                return r;
            }

            float3 O;
            float3 D;
        };
    }
}
