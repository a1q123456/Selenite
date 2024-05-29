#pragma once
#include "Engine/Nurbs/GPU/Math/NurbsPatch/NurbsPatch.h"
#include "Engine/Nurbs/GPU/Math/BasisFunction/BasisFunction.h"
#include "Engine/Nurbs/GPU/Math/BasisFunctionDerivative/BasisFunctionDerivative.h"
#include "Engine/Nurbs/GPU/Math/NurbsPatchIndex/NurbsPatchIndex.h"

StructuredBuffer<Engine::Math::BasisFunction> basisFunctions : register(t0);
StructuredBuffer<Engine::Math::BasisFunctionDerivative> basisFunctionDerivatives : register(t1);
StructuredBuffer<Engine::Math::NurbsPatchIndex> nurbsPatchIndex : register(t2);
Texture2D controlPoints : register(t3);

RWStructuredBuffer<Engine::Math::NurbsPatch> nurbsPatches : register(u0);


#define NurbsRationaliserRS \
              "RootFlags(0), " \
              "SRV(t0, flags = DATA_STATIC), " \
              "SRV(t1, flags = DATA_STATIC), " \
              "SRV(t2, flags = DATA_STATIC), " \
              "DescriptorTable( "\
                "SRV(t3)," \
                "UAV(u0), " \
                "visibility=SHADER_VISIBILITY_ALL" \
              ") " \
