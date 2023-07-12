#include "InspectorView.hpp"

#include "Externals/ImGui/imgui.h"

void InspectorView::Show() {
    if (isVisible_) {
        if (ImGui::Begin("Inspector", &isVisible_)) {
            if (gameObject_) {
                //ImGui::BeginTable();
                gameObject_->ShowUI();
            }
        }
        ImGui::End();
    }
}
