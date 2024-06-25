#pragma once
#include "Engine/Nurbs/GPU/Math/Ray/Ray.h"

namespace Engine
{
    namespace NurbsRayTracer
    {
        struct AABB
        {
            float4 minPosition;
            float4 maxPosition;

            void Initialise()
            {
                minPosition = float4(0, 0, 0, 0);
                maxPosition = float4(0, 0, 0, 0);
            }

            bool TryIntersect(in Math::Ray ray, out float t)
            {
                t = -1;
                float3 tMin = ((float3)minPosition - ray.O) / ray.D;
                float3 tMax = ((float3)maxPosition - ray.O) / ray.D;

                float tmin = max(max(min(tMin.x, tMax.x), min(tMin.y, tMax.y)), min(tMin.z, tMax.z));
                float tmax = min(min(max(tMin.x, tMax.x), max(tMin.y, tMax.y)), max(tMin.z, tMax.z));

                if (tmax < 0)
                {
                    return false;
                }

                if (tmin > tmax)
                {
                    return false;
                }

                if (tmin < 0)
                {
                    t = tmax;
                }
                else
                {
                    t = tmin;
                }
                return true;
            }
        };
    }
}

