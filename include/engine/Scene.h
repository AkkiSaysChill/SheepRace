#pragma once
class Engine;

class Scene {
public:
  virtual ~Scene() = default;
  virtual void Initialize(Engine *engine) = 0;
  virtual void Update(float deltaTime) = 0;
  virtual void Draw3D() {} // <-- ADD THIS (not pure virtual, default is no-op)
  virtual void Draw() = 0;
  virtual void Shutdown() = 0;
  virtual void OnResize(int width, int height) {}
};
