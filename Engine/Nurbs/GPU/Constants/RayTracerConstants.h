#pragma once
#include "Engine/Nurbs/GPU/Constants/CameraData.h"
#include "Engine/Nurbs/GPU/Constants/NurbsTracingConfiguration.h"
#include "Engine/Nurbs/GPU/TraceableSurface/TraceableSurface.h"

ConstantBuffer<Engine::Rendering::CameraData> cameraData : register(b0);
ConstantBuffer<Engine::Rendering::NurbsTracingConfiguration> nurbsTracingConfiguration : register(b1);
StructuredBuffer<Engine::NurbsRayTracer::TraceableSurface> traceablePatches : register(t0);
RWTexture2D<float4> renderTarget : register(u0);

#define NurbsRaytracerRS \
              "RootFlags(0), " \
              "RootConstants(num32BitConstants = 24, b0), " \
              "RootConstants(num32BitConstants = 4, b1), " \
              "SRV(t0, flags = DATA_STATIC), " \
              "DescriptorTable( UAV(u0), visibility=SHADER_VISIBILITY_ALL) "

