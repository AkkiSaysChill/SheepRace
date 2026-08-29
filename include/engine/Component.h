#pragma once

class GameObject; // Forward declaration

class Component {
public:
    virtual ~Component() = default;
    virtual void Update(float deltaTime) {}

    void SetOwner(GameObject* owner) { m_owner = owner; }
    GameObject* GetOwner() { return m_owner; }

protected:
    GameObject* m_owner = nullptr;
};
