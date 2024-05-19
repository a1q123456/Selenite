export module Engine.Support.IStartup;

import Engine.Core.String;
import Engine.Core.Threading;
import Engine.Graphics.Device;
import Engine.Support.Private;
import Engine.Graphics.Renderable;
import std;

namespace Engine::Support
{
    export class IStartup
    {
    public:
        virtual auto Initialise() -> Task<void> = 0;

        virtual auto GetAppName() -> CZString = 0;

        virtual auto GetDefaultSize(int& w, int& h) -> void = 0;

        virtual auto GetStartupScene() -> std::unique_ptr<Graphics::Renderable> = 0;

        virtual auto Teardown() -> Task<void> = 0;

        virtual ~IStartup() noexcept;
    };

    export struct Bootstrap
    {
        static IStartup* startup;
        static Graphics::Device::Context* context;

        Bootstrap(IStartup& startup);
    };
}
