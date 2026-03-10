#include "ImGuiLayer.h"

#if ENGINEHOST_WITH_IMGUI
#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

ImGuiLayer::ImGuiLayer()
    : initialized_(false),
      frameTimeMs_(0.0f) {
}

ImGuiLayer::~ImGuiLayer() {
    Shutdown();
}

bool ImGuiLayer::Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context) {
#if ENGINEHOST_WITH_IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    if (!ImGui_ImplWin32_Init(hwnd)) {
        return false;
    }

    if (!ImGui_ImplDX11_Init(device, context)) {
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    initialized_ = true;
    return true;
#else
    (void)hwnd;
    (void)device;
    (void)context;
    return false;
#endif
}

bool ImGuiLayer::HandleWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) const {
#if ENGINEHOST_WITH_IMGUI
    if (!initialized_) {
        return false;
    }

    return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam) != 0;
#else
    (void)hwnd;
    (void)msg;
    (void)wParam;
    (void)lParam;
    return false;
#endif
}

void ImGuiLayer::BeginFrame(float deltaSeconds) {
#if ENGINEHOST_WITH_IMGUI
    if (!initialized_) {
        return;
    }

    frameTimeMs_ = deltaSeconds * 1000.0f;
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("EngineHost") ;
    ImGui::Text("ImGui support is active.");
    ImGui::Text("Frame time: %.2f ms", frameTimeMs_);
    ImGui::Text("FPS: %.1f", deltaSeconds > 0.0f ? (1.0f / deltaSeconds) : 0.0f);
    ImGui::Separator();
    ImGui::Text("F1: Toggle this panel (coming next)");
    ImGui::End();
#else
    (void)deltaSeconds;
#endif
}

void ImGuiLayer::Render() {
#if ENGINEHOST_WITH_IMGUI
    if (!initialized_) {
        return;
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif
}

void ImGuiLayer::Shutdown() {
#if ENGINEHOST_WITH_IMGUI
    if (!initialized_) {
        return;
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
#endif
    initialized_ = false;
}
