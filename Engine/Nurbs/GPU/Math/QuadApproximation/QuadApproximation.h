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
                float3 position = plane.Project(aabbIntersection);

                float3 p0 = (float3)positions[0];
                float3 p1 = (float3)positions[1];
                float3 p2 = (float3)positions[2];

                // Vectors for bilinear interpolation
                float3 v0 = p1 - p0;
                float3 v1 = p2 - p0;
                float3 v2 = position - p0;

                // Calculate dot products
                float d00 = dot(v0, v0);
                float d01 = dot(v0, v1);
                float d11 = dot(v1, v1);
                float d20 = dot(v2, v0);
                float d21 = dot(v2, v1);

                // Calculate denominators
                float denom = d00 * d11 - d01 * d01;

                // Calculate barycentric coordinates
                float v = (d11 * d20 - d01 * d21) / denom;
                float w = (d00 * d21 - d01 * d20) / denom;
                float u = 1.0f - v - w;

                float2 uv0 = uv[0];
                float2 uv1 = uv[1];
                float2 uv2 = uv[2];
                float2 uv3 = uv[3];

                float2 initialGuess = uv0 * u + uv1 * v + uv2 * w + uv3 * (1.0f - u - v - w);
                return initialGuess;
            }
        };
    }
}

