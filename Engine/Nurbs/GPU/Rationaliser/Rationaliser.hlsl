#include "Engine/Nurbs/GPU/Math/Polynomial/Polynomial.h"
#include "Engine/Nurbs/GPU/Constants/RationaliserConstants.h"

[RootSignature(NurbsRationaliserRS)]
[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID, uint3 groupID : SV_GroupID)
{
    const int degreeU = 3;
    const int degreeV = 3;

    const uint2 degreeUV = uint2(degreeU, degreeV);

    uint3 groupSize = uint3(32, 1, 1);
    uint3 coordinate = groupSize * groupID + tid;

    uint index = coordinate.x;

    if (index >= rationaliserData.patchesCount)
    {
        return;
    }

    Engine::Math::BasisFunction patchBasisFunctions = basisFunctions[index];
    Engine::Math::BasisFunctionDerivative patchDerivatives = basisFunctionDerivatives[index];
    Engine::Math::NurbsPatchIndex patchIndex = nurbsPatchIndex[index];

    uint2 uvIndex = patchIndex.index;

    nurbsPatches[index].Initialise();
    nurbsPatches[index].minUV = patchIndex.minUV;
    nurbsPatches[index].maxUV = patchIndex.maxUV;

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
            nurbsPatches[index].nurbsFunction.numerator.x.coefficients += polynomial[i][j][0];
            nurbsPatches[index].nurbsFunction.numerator.y.coefficients += polynomial[i][j][1];
            nurbsPatches[index].nurbsFunction.numerator.z.coefficients += polynomial[i][j][2];

            nurbsPatches[index].partialDerivativeU.numerator.x.coefficients += derivativeUNum[i][j][0];
            nurbsPatches[index].partialDerivativeU.numerator.y.coefficients += derivativeUNum[i][j][1];
            nurbsPatches[index].partialDerivativeU.numerator.z.coefficients += derivativeUNum[i][j][2];

            nurbsPatches[index].partialDerivativeV.numerator.x.coefficients += derivativeVNum[i][j][0];
            nurbsPatches[index].partialDerivativeV.numerator.y.coefficients += derivativeVNum[i][j][1];
            nurbsPatches[index].partialDerivativeV.numerator.z.coefficients += derivativeVNum[i][j][2];

            if (isRational)
            {
                nurbsPatches[index].nurbsFunction.denominator.coefficients += weighted[i][j];
                nurbsPatches[index].partialDerivativeU.denominator.coefficients += derivativeUDen[i][j];
                nurbsPatches[index].partialDerivativeV.denominator.coefficients += derivativeVDen[i][j];
            }
        }
    }

    nurbsPatches[index].nurbsFunction.isRational = isRational;
    nurbsPatches[index].partialDerivativeU.isRational = isRational;
    nurbsPatches[index].partialDerivativeV.isRational = isRational;
}




