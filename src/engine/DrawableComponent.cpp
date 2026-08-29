#include "engine/DrawableComponent.h"
#include "engine/GameObject.h"
#include "raymath.h"

DrawableComponent::DrawableComponent(Model model, bool shouldUnload)
    : m_model(model), m_shouldUnload(shouldUnload) {
}

DrawableComponent::~DrawableComponent() {
    // ONLY unload if we own this model (like created Cubes/Spheres)
    // Do NOT unload if it came from AssetLoader (like the Map or Player)
    if (m_shouldUnload) {
        UnloadModel(m_model);
    }
}

void DrawableComponent::Draw() {
    if (m_owner) {
        Vector3 axis;
        float angle;
        QuaternionToAxisAngle(m_owner->rotation, &axis, &angle);
        DrawModelEx(m_model, m_owner->position, axis, angle * RAD2DEG, m_owner->scale, WHITE);
    }
}
