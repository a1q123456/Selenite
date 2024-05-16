export module Engine.Support.IStartup;

import Engine.Core.String;
import Engine.Core.Threading;
import Engine.Support.IntPtr;

namespace Engine::Support
{
    export class IStartup
    {
    public:
        virtual auto Initialise(IntPtr hwnd, int w, int h) -> Engine::Core::Threading::Task<void> = 0;

        virtual auto GetAppName() -> Engine::Core::CZString = 0;

        virtual auto GetDefaultSize(int& w, int& h) -> void = 0;

        virtual auto Tick() -> Engine::Core::Threading::Task<void> = 0;

        virtual auto Teardown() -> Engine::Core::Threading::Task<void> = 0;

        virtual ~IStartup();
    };

    export struct Bootstrap
    {
        static IStartup* startup;

        Bootstrap(IStartup& startup);
    };
}
