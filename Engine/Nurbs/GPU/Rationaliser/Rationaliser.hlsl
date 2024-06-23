#include "Engine/Nurbs/GPU/Math/Polynomial/Polynomial.h"
#include "Engine/Nurbs/GPU/Constants/RationaliserConstants.h"

[RootSignature(NurbsRationaliserRS)]
[numthreads(64, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    const int degreeU = 3;
    const int degreeV = 3;

    const uint2 degreeUV = uint2(degreeU, degreeV);

    uint3 coordinate = tid;

    uint index = coordinate.x;

    if (index >= rationaliserData.patchesCount)
    {
        return;
    }

    Engine::Math::BasisFunction patchBasisFunctions = basisFunctions[index];
    Engine::Math::BasisFunctionDerivative patchDerivatives = basisFunctionDerivatives[index];
    Engine::Math::NurbsPatchIndex patchIndex = nurbsPatchIndex[index];

    uint2 uvIndex = patchIndex.index;

    // TODO: Don't initialise in shipping builds
    traceablePatches[index].Initialise();

    float4x4 polynomial[4][4][3];
    float4x4 weighted[4][4];
    float4x4 derivativeUNum[4][4][3];
    float4x4 derivativeUDen[4][4];
    float4x4 derivativeVNum[4][4][3];
    float4x4 derivativeVDen[4][4];
    
    bool isRational = false;
    float previousW = 0;
    bool isFirst = true;
    for (int i = 0; i <= degreeU; i++)
    {
        for (int j = 0; j <= degreeV; j++)
        {
            uint2 controlPointsIndex = uvIndex - degreeUV + uint2(i, j);

            float4 controlPoint = controlPoints[controlPointsIndex.x * rationaliserData.controlPointsStride + controlPointsIndex.y];

            if (isFirst)
            {
                previousW = controlPoint.w;
                isFirst = false;
            }
            else if (controlPoint.w != previousW)
            {
                isRational = true;
            }

            weighted[i][j] = patchBasisFunctions.basisFunctions[i][j].coefficients * controlPoint.w;

            polynomial[i][j][0] = weighted[i][j] * controlPoint.x;
            polynomial[i][j][1] = weighted[i][j] * controlPoint.y;
            polynomial[i][j][2] = weighted[i][j] * controlPoint.z;

            derivativeUDen[i][j] = patchDerivatives.derivativeU[i][j].coefficients * controlPoint.w;
            derivativeUNum[i][j][0] = derivativeUDen[i][j] * controlPoint.x;
            derivativeUNum[i][j][1] = derivativeUDen[i][j] * controlPoint.y;
            derivativeUNum[i][j][2] = derivativeUDen[i][j] * controlPoint.z;

            derivativeVDen[i][j] = patchDerivatives.derivativeV[i][j].coefficients * controlPoint.w;
            derivativeVNum[i][j][0] = derivativeVDen[i][j] * controlPoint.x;
            derivativeVNum[i][j][1] = derivativeVDen[i][j] * controlPoint.y;
            derivativeVNum[i][j][2] = derivativeVDen[i][j] * controlPoint.z;
        }
    }

    for (int i = 0; i <= degreeU; i++)
    {
        for (int j = 0; j <= degreeV; j++)
        {
            traceablePatches[index].nurbsPatch.nurbsFunction.numerator.x.coefficients += polynomial[i][j][0];
            traceablePatches[index].nurbsPatch.nurbsFunction.numerator.y.coefficients += polynomial[i][j][1];
            traceablePatches[index].nurbsPatch.nurbsFunction.numerator.z.coefficients += polynomial[i][j][2];

            traceablePatches[index].nurbsPatch.partialDerivativeU.numerator.x.coefficients += derivativeUNum[i][j][0];
            traceablePatches[index].nurbsPatch.partialDerivativeU.numerator.y.coefficients += derivativeUNum[i][j][1];
            traceablePatches[index].nurbsPatch.partialDerivativeU.numerator.z.coefficients += derivativeUNum[i][j][2];

            traceablePatches[index].nurbsPatch.partialDerivativeV.numerator.x.coefficients += derivativeVNum[i][j][0];
            traceablePatches[index].nurbsPatch.partialDerivativeV.numerator.y.coefficients += derivativeVNum[i][j][1];
            traceablePatches[index].nurbsPatch.partialDerivativeV.numerator.z.coefficients += derivativeVNum[i][j][2];

            if (isRational)
            {
                traceablePatches[index].nurbsPatch.nurbsFunction.denominator.coefficients += weighted[i][j];
                traceablePatches[index].nurbsPatch.partialDerivativeU.denominator.coefficients += derivativeUDen[i][j];
                traceablePatches[index].nurbsPatch.partialDerivativeV.denominator.coefficients += derivativeVDen[i][j];
            }
        }
    }

    traceablePatches[index].nurbsPatch.nurbsFunction.isRational = isRational;
    traceablePatches[index].nurbsPatch.partialDerivativeU.isRational = isRational;
    traceablePatches[index].nurbsPatch.partialDerivativeV.isRational = isRational;

    traceablePatches[index].approximation.uv[0] = patchIndex.minUV;
    traceablePatches[index].approximation.uv[1] = float2(patchIndex.maxUV.x, patchIndex.minUV.y);
    traceablePatches[index].approximation.uv[2] = float2(patchIndex.minUV.x, patchIndex.maxUV.y);
    traceablePatches[index].approximation.uv[3] = patchIndex.maxUV;

    float3 minPosition;
    float3 maxPosition;

    isFirst = true;
    for (int i = 0; i < 4; i++)
    {
        float2 uv = traceablePatches[index].approximation.uv[i];
        float3 position = traceablePatches[index].nurbsPatch.nurbsFunction.Evaluate(
            uv.x,
            uv.y);

        if (isFirst)
        {
            minPosition = position;
            maxPosition = position;
        }
        else
        {
            minPosition = float3(
                min(minPosition.x, position.x),
                min(minPosition.y, position.y),
                min(minPosition.z, position.z)
            );
            maxPosition = float3(
                max(maxPosition.x, position.x),
                max(maxPosition.y, position.y),
                max(maxPosition.z, position.z)
            );
        }
        if (i < 3)
        {
            traceablePatches[index].approximation.positions[i] = float4(position, 0);
        }
    }

    traceablePatches[index].approximation.plane = Engine::Math::Plane::FromPoints(
        (float3)traceablePatches[index].approximation.positions[0],
        (float3)traceablePatches[index].approximation.positions[1],
        (float3)traceablePatches[index].approximation.positions[2]
    );

    for (int i = 0; i <= degreeU; i++)
    {
        for (int j = 0; j <= degreeV; j++)
        {
            uint2 controlPointsIndex = uvIndex - degreeUV + uint2(i, j);

            float4 controlPoint = controlPoints[controlPointsIndex.x * rationaliserData.controlPointsStride + controlPointsIndex.y];

            float distance = dot((float3) controlPoint, traceablePatches[index].approximation.plane.normal);

            float3 projectedPoint = traceablePatches[index].approximation.plane.normal * distance;

            
            minPosition = float3(
                min(minPosition.x, projectedPoint.x),
                min(minPosition.y, projectedPoint.y),
                min(minPosition.z, projectedPoint.z)
            );
            maxPosition = float3(
                max(maxPosition.x, projectedPoint.x),
                max(maxPosition.y, projectedPoint.y),
                max(maxPosition.z, projectedPoint.z)
            );
        }
    }
    traceablePatches[index].boundingBox.minPosition = float4(minPosition, 0);
    traceablePatches[index].boundingBox.maxPosition = float4(maxPosition, 0);
}

