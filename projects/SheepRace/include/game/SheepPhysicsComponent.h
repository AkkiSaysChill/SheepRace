#pragma once

#include "engine/PhysicsComponent.h"

class SheepPhysicsComponent : public PhysicsComponent {
public:
    //using PhysicsComponent::PhysicsComponent;
    SheepPhysicsComponent(PhysicsSystem* system, btCollisionShape* shape, PhysicsShapeType type, float mass, Vector3 pos, Quaternion rot);

    void OnCollision(PhysicsComponent* other) override;
};
