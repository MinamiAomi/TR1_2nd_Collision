#include "HierarchyView.hpp"

#include "Externals/ImGui/imgui.h"

void HierarchyView::Show() {
    if (isVisible_) {
        if (ImGui::Begin("Hierarchy", &isVisible_)) {
            if (scene_) {
                ImGui::TreeNodeEx(scene_->GetName().c_str(), ImGuiTreeNodeFlags_Leaf);
                
                auto& go = scene_->GetGameObjects();
                for (auto& iter : go) {
                    // 親がいる場合
                    if (iter->GetParent()) { continue; }
                    DisplayGameObject(iter.get());
                }
                ImGui::TreePop();

            }
            else {
                ImGui::Text("シーンがありません");
            }
        }
   
        ImGui::End();
    }
}

void HierarchyView::DisplayGameObject(GameObject* gameObject) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;
    if (gameObject->GetChildren().empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    if (ImGui::TreeNodeEx(gameObject->GetName().c_str(), flags)) {
        if (inspectorView_ && ImGui::IsItemClicked()) {
            inspectorView_->SetGameObejct(gameObject);
        }
        for (auto c : gameObject->GetChildren()) {
            DisplayGameObject(c);
        }
        
        ImGui::TreePop();
    }
}
