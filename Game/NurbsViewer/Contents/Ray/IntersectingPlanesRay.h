#pragma once
#include "Game/NurbsViewer/Contents/Math/NurbsPatch/NurbsPatch.h"
#include "Game/NurbsViewer/Contents/Math/RationalFunction3D/RationalFunction3D.h"
#include "Game/NurbsViewer/Contents/Constants/RayTracerConstants.h"
#include "Game/NurbsViewer/Contents/Plane/Plane.h"

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
            float2 DistanceToRoot(float3 position)
            {
                return float2(
                    dot(plane1.normal, position) + plane1.offset,
                    dot(plane2.normal, position) + plane2.offset
                );
            }

            static float PickElement(float2x2 J, int row, int column)
            {
                int otherRow = 1 - row;
                int otherColumn = 1 - column;
                return J[otherRow][otherColumn];
            }

            float2x2 AdjugateMatrix(float2x2 J)
            {
                float2x2 cofactors = float2x2(0, 0, 0, 0);

                for (int i = 0; i < 2; i++)
                {
                    for (int j = 0; j < 2; j++)
                    {
                        float minor = PickElement(J, i, j);

                        bool sign = (i + j) % 2 == 0;

                        minor = sign ? minor : -minor;
                        cofactors[i][j] = minor;
                    }
                }

                return transpose(cofactors);
            }

            float2x2 InverseJacobianMatrix(
                Math::RationalFunction3D Su,
                Math::RationalFunction3D Sv,
                inout float2 currentGuess,
                out float3 suVal, out float3 svVal)
            {
                float det = 0;
                float2x2 J;
                for (int i = 0; i < 2; i++)
                {
                    suVal = Su.EvaluateRationalFunction(currentGuess.x, currentGuess.y);
                    svVal = Sv.EvaluateRationalFunction(currentGuess.x, currentGuess.y);

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

                float invDet = 1 / det;
                return invDet * AdjugateMatrix(J);
            }

            bool TraceRay(Math::NurbsPatch nurbsPatch, float errorTolerance, int maxIteration, out float2 uv, out float3 position, out float3 normal)

            {
                float2 currentGuess = nurbsPatch.initialGuess;
                float3 Suv = nurbsPatch.nurbsFunction.EvaluateRationalFunction(currentGuess.x, currentGuess.y);

                float3 su;
                float3 sv;
                float2 distance = DistanceToRoot(Suv);
                float error = -1;

                for (int i = 0; i <= maxIteration; i++)
                {
                    if (i == maxIteration)
                    {
                        return false;
                    }
                    float2x2 J = InverseJacobianMatrix(nurbsPatch.partialDerivativeU, nurbsPatch.partialDerivativeV, currentGuess, su, sv);

                    currentGuess = currentGuess - mul(J, distance);
                    Suv = nurbsPatch.nurbsFunction.EvaluateRationalFunction(currentGuess.x, currentGuess.y);
                    distance = DistanceToRoot(Suv);
                    float newError = dot(distance, distance);

                    if (newError > error)
                    {
                        return false;
                    }

                    if (error <= errorTolerance)
                    {
                        break;
                    }
                }

                uv = currentGuess;
                position = Suv;
                normal = cross(su, sv);
                normal /= length(normal);
                return true;
            }

            Plane plane1;
            Plane plane2;
        };
    }
}
