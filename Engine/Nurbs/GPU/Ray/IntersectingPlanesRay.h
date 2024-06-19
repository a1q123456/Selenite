#pragma once
#include "Engine/Nurbs/GPU/Math/NurbsPatch/NurbsPatch.h"
#include "Engine/Nurbs/GPU/Math/RationalFunction3D/RationalFunction3D.h"
#include "Engine/Nurbs/GPU/Constants/RayTracerConstants.h"
#include "Engine/Nurbs/GPU/Plane/Plane.h"
#include "Engine/Nurbs/GPU/Ray/Ray.h"

float2 RandomVector()
{
    uint value = nurbsTracingConfiguration.seed * 1103515245 + 12345;
    uint x = (value / 65536) % 32768;
    value = value * 1103515245 + 12345;
    uint y = (value / 65536) % 32768;
    return float2(x, y) / 32768.0;
}

namespace Engine
{
    namespace NurbsRayTracer
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
                result.O = ray.O;
                result.D = ray.D;
                return result;
            }

            float2 DistanceToRoot(float3 position)
            {
                return float2(
                    dot(plane1.normal, position) + plane1.offset,
                    dot(plane2.normal, position) + plane2.offset
                );
            }

            float2x2 InverseJacobianMatrix(
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
                        dot(plane1.normal, suVal), dot(plane2.normal, suVal),
                        dot(plane1.normal, svVal), dot(plane2.normal, svVal)
                    );
                    det = determinant(J);
                    if (det == 0)
                    {
                        currentGuess += RandomVector();
                    }
                    else
                    {
                        break;
                    }
                }

                return float2x2(
                    J._m11, -J._m10,
                    -J._m01 , J._m00) / det;
            }

            bool TraceRay(Math::NurbsPatch nurbsPatch, float errorTolerance, int maxIteration, out float2 uv, out float3 position, out float3 normal, out float t)
            {
                t = -1;
                float2 currentGuess = lerp(nurbsPatch.maxUV, nurbsPatch.minUV, 0.5);
                float3 s = nurbsPatch.nurbsFunction.Evaluate(currentGuess.x, currentGuess.y);

                float3 su = float3(0, 0, 0);
                float3 sv = float3(0, 0, 0);
                float2 distance = DistanceToRoot(s);
                float error = -1;

                for (int i = 0; i <= maxIteration; i++)
                {
                    if (i == maxIteration)
                    {
                        return false;
                    }
                    float2x2 J = InverseJacobianMatrix(
                        nurbsPatch.nurbsFunction,
                        nurbsPatch.partialDerivativeU, 
                        nurbsPatch.partialDerivativeV, 
                        currentGuess, 
                        su, sv);

                    currentGuess = currentGuess - mul(J, distance);
                    s = nurbsPatch.nurbsFunction.Evaluate(currentGuess.x, currentGuess.y);
                    distance = DistanceToRoot(s);
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

                if (any(currentGuess > nurbsPatch.maxUV) || any(currentGuess < nurbsPatch.minUV))
                {
                    return false;
                }

                t = dot(s - O, D);
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

            Plane plane1;
            Plane plane2;
            float3 O;
            float3 D;
        };
    }
}
