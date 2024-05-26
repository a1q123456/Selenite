module;
#include "Engine/Support/Startup/Startup.hpp"
#include <Windows.h>
module NurbsViewer.Startup;
import Engine.Support.IStartup;
import Engine.Core.Threading.Tasks;
import Engine.Core.String;
import Engine.Graphics.Renderable;
import NurbsViewer.MyRenderable;
import NurbsViewer;
import std;

import Engine.Core.Memory.FastLocalHeap;

using namespace Engine::Core::Threading;
using namespace Engine::Core;

namespace NurbsViewer
{
    class Startup : public Engine::Support::IStartup
    {
        auto Initialise() -> Task<void> override
        {
            return {};
        }

        auto GetAppName() -> Engine::Core::CZString override
        {
            return L"NurbsViewer";
        }

        auto GetDefaultSize(int& w, int& h) -> void override
        {
            w = 1024;
            h = 768;
        }

        auto GetStartupScene() -> std::unique_ptr<Engine::Graphics::Renderable> override
        {
            return std::make_unique<MyRenderable>();
        }

        auto Teardown() -> Task<void> override
        {
            return {};
        }
    };

    REGISTER_STARTUP(Startup);
}

