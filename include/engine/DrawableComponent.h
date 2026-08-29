#pragma once

#include "Component.h"
#include "raylib.h"

class DrawableComponent : public Component {
public:
    DrawableComponent(Model model, bool shouldUnload = false);
    ~DrawableComponent();

    void Draw();

    Model& GetModel() { return m_model; }
    Model& GetModelRef() { return m_model; }

private:
    Model m_model;
    bool m_shouldUnload;
};
