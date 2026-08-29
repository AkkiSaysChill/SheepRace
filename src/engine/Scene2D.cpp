#include "engine/Scene2D.h"
#include "engine/Engine.h"

void Scene2D::Initialize(Engine* engine) {
    // Configure the engine's camera for 2D orthographic mode
    engine->GetCamera().projection = CAMERA_ORTHOGRAPHIC;
}
