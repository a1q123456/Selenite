#include "Engine/Nurbs/GPU/Constants/CameraData.h"
#include "Engine/Nurbs/GPU/Constants/NurbsTracingConfiguration.h"
#include "Engine/Nurbs/GPU/Constants/RayTracerConstants.h"

#include "Engine/Nurbs/GPU/Ray/Ray.h"

[RootSignature(NurbsRaytracerRS)]
[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    uint3 coordinate = tid.xyz;

    float2 ndc = float2(coordinate.xy) / float2(cameraData.outputSize);
    float3 direction = mul(cameraData.iProjMatrix, float4(ndc.xy, 0.0, 1.0)).xyz;
    renderTarget[coordinate.xy] = float4(0, 0, 0, 0);

    direction /= length(direction);

    Engine::NurbsRayTracer::Ray ray = Engine::NurbsRayTracer::Ray::MakeRay(cameraData.origin.xyz,
        direction);

    Engine::NurbsRayTracer::IntersectingPlanesRay intersectingRay = ray.DefineRayAsPlaneIntersection();

    for (int i = 0; i < nurbsTracingConfiguration.patchesCount; i++)
    {
        float2 uv = float2(0, 0);
        float3 position = float3(0, 0, 0);
        float3 normal = float3(0, 0, 0);

        //if (coordinate.x <= 10 && coordinate.y <= 10)
        {
            if (intersectingRay.TraceRay(
                    nurbsPatches[i], 
                    nurbsTracingConfiguration.errorThreshold, 
                    nurbsTracingConfiguration.maxIteration, 
                    uv, position, normal))
            {
                // TODO coloring, texturing, etc.
                renderTarget[coordinate.xy] = float4(uv.xy / 25.0f, 1, 1);
            }
        }
    }

}

