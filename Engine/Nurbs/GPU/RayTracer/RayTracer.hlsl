#include "Engine/Nurbs/GPU/Constants/CameraData.h"
#include "Engine/Nurbs/GPU/Constants/NurbsTracingConfiguration.h"
#include "Engine/Nurbs/GPU/Constants/RayTracerConstants.h"

#include "Engine/Nurbs/GPU/Math/Ray/Ray.h"
#include "Engine/Nurbs/GPU/Math/Ray/IntersectingPlanesRay.h"

[RootSignature(NurbsRaytracerRS)]
[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    uint2 coordinate = tid.xy;

    float2 ndc = float2(coordinate) / float2(cameraData.outputSize) - float2(0.5, 0.5);
    ndc *= float2(1, -1);
    float3 direction = mul(cameraData.iProjMatrix, float4(ndc.xy, 0.0, 0.01)).xyz;

    direction /= length(direction);

    Engine::Math::Ray ray = Engine::Math::Ray::MakeRay(cameraData.origin.xyz,
        direction);

    Engine::Math::IntersectingPlanesRay intersectingRay = Engine::Math::IntersectingPlanesRay::FromRay(ray);

    uint patchIndex = -1;
    float minDistance = -1;
    float4 finalColour = float4(0, 0, 0, 0);
    for (uint i = 0; i < nurbsTracingConfiguration.patchesCount; i++)
    {
        float2 uv = float2(0, 0);
        float3 position = float3(0, 0, 0);
        float3 normal = float3(0, 0, 0);
        float distance = 0;

        if (Engine::NurbsRayTracer::TraceableSurface::TraceAABB(
            intersectingRay,
            traceablePatches[i],
            distance))
        {
            //renderTarget[coordinate] = float4(1, 1, 1, 1);
            //return;
            if (Engine::NurbsRayTracer::TraceableSurface::TraceRay(
                intersectingRay,
                traceablePatches[i],
                nurbsTracingConfiguration.seed,
                nurbsTracingConfiguration.errorThreshold,
                nurbsTracingConfiguration.maxIteration,
                uv, position, normal, distance))
            {
                if (minDistance != -1 && distance >= minDistance)
                {
                    continue;
                }
                minDistance = distance;
                // TODO coloring, texturing, etc.
                finalColour = float4(uv / 5.0f, 1, 1);
            }
        }
    }

    renderTarget[coordinate] = finalColour;
}

