#pragma once
#include "Engine/Nurbs/GPU/Math/BasisFunction/BasisFunction.h"
#include "Engine/Nurbs/GPU/Math/BasisFunctionDerivative/BasisFunctionDerivative.h"
#include "Engine/Nurbs/GPU/Math/NurbsPatchIndex/NurbsPatchIndex.h"
#include "Engine/Nurbs/GPU/Constants/RationaliserData.h"
#include "Engine/Nurbs/GPU/TraceableSurface/TraceableSurface.h"

Engine::Rendering::RationaliserData rationaliserData : register(b0);
StructuredBuffer<Engine::Math::BasisFunction> basisFunctions : register(t0);
StructuredBuffer<Engine::Math::BasisFunctionDerivative> basisFunctionDerivatives : register(t1);
StructuredBuffer<Engine::Math::NurbsPatchIndex> nurbsPatchIndex : register(t2);
StructuredBuffer<float4> controlPoints : register(t3);

RWStructuredBuffer<Engine::NurbsRayTracer::TraceableSurface> traceablePatches : register(u0);


#define NurbsRationaliserRS \
              "RootFlags(0), " \
              "RootConstants(num32BitConstants = 4, b0, visibility=SHADER_VISIBILITY_ALL)," \
              "SRV(t0, flags = DATA_STATIC), " \
              "SRV(t1, flags = DATA_STATIC), " \
              "SRV(t2, flags = DATA_STATIC), " \
              "SRV(t3, flags = DATA_STATIC), " \
              "DescriptorTable( "\
                "UAV(u0, flags = DATA_VOLATILE), " \
                "visibility=SHADER_VISIBILITY_ALL" \
              ") " \
