module;
#include <DirectXMath.h>
export module Engine.Nurbs.GPUInterop.RationaliserData;
import std;

namespace Engine::Nurbs::GPUInterop
{
    export struct RationaliserData
    {
        std::uint32_t controlPointsStride;
        std::uint32_t patchesCount;
        DirectX::XMUINT2 reserved;
    };

}

