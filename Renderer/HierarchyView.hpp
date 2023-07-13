#pragma once
#include "ViewWindow.hpp"

#include "Scene.hpp"
#include "InspectorView.hpp"

class HierarchyView :
    public ViewWindow {
public:

    HierarchyView();

    void Show() override;

    void SetScene(Scene* scene) { scene_ = scene; }
    void SetInspectorView(InspectorView* inspectorView) { inspectorView_ = inspectorView; }
    
    Scene* GetScene() { return scene_; }
    const Scene* GetScene() const { return scene_; }


private:
    void DisplayGameObject(GameObject* gameObject);
    void DragGameObject(GameObject* gameObject);
    void DropGameObject(GameObject* parent);

    Scene* scene_;
    InspectorView* inspectorView_;
    GameObject* dropDest_;
    GameObject* dragSrc_;
};