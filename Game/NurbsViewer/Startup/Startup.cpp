import Engine.Support.IStartup;
import Engine.Core.Threading;
import Engine.Support.IntPtr;

import NurbsViewer;
#include "Engine/Support/Startup/Startup.hpp"

using namespace Engine::Core::Threading;
using namespace Engine::Support;

namespace NurbsViewer
{
    class Startup : public Engine::Support::IStartup
    {
        auto Initialise(IntPtr hwnd, int w, int h) -> Task<void> override
        {
            return {};
        }

        auto GetAppName() -> Engine::Core::CZString
        {
            return L"NurbsViewer";
        }

        auto GetDefaultSize(int& w, int& h) -> void
        {
            w = 1024;
            h = 768;
        }

        // TODO: Move this to the engine
        auto Tick() -> Task<void> override
        {
            return {};
        }

        auto Teardown() -> Task<void> override
        {
            return {};
        }
    };

    REGISTER_STARTUP(Startup);
}

