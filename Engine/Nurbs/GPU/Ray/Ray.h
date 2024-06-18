#pragma once
#include "Engine/Nurbs/GPU/AABB/AABB.h"

namespace Engine
{
    namespace NurbsRayTracer
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

            bool IntersectsAABB(AABB boundingBox)
            {
                return false;
            }

            float3 O;
            float3 D;
        };
    }
}
