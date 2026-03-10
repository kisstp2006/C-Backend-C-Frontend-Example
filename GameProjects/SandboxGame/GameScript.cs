using EngineScripting.Abstractions;

namespace SandboxGame;

public sealed class GameScript : IGameScript
{
    private float _time;

    public void Start(GameContext context)
    {
        context.SetWindowTitle("SandboxGame");
        context.RequestWindowResize(1280, 720);
        context.SetPrimitiveType(1);
    }

    public void Update(GameContext context, float deltaTime)
    {
        _time += deltaTime;
        float pulse = (System.MathF.Sin(_time) + 1.0f) * 0.5f;
        context.SetClearColor(0.10f, 0.08f + pulse * 0.25f, 0.18f, 1.0f);
        context.SetPrimitiveType(((int)(_time * 1.2f) % 2 == 0) ? 1 : 2);
    }
}

