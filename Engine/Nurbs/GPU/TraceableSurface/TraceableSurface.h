#pragma once
#include "Engine/Nurbs/GPU/Math/NurbsPatch/NurbsPatch.h"
#include "Engine/Nurbs/GPU/AABB/AABB.h"
#include "Engine/Nurbs/GPU/Math/Ray/IntersectingPlanesRay.h"
#include "Engine/Nurbs/GPU/Math/QuadApproximation/QuadApproximation.h"

float2 RandomVector(uint seed)
{
    uint value = seed * 1103515245 + 12345;
    uint x = (value / 65536) % 32768;
    value = value * 1103515245 + 12345;
    uint y = (value / 65536) % 32768;
    return float2(x, y) / 32768.0;
}

namespace Engine
{
    namespace NurbsRayTracer
    {
        struct TraceableSurface
        {
            Math::NurbsPatch nurbsPatch;
            AABB boundingBox;
            Math::QuadApproximation approximation;

            void Initialise()
            {
                nurbsPatch.Initialise();
                boundingBox.Initialise();
                approximation.Initialise();
            }

            static float2 DistanceToRoot(in Math::IntersectingPlanesRay ray, float3 position)
            {
                return float2(
                    dot(ray.plane1.normal, position) + ray.plane1.offset,
                    dot(ray.plane2.normal, position) + ray.plane2.offset
                );
            }

            static float2x2 InverseJacobianMatrix(
                in Math::IntersectingPlanesRay ray,
                uint seed,
                Math::RationalFunction3D S,
                Math::RationalFunction3D S_u,
                Math::RationalFunction3D S_v,
                inout float2 currentGuess,
                out float3 suVal, out float3 svVal)
            {
                float det = 0;
                float2x2 J;
                for (int i = 0; i < 2; i++)
                {
                    S.EvaluateFirstDerivative(
                        S_u,
                        S_v,
                        currentGuess.x,
                        currentGuess.y,
                        suVal, svVal);

                    J = float2x2(
                        dot(ray.plane1.normal, suVal), dot(ray.plane2.normal, suVal),
                        dot(ray.plane1.normal, svVal), dot(ray.plane2.normal, svVal)
                    );
                    det = determinant(J);
                    if (det == 0)
                    {
                        currentGuess += RandomVector(seed);
                    }
                    else
                    {
                        break;
                    }
                }

                return float2x2(
                    J._m11, -J._m10,
                    -J._m01, J._m00) / det;
            }

            static bool TraceAABB(
                in Math::IntersectingPlanesRay ray,
                in TraceableSurface surface, 
                out float t)
            {
                return surface.boundingBox.TryIntersect(ray.baseRay, t);
            }

            static bool TraceRay(
                in Math::IntersectingPlanesRay ray,
                in TraceableSurface surface,
                uint seed,
                float errorTolerance, int maxIteration, 
                out float2 uv, out float3 position, out float3 normal, inout float t)
            {
                float3 aabbIntersection = ray.baseRay.D * t + ray.baseRay.O;

                float2 currentGuess = surface.approximation.GetInitialGuess(aabbIntersection);
                float3 s = surface.nurbsPatch.nurbsFunction.Evaluate(currentGuess.x, currentGuess.y);

                float3 su = float3(0, 0, 0);
                float3 sv = float3(0, 0, 0);
                float2 distance = DistanceToRoot(ray, s);
                float error = -1;

                for (int i = 0; i <= maxIteration; i++)
                {
                    if (i == maxIteration)
                    {
                        return false;
                    }
                    float2x2 J = InverseJacobianMatrix(
                        ray,
                        seed,
                        surface.nurbsPatch.nurbsFunction,
                        surface.nurbsPatch.partialDerivativeU,
                        surface.nurbsPatch.partialDerivativeV,
                        currentGuess,
                        su, sv);

                    currentGuess = currentGuess - mul(J, distance);
                    s = surface.nurbsPatch.nurbsFunction.Evaluate(currentGuess.x, currentGuess.y);
                    distance = DistanceToRoot(ray, s);
                    float newError = dot(distance, distance);

                    if (newError > error && error > 0)
                    {
                        return false;
                    }
                    error = newError;

                    if (error <= errorTolerance)
                    {
                        break;
                    }
                }

                if (any(currentGuess > surface.approximation.uv[3]) || any(currentGuess < surface.approximation.uv[0]))
                {
                    return false;
                }

                t = dot(s - ray.baseRay.O, ray.baseRay.D);
                if (t < 0)
                {
                    return false;
                }

                uv = currentGuess;
                position = s;
                normal = cross(su, sv);
                normal /= length(normal);
                return true;
            }

        };
    }
}
