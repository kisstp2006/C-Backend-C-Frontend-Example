#include <windows.h>

#include <filesystem>
#include <chrono>
#include <string>
#include <vector>

#include <shellapi.h>

#include "Dx11Renderer.h"
#include "HostFxrBridge.h"
#include "Backend.h"

namespace {

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

}  // namespace

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"CppEngineWindowClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    if (RegisterClassW(&wc) == 0) {
        return 1;
    }

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"C++ Engine Host (DX11 + C# Runtime)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1280,
        720,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (hwnd == nullptr) {
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);

    Dx11Renderer renderer;
    if (!renderer.Initialize(hwnd, 1280, 720)) {
        return 1;
    }

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    const std::filesystem::path root = std::filesystem::current_path();
    std::filesystem::path runtimeConfigPath = root / "ScriptingHost" / "bin" / "Debug" / "net10.0" / "ScriptingHost.runtimeconfig.json";
    std::filesystem::path assemblyPath = root / "ScriptingHost" / "bin" / "Debug" / "net10.0" / "ScriptingHost.dll";

    if (argv != nullptr) {
        if (argc > 1) {
            runtimeConfigPath = std::filesystem::path(argv[1]);
        }

        if (argc > 2) {
            assemblyPath = std::filesystem::path(argv[2]);
        }

        LocalFree(argv);
    }

    HostFxrBridge runtimeHost;
    const bool managedLoaded = runtimeHost.Initialize(
        runtimeConfigPath.wstring(),
        assemblyPath.wstring(),
        L"ScriptingHost.Bindings.NativeEntryPoints, ScriptingHost",
        L"ManagedStart",
        L"ManagedUpdate");

    if (!managedLoaded) {
        OutputDebugStringW(L"Failed to initialize managed runtime host.\n");
    } else {
        runtimeHost.InvokeStart();
    }

    auto previousTick = std::chrono::high_resolution_clock::now();
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        const auto now = std::chrono::high_resolution_clock::now();
        const float deltaSeconds = std::chrono::duration<float>(now - previousTick).count();
        previousTick = now;

        if (managedLoaded) {
            runtimeHost.InvokeUpdate(deltaSeconds);
        }

        BackendRenderState renderState = {};
        if (Backend_GetRenderState(&renderState) != 0) {
            renderer.SetClearColor(
                renderState.clearColor[0],
                renderState.clearColor[1],
                renderState.clearColor[2],
                renderState.clearColor[3]);
            renderer.SetPrimitiveType(renderState.primitiveType);
        }

        wchar_t titleBuffer[256] = {};
        if (Backend_ConsumeWindowTitle(titleBuffer, 256) != 0) {
            SetWindowTextW(hwnd, titleBuffer);
        }

        int newWidth = 0;
        int newHeight = 0;
        if (Backend_ConsumeWindowResize(&newWidth, &newHeight) != 0) {
            RECT rect = {};
            rect.left = 0;
            rect.top = 0;
            rect.right = newWidth;
            rect.bottom = newHeight;

            AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
            const int totalWidth = rect.right - rect.left;
            const int totalHeight = rect.bottom - rect.top;

            SetWindowPos(hwnd, nullptr, 0, 0, totalWidth, totalHeight, SWP_NOMOVE | SWP_NOZORDER);
            renderer.Resize(newWidth, newHeight);
        }

        renderer.RenderFrame();
    }

    runtimeHost.Shutdown();
    renderer.Shutdown();
    return 0;
}
