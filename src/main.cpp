#include "engine/Engine.h"
#include "raylib.h"
#include <memory>

void RegisterGameScenes(Engine &engine);

int main() {
  Engine engine;
  InitAudioDevice();

  engine.Initialize();

  RegisterGameScenes(engine);

  engine.Run();
  CloseAudioDevice();
  engine.Shutdown();

  return 0;
}
