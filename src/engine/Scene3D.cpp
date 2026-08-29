#include "engine/Scene3D.h"
#include "engine/Engine.h"

void Scene3D::Initialize(Engine* engine) {
    // Configure the engine's camera for 3D perspective mode
    engine->GetCamera().projection = CAMERA_PERSPECTIVE;
}
