#pragma once


namespace Engine
{
    namespace Math
    {
        struct Polynomial
        {
            float4x4 coefficients;

            float4x4 TensorProduct(float4 u, float4 v) {
                return float4x4(
                    u.x * v.x, u.x * v.y, u.x * v.z, u.x * v.w,
                    u.y * v.x, u.y * v.y, u.y * v.z, u.y * v.w,
                    u.z * v.x, u.z * v.y, u.z * v.z, u.z * v.w,
                    u.w * v.x, u.w * v.y, u.w * v.z, u.w * v.w
                );
            }

            float Evaluate(float u, float v)
            {
                float4 uPowers = float4(1, u, u * u, u * u * u);
                float4 vPowers = float4(1, v, v * v, v * v * v);
                float4x4 poweredUV = TensorProduct(vPowers, uPowers);

                float4x4 results = poweredUV * coefficients;

                return
                    results._m00 + results._m01 + results._m02 + results._m03 +
                    results._m10 + results._m11 + results._m12 + results._m13 +
                    results._m20 + results._m21 + results._m22 + results._m23 +
                    results._m30 + results._m31 + results._m32 + results._m33;
            }

            void Initialise()
            {
                coefficients = float4x4(
                    0, 0, 0, 0,
                    0, 0, 0, 0,
                    0, 0, 0, 0,
                    0, 0, 0, 0
                );
            }
        };
    }
}
