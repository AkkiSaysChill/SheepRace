#pragma once

#include "engine/Component.h"
#include <btBulletDynamicsCommon.h>
#include "BulletCollision/CollisionShapes/btShapeHull.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

class PhysicsSystem; // Forward declaration
class GameObject; // Forward declaration

enum class PhysicsShapeType {
    BOX,
    SPHERE,
    CAPSULE,
    HULL,
    MESH
};

// Helper class to draw Concave Meshes (like the Map)
class DrawTriangleCallback : public btTriangleCallback {
public:
    void processTriangle(btVector3* triangle, int partId, int triangleIndex) override {
        // Draw the 3 lines of the triangle
        btVector3 v1 = triangle[0];
        btVector3 v2 = triangle[1];
        btVector3 v3 = triangle[2];

        rlVertex3f(v1.x(), v1.y(), v1.z()); rlVertex3f(v2.x(), v2.y(), v2.z());
        rlVertex3f(v2.x(), v2.y(), v2.z()); rlVertex3f(v3.x(), v3.y(), v3.z());
        rlVertex3f(v3.x(), v3.y(), v3.z()); rlVertex3f(v1.x(), v1.y(), v1.z());
    }
};

class PhysicsComponent : public Component {
public:
    PhysicsComponent(PhysicsSystem* physicsSystem, btCollisionShape* shape, PhysicsShapeType type, float mass, const Vector3& initialPosition, const Quaternion& initialRotation);

    virtual ~PhysicsComponent();

    void Update(float deltaTime) override;
    void DrawDebug();

    btRigidBody* GetRigidBody() { return m_rigidBody; }


    PhysicsShapeType GetShapeType() const { return m_shapeType; }

    Vector3 GetPosition();
    Quaternion GetRotation();

    void SetOwner(GameObject* owner) { m_owner = owner; }
    virtual void OnCollision(PhysicsComponent* other) {}

private:
    PhysicsSystem* m_physicsSystem;
    btRigidBody* m_rigidBody;
    btDefaultMotionState* m_motionState;
    btCollisionShape* m_shape;
    GameObject* m_owner;

    PhysicsShapeType m_shapeType;

    btShapeHull* m_debugHull;
};