#pragma once
#include "Engine/Nurbs/GPU/Ray/Ray.h"

namespace Engine
{
    namespace NurbsRayTracer
    {
        struct AABB
        {
            float2 aUV;
            float2 bUV;
            float2 cUV;
            float2 dUV;

            float4 vertices[4];

            uint minIndex;
            uint maxIndex;
            uint nurbsPatchIndex;
            uint pad;

            float4 GetMin()
            {
                return vertices[minIndex];
            }

            float4 GetMax()
            {
                return vertices[maxIndex];
            }

            float4 GetNurbsInitialGuess(Ray ray)
            {
                return float4(0, 0, 0, 0);
                // TODO
                // 1. Calculate the intersection
                // 2. Project the point onto the plane
                // 3. Use bi-linear interpolation to obtain the initial guess
            }
        };
    }
}

