using System.Runtime.InteropServices;

namespace Frontend.Bindings;

internal static class EngineApi
{
    [DllImport("Backend.dll", CallingConvention = CallingConvention.Cdecl)]
    internal static extern void Backend_SetClearColor(float r, float g, float b, float a);

    [DllImport("Backend.dll", CallingConvention = CallingConvention.Cdecl)]
    internal static extern void Backend_SetPrimitiveType(int primitiveType);

    [DllImport("Backend.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    internal static extern void Backend_SetWindowTitle(string title);

    [DllImport("Backend.dll", CallingConvention = CallingConvention.Cdecl)]
    internal static extern void Backend_RequestWindowResize(int width, int height);
}
