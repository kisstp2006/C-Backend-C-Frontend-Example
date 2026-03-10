# Backend C++ DLL

Ez a mappa tartalmazza a C++ Backend DLL-t, amelyet a C# frontend használ.

## Build

1. Szükséges: CMake, Visual Studio (MSVC)
2. Parancs:
   ```
   cmake -S Backend -B Backend/build
   cmake --build Backend/build --config Release
   ```
3. A DLL a `Backend/build/Release/Backend.dll` helyen lesz.

## Exportált függvények
- `int AddNumbers(int a, int b)`

## Használat
Másold a DLL-t a C# bin/Debug/net10.0 mappába, hogy a C# program megtalálja.
