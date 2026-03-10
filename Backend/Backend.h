#pragma once

#ifdef BACKEND_EXPORTS
#define BACKEND_API __declspec(dllexport)
#else
#define BACKEND_API __declspec(dllimport)
#endif

enum BackendPrimitiveType {
	BACKEND_PRIMITIVE_NONE = 0,
	BACKEND_PRIMITIVE_SQUARE = 1,
	BACKEND_PRIMITIVE_CIRCLE = 2,
	BACKEND_PRIMITIVE_CUBE = 3,
};

struct BackendRenderState {
	float clearColor[4];
	int primitiveType;
};

extern "C" BACKEND_API int AddNumbers(int a, int b);

extern "C" BACKEND_API void Backend_SetClearColor(float r, float g, float b, float a);
extern "C" BACKEND_API void Backend_SetPrimitiveType(int primitiveType);
extern "C" BACKEND_API int Backend_GetRenderState(BackendRenderState* outState);

extern "C" BACKEND_API void Backend_SetWindowTitle(const wchar_t* title);
extern "C" BACKEND_API int Backend_ConsumeWindowTitle(wchar_t* buffer, int capacity);

extern "C" BACKEND_API void Backend_RequestWindowResize(int width, int height);
extern "C" BACKEND_API int Backend_ConsumeWindowResize(int* outWidth, int* outHeight);
