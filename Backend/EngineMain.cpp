#include <windows.h>

#include <filesystem>
#include <chrono>
#include <string>
#include <vector>
#include <array>
#include <cstdlib>

#include <shellapi.h>

#include "Dx11Renderer.h"
#include "HostFxrBridge.h"
#include "Backend.h"
#include "ImGuiLayer.h"

#if ENGINEHOST_WITH_IMGUI
#include "imgui.h"
#endif

namespace {

ImGuiLayer* gImGuiLayer = nullptr;

std::string ToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return std::string();
    }

    const int required = ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (required <= 1) {
        return std::string();
    }

    std::string result(static_cast<size_t>(required), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, result.data(), required, nullptr, nullptr);
    result.resize(static_cast<size_t>(required - 1));
    return result;
}

std::wstring ToWide(const std::string& text) {
    if (text.empty()) {
        return std::wstring();
    }

    const int required = ::MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (required <= 1) {
        return std::wstring();
    }

    std::wstring result(static_cast<size_t>(required), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), required);
    result.resize(static_cast<size_t>(required - 1));
    return result;
}

int RunCommand(const std::wstring& command) {
    return _wsystem(command.c_str());
}

#if ENGINEHOST_WITH_IMGUI
struct GameProjectEntry {
    std::wstring name;
    std::filesystem::path csprojPath;
    std::filesystem::path dllPath;
};

std::vector<GameProjectEntry> DiscoverGameProjects(const std::filesystem::path& root) {
    std::vector<GameProjectEntry> projects;

    const std::filesystem::path gameProjectsRoot = root / "GameProjects";
    if (!std::filesystem::exists(gameProjectsRoot)) {
        return projects;
    }

    for (const auto& entry : std::filesystem::directory_iterator(gameProjectsRoot)) {
        if (!entry.is_directory()) {
            continue;
        }

        const std::filesystem::path dir = entry.path();
        const std::wstring projectName = dir.filename().wstring();
        const std::filesystem::path csprojPath = dir / (projectName + L".csproj");

        if (!std::filesystem::exists(csprojPath)) {
            continue;
        }

        GameProjectEntry project = {};
        project.name = projectName;
        project.csprojPath = csprojPath;
        project.dllPath = dir / "bin" / "Debug" / "net10.0" / (projectName + L".dll");
        projects.push_back(project);
    }

    return projects;
}

bool IsValidProjectName(const std::wstring& name) {
    if (name.empty()) {
        return false;
    }

    for (wchar_t ch : name) {
        const bool valid =
            (ch >= L'a' && ch <= L'z') ||
            (ch >= L'A' && ch <= L'Z') ||
            (ch >= L'0' && ch <= L'9') ||
            ch == L'_';

        if (!valid) {
            return false;
        }
    }

    return true;
}
#endif

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (gImGuiLayer != nullptr && gImGuiLayer->HandleWndProc(hwnd, uMsg, wParam, lParam)) {
        return 1;
    }

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

    ImGuiLayer imguiLayer;
    gImGuiLayer = &imguiLayer;
    const bool imguiReady = imguiLayer.Initialize(hwnd, renderer.GetDevice(), renderer.GetContext());

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
    bool managedLoaded = false;

#if ENGINEHOST_WITH_IMGUI
    std::vector<GameProjectEntry> gameProjects = DiscoverGameProjects(root);
    int selectedProjectIndex = gameProjects.empty() ? -1 : 0;
    std::array<char, 128> newProjectName = {};
    std::string launcherStatus = "Pick a game project or generate a new one.";

    auto tryStartManaged = [&](const std::wstring& selectedGameDll) {
        if (!selectedGameDll.empty()) {
            SetEnvironmentVariableW(L"ENGINE_GAME_DLL", selectedGameDll.c_str());
        }

        managedLoaded = runtimeHost.Initialize(
            runtimeConfigPath.wstring(),
            assemblyPath.wstring(),
            L"ScriptingHost.Bindings.NativeEntryPoints, ScriptingHost",
            L"ManagedStart",
            L"ManagedUpdate");

        if (!managedLoaded) {
            launcherStatus = "Failed to initialize managed runtime host.";
            return;
        }

        if (!runtimeHost.InvokeStart()) {
            launcherStatus = "Managed Start() failed.";
            managedLoaded = false;
            runtimeHost.Shutdown();
            return;
        }

        launcherStatus = "Managed runtime started.";
    };
#else
    managedLoaded = runtimeHost.Initialize(
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
#endif

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

        renderer.BeginFrame();
    #if ENGINEHOST_WITH_IMGUI
        if (imguiReady) {
            imguiLayer.BeginFrame(deltaSeconds);
        }
    #endif

#if ENGINEHOST_WITH_IMGUI
        if (!imguiReady) {
            if (!managedLoaded) {
                managedLoaded = runtimeHost.Initialize(
                    runtimeConfigPath.wstring(),
                    assemblyPath.wstring(),
                    L"ScriptingHost.Bindings.NativeEntryPoints, ScriptingHost",
                    L"ManagedStart",
                    L"ManagedUpdate");

                if (managedLoaded) {
                    runtimeHost.InvokeStart();
                }
            }

            if (managedLoaded) {
                runtimeHost.InvokeUpdate(deltaSeconds);
            }
        } else if (!managedLoaded) {
            ImGui::Begin("Project Launcher");
            ImGui::Text("Select project to run in EngineHost");
            ImGui::Separator();

            if (ImGui::Button("Refresh Projects")) {
                gameProjects = DiscoverGameProjects(root);
                if (gameProjects.empty()) {
                    selectedProjectIndex = -1;
                    launcherStatus = "No projects found under GameProjects/.";
                } else if (selectedProjectIndex >= static_cast<int>(gameProjects.size())) {
                    selectedProjectIndex = 0;
                }
            }

            if (!gameProjects.empty()) {
                std::vector<const char*> labels;
                labels.reserve(gameProjects.size());

                std::vector<std::string> names;
                names.reserve(gameProjects.size());
                for (const auto& project : gameProjects) {
                    names.push_back(ToUtf8(project.name));
                }
                for (const auto& name : names) {
                    labels.push_back(name.c_str());
                }

                ImGui::ListBox("Game Projects", &selectedProjectIndex, labels.data(), static_cast<int>(labels.size()), 7);

                if (selectedProjectIndex >= 0 && selectedProjectIndex < static_cast<int>(gameProjects.size())) {
                    const auto& selected = gameProjects[static_cast<size_t>(selectedProjectIndex)];
                    const std::wstring buildCommand = L"dotnet build \"" + selected.csprojPath.wstring() + L"\" -c Debug";

                    ImGui::Text("Selected: %s", ToUtf8(selected.name).c_str());
                    if (ImGui::Button("Open Selected Project")) {
                        launcherStatus = "Building selected project...";
                        const int buildExitCode = RunCommand(buildCommand);
                        if (buildExitCode != 0) {
                            launcherStatus = "Build failed for selected project.";
                        } else if (!std::filesystem::exists(selected.dllPath)) {
                            launcherStatus = "Built project, but output DLL was not found.";
                        } else {
                            launcherStatus = "Starting managed runtime...";
                            tryStartManaged(selected.dllPath.wstring());
                        }
                    }
                }
            } else {
                ImGui::Text("No game project found.");
            }

            ImGui::Separator();
            ImGui::InputText("New Project Name", newProjectName.data(), newProjectName.size());
            if (ImGui::Button("Generate New Project")) {
                const std::wstring projectName = ToWide(std::string(newProjectName.data()));
                if (!IsValidProjectName(projectName)) {
                    launcherStatus = "Invalid name. Use A-Z, a-z, 0-9, _.";
                } else {
                    const std::wstring generateCommand =
                        L"powershell -ExecutionPolicy Bypass -File \"" +
                        (root / "Scripts" / "new-game-project.ps1").wstring() +
                        L"\" -Name \"" + projectName + L"\"";

                    launcherStatus = "Generating new game project...";
                    const int generateExitCode = RunCommand(generateCommand);
                    if (generateExitCode != 0) {
                        launcherStatus = "Project generation failed.";
                    } else {
                        gameProjects = DiscoverGameProjects(root);
                        selectedProjectIndex = 0;
                        for (size_t i = 0; i < gameProjects.size(); ++i) {
                            if (gameProjects[i].name == projectName) {
                                selectedProjectIndex = static_cast<int>(i);
                                break;
                            }
                        }
                        launcherStatus = "Project generated. Select and open it.";
                    }
                }
            }

            if (ImGui::Button("Run Fallback (No Game DLL)")) {
                launcherStatus = "Starting fallback managed runtime...";
                tryStartManaged(L"");
            }

            ImGui::Separator();
            ImGui::TextWrapped("Status: %s", launcherStatus.c_str());
            ImGui::End();
        } else {
            runtimeHost.InvokeUpdate(deltaSeconds);
        }
#else
        if (managedLoaded) {
            runtimeHost.InvokeUpdate(deltaSeconds);
        }
#endif

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

#if ENGINEHOST_WITH_IMGUI
        if (imguiReady) {
            imguiLayer.Render();
        }
#endif
        renderer.EndFrame();
    }

    gImGuiLayer = nullptr;
    imguiLayer.Shutdown();
    runtimeHost.Shutdown();
    renderer.Shutdown();
    return 0;
}
