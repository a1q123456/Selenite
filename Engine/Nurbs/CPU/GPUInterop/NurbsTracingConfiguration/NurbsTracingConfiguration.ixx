module;
#include <DirectXMath.h>
export module Engine.Nurbs.GPUInterop.NurbsTracingConfiguration;
import std;

export namespace Engine::Nurbs::GPUInterop
{
    struct NurbsTracingConfiguration
    {
        std::int32_t maxIteration;
        float errorThreshold;
        std::uint32_t seed;
        std::uint32_t patchesCount;
    };
}