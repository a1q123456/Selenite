#pragma once
#include "Engine/Nurbs/GPU/Math/Plane/Plane.h"
#include "Engine/Nurbs/GPU/Math/Ray/Ray.h"


namespace Engine
{
    namespace Math
    {
        struct IntersectingPlanesRay
        {
            static IntersectingPlanesRay FromRay(Ray ray)
            {
                Plane p1;
                Plane p2;
                float3 absD = abs(ray.D);
                if (absD.x > absD.y && absD.x > absD.z)
                {
                    p1.normal = float3(ray.D.y, -ray.D.x, 0);
                }
                else
                {
                    p1.normal = float3(0, ray.D.z, -ray.D.y);
                }

                p1.normal /= length(p1.normal);
                p2.normal = cross(p1.normal, ray.D);

                p1.offset = -dot(p1.normal, ray.O);
                p2.offset = -dot(p2.normal, ray.O);

                IntersectingPlanesRay result;

                result.plane1 = p1;
                result.plane2 = p2;
                result.baseRay = ray;
                return result;
            }

            Plane plane1;
            Plane plane2;

            Ray baseRay;
        };
    }
}
