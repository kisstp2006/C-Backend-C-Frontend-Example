using System;
using System.Runtime.InteropServices;

namespace Frontend.Bindings;

internal static class NativeEntryPoints
{
    private static float _time;

    [UnmanagedCallersOnly(EntryPoint = "ManagedStart")]
    public static int ManagedStart(IntPtr args, int sizeBytes)
    {
        try
        {
            Console.WriteLine("Managed Start() from C++ host");
            EngineApi.Backend_SetWindowTitle("C++ Engine + C# Start/Update");
            EngineApi.Backend_RequestWindowResize(1280, 720);
            EngineApi.Backend_SetClearColor(0.06f, 0.09f, 0.16f, 1.0f);
            EngineApi.Backend_SetPrimitiveType(1);
            return 0;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"ManagedStart error: {ex.Message}");
            return 1;
        }
    }

    [UnmanagedCallersOnly(EntryPoint = "ManagedUpdate")]
    public static unsafe int ManagedUpdate(IntPtr args, int sizeBytes)
    {
        try
        {
            float delta = 0.016f;
            if (args != IntPtr.Zero && sizeBytes >= sizeof(float))
            {
                delta = *(float*)args.ToPointer();
            }

            _time += delta;

            float phase = (MathF.Sin(_time) + 1.0f) * 0.5f;
            EngineApi.Backend_SetClearColor(0.05f + 0.15f * phase, 0.10f, 0.20f + 0.10f * (1.0f - phase), 1.0f);

            int primitive = ((int)(_time * 0.8f) % 2 == 0) ? 1 : 2;
            EngineApi.Backend_SetPrimitiveType(primitive);

            return 0;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"ManagedUpdate error: {ex.Message}");
            return 1;
        }
    }
}
