#pragma once
#include "Component.h"
#include "raylib.h"
#include "DrawableComponent.h"

class AnimatorComponent : public Component {
public:
    AnimatorComponent(Model* model, ModelAnimation* anims, int count);
    ~AnimatorComponent();

    void Update(float dt) override;

    void PlayAnimation(int index);
    void Stop();

private:
    Model* m_model;             // Pointer to the model inside DrawableComponent
    ModelAnimation* m_anims;    // Array of animations
    int m_animCount;

    int m_currentIndex;         // Which animation is playing
    int m_currentFrame;         // Current frame of animation
    bool m_isPlaying;
};