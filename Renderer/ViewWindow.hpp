#pragma once

class ViewWindow {
public:
    ViewWindow() : isVisible_(true) {}
    virtual ~ViewWindow() {}

    virtual void Show() = 0;

    void SetIsVisible(bool isVisible) { isVisible_ = isVisible; }
    bool IsVisible() const { return isVisible_; }

protected:
    bool isVisible_;
};