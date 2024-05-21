export module NurbsViewer.MyRenderable;
import Engine.Graphics.Device.D3DX12;
import Engine.Graphics.Renderable;
import std;

namespace NurbsViewer
{
    export class MyRenderable : public Engine::Graphics::Renderable
    {
    public:
        auto Render(float time) -> void override;
    };
}

