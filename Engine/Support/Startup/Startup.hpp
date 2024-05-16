#pragma once

#define REGISTER_STARTUP(STARTUP) \
namespace \
{ \
    STARTUP startup; \
    Engine::Support::Bootstrap si {startup}; \
}

