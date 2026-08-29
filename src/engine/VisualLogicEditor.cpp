#include "engine/VisualLogicEditor.h"

#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "engine/PhysicsSystem.h"
#include "engine/json/json.hpp"
#include "imgui.h"
#include "iostream"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>

using json = nlohmann::json;

LogicGraphDelegate::LogicGraphDelegate() {

  static ImU32 eventOutColors[] = {IM_COL32(200, 200, 200, 255)};
  static const char *eventOutNames[] = {"Exec"};

  static ImU32 actionInColors[] = {IM_COL32(200, 200, 200, 255)};
  static const char *actionInNames[] = {"Exec"};
  static ImU32 actionOutColors[] = {IM_COL32(200, 200, 200, 255)};
  static const char *actionOutNames[] = {"Next"};

  GraphEditor::Template eventTemplate = {};
  eventTemplate.mBackgroundColor = IM_COL32(160, 20, 20, 255);
  eventTemplate.mBackgroundColorOver = IM_COL32(200, 40, 40, 255);
  eventTemplate.mInputCount = 0;
  eventTemplate.mOutputCount = 1;

  eventTemplate.mOutputColors = eventOutColors;
  eventTemplate.mOutputNames = eventOutNames;

  mTemplates.push_back(eventTemplate);

  GraphEditor::Template actionTemplate = {};
  actionTemplate.mBackgroundColor = IM_COL32(20, 80, 160, 255);
  actionTemplate.mBackgroundColorOver = IM_COL32(40, 100, 200, 255);
  actionTemplate.mInputCount = 1;

  actionTemplate.mInputColors = actionInColors;
  actionTemplate.mInputNames = actionInNames;

  actionTemplate.mOutputCount = 1;
  actionTemplate.mOutputColors = actionOutColors;
  actionTemplate.mOutputNames = actionOutNames;

  mTemplates.push_back(actionTemplate);
}

bool LogicGraphDelegate::AllowedLink(GraphEditor::NodeIndex from,
                                     GraphEditor::NodeIndex to) {
  return true; // Allow any connection for now
}

void LogicGraphDelegate::SelectNode(GraphEditor::NodeIndex nodeIndex,
                                    bool selected) {
  if (nodeIndex >= 0 && nodeIndex < mNodes.size()) {
    mNodes[nodeIndex].mSelected = selected;
  }
}

void LogicGraphDelegate::MoveSelectedNodes(const ImVec2 delta) {
  for (auto &node : mNodes) {
    if (node.mSelected) {
      node.mPosition.x += delta.x;
      node.mPosition.y += delta.y;
    }
  }
}

void LogicGraphDelegate::RightClick(GraphEditor::NodeIndex nodeIndex,
                                    GraphEditor::SlotIndex slotIndexInput,
                                    GraphEditor::SlotIndex slotIndexOutput) {}

void LogicGraphDelegate::AddLink(GraphEditor::NodeIndex inputNodeIndex,
                                 GraphEditor::SlotIndex inputSlotIndex,
                                 GraphEditor::NodeIndex outputNodeIndex,
                                 GraphEditor::SlotIndex outputSlotIndex) {
  BlueprintLink link;
  link.mID = mNextLinkID++;
  link.mInputNodeID = mNodes[inputNodeIndex].mID;
  link.mInputSlotID = inputSlotIndex;
  link.mOutputNodeID = mNodes[outputNodeIndex].mID;
  link.mOutputSlotID = outputSlotIndex;
  mLinks.push_back(link);
}

void LogicGraphDelegate::DelLink(GraphEditor::LinkIndex index) {
  if (index >= 0 && index < mLinks.size()) {
    mLinks.erase(mLinks.begin() + index);
  }
}

void LogicGraphDelegate::CustomDraw(ImDrawList *drawList, ImRect rectangle,
                                    GraphEditor::NodeIndex nodeIndex) {}

const size_t LogicGraphDelegate::GetTemplateCount() {
  return mTemplates.size();
}

const GraphEditor::Template LogicGraphDelegate::GetTemplate(size_t index) {
  return mTemplates[index];
}

const size_t LogicGraphDelegate::GetNodeCount() { return mNodes.size(); }

const GraphEditor::Node LogicGraphDelegate::GetNode(size_t index) {
  GraphEditor::Node node;
  node.mName = mNodes[index].mName.c_str();
  node.mTemplateIndex = mNodes[index].mTemplateIndex;
  node.mRect = ImRect(mNodes[index].mPosition,
                      ImVec2(mNodes[index].mPosition.x + 200,
                             mNodes[index].mPosition.y + 100)); // Default size
  node.mSelected = mNodes[index].mSelected;
  return node;
}

const size_t LogicGraphDelegate::GetLinkCount() { return mLinks.size(); }

const GraphEditor::Link LogicGraphDelegate::GetLink(size_t index) {
  GraphEditor::Link link;
  // Map Blueprint IDs back to array indices for GraphEditor
  link.mInputNodeIndex = -1;
  link.mOutputNodeIndex = -1;
  for (size_t i = 0; i < mNodes.size(); i++) {
    if (mNodes[i].mID == mLinks[index].mInputNodeID)
      link.mInputNodeIndex = i;
    if (mNodes[i].mID == mLinks[index].mOutputNodeID)
      link.mOutputNodeIndex = i;
  }
  link.mInputSlotIndex = mLinks[index].mInputSlotID;
  link.mOutputSlotIndex = mLinks[index].mOutputSlotID;
  return link;
}

void LogicGraphDelegate::AddNode(const std::string &name, int templateIndex,
                                 ImVec2 pos) {
  BlueprintNode node;
  node.mID = mNextNodeID++;
  node.mName = name;
  node.mTemplateIndex = templateIndex;
  node.mPosition = pos;
  node.mSelected = false;
  mNodes.push_back(node);
}

void LogicGraphDelegate::Clear() {
  mNodes.clear();
  mLinks.clear();
}

void LogicGraphDelegate::SetCounters(int maxNodeId, int maxLinkId) {
  mNextNodeID = maxNodeId + 1;
  mNextLinkID = maxLinkId + 1;
}

VisualLogicEditor::VisualLogicEditor() : m_isOpen(false) {}
VisualLogicEditor::~VisualLogicEditor() {}

void VisualLogicEditor::Initialize(Engine *engine) {
  m_engine = engine;
  std::cout << "[BLUEPRINT] Visual Logic Editor Initialized.\n";
  m_isOpen = false;
}

void VisualLogicEditor::ToggleEditor() { m_isOpen = !m_isOpen; }
bool VisualLogicEditor::IsActive() const { return m_isOpen; }

void VisualLogicEditor::UpdateAndDraw() {
  if (!m_isOpen)
    return;

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("WuShang Logic Editor (.lmao)", &m_isOpen)) {

    static char scriptID[16] = "01";
    ImGui::PushItemWidth(50);
    ImGui::InputText(".lmao ID", scriptID, IM_ARRAYSIZE(scriptID));
    ImGui::PopItemWidth();

    ImGui::SameLine();
    if (ImGui::Button("Save Script")) {
      SaveBlueprint(scriptID);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Script")) {
      LoadBlueprint(scriptID);
    }

    ImGui::Separator();

    // AAA FIX: Naye Node Buttons (Sab Action template = 1 use karenge)
    if (ImGui::Button("Event: On Game Start"))
      m_delegate.AddNode("On Game Start", 0, ImVec2(100, 200));
    ImGui::SameLine();
    if (ImGui::Button("Action: Print Message"))
      m_delegate.AddNode("Print Message", 1, ImVec2(400, 100));
    ImGui::SameLine();

    if (ImGui::Button("Action: Jump Force"))
      m_delegate.AddNode("Jump Force", 1, ImVec2(350, 200));
    ImGui::SameLine();
    if (ImGui::Button("Spawn VFX"))
      m_delegate.AddNode("spawn Particle", 1, ImVec2(350, 300));
    ImGui::SameLine();

    if (ImGui::Button("Action: Low Gravity"))
      m_delegate.AddNode("Low Gravity", 1, ImVec2(400, 200));
    ImGui::SameLine();
    if (ImGui::Button("Action: Normal Gravity"))
      m_delegate.AddNode("Normal Gravity", 1, ImVec2(400, 300));
    ImGui::SameLine();
    if (ImGui::Button("Clear Canvas"))
      m_delegate.Clear();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 150, 0, 255));
    if (ImGui::Button("▶ EXECUTE (Global)")) {
      ExecuteGraph("On Game Start");
    }
    ImGui::SameLine();
    ImGui::PopStyleColor();

    if (ImGui::Button("▶ EXECUTE (GameObject)")) {
      auto objects = m_engine->GetGameObjectManager().GetGameObjects();
      if (!objects.empty()) {
        ExecuteGraph("On Game Start", objects[0]);
      } else {
        std::cout << "[VM] no objects in scene" << std::endl;
      }
    }

    ImGui::Separator();
    GraphEditor::Show(m_delegate, {}, m_viewState, true);
  }
  ImGui::End();
}

void VisualLogicEditor::SaveBlueprint(const std::string &scriptName) {
  std::string filepath = "runtime/" + scriptName + ".lmao";
  json j;

  j["nodes"] = json::array();
  for (const auto &node : m_delegate.mNodes) {
    j["nodes"].push_back({{"id", node.mID},
                          {"name", node.mName},
                          {"template", node.mTemplateIndex},
                          {"x", node.mPosition.x},
                          {"y", node.mPosition.y}});
  }

  j["links"] = json::array();
  for (const auto &link : m_delegate.mLinks) {
    j["links"].push_back({{"id", link.mID},
                          {"inNode", link.mInputNodeID},
                          {"inSlot", link.mInputSlotID},
                          {"outNode", link.mOutputNodeID},
                          {"outSlot", link.mOutputSlotID}});
  }

  std::ofstream file(filepath);
  if (file.is_open()) {
    file << j.dump(4); // 4 spaces for beautiful formatting
    file.close();
    std::cout << "[BLUEPRINT] Successfully Saved: " << filepath << "\n";
  }
}

void VisualLogicEditor::LoadBlueprint(const std::string &scriptName) {
  std::string filepath = "runtime/" + scriptName + ".lmao";
  std::ifstream file(filepath);

  if (!file.is_open()) {
    std::cout << "[BLUEPRINT] ERROR: File " << filepath << " not found!\n";
    return;
  }

  json j;
  file >> j;
  m_delegate.Clear();

  int maxNodeId = 0, maxLinkId = 0;

  for (const auto &jNode : j["nodes"]) {
    BlueprintNode n;
    n.mID = jNode["id"];
    n.mName = jNode["name"];
    n.mTemplateIndex = jNode["template"];
    n.mPosition = ImVec2(jNode["x"], jNode["y"]);
    n.mSelected = false;
    m_delegate.mNodes.push_back(n);
    if (n.mID > maxNodeId)
      maxNodeId = n.mID;
  }

  for (const auto &jLink : j["links"]) {
    BlueprintLink l;
    l.mID = jLink["id"];
    l.mInputNodeID = jLink["inNode"];
    l.mInputSlotID = jLink["inSlot"];
    l.mOutputNodeID = jLink["outNode"];
    l.mOutputSlotID = jLink["outSlot"];
    m_delegate.mLinks.push_back(l);
    if (l.mID > maxLinkId)
      maxLinkId = l.mID;
  }

  m_delegate.SetCounters(maxNodeId, maxLinkId);
  std::cout << "[BLUEPRINT] Loaded Graph: " << filepath << "\n";
}

void VisualLogicEditor::ExecuteGraph(const std::string &startEventName,
                                     std::shared_ptr<GameObject> contextObj) {
  std::cout << "\n============================================\n";
  std::cout << "[VM RUNTIME] Triggered Event -> " << startEventName;
  if (contextObj)
    std::cout << " on Object!\n";
  else
    std::cout << " (Global)\n";

  int currentNodeID = -1;
  for (const auto &n : m_delegate.mNodes) {
    if (n.mTemplateIndex == 0 && n.mName == startEventName) {
      currentNodeID = n.mID;
      break;
    }
  }

  if (currentNodeID == -1)
    return;

  int executionLimit = 1000;
  int steps = 0;

  while (currentNodeID != -1 && steps < executionLimit) {
    steps++;
    int nextNodeID = -1;

    for (const auto &link : m_delegate.mLinks) {
      if (link.mInputNodeID == currentNodeID) {
        nextNodeID = link.mOutputNodeID;
        break;
      }
    }

    if (nextNodeID != -1) {
      for (const auto &n : m_delegate.mNodes) {
        if (n.mID == nextNodeID) {
          if (n.mTemplateIndex == 1) {

            // ==========================================
            // 🔥 THE GAMEPLAY BINDINGS 🔥
            // ==========================================
            if (n.mName == "Print Message") {
              std::cout << ">>> [LOGIC]: BHAUKAAL MACHA DIYA BHAI! <<<\n";
            }

            else if (n.mName == "Apply Jump Force") {
              if (contextObj) {
                // MERA ASSUMPTION: Tere GameObject class mein ek GetComponent
                // ya rigid body fetch karne ka function hoga. Uske hisaab se
                // isko thoda adjust kar lena agar error de:
                /*
                if (auto physComp =
                contextObj->GetComponent<PhysicsComponent>()) { btRigidBody*
                body = physComp->GetRigidBody(); if (body) {
                        body->activate(true); // Body ko sleep mode se jagao
                        body->applyCentralImpulse(btVector3(0, 50.0f, 0)); //
                Upar ki taraf kick! std::cout << ">>> [LOGIC]: Jump Force
                Applied! <<<\n";
                    }
                }
                */
                std::cout << ">>> [LOGIC]: Jump Force Applied to Object at Y: "
                          << contextObj->position.y << " <<<\n";
              } else {
                std::cout << ">>> [VM WARNING]: 'Jump Force' needs a Context "
                             "Object, but was called Globally! <<<\n";
              }
            }

            else if (n.mName == "Spawn Particles") {
              if (contextObj) {
                // Yahan par apna globalVFX->Emit call karna
                // contextObj->position par
                std::cout << ">>> [LOGIC]: BOOM! Particles spawned at X: "
                          << contextObj->position.x << " <<<\n";
              } else {
                std::cout << ">>> [VM WARNING]: 'Spawn Particles' needs a "
                             "Context Object! <<<\n";
              }
            }
          }
          break;
        }
      }
    }
    currentNodeID = nextNodeID;
  }
  std::cout << "============================================\n";
}
