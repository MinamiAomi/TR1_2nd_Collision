#include "HierarchyView.hpp"

#include "Externals/ImGui/imgui.h"

#include <format>

constexpr ImGuiTreeNodeFlags kSceneTreeNodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_SpanAvailWidth;
constexpr ImGuiTreeNodeFlags kGameObjectTreeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

HierarchyView::HierarchyView() :
    scene_(nullptr),
    inspectorView_(nullptr),
    dragSrc_(nullptr),
    dropDest_(nullptr) {
}

void HierarchyView::Show() {
    if (isVisible_) {
        if (ImGui::Begin("Hierarchy", &isVisible_)) {
            if (scene_) {

                dragSrc_ = dropDest_ = nullptr;

                ImGui::TreeNodeEx(scene_->GetName().c_str(), kSceneTreeNodeFlags);

                // シーンにドロップ
                DropGameObject(nullptr);
                auto& gameObjects = scene_->GetGameObjects();
                for (auto& gameObject : gameObjects) {
                    // 親がいる場合
                    if (!gameObject->GetParent()) {
                        DisplayGameObject(gameObject.get());
                    }
                }
                ImGui::TreePop();

                if (dragSrc_ != dropDest_) {
                    dragSrc_->SetParent(dropDest_);
                }
            }
            else {
                ImGui::Text("シーンがありません");
            }
        }

        ImGui::End();
    }
}

void HierarchyView::DisplayGameObject(GameObject* gameObject) {
    ImGuiTreeNodeFlags flags = kGameObjectTreeNodeFlags;
    if (gameObject->GetChildren().empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (ImGui::TreeNodeEx(gameObject->GetName().c_str(), flags)) {
        if (inspectorView_ && ImGui::IsItemClicked()) {
            inspectorView_->SetGameObejct(gameObject);
        }
        DragGameObject(gameObject);
        DropGameObject(gameObject);
        for (auto c : gameObject->GetChildren()) {
            DisplayGameObject(c);
        }
        ImGui::TreePop();
    }
}

void HierarchyView::DragGameObject(GameObject* gameObject) {
    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("GameObject", &gameObject, sizeof(gameObject));
        ImGui::Text(gameObject->GetName().c_str());
        ImGui::EndDragDropSource();
    }
}

void HierarchyView::DropGameObject(GameObject* parent) {
    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GameObject");
        if (payload) {
            GameObject* gameObject = *static_cast<GameObject**>(payload->Data);
            dropDest_ = parent;
            dragSrc_ = gameObject;
        }
        ImGui::EndDragDropTarget();
    }
}
