using System;

namespace EngineScripting.Abstractions;

public sealed class GameContext
{
    private readonly Action<string> _setWindowTitle;
    private readonly Action<int, int> _requestWindowResize;
    private readonly Action<float, float, float, float> _setClearColor;
    private readonly Action<int> _setPrimitiveType;

    public GameContext(
        Action<string> setWindowTitle,
        Action<int, int> requestWindowResize,
        Action<float, float, float, float> setClearColor,
        Action<int> setPrimitiveType)
    {
        _setWindowTitle = setWindowTitle;
        _requestWindowResize = requestWindowResize;
        _setClearColor = setClearColor;
        _setPrimitiveType = setPrimitiveType;
    }

    public void SetWindowTitle(string title) => _setWindowTitle(title);

    public void RequestWindowResize(int width, int height) => _requestWindowResize(width, height);

    public void SetClearColor(float r, float g, float b, float a) => _setClearColor(r, g, b, a);

    public void SetPrimitiveType(int primitiveType) => _setPrimitiveType(primitiveType);
}
