#pragma once
#include "IntersectingPlanesRay.h"
#include "Game/NurbsViewer/Contents/Plane/Plane.h"


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

            IntersectingPlanesRay DefineRayAsPlaneIntersection()
            {
                Plane p1;
                Plane p2;
                float3 absD = abs(D);
                if (absD.x > absD.y && absD.x > absD.z)
                {
                    p1.normal = float3(absD.y, -absD.x, 0);
                }
                else
                {
                    p1.normal = float3(0, absD.z, -absD.y);
                }

                p1.normal /= length(p1.normal);

                p2.normal = cross(p1.normal, D);
                p1.offset = dot(p1.normal, O);
                p2.offset = dot(p2.normal, O);

                IntersectingPlanesRay result;

                result.plane1 = p1;
                result.plane2 = p2;
                return result;
            }

            float3 O;
            float3 D;
        };
    }
}
