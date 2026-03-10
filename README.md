# C++ Engine + C# Scripting Architecture

Ez a repository most 3 külön rétegre van szétválasztva:

1. `Backend/` (C++ engine)
- Natív engine komponensek (`Backend.dll`, `EngineHost.exe`)
- DX11 render loop, ablakkezelés, hostfxr indítás

2. `ScriptingHost/` + `Managed/EngineScripting.Abstractions/` (C++ <-> C# bridge)
- `ScriptingHost`: a managed belépési pont (`ManagedStart`, `ManagedUpdate`)
- `EngineScripting.Abstractions`: közös interfész (`IGameScript`, `GameContext`)

3. `GameProjects/<Nev>/` (generált C# játék projekt)
- A játéklogika itt van, `IGameScript` implementációval
- Engine által használható külön projekt

## Új játék projekt generálása

PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File Scripts\new-game-project.ps1 -Name SandboxGame
```

## Unified solution generálása (C++ + bridge + game)

```powershell
powershell -ExecutionPolicy Bypass -File Scripts\create-unified-sln.ps1
```

Ez létrehozza a `UnifiedHost.sln` fájlt, benne:
- `Backend` (C++)
- `EngineHost` (C++)
- `EngineScripting.Abstractions` (C#)
- `ScriptingHost` (C#)
- minden `GameProjects/**/*.csproj`

## Build sorrend

```powershell
dotnet build Managed\EngineScripting.Abstractions\EngineScripting.Abstractions.csproj
dotnet build ScriptingHost\ScriptingHost.csproj
dotnet build GameProjects\SandboxGame\SandboxGame.csproj
cmake -S Backend -B Backend/build
cmake --build Backend/build --config Debug
```

## Futtatás

```powershell
Backend\build\Debug\EngineHost.exe
```

Alapértelmezésben a host ezt keresi:
- `ScriptingHost/bin/Debug/net10.0/ScriptingHost.dll`
- `GameProjects/SandboxGame/bin/Debug/net10.0/SandboxGame.dll`

Másik játék DLL megadásához állítsd be:

```powershell
$env:ENGINE_GAME_DLL = "C:\...\GameProjects\MyGame\bin\Debug\net10.0\MyGame.dll"
Backend\build\Debug\EngineHost.exe
```

## ImGui support az EngineHostban

Az EngineHost támogatja a Dear ImGui overlayt (DX11 + Win32 backend).

1. ImGui letöltése:

```powershell
powershell -ExecutionPolicy Bypass -File Scripts\setup-imgui.ps1
```

2. Újrakonfigurálás és build:

```powershell
cmake -S Backend -B Backend/build
cmake --build Backend/build --config Debug
```

3. Futtatás:

```powershell
Backend\build\Debug\EngineHost.exe
```

Ha a `third_party/imgui` mappa létezik, az ImGui automatikusan engedélyezve lesz.

### Startup Project Launcher UI

ImGui bekapcsolt build esetén az EngineHost induláskor egy `Project Launcher` UI panelt mutat.

Itt tudsz:
- meglévő `GameProjects/*` projektet kiválasztani és megnyitni (`Open Selected Project`),
- új projektet generálni (`Generate New Project`),
- fallback módban indítani game DLL nélkül (`Run Fallback (No Game DLL)`).

Az `Open Selected Project` gomb futtat egy `dotnet build` lépést a kiválasztott projektre, majd a host azt tölti be.