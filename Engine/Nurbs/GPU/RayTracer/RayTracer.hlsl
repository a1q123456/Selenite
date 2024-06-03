#include "Engine/Nurbs/GPU/Constants/CameraData.h"
#include "Engine/Nurbs/GPU/Constants/NurbsTracingConfiguration.h"
#include "Engine/Nurbs/GPU/Constants/RayTracerConstants.h"

#include "Engine/Nurbs/GPU/Ray/Ray.h"

[RootSignature(NurbsRaytracerRS)]
[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    uint2 coordinate = tid.xy;

    float2 ndc = float2(coordinate) / float2(cameraData.outputSize) - float2(0.5, 0.5);
    float3 direction = mul(cameraData.iProjMatrix, float4(ndc.xy, 0.0, 0.01)).xyz;
    renderTarget[coordinate] = float4(0, 0, 0, 0);

    direction /= length(direction);
    //renderTarget[coordinate] = float4((direction + float3(1, 1, 1)) / 2, 1);

    //return;
    Engine::NurbsRayTracer::Ray ray = Engine::NurbsRayTracer::Ray::MakeRay(cameraData.origin.xyz,
        direction);

    Engine::NurbsRayTracer::IntersectingPlanesRay intersectingRay = ray.DefineRayAsPlaneIntersection();

    for (int i = 0; i < nurbsTracingConfiguration.patchesCount; i++)
    {
        float2 uv = float2(0, 0);
        float3 position = float3(0, 0, 0);
        float3 normal = float3(0, 0, 0);

        if (intersectingRay.TraceRay(
                nurbsPatches[i], 
                nurbsTracingConfiguration.errorThreshold, 
                nurbsTracingConfiguration.maxIteration, 
                uv, position, normal))
        {
            // TODO coloring, texturing, etc.
            renderTarget[coordinate] = float4(normal, 1);
        }
    }

}

