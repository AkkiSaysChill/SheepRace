#include "engine/AnimationComponent.h"

AnimatorComponent::AnimatorComponent(Model* model, ModelAnimation* anims, int count)
    : m_model(model), m_anims(anims), m_animCount(count), m_currentIndex(0), m_currentFrame(0), m_isPlaying(true)
{
}

AnimatorComponent::~AnimatorComponent() {
    // Unload animations when component dies
    if (m_anims) {
        UnloadModelAnimations(m_anims, m_animCount);
    }
}

void AnimatorComponent::Update(float dt) {
    if (!m_isPlaying || m_animCount == 0) return;

    ModelAnimation& anim = m_anims[m_currentIndex];

    // Increment frame counter
    m_currentFrame++;

    // Loop animation
    if (m_currentFrame >= anim.frameCount) {
        m_currentFrame = 0;
    }

    // Raylib Magic: Update the bones of the 3D model to match this frame
    UpdateModelAnimation(*m_model, anim, m_currentFrame);
}

void AnimatorComponent::PlayAnimation(int index) {
    if (index >= 0 && index < m_animCount) {
        m_currentIndex = index;
        m_currentFrame = 0;
        m_isPlaying = true;
    }
}

void AnimatorComponent::Stop() {
    m_isPlaying = false;
}