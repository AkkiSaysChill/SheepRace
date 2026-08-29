#include "game/SheepPhysicsComponent.h"
#include <iostream>


SheepPhysicsComponent::SheepPhysicsComponent(PhysicsSystem* system, btCollisionShape* shape, PhysicsShapeType type, float mass, Vector3 pos, Quaternion rot)
    : PhysicsComponent(system, shape, type, mass, pos, rot) // <--- Pass 'type' to parent here
{
    // ... your existing vehicle setup code ...
}

void SheepPhysicsComponent::OnCollision(PhysicsComponent* other) {
// add custom physical behaviour for the sheep later 
}
