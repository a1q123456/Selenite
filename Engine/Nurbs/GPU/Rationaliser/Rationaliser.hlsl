#include "Engine/Nurbs/GPU/Math/Polynomial/Polynomial.h"
#include "Engine/Nurbs/GPU/Constants/RationaliserConstants.h"

[RootSignature(NurbsRationaliserRS)]
[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID, uint3 groupID : SV_GroupID)
{
    const int degreeU = 3;
    const int degreeV = 3;

    uint3 groupSize = uint3(32, 1, 1);
    uint3 coordinate = groupSize * groupID + tid;

    uint index = coordinate.x;

    uint structsCount = 0;
    uint stride = 0;
    basisFunctions.GetDimensions(structsCount, stride);

    if (index >= structsCount)
    {
        return;
    }

    Engine::Math::BasisFunction patchBasisFunctions = basisFunctions[index];
    Engine::Math::BasisFunctionDerivative patchDerivatives = basisFunctionDerivatives[index];
    Engine::Math::NurbsPatchIndex patchIndex = nurbsPatchIndex[index];

    uint2 uvIndex = patchIndex.index;

    float4x4 polynomial[4][4][3];
    float4x4 weighted[4][4];
    float4x4 derivativeUNum[4][4][3];
    float4x4 derivativeUDen[4][4];
    float4x4 derivativeVNum[4][4][3];
    float4x4 derivativeVDen[4][4];

    for (int i = degreeU; i >= 0; i--)
    {
        for (int j = degreeV; j >= 0; j--)
        {
            uint2 controlPointsIndex = uint2(uvIndex.x - i, uvIndex.y - j);

            float3 controlPoint = controlPoints[controlPointsIndex];
            float weight = weights[controlPointsIndex];

            weighted[degreeU - i][degreeV - j] = patchBasisFunctions.basisFunctions[degreeU - i][degreeV - j].coefficients * weight;

            polynomial[degreeU - i][degreeV - j][0] = weighted[degreeU - i][degreeV - j] * controlPoint.x;
            polynomial[degreeU - i][degreeV - j][1] = weighted[degreeU - i][degreeV - j] * controlPoint.y;
            polynomial[degreeU - i][degreeV - j][2] = weighted[degreeU - i][degreeV - j] * controlPoint.z;

            derivativeUDen[degreeU - i][degreeV - j] = patchDerivatives.derivativeU[degreeU - i][degreeV - j].coefficients * weight;
            derivativeUNum[degreeU - i][degreeV - j][0] = derivativeUDen[degreeU - i][degreeV - j] * controlPoint.x;
            derivativeUNum[degreeU - i][degreeV - j][1] = derivativeUDen[degreeU - i][degreeV - j] * controlPoint.y;
            derivativeUNum[degreeU - i][degreeV - j][2] = derivativeUDen[degreeU - i][degreeV - j] * controlPoint.z;

            derivativeVDen[degreeU - i][degreeV - j] = patchDerivatives.derivativeV[degreeU - i][degreeV - j].coefficients * weight;
            derivativeVNum[degreeU - i][degreeV - j][0] = derivativeVDen[degreeU - i][degreeV - j] * controlPoint.x;
            derivativeVNum[degreeU - i][degreeV - j][1] = derivativeVDen[degreeU - i][degreeV - j] * controlPoint.y;
            derivativeVNum[degreeU - i][degreeV - j][2] = derivativeVDen[degreeU - i][degreeV - j] * controlPoint.z;
        }
    }

    for (int i = 0; i <= degreeU; i++)
    {
        for (int j = 0; j <= degreeV; j++)
        {
            nurbsPatches[index].nurbsFunction.numerator.x.coefficients += polynomial[i][j][0];
            nurbsPatches[index].nurbsFunction.numerator.y.coefficients += polynomial[i][j][1];
            nurbsPatches[index].nurbsFunction.numerator.z.coefficients += polynomial[i][j][2];

            nurbsPatches[index].nurbsFunction.denominator.coefficients += weighted[i][j];

            nurbsPatches[index].partialDerivativeU.numerator.x.coefficients += derivativeUNum[i][j][0];
            nurbsPatches[index].partialDerivativeU.numerator.y.coefficients += derivativeUNum[i][j][1];
            nurbsPatches[index].partialDerivativeU.numerator.z.coefficients += derivativeUNum[i][j][2];

            nurbsPatches[index].partialDerivativeU.denominator.coefficients += derivativeUDen[i][j];

            nurbsPatches[index].partialDerivativeV.numerator.x.coefficients += derivativeVNum[i][j][0];
            nurbsPatches[index].partialDerivativeV.numerator.y.coefficients += derivativeVNum[i][j][1];
            nurbsPatches[index].partialDerivativeV.numerator.z.coefficients += derivativeVNum[i][j][2];

            nurbsPatches[index].partialDerivativeV.denominator.coefficients += derivativeVDen[i][j];

        }
    }
}




