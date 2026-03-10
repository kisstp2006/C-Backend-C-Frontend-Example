#pragma once

#include <windows.h>
#include <d3d11.h>

class ImGuiLayer {
public:
    ImGuiLayer();
    ~ImGuiLayer();

    bool Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
    bool HandleWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) const;
    void BeginFrame(float deltaSeconds);
    void Render();
    void Shutdown();

private:
    bool initialized_;
    float frameTimeMs_;
};
