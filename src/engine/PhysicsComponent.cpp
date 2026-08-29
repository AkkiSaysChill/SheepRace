#include "engine/PhysicsComponent.h"
#include "engine/PhysicsSystem.h"
#include "engine/GameObject.h"
#include "rlgl.h"
#include "raymath.h"
#include "BulletCollision/CollisionShapes/btShapeHull.h"

// Helper function to convert Raylib Vector3 to Bullet btVector3
static btVector3 toBullet(const Vector3& vec) {
    return btVector3(vec.x, vec.y, vec.z);
}

// Helper function to convert Bullet btVector3 to Raylib Vector3
static Vector3 toRaylib(const btVector3& vec) {
    return Vector3{ vec.x(), vec.y(), vec.z() };
}

// Helper function to convert Raylib Quaternion to Bullet btQuaternion
static btQuaternion toBullet(const Quaternion& q) {
    return btQuaternion(q.x, q.y, q.z, q.w);
}

// Helper function to convert Bullet btQuaternion to Raylib Quaternion
static Quaternion toRaylib(const btQuaternion& q) {
    return Quaternion{ q.x(), q.y(), q.z(), q.w() };
}

PhysicsComponent::PhysicsComponent(PhysicsSystem* physicsSystem, btCollisionShape* shape, PhysicsShapeType type, float mass, const Vector3& initialPosition, const Quaternion& initialRotation)
    : m_physicsSystem(physicsSystem), m_shape(shape), m_shapeType(type), m_owner(nullptr), m_debugHull(nullptr) { // <--- Initialized m_shapeType

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(toBullet(initialPosition));
    transform.setRotation(toBullet(initialRotation));

    m_motionState = new btDefaultMotionState(transform);

    btVector3 localInertia(0, 0, 0);
    if (mass != 0.0f) {
        m_shape->calculateLocalInertia(mass, localInertia);
    }

    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, m_motionState, m_shape, localInertia);
    m_rigidBody = new btRigidBody(rbInfo);
    m_rigidBody->setUserPointer(this);

    m_physicsSystem->AddPhysicsComponent(this);
}


PhysicsComponent::~PhysicsComponent() {
    m_physicsSystem->RemovePhysicsComponent(this);
    delete m_rigidBody;
    delete m_motionState;
    delete m_shape;

    if (m_debugHull) {
        delete m_debugHull;
    }
}

void PhysicsComponent::Update(float deltaTime) {
    if (m_owner) {
        btTransform transform;
        m_motionState->getWorldTransform(transform);
        m_owner->position = toRaylib(transform.getOrigin());
        m_owner->rotation = toRaylib(transform.getRotation());
    }
}

Vector3 PhysicsComponent::GetPosition() {
    btTransform transform;
    m_motionState->getWorldTransform(transform);
    return toRaylib(transform.getOrigin());
}

Quaternion PhysicsComponent::GetRotation() {
    btTransform transform;
    m_motionState->getWorldTransform(transform);
    return toRaylib(transform.getRotation());
}

void PhysicsComponent::DrawDebug() {
    if (!m_rigidBody || !m_shape) return;

    // Get World Transform
    btTransform trans = m_rigidBody->getWorldTransform();
    btVector3 pos = trans.getOrigin();
    btQuaternion rot = trans.getRotation();

    Vector3 axis;
    float angle;
    Quaternion q = { (float)rot.x(), (float)rot.y(), (float)rot.z(), (float)rot.w() };
    QuaternionToAxisAngle(q, &axis, &angle);

    rlPushMatrix();

    // NOTE: Concave shapes (Maps) usually have identity transforms because 
    // the vertices are already in world space. Convex shapes need transforms.
    if (m_shape->isConvex()) {
        rlTranslatef((float)pos.x(), (float)pos.y(), (float)pos.z());
        rlRotatef(angle * RAD2DEG, axis.x, axis.y, axis.z);
    }

    rlBegin(RL_LINES);
    rlColor4ub(0, 0, 0, 255); // color of the Wireframe

    // CONVEX SHAPES (Cube, Sphere, Player)
    if (m_shape->isConvex()) {
        if (m_debugHull == nullptr) {
            m_debugHull = new btShapeHull((btConvexShape*)m_shape);
            m_debugHull->buildHull(m_shape->getMargin());
        }

        const btVector3* vertices = m_debugHull->getVertexPointer();
        const unsigned int* indices = m_debugHull->getIndexPointer();
        int numTriangles = m_debugHull->numTriangles();

        for (int i = 0; i < numTriangles; i++) {
            int i1 = indices[i * 3];
            int i2 = indices[i * 3 + 1];
            int i3 = indices[i * 3 + 2];

            btVector3 v1 = vertices[i1];
            btVector3 v2 = vertices[i2];
            btVector3 v3 = vertices[i3];

            rlVertex3f(v1.x(), v1.y(), v1.z()); rlVertex3f(v2.x(), v2.y(), v2.z());
            rlVertex3f(v2.x(), v2.y(), v2.z()); rlVertex3f(v3.x(), v3.y(), v3.z());
            rlVertex3f(v3.x(), v3.y(), v3.z()); rlVertex3f(v1.x(), v1.y(), v1.z());
        }
    }
    // CONCAVE SHAPES (The Map) 
    else if (m_shape->isConcave()) {
        // cannot use btShapeHull here. We must iterate the triangles directly.
        btConcaveShape* concaveMesh = (btConcaveShape*)m_shape;

        DrawTriangleCallback drawCallback;

        // Define a range to draw (Everything in the world)
        btVector3 aabbMin(-10000, -10000, -10000);
        btVector3 aabbMax(10000, 10000, 10000);

        // This function calls 'processTriangle' inside our helper class for every triangle
        concaveMesh->processAllTriangles(&drawCallback, aabbMin, aabbMax);
    }

    rlEnd();
    rlPopMatrix();
}