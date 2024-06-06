#include "Engine/Nurbs/GPU/Constants/CameraData.h"
#include "Engine/Nurbs/GPU/Constants/NurbsTracingConfiguration.h"
#include "Engine/Nurbs/GPU/Constants/RayTracerConstants.h"

#include "Engine/Nurbs/GPU/Ray/Ray.h"

[RootSignature(NurbsRaytracerRS)]
[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    uint2 coordinate = tid.xy;

    //if (coordinate.x > 700 || coordinate.y > 500 || coordinate.x < 400 || coordinate.y < 200)
    //{
    //    return;
    //}

    float2 ndc = float2(coordinate) / float2(cameraData.outputSize) - float2(0.5, 0.5);
    ndc *= float2(1, -1);
    float3 direction = mul(cameraData.iProjMatrix, float4(ndc.xy, 0.0, 0.01)).xyz;
    renderTarget[coordinate] = float4(0, 0, 0, 0);

    direction /= length(direction);

    Engine::NurbsRayTracer::Ray ray = Engine::NurbsRayTracer::Ray::MakeRay(cameraData.origin.xyz,
        direction);

    Engine::NurbsRayTracer::IntersectingPlanesRay intersectingRay = ray.DefineRayAsPlaneIntersection();

    float minDistance = -1;
    for (int i = 0; i < nurbsTracingConfiguration.patchesCount; i++)
    {
        float2 uv = float2(0, 0);
        float3 position = float3(0, 0, 0);
        float3 normal = float3(0, 0, 0);
        float distance = 0;

        if (intersectingRay.TraceRay(
                nurbsPatches[i], 
                nurbsTracingConfiguration.errorThreshold, 
                nurbsTracingConfiguration.maxIteration, 
                uv, position, normal, distance))
        {
            if (minDistance > 0 && distance >= minDistance)
            {
                continue;
            }
            minDistance = distance;
            // TODO coloring, texturing, etc.
            renderTarget[coordinate] = float4(normal, 1);
        }
    }

}

