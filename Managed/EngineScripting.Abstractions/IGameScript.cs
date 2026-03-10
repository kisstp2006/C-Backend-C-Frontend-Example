namespace EngineScripting.Abstractions;

public interface IGameScript
{
    void Start(GameContext context);
    void Update(GameContext context, float deltaTime);
}
