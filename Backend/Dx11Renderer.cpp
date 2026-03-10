#include "Dx11Renderer.h"

Dx11Renderer::Dx11Renderer()
    : hwnd_(nullptr),
      width_(0),
      height_(0),
      clearColor_{0.07f, 0.12f, 0.18f, 1.0f},
      primitiveType_(0),
      device_(nullptr),
      context_(nullptr),
      swapChain_(nullptr),
      renderTargetView_(nullptr),
      vertexShader_(nullptr),
      pixelShader_(nullptr),
      inputLayout_(nullptr),
      vertexBuffer_(nullptr) {
}

Dx11Renderer::~Dx11Renderer() {
    Shutdown();
}

bool Dx11Renderer::Initialize(HWND hwnd, int width, int height) {
    hwnd_ = hwnd;
    width_ = width;
    height_ = height;

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
#if defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        &featureLevel,
        1,
        D3D11_SDK_VERSION,
        &swapChainDesc,
        &swapChain_,
        &device_,
        nullptr,
        &context_);

    if (FAILED(hr)) {
        return false;
    }

    ID3D11Texture2D* backBuffer = nullptr;
    hr = swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    if (FAILED(hr)) {
        return false;
    }

    hr = device_->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView_);
    backBuffer->Release();

    if (FAILED(hr)) {
        return false;
    }

    return InitializePipeline() && RecreateRenderTarget();
}

void Dx11Renderer::BeginFrame() {
    if (context_ == nullptr || renderTargetView_ == nullptr) {
        return;
    }

    DrawSelectedPrimitive();
    context_->OMSetRenderTargets(1, &renderTargetView_, nullptr);
    context_->ClearRenderTargetView(renderTargetView_, clearColor_);
}

void Dx11Renderer::RenderFrame() {
    BeginFrame();
    EndFrame();
}

void Dx11Renderer::EndFrame() {
    if (swapChain_ == nullptr) {
        return;
    }

    swapChain_->Present(1, 0);
}

void Dx11Renderer::SetClearColor(float r, float g, float b, float a) {
    clearColor_[0] = r;
    clearColor_[1] = g;
    clearColor_[2] = b;
    clearColor_[3] = a;
}

void Dx11Renderer::SetPrimitiveType(int primitiveType) {
    primitiveType_ = primitiveType;
}

bool Dx11Renderer::Resize(int width, int height) {
    if (swapChain_ == nullptr || width <= 0 || height <= 0) {
        return false;
    }

    width_ = width;
    height_ = height;

    if (renderTargetView_ != nullptr) {
        renderTargetView_->Release();
        renderTargetView_ = nullptr;
    }

    const HRESULT hr = swapChain_->ResizeBuffers(0, width_, height_, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) {
        return false;
    }

    return RecreateRenderTarget();
}

ID3D11Device* Dx11Renderer::GetDevice() const {
    return device_;
}

ID3D11DeviceContext* Dx11Renderer::GetContext() const {
    return context_;
}

bool Dx11Renderer::RecreateRenderTarget() {
    if (device_ == nullptr || swapChain_ == nullptr) {
        return false;
    }

    ID3D11Texture2D* backBuffer = nullptr;
    HRESULT hr = swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    if (FAILED(hr)) {
        return false;
    }

    hr = device_->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView_);
    backBuffer->Release();
    return SUCCEEDED(hr);
}

bool Dx11Renderer::InitializePipeline() {
    return true;
}

void Dx11Renderer::DrawSelectedPrimitive() {
    switch (primitiveType_) {
        case 1:
            clearColor_[1] = 0.22f;
            break;
        case 2:
            clearColor_[2] = 0.30f;
            break;
        case 3:
            clearColor_[0] = 0.30f;
            break;
        default:
            break;
    }
}

void Dx11Renderer::Shutdown() {
    if (vertexBuffer_ != nullptr) {
        vertexBuffer_->Release();
        vertexBuffer_ = nullptr;
    }

    if (inputLayout_ != nullptr) {
        inputLayout_->Release();
        inputLayout_ = nullptr;
    }

    if (pixelShader_ != nullptr) {
        pixelShader_->Release();
        pixelShader_ = nullptr;
    }

    if (vertexShader_ != nullptr) {
        vertexShader_->Release();
        vertexShader_ = nullptr;
    }

    if (renderTargetView_ != nullptr) {
        renderTargetView_->Release();
        renderTargetView_ = nullptr;
    }

    if (swapChain_ != nullptr) {
        swapChain_->Release();
        swapChain_ = nullptr;
    }

    if (context_ != nullptr) {
        context_->Release();
        context_ = nullptr;
    }

    if (device_ != nullptr) {
        device_->Release();
        device_ = nullptr;
    }
}
