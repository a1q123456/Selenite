#pragma once

namespace Engine
{
    namespace Rendering
    {
        struct NurbsTracingConfiguration
        {
            int maxIteration;
            float errorThreshold;
            uint seed;
        };
    }
}
