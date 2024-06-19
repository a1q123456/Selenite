#pragma once


namespace Engine
{
    namespace Math
    {
        struct Polynomial
        {
            float4x4 coefficients;

            float Evaluate(float u, float v)
            {
                const int degree = 3;
                float result = 0;

                float uToI = 1;
                for (int i = 0; i <= degree; i++)
                {
                    float vToJ = 1;
                    for (int j = 0; j <= degree; j++)
                    {
                        result += coefficients[j][i] * uToI * vToJ;
                        vToJ *= v;
                    }
                    uToI *= u;
                }

                return result;
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
