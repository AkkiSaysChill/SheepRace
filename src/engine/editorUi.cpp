#include "engine/editorUi.h"
#include "engine/AnimationComponent.h"
#include "engine/DrawableComponent.h"
#include "engine/GameObject.h"
#include "engine/PhysicsComponent.h"
#include "imgui.h"
#include "rlImGui.h"
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btConvexHullShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>

void EditorUI::RenderHierarchy(GameObjectManager &scene,
                               PhysicsSystem &physicsSystem,
                               Shader defaultShader, PlayerProgress &m_progress,
                               AssetPacking::AssetLoader &assetLoader) {

  ImGui::Begin("Scene Hierarchy");

  const auto &gameObjects = scene.GetGameObjects();
  static int selectedIndex = -1;

  if (ImGui::Button("Create...")) {
    ImGui::OpenPopup("create_object_popup");
  }

  if (ImGui::BeginPopup("create_object_popup")) {

    // Empty Object
    if (ImGui::Selectable("Empty Object")) {
      auto newObj = std::make_shared<GameObject>();
      newObj->name = "Empty Object";
      newObj->position = {0, 5, 0};
      scene.AddGameObject(newObj);
    }

    // Create Cube
    if (ImGui::Selectable("Cube")) {
      auto newObj = std::make_shared<GameObject>();
      newObj->name = "Cube";
      newObj->position = {0, 5, 0};
      newObj->scale = {1, 1, 1};

      Mesh mesh = GenMeshCube(1.0f, 1.0f, 1.0f);
      Model model = LoadModelFromMesh(mesh);
      model.materials[0].shader = defaultShader;

      newObj->AddComponent<DrawableComponent>(model, true);

      btCollisionShape *shape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
      newObj->AddComponent<PhysicsComponent>(
          &physicsSystem, shape, PhysicsShapeType::BOX, 1.0f, newObj->position,
          newObj->rotation);

      scene.AddGameObject(newObj);
    }

    // Create Sphere
    if (ImGui::Selectable("Sphere")) {
      auto newObj = std::make_shared<GameObject>();
      newObj->name = "Sphere";
      newObj->position = {0, 5, 0};
      newObj->scale = {1, 1, 1};

      Mesh mesh = GenMeshSphere(0.5f, 16, 16);
      Model model = LoadModelFromMesh(mesh);
      model.materials[0].shader = defaultShader;

      newObj->AddComponent<DrawableComponent>(model, true);

      btCollisionShape *shape = new btSphereShape(0.5f);
      newObj->AddComponent<PhysicsComponent>(
          &physicsSystem, shape, PhysicsShapeType::SPHERE, 1.0f,
          newObj->position, newObj->rotation);

      scene.AddGameObject(newObj);
    }

    ImGui::EndPopup();
  }

  ImGui::Separator();

  if (ImGui::CollapsingHeader("Player Stats (Save Data)")) {
    ImGui::Text("Currency: %d", m_progress.totalCurrency);
    ImGui::Text("XP: %d", m_progress.totalXP);

    for (size_t i = 0; i < m_progress.levels.size(); i++) {
      const auto &lvl = m_progress.levels[i];
      ImGui::Text("Level %d: %s (Stars: %d)", i,
                  lvl.unlocked ? "Unlocked" : "Locked", lvl.stars);
    }

    if (ImGui::Button("add xp")) {
      m_progress.AddXp(100.0f);
    }

    if (ImGui::Button("Save Game")) {
      m_progress.Save("player.save");
    }
  }

  if (ImGui::BeginChild("ObjectList", ImVec2(0, 200), true)) {
    int id = 0;
    for (const auto &obj : gameObjects) {
      if (!obj) {
        id++;
        continue;
      }

      ImGui::PushID(id);

      std::string label =
          obj->name.empty() ? "GameObject " + std::to_string(id) : obj->name;

      if (ImGui::Selectable(label.c_str(), selectedIndex == id)) {
        selectedIndex = id;
      }

      // Context Menu (Right Click Delete)
      if (ImGui::BeginPopupContextItem()) {
        if (ImGui::Selectable("Delete")) {
          scene.RemoveGameObject(obj);
          if (selectedIndex == id)
            selectedIndex = -1;
        }
        ImGui::EndPopup();
      }

      ImGui::PopID();
      id++;
    }
  }
  ImGui::EndChild();

  ImGui::Separator();

  if (selectedIndex >= 0 && selectedIndex < gameObjects.size()) {
    auto obj = gameObjects[selectedIndex];
    auto drawable = obj->GetComponent<DrawableComponent>();

    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Inspector: %s", obj->name.c_str());

    // Transform Controls
    ImGui::DragFloat3("Position", (float *)&obj->position, 0.1f);
    ImGui::DragFloat3("Scale", (float *)&obj->scale, 0.01f);
    ImGui::Spacing();
    ImGui::Checkbox("Show Collider Wireframe", &obj->debugPhysics);

    ImGui::Separator();
    ImGui::Text("Physics");

    static float bodyMass = 1.0f;
    ImGui::DragFloat("Mass (0=Static)", &bodyMass, 0.1f, 0.0f, 100.0f);

    if (obj->GetComponent<PhysicsComponent>()) {
      ImGui::TextColored(ImVec4(1, 0, 0, 1), "Physics Component Active");
    } else {
      // Box Collider
      if (ImGui::Button("Add Box Collider")) {
        btCollisionShape *shape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
        shape->setLocalScaling(
            btVector3(obj->scale.x, obj->scale.y, obj->scale.z));
        obj->AddComponent<PhysicsComponent>(&physicsSystem, shape,
                                            PhysicsShapeType::BOX, bodyMass,
                                            obj->position, obj->rotation);
      }
      ImGui::SameLine();

      // Sphere Collider
      if (ImGui::Button("Add Sphere Collider")) {
        btCollisionShape *shape = new btSphereShape(0.5f);
        shape->setLocalScaling(
            btVector3(obj->scale.x, obj->scale.y, obj->scale.z));
        obj->AddComponent<PhysicsComponent>(&physicsSystem, shape,
                                            PhysicsShapeType::SPHERE, bodyMass,
                                            obj->position, obj->rotation);
      }

      // Convex Hull (Mesh) Collider
      if (drawable) {
        if (ImGui::Button("Add Mesh Collider")) {
          Model model = drawable->GetModel();
          btConvexHullShape *shape = new btConvexHullShape();

          // Extract vertices
          for (int i = 0; i < model.meshCount; i++) {
            Mesh mesh = model.meshes[i];
            for (int v = 0; v < mesh.vertexCount; v++) {
              // Extract X, Y, Z
              float x = mesh.vertices[v * 3];
              float y = mesh.vertices[v * 3 + 1];
              float z = mesh.vertices[v * 3 + 2];
              shape->addPoint(btVector3(x, y, z));
            }
          }
          shape->setLocalScaling(
              btVector3(obj->scale.x, obj->scale.y, obj->scale.z));
          shape->optimizeConvexHull();

          obj->AddComponent<PhysicsComponent>(&physicsSystem, shape,
                                              PhysicsShapeType::HULL, bodyMass,
                                              obj->position, obj->rotation);
        }
      } else {
        ImGui::TextDisabled("Add Drawable to enable Mesh Collider");
      }
    }

    ImGui::Separator();
    ImGui::Text("Animation");

    auto animator = obj->GetComponent<AnimatorComponent>();

    if (animator) {
      ImGui::TextColored(ImVec4(0, 1, 0, 1), "Animator Active");

      // stop button
      if (ImGui::Button("Stop")) {
        animator->Stop();
      }

      ImGui::SameLine();

      if (ImGui::Button("Play")) {
        animator->PlayAnimation(0); // Plays the 1st animation
      }

      // switch animation for models with multiple clips
      static int animIndexToPlay = 0;

      ImGui::Spacing();
      ImGui::SetNextItemWidth(80); // Make the input box small
      ImGui::InputInt("Index", &animIndexToPlay);

      ImGui::SameLine();

      if (ImGui::Button("Switch Anim")) {
        // Play the specific index you typed
        animator->PlayAnimation(animIndexToPlay);
      }
    } else {
      // Only allow adding Animator if we have a Model
      if (drawable) {
        static char animFileName[128] = "runner.glb";
        ImGui::InputText("Anim File", animFileName, 128);

        if (ImGui::Button("Add Animator Component")) {
          // get asset data
          auto data = assetLoader.getAssetData(animFileName);

          if (!data.empty()) {
            // load Animations
            int animCount = 0;
            ModelAnimation *anims =
                assetLoader.LoadAnimationsFromAssetData(data, &animCount);

            if (animCount > 0) {

              obj->AddComponent<AnimatorComponent>(&drawable->GetModelRef(),
                                                   anims, animCount);
            } else {
              std::cout << "[Editor] No animations found in file: "
                        << animFileName << std::endl;
            }
          } else {
            std::cout << "[Editor] Asset not found in pack: " << animFileName
                      << std::endl;
          }
        }
      } else {
        ImGui::TextDisabled("Add Drawable to enable Animation");
      }
    }

    ImGui::Separator();
    ImGui::Text("Visuals");

    if (!drawable) {
      if (ImGui::Button("Add Cube Mesh")) {
        Mesh mesh = GenMeshCube(1.0f, 1.0f, 1.0f);
        Model model = LoadModelFromMesh(mesh);
        model.materials[0].shader = defaultShader;
        obj->AddComponent<DrawableComponent>(model, true);
      }
    } else {
      ImGui::TextColored(ImVec4(0, 1, 0, 1), "Drawable Component Active");
    }
  } else {
    ImGui::Text("Select an object to edit properties.");
  }

  ImGui::End();
}
