#pragma once

#include <d3d11.h>
#include <windows.h>

class Dx11Renderer {
public:
    Dx11Renderer();
    ~Dx11Renderer();

    bool Initialize(HWND hwnd, int width, int height);
    void BeginFrame();
    void RenderFrame();
    void EndFrame();
    void SetClearColor(float r, float g, float b, float a);
    void SetPrimitiveType(int primitiveType);
    bool Resize(int width, int height);
    ID3D11Device* GetDevice() const;
    ID3D11DeviceContext* GetContext() const;
    void Shutdown();

private:
    bool RecreateRenderTarget();
    bool InitializePipeline();
    void DrawSelectedPrimitive();

    HWND hwnd_;
    int width_;
    int height_;
    float clearColor_[4];
    int primitiveType_;

    ID3D11Device* device_;
    ID3D11DeviceContext* context_;
    IDXGISwapChain* swapChain_;
    ID3D11RenderTargetView* renderTargetView_;
    ID3D11VertexShader* vertexShader_;
    ID3D11PixelShader* pixelShader_;
    ID3D11InputLayout* inputLayout_;
    ID3D11Buffer* vertexBuffer_;
};
