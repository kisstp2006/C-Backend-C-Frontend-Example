using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using EngineScripting.Abstractions;

namespace ScriptingHost.Bindings;

internal static class NativeEntryPoints
{
    private static float _time;
    private static IGameScript? _gameScript;
    private static GameContext? _context;
    private static bool _started;

    [UnmanagedCallersOnly(EntryPoint = "ManagedStart")]
    public static int ManagedStart(IntPtr args, int sizeBytes)
    {
        try
        {
            _context = new GameContext(
                EngineApi.Backend_SetWindowTitle,
                EngineApi.Backend_RequestWindowResize,
                EngineApi.Backend_SetClearColor,
                EngineApi.Backend_SetPrimitiveType);

            LoadGameScript();

            _context.SetWindowTitle("C++ Engine + C# ScriptingHost");
            _context.RequestWindowResize(1280, 720);

            _gameScript?.Start(_context);
            _started = true;
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
            if (!_started || _context is null)
            {
                return 1;
            }

            float delta = 0.016f;
            if (args != IntPtr.Zero && sizeBytes >= sizeof(float))
            {
                delta = *(float*)args.ToPointer();
            }

            _time += delta;

            if (_gameScript is null)
            {
                float phase = (MathF.Sin(_time) + 1.0f) * 0.5f;
                _context.SetClearColor(0.05f + 0.15f * phase, 0.10f, 0.20f + 0.10f * (1.0f - phase), 1.0f);
                _context.SetPrimitiveType(((int)(_time * 0.8f) % 2 == 0) ? 1 : 2);
                return 0;
            }

            _gameScript.Update(_context, delta);
            return 0;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"ManagedUpdate error: {ex.Message}");
            return 1;
        }
    }

    private static void LoadGameScript()
    {
        string? configuredPath = Environment.GetEnvironmentVariable("ENGINE_GAME_DLL");
        string gameAssemblyPath = string.IsNullOrWhiteSpace(configuredPath)
            ? Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "GameProjects", "SandboxGame", "bin", "Debug", "net10.0", "SandboxGame.dll"))
            : configuredPath;

        if (!File.Exists(gameAssemblyPath))
        {
            Console.WriteLine($"Game assembly not found, fallback mode: {gameAssemblyPath}");
            return;
        }

        Assembly assembly = Assembly.LoadFrom(gameAssemblyPath);
        foreach (Type type in assembly.GetTypes())
        {
            if (!typeof(IGameScript).IsAssignableFrom(type) || type.IsAbstract || type.IsInterface)
            {
                continue;
            }

            _gameScript = (IGameScript?)Activator.CreateInstance(type);
            if (_gameScript != null)
            {
                Console.WriteLine($"Loaded game script: {type.FullName}");
                return;
            }
        }

        Console.WriteLine("No IGameScript implementation found in game assembly. Running fallback mode.");
    }
}
