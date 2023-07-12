#pragma once

#include <string>

class Object {
public:
    const std::string& GetName() const { return name_; }
    void SetName(const std::string& name) { name_ = name; }
private:
    std::string name_;
};