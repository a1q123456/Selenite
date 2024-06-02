#pragma once

namespace Engine
{
    namespace Rendering
    {
        struct CameraData
        {
            float4x4 iProjMatrix;
            float4 origin;
            uint2 outputSize;
            uint2 pad;
        };
    }
}
