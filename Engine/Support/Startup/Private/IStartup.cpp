module;
#include <Windows.h>
#include <wrl.h>
#include <DirectXMath.h>
module Engine.Support.IStartup;
import Engine.Core.IntPtr;
import Engine.Graphics.Device;
import std;

namespace Engine::Support
{
    IStartup::~IStartup()
    {
    }

    IStartup* Bootstrap::startup = nullptr;
    Graphics::Device::Context* Bootstrap::context = nullptr;

    Bootstrap::Bootstrap(IStartup& startup)
    {
        Bootstrap::startup = &startup;
        static Graphics::Device::Context context;
        Bootstrap::context = &context;
    }
}

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ExitGame() noexcept;

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR lpCmdLine,
    int nShowCmd
)
{
    using namespace Engine::Support;
    using namespace Engine::Core;
    
    auto startup = Bootstrap::startup;
    auto context = Bootstrap::context;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (!DirectX::XMVerifyCPUSupport())
    {
        return 1;
    }

    Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
    if (FAILED(initialize))
    {
        return 1;
    }

    // Register class and create window
    {
        // Register class
        WNDCLASSEXW wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = hInstance;
        wcex.hIcon = LoadIconW(hInstance, L"IDI_ICON");
        wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wcex.lpszClassName = L"NurbsRendererWindowClass";
        wcex.hIconSm = LoadIconW(wcex.hInstance, L"IDI_ICON");
        if (!RegisterClassExW(&wcex))
        {
            return 1;
        }

        // Create window
        int w = 1024;
        int h = 768;
        startup->GetDefaultSize(w, h);

        RECT rc = { 0, 0, static_cast<LONG>(w), static_cast<LONG>(h) };

        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

        HWND hwnd = CreateWindowExW(0, L"NurbsRendererWindowClass", startup->GetAppName(), WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
            nullptr, nullptr, hInstance,
            startup);
        // TODO: Change to CreateWindowExW(WS_EX_TOPMOST, L"NurbsRendererWindowClass", g_szAppName, WS_POPUP,
        // to default to fullscreen.

        if (!hwnd)
            return 1;

        ShowWindow(hwnd, nShowCmd);
        // TODO: Change nCmdShow to SW_SHOWMAXIMIZED to default to fullscreen.

        GetClientRect(hwnd, &rc);
        IntPtr windowHandle{ reinterpret_cast<IntPtr::HandleType>(hwnd) };
        context->Initialise(windowHandle, rc.right - rc.left, rc.bottom - rc.top);
        startup->Initialise();
        context->GetGPUScheduler()->SetRootRenderable(startup->GetStartupScene());
    }

    // Main message loop
    MSG msg = {};
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            context->GetGPUScheduler()->Tick();
        }
    }

    // Call level->Teardown();
    context->Teardown();
    startup->Teardown();

    return static_cast<int>(msg.wParam);
}


// Windows procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    using namespace Engine::Support;
    static bool s_in_sizemove = false;
    static bool s_in_suspend = false;
    static bool s_minimized = false;
    static bool s_fullscreen = false;
    // TODO: Set s_fullscreen to true if defaulting to fullscreen.

    auto startup = reinterpret_cast<IStartup*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_CREATE:
        if (lParam)
        {
            auto params = reinterpret_cast<LPCREATESTRUCTW>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(params->lpCreateParams));
        }
        break;

    case WM_PAINT:
        if (s_in_sizemove && startup)
        {
            Bootstrap::context->GetGPUScheduler()->Tick();
        }
        else
        {
            PAINTSTRUCT ps;
            std::ignore = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;

    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
        {
            if (!s_minimized)
            {
                s_minimized = true;
                if (!s_in_suspend && startup)
                {
                    // TODO: Call scheduler->OnSuspending();
                }
                s_in_suspend = true;
            }
        }
        else if (s_minimized)
        {
            s_minimized = false;
            if (s_in_suspend && startup)
            {
                // TODO: Call scheduler->OnResuming();
            }
            s_in_suspend = false;
        }
        else if (!s_in_sizemove && startup)
        {
            // TODO: scheduler->OnWindowSizeChanged(LOWORD(lParam), HIWORD(lParam));
        }
        break;

    case WM_ENTERSIZEMOVE:
        s_in_sizemove = true;
        break;

    case WM_EXITSIZEMOVE:
        s_in_sizemove = false;
        if (startup)
        {
            RECT rc;
            GetClientRect(hWnd, &rc);

            // TODO: scheduler->OnWindowSizeChanged(rc.right - rc.left, rc.bottom - rc.top);
        }
        break;

    case WM_GETMINMAXINFO:
        if (lParam)
        {
            auto info = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMinTrackSize.x = 320;
            info->ptMinTrackSize.y = 200;
        }
        break;

    case WM_ACTIVATEAPP:
        if (startup)
        {
            if (wParam)
            {
                // TODO: scheduler->OnActivated();
            }
            else
            {
                // TODO: scheduler->OnDeactivated();
            }
        }
        break;

    case WM_POWERBROADCAST:
        switch (wParam)
        {
        case PBT_APMQUERYSUSPEND:
            if (!s_in_suspend && startup)
            {
                // TODO: scheduler->OnSuspending();
            }
            s_in_suspend = true;
            return TRUE;

        case PBT_APMRESUMESUSPEND:
            if (!s_minimized)
            {
                if (s_in_suspend && startup)
                {
                    // TODO: scheduler->OnResuming();
                }
                s_in_suspend = false;
            }
            return TRUE;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_SYSKEYDOWN:
        if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
        {
            // Implements the classic ALT+ENTER fullscreen toggle
            if (s_fullscreen)
            {
                SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
                SetWindowLongPtr(hWnd, GWL_EXSTYLE, 0);

                int width = 800;
                int height = 600;
                if (startup)
                    startup->GetDefaultSize(width, height);

                ShowWindow(hWnd, SW_SHOWNORMAL);

                SetWindowPos(hWnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
            }
            else
            {
                SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP);
                SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

                SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

                ShowWindow(hWnd, SW_SHOWMAXIMIZED);
            }

            s_fullscreen = !s_fullscreen;
        }
        break;

    case WM_MENUCHAR:
        // A menu is active and the user presses a key that does not correspond
        // to any mnemonic or accelerator key. Ignore so we don't produce an error beep.
        return MAKELRESULT(0, MNC_CLOSE);
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

// Exit helper
void ExitGame() noexcept
{
    PostQuitMessage(0);
}

