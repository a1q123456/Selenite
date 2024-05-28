#include "Engine/Nurbs/GPU/Constants/CameraData.h"
#include "Engine/Nurbs/GPU/Constants/NurbsTracingConfiguration.h"
#include "Engine/Nurbs/GPU/Constants/RayTracerConstants.h"

#include "Engine/Nurbs/GPU/Ray/Ray.h"

[RootSignature(NurbsRaytracerRS)]
[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID, uint3 groupID: SV_GroupID)
{
    uint3 groupSize = uint3(8, 8, 1);
    uint3 coordinate = groupSize * groupID + tid;

    float2 ndc = float2(coordinate.xy) / float2(cameraData.outputSize.xy);
    float3 direction = mul(cameraData.iProjMatrix, float4(ndc.xy, 0.0, 1.0)).xyz;

    direction /= length(direction);

    Engine::NurbsRayTracer::Ray ray = Engine::NurbsRayTracer::Ray::MakeRay(cameraData.origin.xyz, direction);

    Engine::NurbsRayTracer::IntersectingPlanesRay intersectingRay = ray.DefineRayAsPlaneIntersection();
    uint numberOfStructs = 0;
    uint stride = 0;
    nurbsPatches.GetDimensions(numberOfStructs, stride);

    for (int i = 0; i < numberOfStructs; i++)
    {
        float2 uv;
        float3 position;
        float3 normal;
        if (intersectingRay.TraceRay(nurbsPatches[i], nurbsTracingConfiguration.errorThreshold, nurbsTracingConfiguration.maxIteration, uv, position, normal))
        {
            // TODO coloring, texturing, etc.
            renderTarget[coordinate.xy] = normal;
        }
    }

}

