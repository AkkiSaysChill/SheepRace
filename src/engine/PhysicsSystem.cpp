#include "engine/PhysicsSystem.h"
#include "engine/PhysicsComponent.h"
#include <algorithm>

PhysicsSystem::PhysicsSystem() {
  // rerences to btphysics objs
  m_collisionConfiguration =
      std::make_unique<btDefaultCollisionConfiguration>();
  m_dispatcher =
      std::make_unique<btCollisionDispatcher>(m_collisionConfiguration.get());
  m_overlappingPairCache = std::make_unique<btDbvtBroadphase>();
  m_solver = std::make_unique<btSequentialImpulseConstraintSolver>();
  m_dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
      m_dispatcher.get(), m_overlappingPairCache.get(), m_solver.get(),
      m_collisionConfiguration.get());
  g = 10;

  // gravity baby
  m_dynamicsWorld->setGravity(btVector3(0, -10, 0));
}

void PhysicsSystem::setG(float g) {
  m_dynamicsWorld->setGravity(btVector3(0, -g, 0));
}

PhysicsSystem::~PhysicsSystem() {
  // The unique_ptrs will handle cleanup automatically
}

void PhysicsSystem::Update(float deltaTime) {
  m_dynamicsWorld->stepSimulation(deltaTime, 10);

  int numManifolds = m_dynamicsWorld->getDispatcher()->getNumManifolds();
  for (int i = 0; i < numManifolds; i++) {
    btPersistentManifold *contactManifold =
        m_dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
    const btCollisionObject *obA = contactManifold->getBody0();
    const btCollisionObject *obB = contactManifold->getBody1();

    PhysicsComponent *pcA =
        static_cast<PhysicsComponent *>(obA->getUserPointer());
    PhysicsComponent *pcB =
        static_cast<PhysicsComponent *>(obB->getUserPointer());

    if (pcA && pcB) {
      pcA->OnCollision(pcB);
      pcB->OnCollision(pcA);
    }
  }

  for (auto &component : m_physicsComponents) {
    component->Update(deltaTime);
  }
}

void PhysicsSystem::AddPhysicsComponent(PhysicsComponent *component) {
  m_physicsComponents.push_back(component);
  m_dynamicsWorld->addRigidBody(component->GetRigidBody());
}

void PhysicsSystem::RemovePhysicsComponent(PhysicsComponent *component) {
  auto it = std::find(m_physicsComponents.begin(), m_physicsComponents.end(),
                      component);
  if (it != m_physicsComponents.end()) {
    m_physicsComponents.erase(it);

    if (m_dynamicsWorld != nullptr && component != nullptr) {
      btRigidBody *body = component->GetRigidBody();
      if (body != nullptr) {
        m_dynamicsWorld->removeRigidBody(body);
      }
    }
  }
}
