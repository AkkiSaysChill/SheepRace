#pragma once

#include "engine/Scene3D.h"
#include "raylib.h"
#include <memory>

class GameObject;

class VolumetricCloudsScene : public Scene3D {
public:
  VolumetricCloudsScene();
  ~VolumetricCloudsScene() override;

  void Initialize(Engine *engine) override;
  void Update(float deltaTime) override;
  void Draw3D() override;
  void Draw() override;
  void Shutdown() override;

private:
  Engine *m_engine;
  std::shared_ptr<GameObject> m_sampleObject;
  Model m_sampleModel;
  bool m_modelLoaded;
};
