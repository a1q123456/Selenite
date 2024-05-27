#pragma once

namespace Engine
{
    namespace Rendering
    {
        struct CameraData
        {
            float4x4 iProjMatrix;
            float4 origin;
            uint4 outputSize;
        };
    }
}
