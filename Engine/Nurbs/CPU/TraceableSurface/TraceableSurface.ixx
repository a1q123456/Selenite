export module Engine.Nurbs.TraceableSurface;
import Engine.Nurbs.AABB;
import Engine.Nurbs.SurfacePatch;
import Engine.Nurbs.QuadApproximation;
import std;

namespace Engine::Nurbs
{
    export struct TraceableSurface
    {
        SurfacePatch nurbsPatch;
        AABB boundingBox;
        QuadApproximation approximation;
    };
}
