#pragma once
#include "Engine/Nurbs/GPU/Math/NurbsPatch/NurbsPatch.h"
#include "Engine/Nurbs/GPU/Math/BasisFunction/BasisFunction.h"
#include "Engine/Nurbs/GPU/Math/BasisFunctionDerivative/BasisFunctionDerivative.h"
#include "Engine/Nurbs/GPU/Math/NurbsPatchIndex/NurbsPatchIndex.h"

StructuredBuffer<Engine::Math::BasisFunction> basisFunctions : register(t0);
StructuredBuffer<Engine::Math::BasisFunctionDerivative> basisFunctionDerivatives : register(t1);
StructuredBuffer<Engine::Math::NurbsPatchIndex> nurbsPatchIndex : register(t2);
RWStructuredBuffer<Engine::Math::NurbsPatch> nurbsPatches : register(u0);
Texture2D<float3> controlPoints : register(t3);
Texture2D<float> weights : register(t4);


#define NurbsRationaliserRS \
              "RootFlags(0), " \
              "SRV(t0, flags = DATA_STATIC), " \
              "SRV(t1, flags = DATA_STATIC), " \
              "SRV(t2, flags = DATA_STATIC), " \
              "DescriptorTable( SRV(t3), visibility=SHADER_VISIBILITY_ALL), " \
              "DescriptorTable( SRV(t4), visibility=SHADER_VISIBILITY_ALL), " \
              "DescriptorTable( UAV(u0), visibility=SHADER_VISIBILITY_ALL) "
