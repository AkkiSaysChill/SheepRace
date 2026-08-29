#pragma once

#include <btBulletDynamicsCommon.h>
#include <vector>
#include <memory>

class PhysicsComponent; // Forward declaration

class PhysicsSystem {
public:
    PhysicsSystem();
    ~PhysicsSystem();

    void setG(float newG);

    void Update(float deltaTime);
    void AddPhysicsComponent(PhysicsComponent* component);
    void RemovePhysicsComponent(PhysicsComponent* component);

    btDiscreteDynamicsWorld* GetDynamicsWorld() { return m_dynamicsWorld.get(); }

private:
    float g;

    std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfiguration;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_overlappingPairCache;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_dynamicsWorld;

    std::vector<PhysicsComponent*> m_physicsComponents;
};
