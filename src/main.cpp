#include "engine/Engine.h"
#include "raylib.h"
#include <memory>

void RegisterGameScenes(Engine &engine);

int main() {
  Engine engine;

  engine.Initialize();

  RegisterGameScenes(engine);

  engine.Run();
  engine.Shutdown();

  return 0;
}
