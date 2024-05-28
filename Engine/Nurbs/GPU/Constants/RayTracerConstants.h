#pragma once
#include "CameraData.h"
#include "NurbsTracingConfiguration.h"
#include "Engine/Nurbs/GPU/Math/NurbsPatch/NurbsPatch.h"

ConstantBuffer<Engine::Rendering::CameraData> cameraData : register(b0);
ConstantBuffer<Engine::Rendering::NurbsTracingConfiguration> nurbsTracingConfiguration : register(b1);
StructuredBuffer<Engine::Math::NurbsPatch> nurbsPatches : register(t0);
RWTexture2D<float3> renderTarget : register(u0);

#define NurbsRaytracerRS \
              "RootFlags(0), " \
              "RootConstants(num32BitConstants = 24, b0), " \
              "RootConstants(num32BitConstants = 3, b1), " \
              "SRV(t0, flags = DATA_STATIC), " \
              "DescriptorTable( UAV(u0), visibility=SHADER_VISIBILITY_ALL) "

