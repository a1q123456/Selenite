#pragma once

namespace Engine
{
    namespace Math
    {
        struct Plane
        {
            float3 normal;
            float offset;

            void Initialise()
            {
                normal = float3(0, 0, 0);
                offset = 0;
            }

            float3 Project(float3 p)
            {
                // Calculate the distance from the point to the plane
                float distance = dot(normal, p) + offset;

                // Subtract the distance along the normal from the point
                return p - distance * normal;
            }

            static Plane FromPoints(float3 a, float3 b, float3 c)
            {
                Plane result;

                // Calculate the vectors from point a to point b and from point a to point c
                float3 ab = b - a;
                float3 ac = c - a;

                // Calculate the normal using the cross product of ab and ac
                result.normal = normalize(cross(ab, ac));

                // Calculate the offset using the plane equation: d = - (a*x0 + b*y0 + c*z0)
                result.offset = -dot(result.normal, a);

                return result;
            }

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
