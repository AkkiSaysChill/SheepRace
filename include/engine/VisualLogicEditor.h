#pragma once
#include "GraphEditor.h"
#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "imgui.h"
#include <memory>
#include <string>
#include <vector>

// Blueprint System structs
struct BlueprintLink {
  int mID;
  int mInputNodeID;
  int mInputSlotID;
  int mOutputNodeID;
  int mOutputSlotID;
};

struct BlueprintNode {
  int mID;
  std::string mName;
  int mTemplateIndex;
  ImVec2 mPosition;
  bool mSelected;
};

class LogicGraphDelegate : public GraphEditor::Delegate {
public:
  LogicGraphDelegate();
  ~LogicGraphDelegate() = default;

  bool AllowedLink(GraphEditor::NodeIndex from,
                   GraphEditor::NodeIndex to) override;
  void SelectNode(GraphEditor::NodeIndex nodeIndex, bool selected) override;
  void MoveSelectedNodes(const ImVec2 delta) override;
  void RightClick(GraphEditor::NodeIndex nodeIndex,
                  GraphEditor::SlotIndex slotIndexInput,
                  GraphEditor::SlotIndex slotIndexOutput) override;
  void AddLink(GraphEditor::NodeIndex inputNodeIndex,
               GraphEditor::SlotIndex inputSlotIndex,
               GraphEditor::NodeIndex outputNodeIndex,
               GraphEditor::SlotIndex outputSlotIndex) override;
  void DelLink(GraphEditor::LinkIndex index) override;
  void CustomDraw(ImDrawList *drawList, ImRect rectangle,
                  GraphEditor::NodeIndex nodeIndex) override;

  const size_t GetTemplateCount() override;
  const GraphEditor::Template GetTemplate(size_t index) override;
  const size_t GetNodeCount() override;
  const GraphEditor::Node GetNode(size_t index) override;
  const size_t GetLinkCount() override;
  const GraphEditor::Link GetLink(size_t index) override;

  void AddNode(const std::string &name, int templateIndex, ImVec2 pos);
  void Clear();

  void SetCounters(int maxNodeId, int maxLinkId);

  std::vector<BlueprintNode> mNodes;
  std::vector<BlueprintLink> mLinks;
  std::vector<GraphEditor::Template> mTemplates;

private:
  int mNextNodeID = 1;
  int mNextLinkID = 1;
};

class Engine;
class GameObject;

class VisualLogicEditor {
public:
  VisualLogicEditor();
  ~VisualLogicEditor();

  void Initialize(Engine *engine);
  void UpdateAndDraw();
  void ToggleEditor();
  bool IsActive() const;

  void SaveBlueprint(const std::string &scriptName);
  void LoadBlueprint(const std::string &scriptName);

  void ExecuteGraph(const std::string &startEventName,
                    std::shared_ptr<GameObject> contextObj = nullptr);

private:
  bool m_isOpen = false;
  LogicGraphDelegate m_delegate;
  GraphEditor::ViewState m_viewState = {};

  Engine *m_engine = nullptr;
};
