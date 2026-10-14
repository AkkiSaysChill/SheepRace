#pragma once
#include "raylib.h"
#include "raymath.h"
#include <string>

struct CameraKeyframe {
  Vector3 eyeStart;
  Vector3 eyeEnd;
  Vector3 targetStart;
  Vector3 targetEnd;
  float duration;
};

class CutsceneController {
public:
  static CutsceneController &GetInstance() {
    static CutsceneController instance;
    return instance;
  }

  void PlayCutscene(const std::string &cutsceneId, CameraKeyframe keyframe) {
    m_active = true;
    m_currentId = cutsceneId;
    m_keyframe = keyframe;
    m_elapsed = 0.0f;
  }

  void Update(float deltaTime, Camera3D &camera) {
    if (!m_active)
      return;

    m_elapsed += deltaTime;
    float t = Clamp(m_elapsed / m_keyframe.duration, 0.0f, 1.0f);

    // Smooth ease-in-out curve
    float smoothT = t * t * (3.0f - 2.0f * t);

    camera.position =
        Vector3Lerp(m_keyframe.eyeStart, m_keyframe.eyeEnd, smoothT);
    camera.target =
        Vector3Lerp(m_keyframe.targetStart, m_keyframe.targetEnd, smoothT);

    if (m_elapsed >= m_keyframe.duration) {
      m_active = false;
    }
  }

  bool IsPlaying() const { return m_active; }
  const std::string &GetCurrentCutsceneId() const { return m_currentId; }

private:
  bool m_active = false;
  std::string m_currentId;
  CameraKeyframe m_keyframe;
  float m_elapsed = 0.0f;
};
