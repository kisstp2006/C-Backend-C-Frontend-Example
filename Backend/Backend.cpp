#include "Backend.h"

#include <algorithm>
#include <mutex>
#include <string>

namespace {

std::mutex g_stateMutex;
BackendRenderState g_renderState = {{0.07f, 0.12f, 0.18f, 1.0f}, BACKEND_PRIMITIVE_CIRCLE};

std::wstring g_pendingWindowTitle;
bool g_hasPendingTitle = false;

int g_pendingWidth = 1280;
int g_pendingHeight = 720;
bool g_hasPendingResize = false;

float Clamp01(float value) {
    return std::max(0.0f, std::min(1.0f, value));
}

}  // namespace

extern "C" BACKEND_API int AddNumbers(int a, int b) {
    return a + b;
}

extern "C" BACKEND_API void Backend_SetClearColor(float r, float g, float b, float a) {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    g_renderState.clearColor[0] = Clamp01(r);
    g_renderState.clearColor[1] = Clamp01(g);
    g_renderState.clearColor[2] = Clamp01(b);
    g_renderState.clearColor[3] = Clamp01(a);
}

extern "C" BACKEND_API void Backend_SetPrimitiveType(int primitiveType) {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    g_renderState.primitiveType = primitiveType;
}

extern "C" BACKEND_API int Backend_GetRenderState(BackendRenderState* outState) {
    if (outState == nullptr) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(g_stateMutex);
    *outState = g_renderState;
    return 1;
}

extern "C" BACKEND_API void Backend_SetWindowTitle(const wchar_t* title) {
    if (title == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_stateMutex);
    g_pendingWindowTitle = title;
    g_hasPendingTitle = true;
}

extern "C" BACKEND_API int Backend_ConsumeWindowTitle(wchar_t* buffer, int capacity) {
    if (buffer == nullptr || capacity <= 0) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(g_stateMutex);
    if (!g_hasPendingTitle) {
        return 0;
    }

    const size_t maxWrite = static_cast<size_t>(capacity - 1);
    const size_t copyCount = std::min(maxWrite, g_pendingWindowTitle.size());
    g_pendingWindowTitle.copy(buffer, copyCount);
    buffer[copyCount] = L'\0';

    g_pendingWindowTitle.clear();
    g_hasPendingTitle = false;
    return 1;
}

extern "C" BACKEND_API void Backend_RequestWindowResize(int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_stateMutex);
    g_pendingWidth = width;
    g_pendingHeight = height;
    g_hasPendingResize = true;
}

extern "C" BACKEND_API int Backend_ConsumeWindowResize(int* outWidth, int* outHeight) {
    if (outWidth == nullptr || outHeight == nullptr) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(g_stateMutex);
    if (!g_hasPendingResize) {
        return 0;
    }

    *outWidth = g_pendingWidth;
    *outHeight = g_pendingHeight;
    g_hasPendingResize = false;
    return 1;
}
