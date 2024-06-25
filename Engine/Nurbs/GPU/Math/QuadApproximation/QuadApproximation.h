#pragma once
#include "Engine/Nurbs/GPU/Math/Plane/Plane.h"
#include "Engine/Nurbs/GPU/Math/Ray/Ray.h"

namespace Engine
{
    namespace Math
    {
        struct QuadApproximation
        {
            float4 positions[3];
            Plane plane;

            float2 uv[4];

            void Initialise()
            {
                plane.Initialise();
                for (int i = 0; i < 4; i++)
                {
                    uv[i] = float2(0, 0);
                    positions[i] = float4(0, 0, 0, 0);
                }
            }

            float2 GetInitialGuess(float3 aabbIntersection)
            {
                //return lerp(uv[0], uv[3], 0.5);
                float3 p = plane.Project(aabbIntersection);

                float3 a = (float3)positions[0];
                float3 b = (float3)positions[1];
                float3 c = (float3)positions[2];

                float3 pa = p - a;

                float x = dot(pa, normalize(b - a));
                float y = dot(pa, normalize(c - a));

                //return coordinates;
                return float2(lerp(uv[0].x, uv[1].x, x), lerp(uv[0].y, uv[2].y, y));
            }
        };
    }
}

